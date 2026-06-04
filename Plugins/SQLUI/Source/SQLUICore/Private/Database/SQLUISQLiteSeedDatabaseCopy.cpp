#include "Database/SQLUISQLiteSeedDatabaseCopy.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
void AppendSQLUISQLiteSeedDatabaseCopyError(
	FSQLUISQLiteSeedDatabaseCopyResult& Result,
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

FString NormalizeSQLUISQLiteSeedDatabaseCopyPath(FString Path)
{
	FPaths::NormalizeFilename(Path);
	return FPaths::ConvertRelativePathToFull(Path);
}

bool AreSQLUISQLiteSeedDatabaseCopyPathsSame(
	const FString& FirstPath,
	const FString& SecondPath)
{
	return FirstPath.Equals(SecondPath, ESearchCase::IgnoreCase);
}

TArray<FString> MakeSQLUISQLiteSeedDatabaseCopyTargetSidecarPaths(
	const FString& DatabasePath)
{
	return {
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};
}

bool DeleteSQLUISQLiteSeedDatabaseCopyPath(
	const FString& Path,
	FSQLUISQLiteSeedDatabaseCopyResult& Result)
{
	if (!FPaths::FileExists(Path))
	{
		return true;
	}

	if (IFileManager::Get().Delete(*Path, false, true, true))
	{
		return true;
	}

	AppendSQLUISQLiteSeedDatabaseCopyError(
		Result,
		FString::Printf(
			TEXT("SQLUI SQLite seed database copy failed: could not remove target file '%s'."),
			*Path));
	return false;
}

bool DeleteSQLUISQLiteSeedDatabaseCopyTargetFiles(
	const FString& TargetDatabasePath,
	FSQLUISQLiteSeedDatabaseCopyResult& Result)
{
	bool bRemoved = DeleteSQLUISQLiteSeedDatabaseCopyPath(TargetDatabasePath, Result);
	for (const FString& SidecarPath : MakeSQLUISQLiteSeedDatabaseCopyTargetSidecarPaths(TargetDatabasePath))
	{
		bRemoved = DeleteSQLUISQLiteSeedDatabaseCopyPath(SidecarPath, Result) && bRemoved;
	}

	return bRemoved;
}
}

FSQLUISQLiteSeedDatabaseCopyResult FSQLUISQLiteSeedDatabaseCopy::CopySeedDatabase(
	const FSQLUISQLiteSeedDatabaseCopyRequest& Request)
{
	FSQLUISQLiteSeedDatabaseCopyResult Result;

	FString SeedDatabasePath = Request.SeedDatabasePath;
	SeedDatabasePath.TrimStartAndEndInline();
	FString TargetDatabasePath = Request.TargetDatabasePath;
	TargetDatabasePath.TrimStartAndEndInline();

	if (!SeedDatabasePath.IsEmpty())
	{
		SeedDatabasePath = NormalizeSQLUISQLiteSeedDatabaseCopyPath(SeedDatabasePath);
	}

	if (!TargetDatabasePath.IsEmpty())
	{
		TargetDatabasePath = NormalizeSQLUISQLiteSeedDatabaseCopyPath(TargetDatabasePath);
	}

	Result.SeedDatabasePath = SeedDatabasePath;
	Result.TargetDatabasePath = TargetDatabasePath;

	if (SeedDatabasePath.IsEmpty())
	{
		AppendSQLUISQLiteSeedDatabaseCopyError(
			Result,
			TEXT("SQLUI SQLite seed database copy failed: seed database path is empty."));
		return Result;
	}

	if (TargetDatabasePath.IsEmpty())
	{
		AppendSQLUISQLiteSeedDatabaseCopyError(
			Result,
			TEXT("SQLUI SQLite seed database copy failed: target database path is empty."));
		return Result;
	}

	Result.bSeedDatabaseFound = FPaths::FileExists(SeedDatabasePath);
	if (!Result.bSeedDatabaseFound)
	{
		AppendSQLUISQLiteSeedDatabaseCopyError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite seed database copy failed: seed database '%s' does not exist."),
				*SeedDatabasePath));
		return Result;
	}

	if (AreSQLUISQLiteSeedDatabaseCopyPathsSame(SeedDatabasePath, TargetDatabasePath))
	{
		AppendSQLUISQLiteSeedDatabaseCopyError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite seed database copy failed: seed and target database paths are the same: '%s'."),
				*SeedDatabasePath));
		return Result;
	}

	Result.bTargetDatabaseAlreadyExisted = FPaths::FileExists(TargetDatabasePath);
	if (Result.bTargetDatabaseAlreadyExisted && !Request.bOverwriteTarget)
	{
		Result.bTargetDatabaseReady = true;
		Result.bSucceeded = true;
		return Result;
	}

	if (Result.bTargetDatabaseAlreadyExisted
		&& !DeleteSQLUISQLiteSeedDatabaseCopyTargetFiles(TargetDatabasePath, Result))
	{
		return Result;
	}

	if (!Result.bTargetDatabaseAlreadyExisted && !Request.bCopyIfTargetMissing)
	{
		AppendSQLUISQLiteSeedDatabaseCopyError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite seed database copy failed: target database '%s' does not exist and copy-if-missing is disabled."),
				*TargetDatabasePath));
		return Result;
	}

	const FString TargetDirectory = FPaths::GetPath(TargetDatabasePath);
	if (!TargetDirectory.IsEmpty() && !FPaths::DirectoryExists(TargetDirectory))
	{
		if (!Request.bCreateTargetDirectory)
		{
			AppendSQLUISQLiteSeedDatabaseCopyError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite seed database copy failed: target directory '%s' does not exist and directory creation is disabled."),
					*TargetDirectory));
			return Result;
		}

		if (!IFileManager::Get().MakeDirectory(*TargetDirectory, true))
		{
			AppendSQLUISQLiteSeedDatabaseCopyError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite seed database copy failed: could not create target directory '%s'."),
					*TargetDirectory));
			return Result;
		}
	}

	if (IFileManager::Get().Copy(*TargetDatabasePath, *SeedDatabasePath, true, true) != COPY_OK)
	{
		AppendSQLUISQLiteSeedDatabaseCopyError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite seed database copy failed: could not copy seed database '%s' to '%s'."),
				*SeedDatabasePath,
				*TargetDatabasePath));
		return Result;
	}

	Result.bTargetDatabaseCopied = true;
	Result.bTargetDatabaseReady = FPaths::FileExists(TargetDatabasePath);
	Result.bSucceeded = Result.bTargetDatabaseReady;
	if (!Result.bTargetDatabaseReady)
	{
		AppendSQLUISQLiteSeedDatabaseCopyError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite seed database copy failed: copied target database '%s' was not found after copy."),
				*TargetDatabasePath));
	}

	return Result;
}
