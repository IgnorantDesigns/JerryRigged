#include "Database/SQLUISQLiteProbe.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "SQLiteDatabase.h"

namespace
{
void AppendSQLUISQLiteProbeError(FSQLUISQLiteProbeResult& Result, const FString& ErrorMessage)
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

FString NormalizeSQLUISQLiteProbePath(FString DatabasePath)
{
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}
}

FString FSQLUISQLiteProbe::GetDefaultProbeDatabasePath()
{
	return NormalizeSQLUISQLiteProbePath(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteCoreProbe"),
		TEXT("SQLiteCoreProbe.db")));
}

FSQLUISQLiteProbeResult FSQLUISQLiteProbe::RunOpenCloseProbe(
	const FString& DatabasePath,
	const bool bRemoveDatabaseAfterClose)
{
	FSQLUISQLiteProbeResult Result;
	Result.DatabasePath = DatabasePath.IsEmpty()
		? GetDefaultProbeDatabasePath()
		: NormalizeSQLUISQLiteProbePath(DatabasePath);

	const FString ProbeDirectory = FPaths::GetPath(Result.DatabasePath);
	if (!IFileManager::Get().DirectoryExists(*ProbeDirectory)
		&& !IFileManager::Get().MakeDirectory(*ProbeDirectory, true))
	{
		AppendSQLUISQLiteProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLiteCore probe failed: could not create probe directory '%s'."),
				*ProbeDirectory));
		return Result;
	}

	FSQLiteDatabase Database;
	if (!Database.Open(*Result.DatabasePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
	{
		AppendSQLUISQLiteProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLiteCore probe failed: could not open database '%s'. SQLiteCore error: %s"),
				*Result.DatabasePath,
				*Database.GetLastError()));
		return Result;
	}

	Result.bDatabaseOpened = true;

	const bool bIntegrityCheckSucceeded = Database.PerformQuickIntegrityCheck();
	if (!bIntegrityCheckSucceeded)
	{
		AppendSQLUISQLiteProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLiteCore probe failed: quick integrity check failed for database '%s'. SQLiteCore error: %s"),
				*Result.DatabasePath,
				*Database.GetLastError()));
	}

	Result.bDatabaseClosed = Database.Close();
	if (!Result.bDatabaseClosed)
	{
		AppendSQLUISQLiteProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLiteCore probe failed: could not close database '%s'. SQLiteCore error: %s"),
				*Result.DatabasePath,
				*Database.GetLastError()));
	}

	if (bRemoveDatabaseAfterClose)
	{
		Result.bDatabaseRemoved = !FPaths::FileExists(Result.DatabasePath)
			|| IFileManager::Get().Delete(*Result.DatabasePath, false, true, true);
		if (!Result.bDatabaseRemoved)
		{
			AppendSQLUISQLiteProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLiteCore probe failed: could not remove probe database '%s'."),
					*Result.DatabasePath));
		}
	}

	Result.bSucceeded =
		Result.bDatabaseOpened
		&& Result.bDatabaseClosed
		&& bIntegrityCheckSucceeded
		&& (!bRemoveDatabaseAfterClose || Result.bDatabaseRemoved);

	return Result;
}
