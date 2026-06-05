#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepository.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"

#include "SQLUILayoutRepositoryRuntimeIntegration.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryRuntimeIntegrationRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryRuntimeConfig RuntimeConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bRunSQLiteSeedCopyPolicy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bTreatSeedCopyFailureAsFatal = true;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryRuntimeIntegrationResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bRepositoryCreated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bBackendUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSeedCopyRequested = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSeedCopySucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSeedCopySkipped = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	ESQLUILayoutRepositoryBackend Backend = ESQLUILayoutRepositoryBackend::Unavailable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString SQLiteDatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString SQLiteSeedDatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "SQLUI|Layout Repository")
	TObjectPtr<USQLUILayoutRepository> Repository = nullptr;
};

/**
 * SQLUICore runtime startup helper for explicit repository selection.
 *
 * This boundary combines resolved runtime config, optional closed-file SQLite
 * seed-copy policy, and repository factory creation. It does not run schema
 * initialization itself, make SQLite the default, or attach repositories to
 * widgets; callers own when to invoke it.
 */
class SQLUICORE_API FSQLUILayoutRepositoryRuntimeIntegration
{
public:
	static FSQLUILayoutRepositoryRuntimeIntegrationResult CreateRepository(
		UObject* Outer,
		const FSQLUILayoutRepositoryRuntimeIntegrationRequest& Request);

	static FSQLUILayoutRepositoryRuntimeIntegrationResult CreateRepositoryFromCommandLine(
		UObject* Outer,
		const TCHAR* CommandLine,
		const FSQLUILayoutRepositoryRuntimeConfig& Defaults);
};
