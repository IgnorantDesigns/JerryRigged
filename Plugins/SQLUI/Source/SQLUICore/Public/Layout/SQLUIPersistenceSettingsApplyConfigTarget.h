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

enum class ESQLUIPersistenceSettingsApplyConfigTargetKind : uint8
{
	Invalid,
	Unavailable,
	SmokeOwned,
	FutureProjectUserConfig
};

struct SQLUICORE_API FSQLUIPersistenceSettingsApplyConfigTargetResolution
{
	ESQLUIPersistenceSettingsApplyConfigTargetKind TargetKind =
		ESQLUIPersistenceSettingsApplyConfigTargetKind::Invalid;
	bool bCanWrite = false;
	bool bIsProductionRuntimeTarget = false;
	bool bIsSmokeOwnedTarget = false;
	bool bRequiresExplicitTarget = true;
	bool bProductionApplyEnabled = false;
	bool bWouldAffectRuntimeDefaults = false;
	FString ConfigFilePath;
	FString TargetDescription;
	FString SummaryText;
	TArray<FSQLUIPersistenceSettingsValidationMessage> Messages;
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
 * Non-mutating policy/resolver for future persistence settings Apply targets.
 *
 * This makes the target decision explicit while production Apply remains
 * unavailable. Only explicit smoke/test-owned targets can resolve writable;
 * future project/user config targets are represented as unavailable until a
 * later PR scopes and validates them.
 */
class SQLUICORE_API FSQLUIPersistenceSettingsApplyConfigTargetPolicy
{
public:
	static FSQLUIPersistenceSettingsApplyConfigTargetResolution ResolveDefaultRuntimeTarget();

	static FSQLUIPersistenceSettingsApplyConfigTargetResolution ResolveExplicitTarget(
		const FSQLUIPersistenceSettingsApplyConfigTarget& Target);

	static FSQLUIPersistenceSettingsApplyConfigTargetResolution ResolveFutureProjectUserConfigTarget();
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
