#include "Database/SQLUISQLiteMigrationRunner.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "SQLiteDatabase.h"

namespace
{
const TCHAR* SQLUISQLiteMigrationTableName = TEXT("sqlui_schema_migrations");
const TCHAR* SQLUISQLiteProbeMigrationId = TEXT("sqlui.smoke.sqlite-migration-probe.v1");

void AppendSQLUISQLiteMigrationError(
	FSQLUISQLiteMigrationResult& Result,
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

FString NormalizeSQLUISQLiteMigrationProbePath(FString DatabasePath)
{
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUISQLiteMigrationProbeFile(
	const FString& DatabasePath,
	FSQLUISQLiteMigrationResult& Result)
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
			AppendSQLUISQLiteMigrationError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite migration probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool ExecuteSQLUISQLiteMigrationStatement(
	FSQLiteDatabase& Database,
	FSQLUISQLiteMigrationResult& Result,
	const FString& Statement,
	const TCHAR* OperationName)
{
	if (Statement.IsEmpty())
	{
		return true;
	}

	if (Database.Execute(*Statement))
	{
		return true;
	}

	AppendSQLUISQLiteMigrationError(
		Result,
		FString::Printf(
			TEXT("SQLUI SQLite migration probe failed: %s failed. SQLiteCore error: %s"),
			OperationName,
			*Database.GetLastError()));
	return false;
}

bool CreateSQLUISQLiteMigrationTable(
	FSQLiteDatabase& Database,
	FSQLUISQLiteMigrationResult& Result)
{
	const FString Statement = FString::Printf(
		TEXT("CREATE TABLE IF NOT EXISTS %s (")
		TEXT("migration_id TEXT PRIMARY KEY, ")
		TEXT("applied_at_utc TEXT NOT NULL, ")
		TEXT("description TEXT")
		TEXT(");"),
		SQLUISQLiteMigrationTableName);

	Result.bMigrationTableCreated = ExecuteSQLUISQLiteMigrationStatement(
		Database,
		Result,
		Statement,
		TEXT("create migration table"));

	return Result.bMigrationTableCreated;
}

bool RecordSQLUISQLiteMigration(
	FSQLiteDatabase& Database,
	FSQLUISQLiteMigrationResult& Result,
	const FSQLUISQLiteMigrationStep& Step)
{
	const FString RecordStatement = FString::Printf(
		TEXT("INSERT INTO %s (migration_id, applied_at_utc, description) ")
		TEXT("VALUES (?, datetime('now'), ?);"),
		SQLUISQLiteMigrationTableName);
	FSQLitePreparedStatement Statement = Database.PrepareStatement(*RecordStatement);

	if (!Statement.IsValid())
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: could not prepare migration record statement. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	if (!Statement.SetBindingValueByIndex(1, Step.MigrationId)
		|| !Statement.SetBindingValueByIndex(2, Step.Description)
		|| !Statement.Execute())
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: could not record migration '%s'. SQLiteCore error: %s"),
				*Step.MigrationId,
				*Database.GetLastError()));
		return false;
	}

	return true;
}

bool QuerySQLUISQLiteMigrationRecordedCount(
	FSQLiteDatabase& Database,
	FSQLUISQLiteMigrationResult& Result,
	const FString& MigrationId,
	int32& OutRecordedCount)
{
	OutRecordedCount = 0;
	const FString VerificationStatement = FString::Printf(
		TEXT("SELECT COUNT(*) FROM %s WHERE migration_id = ?;"),
		SQLUISQLiteMigrationTableName);
	FSQLitePreparedStatement Statement = Database.PrepareStatement(*VerificationStatement);

	if (!Statement.IsValid())
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: could not prepare migration verification statement. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	if (!Statement.SetBindingValueByIndex(1, MigrationId))
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: could not bind migration verification statement. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	const int64 RowCount = Statement.Execute(
		[&OutRecordedCount](const FSQLitePreparedStatement& Row)
		{
			return Row.GetColumnValueByIndex(0, OutRecordedCount)
				? ESQLitePreparedStatementExecuteRowResult::Continue
				: ESQLitePreparedStatementExecuteRowResult::Error;
		});

	if (RowCount == INDEX_NONE)
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: migration verification query failed. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	return true;
}

bool IsSQLUISQLiteMigrationRecorded(
	FSQLiteDatabase& Database,
	FSQLUISQLiteMigrationResult& Result,
	const FString& MigrationId,
	bool& bOutRecorded)
{
	bOutRecorded = false;
	int32 RecordedCount = 0;
	if (!QuerySQLUISQLiteMigrationRecordedCount(
		Database,
		Result,
		MigrationId,
		RecordedCount))
	{
		return false;
	}

	bOutRecorded = RecordedCount > 0;
	return true;
}

bool VerifySQLUISQLiteMigrationRecorded(
	FSQLiteDatabase& Database,
	FSQLUISQLiteMigrationResult& Result,
	const FString& MigrationId)
{
	int32 RecordedCount = 0;
	if (!QuerySQLUISQLiteMigrationRecordedCount(
		Database,
		Result,
		MigrationId,
		RecordedCount))
	{
		return false;
	}

	Result.bMigrationRecorded = RecordedCount > 0;
	if (!Result.bMigrationRecorded)
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: migration '%s' was not recorded."),
				*MigrationId));
	}

	return Result.bMigrationRecorded;
}

bool ApplySQLUISQLiteMigrationStep(
	FSQLiteDatabase& Database,
	FSQLUISQLiteMigrationResult& Result,
	const FSQLUISQLiteMigrationStep& Step)
{
	if (Step.MigrationId.IsEmpty())
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			TEXT("SQLUI SQLite migration probe failed: migration id is empty."));
		return false;
	}

	if (Step.Statements.Num() == 0)
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: migration '%s' has no statements."),
				*Step.MigrationId));
		return false;
	}

	if (!ExecuteSQLUISQLiteMigrationStatement(
		Database,
		Result,
		TEXT("BEGIN TRANSACTION;"),
		TEXT("begin migration transaction")))
	{
		return false;
	}

	for (const FString& Statement : Step.Statements)
	{
		if (!ExecuteSQLUISQLiteMigrationStatement(
			Database,
			Result,
			Statement,
			TEXT("apply probe migration statement")))
		{
			Database.Execute(TEXT("ROLLBACK;"));
			return false;
		}
	}

	if (!RecordSQLUISQLiteMigration(Database, Result, Step))
	{
		Database.Execute(TEXT("ROLLBACK;"));
		return false;
	}

	if (!ExecuteSQLUISQLiteMigrationStatement(
		Database,
		Result,
		TEXT("COMMIT;"),
		TEXT("commit migration transaction")))
	{
		Database.Execute(TEXT("ROLLBACK;"));
		return false;
	}

	Result.AppliedMigrationCount += 1;
	Result.bMigrationApplied = true;
	return true;
}

FSQLUISQLiteMigrationStep MakeSQLUISQLiteSmokeProbeMigration()
{
	FSQLUISQLiteMigrationStep Step;
	Step.MigrationId = SQLUISQLiteProbeMigrationId;
	Step.Description =
		TEXT("Smoke/probe migration only. This is not the SQLUI layout repository schema.");
	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS sqlui_migration_probe (")
		TEXT("probe_id TEXT PRIMARY KEY, ")
		TEXT("created_at_utc TEXT NOT NULL")
		TEXT(");"));
	return Step;
}
}

FString FSQLUISQLiteMigrationRunner::GetDefaultProbeDatabasePath()
{
	return NormalizeSQLUISQLiteMigrationProbePath(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteMigrationProbe"),
		TEXT("SQLiteMigrationProbe.db")));
}

FSQLUISQLiteMigrationResult FSQLUISQLiteMigrationRunner::RunMigrationProbe(
	const FString& DatabasePath,
	const bool bRemoveDatabaseAfterClose)
{
	TArray<FSQLUISQLiteMigrationStep> MigrationSteps;
	MigrationSteps.Add(MakeSQLUISQLiteSmokeProbeMigration());

	return RunMigrations(
		DatabasePath.IsEmpty() ? GetDefaultProbeDatabasePath() : DatabasePath,
		MigrationSteps,
		bRemoveDatabaseAfterClose);
}

FSQLUISQLiteMigrationResult FSQLUISQLiteMigrationRunner::RunMigrations(
	const FString& DatabasePath,
	const TArray<FSQLUISQLiteMigrationStep>& MigrationSteps,
	const bool bRemoveDatabaseAfterClose)
{
	FSQLUISQLiteMigrationResult Result;
	Result.DatabasePath = DatabasePath.IsEmpty()
		? GetDefaultProbeDatabasePath()
		: NormalizeSQLUISQLiteMigrationProbePath(DatabasePath);

	const FString ProbeDirectory = FPaths::GetPath(Result.DatabasePath);
	if (!IFileManager::Get().DirectoryExists(*ProbeDirectory)
		&& !IFileManager::Get().MakeDirectory(*ProbeDirectory, true))
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: could not create probe directory '%s'."),
				*ProbeDirectory));
		return Result;
	}

	if (bRemoveDatabaseAfterClose
		&& !DeleteSQLUISQLiteMigrationProbeFile(Result.DatabasePath, Result))
	{
		return Result;
	}

	FSQLiteDatabase Database;
	if (!Database.Open(*Result.DatabasePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: could not open database '%s'. SQLiteCore error: %s"),
				*Result.DatabasePath,
				*Database.GetLastError()));
		return Result;
	}

	Result.bDatabaseOpened = true;

	if (CreateSQLUISQLiteMigrationTable(Database, Result))
	{
		for (const FSQLUISQLiteMigrationStep& Step : MigrationSteps)
		{
			bool bMigrationAlreadyRecorded = false;
			if (!IsSQLUISQLiteMigrationRecorded(
				Database,
				Result,
				Step.MigrationId,
				bMigrationAlreadyRecorded))
			{
				break;
			}

			if (bMigrationAlreadyRecorded)
			{
				continue;
			}

			if (!ApplySQLUISQLiteMigrationStep(Database, Result, Step))
			{
				break;
			}
		}

		if (MigrationSteps.Num() > 0)
		{
			VerifySQLUISQLiteMigrationRecorded(
				Database,
				Result,
				MigrationSteps.Last().MigrationId);
		}
	}

	Result.bDatabaseClosed = Database.Close();
	if (!Result.bDatabaseClosed)
	{
		AppendSQLUISQLiteMigrationError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration probe failed: could not close database '%s'. SQLiteCore error: %s"),
				*Result.DatabasePath,
				*Database.GetLastError()));
	}

	if (bRemoveDatabaseAfterClose)
	{
		Result.bDatabaseRemoved = DeleteSQLUISQLiteMigrationProbeFile(
			Result.DatabasePath,
			Result);
	}

	Result.bSucceeded =
		Result.bDatabaseOpened
		&& Result.bMigrationTableCreated
		&& (MigrationSteps.Num() == 0 || Result.bMigrationRecorded)
		&& Result.bDatabaseClosed
		&& (!bRemoveDatabaseAfterClose || Result.bDatabaseRemoved);

	return Result;
}
