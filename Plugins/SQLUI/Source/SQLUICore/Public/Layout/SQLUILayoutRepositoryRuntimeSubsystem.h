#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepositoryRuntimeProvider.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "SQLUILayoutRepositoryRuntimeSubsystem.generated.h"

/**
 * Passive app-level holder for the active SQLUI layout repository provider.
 *
 * The subsystem is created by Unreal with the GameInstance, but it does not
 * initialize a repository unless explicit caller code does so or the
 * -SQLUILayoutRepositoryProviderAutoInit command-line flag is present.
 */
UCLASS()
class SQLUICORE_API USQLUILayoutRepositoryRuntimeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	USQLUILayoutRepositoryRuntimeProvider* GetProvider() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	USQLUILayoutRepositoryRuntimeProvider* GetOrCreateProvider();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	USQLUILayoutRepository* GetRepository() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool HasRepository() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool WasAutoInitializationRequested() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool WasAutoInitializationSuccessful() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool InitializeRepositoryFromRuntimeConfig(
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	bool InitializeRepositoryFromCommandLine(
		const FString& CommandLine,
		const FSQLUILayoutRepositoryRuntimeConfig& Defaults);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	void ResetRepository();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	ESQLUILayoutRepositoryBackend GetActiveBackend() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryRuntimeIntegrationResult GetLastIntegrationResult() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<USQLUILayoutRepositoryRuntimeProvider> Provider = nullptr;

	UPROPERTY(Transient)
	bool bAutoInitializationRequested = false;

	UPROPERTY(Transient)
	bool bAutoInitializationSuccessful = false;
};
