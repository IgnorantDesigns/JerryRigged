#include "Database/SQLUISQLiteLayoutSchemaMigration.h"

#include "Database/SQLUISQLiteMigrationRunner.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "SQLiteDatabase.h"

namespace
{
const TCHAR* SQLUILayoutSchemaMigrationId = TEXT("001_initial_layout_schema");

void AppendSQLUILayoutSchemaProbeError(
	FSQLUISQLiteLayoutSchemaMigrationProbeResult& Result,
	const FString& ErrorMessage)
{
	if (ErrorMessage.IsEmpty())
	{
		return;
	}

	if (!Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage += TEXT(" ");
	}

	Result.ErrorMessage += ErrorMessage;
}

FString NormalizeSQLUILayoutSchemaProbePath(FString DatabasePath)
{
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUILayoutSchemaProbeFiles(
	const FString& DatabasePath,
	FSQLUISQLiteLayoutSchemaMigrationProbeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUILayoutSchemaProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite layout schema migration probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUISQLiteMigrationStep MakeSQLUILayoutSchemaInitialMigration()
{
	FSQLUISQLiteMigrationStep Step;
	Step.MigrationId = SQLUILayoutSchemaMigrationId;
	Step.Description = TEXT("Create the initial SQLUI layout repository schema.");

	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS layouts (")
		TEXT("layout_id TEXT PRIMARY KEY, ")
		TEXT("display_name TEXT NOT NULL, ")
		TEXT("description TEXT, ")
		TEXT("created_by TEXT, ")
		TEXT("created_at_utc TEXT, ")
		TEXT("updated_at_utc TEXT, ")
		TEXT("current_revision INTEGER, ")
		TEXT("schema_version INTEGER NOT NULL, ")
		TEXT("search_text TEXT, ")
		TEXT("b_deleted INTEGER NOT NULL DEFAULT 0")
		TEXT(");"));

	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS layout_revisions (")
		TEXT("layout_id TEXT NOT NULL, ")
		TEXT("revision INTEGER NOT NULL, ")
		TEXT("label TEXT, ")
		TEXT("document_json TEXT NOT NULL, ")
		TEXT("document_hash TEXT, ")
		TEXT("created_by TEXT, ")
		TEXT("created_at_utc TEXT, ")
		TEXT("change_note TEXT, ")
		TEXT("PRIMARY KEY (layout_id, revision), ")
		TEXT("FOREIGN KEY (layout_id) REFERENCES layouts(layout_id)")
		TEXT(");"));

	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS layout_tags (")
		TEXT("layout_id TEXT NOT NULL, ")
		TEXT("tag TEXT NOT NULL, ")
		TEXT("PRIMARY KEY (layout_id, tag), ")
		TEXT("FOREIGN KEY (layout_id) REFERENCES layouts(layout_id)")
		TEXT(");"));

	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS layout_checkpoints (")
		TEXT("checkpoint_id TEXT PRIMARY KEY, ")
		TEXT("layout_id TEXT NOT NULL, ")
		TEXT("revision INTEGER NOT NULL, ")
		TEXT("label TEXT, ")
		TEXT("created_by TEXT, ")
		TEXT("created_at_utc TEXT, ")
		TEXT("notes TEXT, ")
		TEXT("FOREIGN KEY (layout_id, revision) REFERENCES layout_revisions(layout_id, revision)")
		TEXT(");"));

	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS layout_previews (")
		TEXT("preview_id TEXT PRIMARY KEY, ")
		TEXT("layout_id TEXT NOT NULL, ")
		TEXT("base_revision INTEGER, ")
		TEXT("document_json TEXT NOT NULL, ")
		TEXT("created_by TEXT, ")
		TEXT("created_at_utc TEXT, ")
		TEXT("expires_at_utc TEXT, ")
		TEXT("FOREIGN KEY (layout_id) REFERENCES layouts(layout_id)")
		TEXT(");"));

	Step.Statements.Add(TEXT("CREATE INDEX IF NOT EXISTS idx_layouts_display_name ON layouts(display_name);"));
	Step.Statements.Add(TEXT("CREATE INDEX IF NOT EXISTS idx_layouts_updated_at_utc ON layouts(updated_at_utc);"));
	Step.Statements.Add(TEXT("CREATE INDEX IF NOT EXISTS idx_layouts_b_deleted ON layouts(b_deleted);"));
	Step.Statements.Add(TEXT("CREATE INDEX IF NOT EXISTS idx_layout_revisions_layout_revision ON layout_revisions(layout_id, revision);"));
	Step.Statements.Add(TEXT("CREATE INDEX IF NOT EXISTS idx_layout_tags_tag ON layout_tags(tag);"));
	Step.Statements.Add(TEXT("CREATE INDEX IF NOT EXISTS idx_layout_previews_layout_id ON layout_previews(layout_id);"));

	return Step;
}

bool DoesSQLiteObjectExist(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutSchemaMigrationProbeResult& Result,
	const TCHAR* ObjectType,
	const TCHAR* ObjectName)
{
	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("SELECT COUNT(*) FROM sqlite_master WHERE type = ? AND name = ?;"));
	if (!Statement.IsValid())
	{
		AppendSQLUILayoutSchemaProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema migration probe failed: could not prepare %s verification for '%s'. SQLiteCore error: %s"),
				ObjectType,
				ObjectName,
				*Database.GetLastError()));
		return false;
	}

	if (!Statement.SetBindingValueByIndex(1, ObjectType)
		|| !Statement.SetBindingValueByIndex(2, ObjectName))
	{
		AppendSQLUILayoutSchemaProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema migration probe failed: could not bind %s verification for '%s'. SQLiteCore error: %s"),
				ObjectType,
				ObjectName,
				*Database.GetLastError()));
		return false;
	}

	int32 ObjectCount = 0;
	const int64 RowCount = Statement.Execute(
		[&ObjectCount](const FSQLitePreparedStatement& Row)
		{
			return Row.GetColumnValueByIndex(0, ObjectCount)
				? ESQLitePreparedStatementExecuteRowResult::Continue
				: ESQLitePreparedStatementExecuteRowResult::Error;
		});

	if (RowCount == INDEX_NONE)
	{
		AppendSQLUILayoutSchemaProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema migration probe failed: %s verification query failed for '%s'. SQLiteCore error: %s"),
				ObjectType,
				ObjectName,
				*Database.GetLastError()));
		return false;
	}

	return ObjectCount > 0;
}

bool VerifySQLUILayoutSchemaTables(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutSchemaMigrationProbeResult& Result)
{
	Result.bLayoutsTableExists = DoesSQLiteObjectExist(Database, Result, TEXT("table"), TEXT("layouts"));
	Result.bLayoutRevisionsTableExists = DoesSQLiteObjectExist(Database, Result, TEXT("table"), TEXT("layout_revisions"));
	Result.bLayoutTagsTableExists = DoesSQLiteObjectExist(Database, Result, TEXT("table"), TEXT("layout_tags"));
	Result.bLayoutCheckpointsTableExists = DoesSQLiteObjectExist(Database, Result, TEXT("table"), TEXT("layout_checkpoints"));
	Result.bLayoutPreviewsTableExists = DoesSQLiteObjectExist(Database, Result, TEXT("table"), TEXT("layout_previews"));

	TArray<FString> MissingTables;
	if (!Result.bLayoutsTableExists)
	{
		MissingTables.Add(TEXT("layouts"));
	}
	if (!Result.bLayoutRevisionsTableExists)
	{
		MissingTables.Add(TEXT("layout_revisions"));
	}
	if (!Result.bLayoutTagsTableExists)
	{
		MissingTables.Add(TEXT("layout_tags"));
	}
	if (!Result.bLayoutCheckpointsTableExists)
	{
		MissingTables.Add(TEXT("layout_checkpoints"));
	}
	if (!Result.bLayoutPreviewsTableExists)
	{
		MissingTables.Add(TEXT("layout_previews"));
	}

	if (MissingTables.Num() > 0)
	{
		AppendSQLUILayoutSchemaProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema migration probe failed: missing expected table(s): %s."),
				*FString::Join(MissingTables, TEXT(", "))));
		return false;
	}

	return true;
}

bool VerifySQLUILayoutSchemaIndexes(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutSchemaMigrationProbeResult& Result)
{
	const TArray<FString> ExpectedIndexes = {
		TEXT("idx_layouts_display_name"),
		TEXT("idx_layouts_updated_at_utc"),
		TEXT("idx_layouts_b_deleted"),
		TEXT("idx_layout_revisions_layout_revision"),
		TEXT("idx_layout_tags_tag"),
		TEXT("idx_layout_previews_layout_id")
	};

	TArray<FString> MissingIndexes;
	for (const FString& ExpectedIndex : ExpectedIndexes)
	{
		if (!DoesSQLiteObjectExist(Database, Result, TEXT("index"), *ExpectedIndex))
		{
			MissingIndexes.Add(ExpectedIndex);
		}
	}

	Result.bExpectedIndexesExist = MissingIndexes.Num() == 0;
	if (!Result.bExpectedIndexesExist)
	{
		AppendSQLUILayoutSchemaProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema migration probe failed: missing expected index(es): %s."),
				*FString::Join(MissingIndexes, TEXT(", "))));
	}

	return Result.bExpectedIndexesExist;
}

bool VerifySQLUILayoutSchemaMigration(
	FSQLUISQLiteLayoutSchemaMigrationProbeResult& Result)
{
	FSQLiteDatabase Database;
	if (!Database.Open(*Result.DatabasePath, ESQLiteDatabaseOpenMode::ReadOnly))
	{
		AppendSQLUILayoutSchemaProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema migration probe failed: could not reopen database '%s' for schema verification. SQLiteCore error: %s"),
				*Result.DatabasePath,
				*Database.GetLastError()));
		return false;
	}

	const bool bTablesVerified = VerifySQLUILayoutSchemaTables(Database, Result);
	const bool bIndexesVerified = VerifySQLUILayoutSchemaIndexes(Database, Result);

	if (!Database.Close())
	{
		AppendSQLUILayoutSchemaProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema migration probe failed: could not close verification database '%s'. SQLiteCore error: %s"),
				*Result.DatabasePath,
				*Database.GetLastError()));
		return false;
	}

	return bTablesVerified && bIndexesVerified;
}
}

FString FSQLUISQLiteLayoutSchemaMigration::GetDefaultProbeDatabasePath()
{
	return NormalizeSQLUILayoutSchemaProbePath(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutSchemaMigrationProbe"),
		TEXT("LayoutSchemaMigrationProbe.db")));
}

FSQLUISQLiteLayoutSchemaMigrationProbeResult FSQLUISQLiteLayoutSchemaMigration::RunProbe(
	const FString& DatabasePath,
	const bool bRemoveDatabaseAfterClose)
{
	FSQLUISQLiteLayoutSchemaMigrationProbeResult Result;
	Result.DatabasePath = DatabasePath.IsEmpty()
		? GetDefaultProbeDatabasePath()
		: NormalizeSQLUILayoutSchemaProbePath(DatabasePath);

	if (!DeleteSQLUILayoutSchemaProbeFiles(Result.DatabasePath, Result))
	{
		return Result;
	}

	TArray<FSQLUISQLiteMigrationStep> MigrationSteps;
	MigrationSteps.Add(MakeSQLUILayoutSchemaInitialMigration());

	const FSQLUISQLiteMigrationResult MigrationResult =
		FSQLUISQLiteMigrationRunner::RunMigrations(
			Result.DatabasePath,
			MigrationSteps,
			false);
	Result.bMigrationSucceeded = MigrationResult.bSucceeded;
	if (!MigrationResult.ErrorMessage.IsEmpty())
	{
		AppendSQLUILayoutSchemaProbeError(Result, MigrationResult.ErrorMessage);
	}

	bool bSchemaVerified = false;
	if (Result.bMigrationSucceeded)
	{
		bSchemaVerified = VerifySQLUILayoutSchemaMigration(Result);
	}

	if (bRemoveDatabaseAfterClose)
	{
		Result.bDatabaseRemoved = DeleteSQLUILayoutSchemaProbeFiles(
			Result.DatabasePath,
			Result);
	}

	Result.bSucceeded =
		Result.bMigrationSucceeded
		&& bSchemaVerified
		&& Result.bLayoutsTableExists
		&& Result.bLayoutRevisionsTableExists
		&& Result.bLayoutTagsTableExists
		&& Result.bLayoutCheckpointsTableExists
		&& Result.bLayoutPreviewsTableExists
		&& Result.bExpectedIndexesExist
		&& (!bRemoveDatabaseAfterClose || Result.bDatabaseRemoved);

	return Result;
}
