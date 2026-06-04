#pragma once

#include "CoreMinimal.h"

#include "SQLUISQLiteSeedDatabaseCopy.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUISQLiteSeedDatabaseCopyRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString SeedDatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString TargetDatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bCopyIfTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bOverwriteTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bCreateTargetDirectory = true;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUISQLiteSeedDatabaseCopyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSeedDatabaseFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bTargetDatabaseAlreadyExisted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bTargetDatabaseCopied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bTargetDatabaseReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString SeedDatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString TargetDatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString ErrorMessage;
};

/**
 * Explicit file-copy policy for a closed, stable SQLite seed database.
 *
 * This helper does not open SQLite, initialize schema, create repositories, or
 * choose repository backends. It only copies one existing seed DB file into a
 * writable target path when the caller has explicitly requested that policy.
 */
class SQLUICORE_API FSQLUISQLiteSeedDatabaseCopy
{
public:
	static FSQLUISQLiteSeedDatabaseCopyResult CopySeedDatabase(
		const FSQLUISQLiteSeedDatabaseCopyRequest& Request);
};
