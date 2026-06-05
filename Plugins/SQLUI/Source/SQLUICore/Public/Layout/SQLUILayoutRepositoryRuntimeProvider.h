#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepositoryRuntimeIntegration.h"
#include "UObject/Object.h"

#include "SQLUILayoutRepositoryRuntimeProvider.generated.h"

/**
 * Storage-agnostic runtime holder for the active layout repository.
 *
 * The provider owns the repository selected by explicit runtime config and
 * keeps callers on the base repository contract. It does not auto-run at
 * startup, attach widgets, copy seeds, or initialize schema except through the
 * explicit runtime integration request supplied by the caller.
 */
UCLASS(BlueprintType)
class SQLUICORE_API USQLUILayoutRepositoryRuntimeProvider : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool InitializeRepository(const FSQLUILayoutRepositoryRuntimeIntegrationRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool InitializeRepositoryFromRuntimeConfig(
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool InitializeRepositoryFromCommandLine(
		const FString& CommandLine,
		const FSQLUILayoutRepositoryRuntimeConfig& Defaults);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	USQLUILayoutRepository* GetRepository() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool HasRepository() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool WasLastInitializationSuccessful() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	ESQLUILayoutRepositoryBackend GetActiveBackend() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryRuntimeIntegrationResult GetLastIntegrationResult() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	void ResetRepository();

private:
	UPROPERTY(Transient)
	TObjectPtr<USQLUILayoutRepository> ActiveRepository = nullptr;

	UPROPERTY(Transient)
	FSQLUILayoutRepositoryRuntimeIntegrationResult LastIntegrationResult;
};
