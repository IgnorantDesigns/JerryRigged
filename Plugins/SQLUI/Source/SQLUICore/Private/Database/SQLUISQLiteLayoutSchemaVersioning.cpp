#include "Database/SQLUISQLiteLayoutSchemaVersioning.h"

#include "Database/SQLUISQLiteLayoutSchemaMigration.h"
#include "Misc/Paths.h"
#include "SQLiteDatabase.h"

namespace
{
void AppendSQLUILayoutSchemaVersioningError(
	FSQLUISQLiteLayoutSchemaVersionStatus& Status,
	const FString& ErrorMessage)
{
	if (ErrorMessage.IsEmpty())
	{
		return;
	}

	if (!Status.ErrorMessage.IsEmpty())
	{
		Status.ErrorMessage += TEXT(" ");
	}

	Status.ErrorMessage += ErrorMessage;
}

FString NormalizeSQLUILayoutSchemaVersioningPath(FString DatabasePath)
{
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<TPair<FString, FString>> MakeSQLUILayoutSchemaVersioningExpectedObjects()
{
	return {
		TPair<FString, FString>(TEXT("table"), TEXT("layouts")),
		TPair<FString, FString>(TEXT("table"), TEXT("layout_revisions")),
		TPair<FString, FString>(TEXT("table"), TEXT("layout_tags")),
		TPair<FString, FString>(TEXT("table"), TEXT("layout_checkpoints")),
		TPair<FString, FString>(TEXT("table"), TEXT("layout_previews")),
		TPair<FString, FString>(TEXT("index"), TEXT("idx_layouts_display_name")),
		TPair<FString, FString>(TEXT("index"), TEXT("idx_layouts_updated_at_utc")),
		TPair<FString, FString>(TEXT("index"), TEXT("idx_layouts_b_deleted")),
		TPair<FString, FString>(TEXT("index"), TEXT("idx_layout_revisions_layout_revision")),
		TPair<FString, FString>(TEXT("index"), TEXT("idx_layout_tags_tag")),
		TPair<FString, FString>(TEXT("index"), TEXT("idx_layout_previews_layout_id"))
	};
}

bool QuerySQLUILayoutSchemaVersioningObjectCount(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutSchemaVersionStatus& Status,
	const FString& ObjectType,
	const FString& ObjectName,
	int32& OutObjectCount)
{
	OutObjectCount = 0;

	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("SELECT COUNT(*) FROM sqlite_master WHERE type = ? AND name = ?;"));
	if (!Statement.IsValid())
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema version status failed: could not prepare %s verification for '%s'. SQLiteCore error: %s"),
				*ObjectType,
				*ObjectName,
				*Database.GetLastError()));
		return false;
	}

	if (!Statement.SetBindingValueByIndex(1, ObjectType)
		|| !Statement.SetBindingValueByIndex(2, ObjectName))
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema version status failed: could not bind %s verification for '%s'. SQLiteCore error: %s"),
				*ObjectType,
				*ObjectName,
				*Database.GetLastError()));
		return false;
	}

	const int64 RowCount = Statement.Execute(
		[&OutObjectCount](const FSQLitePreparedStatement& Row)
		{
			return Row.GetColumnValueByIndex(0, OutObjectCount)
				? ESQLitePreparedStatementExecuteRowResult::Continue
				: ESQLitePreparedStatementExecuteRowResult::Error;
		});

	if (RowCount == INDEX_NONE)
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema version status failed: %s verification query failed for '%s'. SQLiteCore error: %s"),
				*ObjectType,
				*ObjectName,
				*Database.GetLastError()));
		return false;
	}

	return true;
}

bool QuerySQLUILayoutSchemaVersioningMigrationIds(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutSchemaVersionStatus& Status)
{
	Status.AppliedMigrationIds.Reset();

	int32 MigrationTableCount = 0;
	if (!QuerySQLUILayoutSchemaVersioningObjectCount(
		Database,
		Status,
		TEXT("table"),
		TEXT("sqlui_schema_migrations"),
		MigrationTableCount))
	{
		return false;
	}

	Status.bMigrationTableExists = MigrationTableCount > 0;
	if (!Status.bMigrationTableExists)
	{
		return true;
	}

	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("SELECT migration_id FROM sqlui_schema_migrations ORDER BY rowid ASC;"));
	if (!Statement.IsValid())
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema version status failed: could not prepare migration id query. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	const int64 RowCount = Statement.Execute(
		[&Status](const FSQLitePreparedStatement& Row)
		{
			FString MigrationId;
			if (!Row.GetColumnValueByIndex(0, MigrationId))
			{
				return ESQLitePreparedStatementExecuteRowResult::Error;
			}

			Status.AppliedMigrationIds.Add(MigrationId);
			return ESQLitePreparedStatementExecuteRowResult::Continue;
		});

	if (RowCount == INDEX_NONE)
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema version status failed: migration id query failed. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	Status.AppliedMigrationCount = Status.AppliedMigrationIds.Num();
	if (Status.AppliedMigrationIds.Num() > 0)
	{
		Status.LatestAppliedMigrationId = Status.AppliedMigrationIds.Last();
	}

	return true;
}

bool VerifySQLUILayoutSchemaVersioningExpectedObjects(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutSchemaVersionStatus& Status,
	int32& OutPresentObjectCount)
{
	OutPresentObjectCount = 0;
	TArray<FString> MissingObjects;

	for (const TPair<FString, FString>& ExpectedObject : MakeSQLUILayoutSchemaVersioningExpectedObjects())
	{
		int32 ObjectCount = 0;
		if (!QuerySQLUILayoutSchemaVersioningObjectCount(
			Database,
			Status,
			ExpectedObject.Key,
			ExpectedObject.Value,
			ObjectCount))
		{
			return false;
		}

		if (ObjectCount > 0)
		{
			OutPresentObjectCount += 1;
		}
		else
		{
			MissingObjects.Add(ExpectedObject.Value);
		}
	}

	Status.bSchemaObjectsReady = MissingObjects.Num() == 0;
	if (!Status.bSchemaObjectsReady
		&& (OutPresentObjectCount > 0 || Status.bInitialSchemaRecorded))
	{
		Status.bHasFailedMigration = true;
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema version status failed: missing expected schema object(s): %s."),
				*FString::Join(MissingObjects, TEXT(", "))));
	}

	return true;
}

void PopulateSQLUILayoutSchemaVersioningKnownState(
	FSQLUISQLiteLayoutSchemaVersionStatus& Status,
	const TArray<FString>& KnownMigrationIds)
{
	Status.KnownMigrationCount = KnownMigrationIds.Num();
	Status.LatestKnownMigrationId =
		KnownMigrationIds.Num() > 0 ? KnownMigrationIds.Last() : FString();

	const FString InitialMigrationId =
		FSQLUISQLiteLayoutSchemaMigration::GetInitialLayoutSchemaMigrationId();
	Status.bInitialSchemaRecorded =
		Status.AppliedMigrationIds.Contains(InitialMigrationId);

	Status.PendingMigrationIds.Reset();
	for (const FString& KnownMigrationId : KnownMigrationIds)
	{
		if (!Status.AppliedMigrationIds.Contains(KnownMigrationId))
		{
			Status.PendingMigrationIds.Add(KnownMigrationId);
		}
	}

	Status.bHasPendingMigrations = Status.PendingMigrationIds.Num() > 0;
}
}

TArray<FString> FSQLUISQLiteLayoutSchemaVersioning::GetKnownLayoutSchemaMigrationIds()
{
	return {
		FSQLUISQLiteLayoutSchemaMigration::GetInitialLayoutSchemaMigrationId()
	};
}

FString FSQLUISQLiteLayoutSchemaVersioning::GetLatestKnownLayoutSchemaMigrationId()
{
	const TArray<FString> KnownMigrationIds = GetKnownLayoutSchemaMigrationIds();
	return KnownMigrationIds.Num() > 0 ? KnownMigrationIds.Last() : FString();
}

FSQLUISQLiteLayoutSchemaVersionStatus
FSQLUISQLiteLayoutSchemaVersioning::GetLayoutSchemaVersionStatus(
	const FString& DatabasePath)
{
	return GetMigrationVersionStatus(
		DatabasePath,
		GetKnownLayoutSchemaMigrationIds(),
		true);
}

FSQLUISQLiteLayoutSchemaVersionStatus
FSQLUISQLiteLayoutSchemaVersioning::GetMigrationVersionStatus(
	const FString& DatabasePath,
	const TArray<FString>& KnownMigrationIds,
	const bool bVerifyLayoutSchemaObjects)
{
	FSQLUISQLiteLayoutSchemaVersionStatus Status;
	Status.DatabasePath = DatabasePath.IsEmpty()
		? FString()
		: NormalizeSQLUILayoutSchemaVersioningPath(DatabasePath);
	Status.KnownMigrationCount = KnownMigrationIds.Num();
	Status.LatestKnownMigrationId =
		KnownMigrationIds.Num() > 0 ? KnownMigrationIds.Last() : FString();

	if (Status.DatabasePath.IsEmpty())
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			TEXT("SQLUI SQLite layout schema version status failed: database path is empty."));
		return Status;
	}

	if (!FPaths::FileExists(Status.DatabasePath))
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema version status failed: database '%s' does not exist."),
				*Status.DatabasePath));
		return Status;
	}

	FSQLiteDatabase Database;
	if (!Database.Open(*Status.DatabasePath, ESQLiteDatabaseOpenMode::ReadOnly))
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema version status failed: could not open database '%s'. SQLiteCore error: %s"),
				*Status.DatabasePath,
				*Database.GetLastError()));
		return Status;
	}

	Status.bDatabaseOpened = true;
	bool bStatusQueried = QuerySQLUILayoutSchemaVersioningMigrationIds(Database, Status);
	if (bStatusQueried)
	{
		PopulateSQLUILayoutSchemaVersioningKnownState(Status, KnownMigrationIds);
	}

	int32 PresentSchemaObjectCount = 0;
	if (bStatusQueried && bVerifyLayoutSchemaObjects)
	{
		bStatusQueried = VerifySQLUILayoutSchemaVersioningExpectedObjects(
			Database,
			Status,
			PresentSchemaObjectCount);
	}

	if (!Database.Close())
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema version status failed: could not close database '%s'. SQLiteCore error: %s"),
				*Status.DatabasePath,
				*Database.GetLastError()));
		bStatusQueried = false;
	}

	Status.bSucceeded = bStatusQueried && !Status.bHasFailedMigration;
	return Status;
}

FSQLUISQLiteLayoutSchemaVersionStatus
FSQLUISQLiteLayoutSchemaVersioning::ApplyKnownLayoutSchemaMigrations(
	const FString& DatabasePath,
	const bool bCreateDatabaseIfMissing)
{
	FSQLUISQLiteLayoutSchemaVersionStatus Status;
	Status.DatabasePath = DatabasePath.IsEmpty()
		? FString()
		: NormalizeSQLUILayoutSchemaVersioningPath(DatabasePath);
	Status.KnownMigrationCount = GetKnownLayoutSchemaMigrationIds().Num();
	Status.LatestKnownMigrationId = GetLatestKnownLayoutSchemaMigrationId();

	if (Status.DatabasePath.IsEmpty())
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			TEXT("SQLUI SQLite layout schema migration application failed: database path is empty."));
		return Status;
	}

	if (!FPaths::FileExists(Status.DatabasePath) && !bCreateDatabaseIfMissing)
	{
		AppendSQLUILayoutSchemaVersioningError(
			Status,
			FString::Printf(
				TEXT("SQLUI SQLite layout schema migration application failed: database '%s' does not exist and database creation is disabled."),
				*Status.DatabasePath));
		return Status;
	}

	const FSQLUISQLiteLayoutSchemaInitializationResult InitializationResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(
			Status.DatabasePath,
			bCreateDatabaseIfMissing);

	FSQLUISQLiteLayoutSchemaVersionStatus VersionStatus =
		FPaths::FileExists(Status.DatabasePath)
			? GetLayoutSchemaVersionStatus(Status.DatabasePath)
			: Status;

	if (!InitializationResult.ErrorMessage.IsEmpty())
	{
		AppendSQLUILayoutSchemaVersioningError(
			VersionStatus,
			InitializationResult.ErrorMessage);
	}

	if (!InitializationResult.bSucceeded)
	{
		VersionStatus.bSucceeded = false;
		VersionStatus.bHasFailedMigration = true;
		if (VersionStatus.ErrorMessage.IsEmpty())
		{
			AppendSQLUILayoutSchemaVersioningError(
				VersionStatus,
				TEXT("SQLUI SQLite layout schema migration application failed."));
		}
		return VersionStatus;
	}

	VersionStatus.bSucceeded =
		VersionStatus.bSucceeded
		&& VersionStatus.bSchemaObjectsReady
		&& !VersionStatus.bHasPendingMigrations;
	return VersionStatus;
}
