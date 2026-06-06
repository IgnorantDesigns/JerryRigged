#include "Layout/SQLUILayoutRepositoryDatabaseManagement.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
struct FSQLUILayoutRepositoryDatabaseFilePaths
{
	FString DatabasePath;
	FString JournalPath;
	FString WalPath;
	FString ShmPath;
};

FSQLUILayoutRepositoryDatabaseFilePaths MakeSQLUILayoutRepositoryDatabaseFilePaths(
	const FString& DatabasePath)
{
	FSQLUILayoutRepositoryDatabaseFilePaths Paths;
	Paths.DatabasePath = DatabasePath;
	Paths.JournalPath = DatabasePath + TEXT("-journal");
	Paths.WalPath = DatabasePath + TEXT("-wal");
	Paths.ShmPath = DatabasePath + TEXT("-shm");
	return Paths;
}

bool DeleteSQLUILayoutRepositoryDatabaseFile(
	const FString& Path,
	bool& bOutAbsentAfterDelete,
	FString& OutErrorMessage)
{
	if (Path.IsEmpty())
	{
		bOutAbsentAfterDelete = true;
		return true;
	}

	if (FPaths::FileExists(Path)
		&& !IFileManager::Get().Delete(*Path, false, true, true))
	{
		OutErrorMessage = FString::Printf(
			TEXT("Could not remove SQLite layout repository database file '%s'."),
			*Path);
		bOutAbsentAfterDelete = false;
		return false;
	}

	bOutAbsentAfterDelete = !FPaths::FileExists(Path);
	if (!bOutAbsentAfterDelete)
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLite layout repository database file '%s' still exists after reset."),
			*Path);
	}
	return bOutAbsentAfterDelete;
}
}

FSQLUILayoutRepositoryDatabaseStatusResult
FSQLUILayoutRepositoryDatabaseManagement::GetStatus(
	const FSQLUILayoutRepositoryDatabaseStatusRequest& Request)
{
	FSQLUILayoutRepositoryDatabaseStatusResult Result;
	Result.bBackendIsSQLite =
		Request.RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::SQLite;

	if (!Result.bBackendIsSQLite)
	{
		Result.bSucceeded = true;
		return Result;
	}

	const FString DatabasePath =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(
			Request.RuntimeConfig.SQLiteDatabasePath);
	Result.DatabasePath = DatabasePath;
	Result.bDatabasePathResolved = !DatabasePath.IsEmpty();

	if (!Result.bDatabasePathResolved)
	{
		Result.bSucceeded = true;
		Result.ErrorMessage =
			TEXT("SQLite layout repository database path is empty.");
		return Result;
	}

	const FSQLUILayoutRepositoryDatabaseFilePaths Paths =
		MakeSQLUILayoutRepositoryDatabaseFilePaths(DatabasePath);
	Result.JournalPath = Paths.JournalPath;
	Result.WalPath = Paths.WalPath;
	Result.ShmPath = Paths.ShmPath;

	Result.bDatabaseExists = FPaths::FileExists(Paths.DatabasePath);
	Result.DatabaseFileSizeBytes =
		Result.bDatabaseExists ? IFileManager::Get().FileSize(*Paths.DatabasePath) : 0;

	if (Request.bIncludeSidecars)
	{
		Result.bJournalExists = FPaths::FileExists(Paths.JournalPath);
		Result.bWalExists = FPaths::FileExists(Paths.WalPath);
		Result.bShmExists = FPaths::FileExists(Paths.ShmPath);
	}

	Result.bAnyDatabaseFileExists =
		Result.bDatabaseExists
		|| Result.bJournalExists
		|| Result.bWalExists
		|| Result.bShmExists;
	Result.bSucceeded = true;
	return Result;
}

FSQLUILayoutRepositoryDatabaseResetResult
FSQLUILayoutRepositoryDatabaseManagement::ResetDatabase(
	const FSQLUILayoutRepositoryDatabaseResetRequest& Request)
{
	FSQLUILayoutRepositoryDatabaseResetResult Result;
	Result.bBackendIsSQLite =
		Request.RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::SQLite;

	if (!Result.bBackendIsSQLite)
	{
		Result.bSucceeded = true;
		Result.bAllDatabaseFilesAbsent = true;
		return Result;
	}

	const FString DatabasePath =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(
			Request.RuntimeConfig.SQLiteDatabasePath);
	Result.DatabasePath = DatabasePath;
	Result.bDatabasePathResolved = !DatabasePath.IsEmpty();

	if (!Result.bDatabasePathResolved)
	{
		Result.ErrorMessage =
			TEXT("Cannot reset SQLite layout repository database because the database path is empty.");
		return Result;
	}

	const FSQLUILayoutRepositoryDatabaseFilePaths Paths =
		MakeSQLUILayoutRepositoryDatabaseFilePaths(DatabasePath);

	bool bDeleteSucceeded = true;
	FString DeleteErrorMessage;
	bDeleteSucceeded =
		DeleteSQLUILayoutRepositoryDatabaseFile(
			Paths.DatabasePath,
			Result.bDatabaseRemoved,
			DeleteErrorMessage)
		&& bDeleteSucceeded;
	if (!DeleteErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = DeleteErrorMessage;
	}

	if (Request.bRemoveSidecars)
	{
		DeleteErrorMessage.Reset();
		bDeleteSucceeded =
			DeleteSQLUILayoutRepositoryDatabaseFile(
				Paths.JournalPath,
				Result.bJournalRemoved,
				DeleteErrorMessage)
			&& bDeleteSucceeded;
		if (!DeleteErrorMessage.IsEmpty())
		{
			Result.ErrorMessage =
				Result.ErrorMessage.IsEmpty()
					? DeleteErrorMessage
					: FString::Printf(TEXT("%s %s"), *Result.ErrorMessage, *DeleteErrorMessage);
		}

		DeleteErrorMessage.Reset();
		bDeleteSucceeded =
			DeleteSQLUILayoutRepositoryDatabaseFile(
				Paths.WalPath,
				Result.bWalRemoved,
				DeleteErrorMessage)
			&& bDeleteSucceeded;
		if (!DeleteErrorMessage.IsEmpty())
		{
			Result.ErrorMessage =
				Result.ErrorMessage.IsEmpty()
					? DeleteErrorMessage
					: FString::Printf(TEXT("%s %s"), *Result.ErrorMessage, *DeleteErrorMessage);
		}

		DeleteErrorMessage.Reset();
		bDeleteSucceeded =
			DeleteSQLUILayoutRepositoryDatabaseFile(
				Paths.ShmPath,
				Result.bShmRemoved,
				DeleteErrorMessage)
			&& bDeleteSucceeded;
		if (!DeleteErrorMessage.IsEmpty())
		{
			Result.ErrorMessage =
				Result.ErrorMessage.IsEmpty()
					? DeleteErrorMessage
					: FString::Printf(TEXT("%s %s"), *Result.ErrorMessage, *DeleteErrorMessage);
		}
	}
	else
	{
		Result.bJournalRemoved = !FPaths::FileExists(Paths.JournalPath);
		Result.bWalRemoved = !FPaths::FileExists(Paths.WalPath);
		Result.bShmRemoved = !FPaths::FileExists(Paths.ShmPath);
	}

	Result.bAllDatabaseFilesAbsent =
		!FPaths::FileExists(Paths.DatabasePath)
		&& !FPaths::FileExists(Paths.JournalPath)
		&& !FPaths::FileExists(Paths.WalPath)
		&& !FPaths::FileExists(Paths.ShmPath);
	Result.bSucceeded = bDeleteSucceeded && Result.bAllDatabaseFilesAbsent;
	return Result;
}
