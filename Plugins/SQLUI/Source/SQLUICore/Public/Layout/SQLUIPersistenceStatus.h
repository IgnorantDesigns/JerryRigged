#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"

#include "SQLUIPersistenceStatus.generated.h"

class USQLUILayoutRepositoryRuntimeProvider;

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceStatusSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	ESQLUILayoutRepositoryBackend ConfiguredBackend = ESQLUILayoutRepositoryBackend::InMemory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FString ConfiguredBackendName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	ESQLUILayoutRepositoryBackend ActiveBackend = ESQLUILayoutRepositoryBackend::Unavailable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FString ActiveBackendName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bProviderInitialized = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bRepositoryActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bSQLiteDatabasePathResolved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FString ResolvedSQLiteDatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bSQLiteDatabaseExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	int64 SQLiteDatabaseSizeBytes = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bSQLiteJournalExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bSQLiteWalExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bSQLiteShmExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bSQLiteSidecarsPresent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FString SQLiteSidecarSummary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bMigrationStatusChecked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bMigrationStatusSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bSQLiteSchemaObjectsReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	bool bSQLiteHasPendingMigrations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FString LatestKnownMigrationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FString LatestAppliedMigrationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FString StatusText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FString WarningText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FString ErrorMessage;
};

/**
 * Read-only persistence status surface for future settings/admin UI.
 *
 * These functions inspect config/provider state and file status only. They do
 * not initialize providers, create repositories, create databases, run
 * migrations, copy seed databases, reset databases, or delete files.
 */
UCLASS(BlueprintType)
class SQLUICORE_API USQLUIPersistenceStatusLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence", meta = (WorldContext = "WorldContextObject"))
	static FSQLUIPersistenceStatusSnapshot GetPersistenceStatus(
		UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence", meta = (WorldContext = "WorldContextObject"))
	static FSQLUIPersistenceStatusSnapshot GetPersistenceStatusFromRuntimeConfig(
		UObject* WorldContextObject,
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence")
	static FSQLUIPersistenceStatusSnapshot GetPersistenceStatusForProvider(
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig,
		USQLUILayoutRepositoryRuntimeProvider* Provider);
};
