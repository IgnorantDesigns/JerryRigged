#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepositoryRuntimeIntegration.h"
#include "UObject/Object.h"

#include "SQLUILayoutRepositoryRuntimeSettings.generated.h"

/**
 * Config-backed runtime repository startup settings.
 *
 * These settings are intentionally passive by default. They do not add a
 * Project Settings UI surface, make SQLite the default backend, or create
 * repositories until the runtime subsystem is explicitly asked to auto-init.
 */
UCLASS(Config = Game, DefaultConfig)
class SQLUICORE_API USQLUILayoutRepositoryRuntimeSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bAutoInitializeProvider = false;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryRuntimeConfig RuntimeConfig;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bRunSQLiteSeedCopyPolicy = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bTreatSeedCopyFailureAsFatal = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bAllowCommandLineOverrides = true;
};

/**
 * Small policy helper for resolving runtime settings without coupling callers
 * to the subsystem. The helper only produces plain request data.
 */
class SQLUICORE_API FSQLUILayoutRepositoryRuntimeSettingsPolicy
{
public:
	static bool ShouldAutoInitializeProvider(
		const USQLUILayoutRepositoryRuntimeSettings* Settings,
		const TCHAR* CommandLine);

	static FSQLUILayoutRepositoryRuntimeConfig MakeRuntimeConfigFromSettingsAndCommandLine(
		const USQLUILayoutRepositoryRuntimeSettings* Settings,
		const TCHAR* CommandLine);

	static FSQLUILayoutRepositoryRuntimeIntegrationRequest MakeRuntimeIntegrationRequestFromSettingsAndCommandLine(
		const USQLUILayoutRepositoryRuntimeSettings* Settings,
		const TCHAR* CommandLine);
};
