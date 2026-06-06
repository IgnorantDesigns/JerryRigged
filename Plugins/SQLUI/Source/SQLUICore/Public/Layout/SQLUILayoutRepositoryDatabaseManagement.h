#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"

#include "SQLUILayoutRepositoryDatabaseManagement.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryDatabaseStatusRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryRuntimeConfig RuntimeConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bIncludeSidecars = true;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryDatabaseStatusResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bBackendIsSQLite = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bDatabasePathResolved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bDatabaseExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bJournalExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bWalExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bShmExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bAnyDatabaseFileExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	int64 DatabaseFileSizeBytes = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString JournalPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString WalPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString ShmPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryDatabaseResetRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryRuntimeConfig RuntimeConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bRemoveSidecars = true;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryDatabaseResetResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bBackendIsSQLite = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bDatabasePathResolved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bDatabaseRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bJournalRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bWalRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bShmRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bAllDatabaseFilesAbsent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString ErrorMessage;
};

/**
 * SQLUICore-owned policy helper for inspecting and resetting the configured
 * SQLite layout repository database path. This helper does not open SQLite,
 * create databases, initialize schema, seed data, or select repositories.
 */
class SQLUICORE_API FSQLUILayoutRepositoryDatabaseManagement
{
public:
	static FSQLUILayoutRepositoryDatabaseStatusResult GetStatus(
		const FSQLUILayoutRepositoryDatabaseStatusRequest& Request);

	static FSQLUILayoutRepositoryDatabaseResetResult ResetDatabase(
		const FSQLUILayoutRepositoryDatabaseResetRequest& Request);
};
