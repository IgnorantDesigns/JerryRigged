#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUIPersistenceSettingsDraft.h"

struct SQLUICORE_API FSQLUIPersistenceSettingsApplyConfigTarget
{
	bool bIsExplicitTestTarget = false;
	bool bAllowWrites = false;
	FString ConfigFilePath;
	FString TargetDescription;
};

struct SQLUICORE_API FSQLUIPersistenceSettingsApplyConfigWriteResult
{
	bool bSucceeded = false;
	bool bDidWriteConfig = false;
	bool bDidChangeSettings = false;
	bool bUsedSmokeOwnedTarget = false;
	bool bWouldAffectRuntimeDefaults = false;
	bool bDidCreateDirectories = false;
	bool bDidInitializeProvider = false;
	bool bDidInitializeRepository = false;
	bool bDidCreateDatabaseFiles = false;
	bool bDidOpenDatabaseForWriting = false;
	bool bDidRunMigrations = false;
	bool bDidCopySeedDatabase = false;
	bool bDidDeleteFiles = false;
	bool bRequiresRestartOrReinitialize = false;
	FString ConfigFilePath;
	FString TargetDescription;
	FString SummaryText;
	TArray<FSQLUIPersistenceSettingsValidationMessage> Messages;
};

/**
 * Explicit config-write target scaffold for future persistence Apply work.
 *
 * The default production Apply entrypoint remains unavailable. This helper
 * only writes to explicitly marked smoke/test-owned config targets under
 * Saved/SQLUI/SmokeTests so smoke tests can prove config serialization without
 * touching project defaults, user config, providers, repositories, SQLite
 * databases, migrations, seeds, or destructive actions.
 */
class SQLUICORE_API FSQLUIPersistenceSettingsApplyConfigTargetWriter
{
public:
	static FSQLUIPersistenceSettingsApplyConfigTarget MakeUnavailableRuntimeTarget();

	static FSQLUIPersistenceSettingsApplyConfigTarget MakeExplicitSmokeTestTarget(
		const FString& ConfigFilePath,
		const FString& TargetDescription);

	static FSQLUIPersistenceSettingsApplyConfigWriteResult WriteToConfigTarget(
		const FSQLUIPersistenceSettingsApplyRequest& Request,
		const FSQLUIPersistenceSettingsApplyConfigTarget& Target);
};
