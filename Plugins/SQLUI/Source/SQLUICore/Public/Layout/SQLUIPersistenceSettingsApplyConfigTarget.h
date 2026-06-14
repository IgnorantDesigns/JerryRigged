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

struct SQLUICORE_API FSQLUIPersistenceSettingsProductionConfigTargetDescriptor
{
	FString SymbolicTargetName;
	FString RelativeConfigPath;
	FString TargetOwnership;
	bool bIsProductionTarget = false;
	bool bIsSmokeOwnedTarget = false;
	bool bCanWrite = false;
	bool bIsImplemented = false;
	bool bWriteEnabled = false;
	bool bWouldUseCommittedConfig = false;
	bool bWouldUseDefaultEngineIni = false;
	bool bWouldUseSavedConfig = false;
	bool bWouldUseUserGlobalEditorSettings = false;
	FString SummaryText;
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
	bool bProductionTargetEnablementRequested = false;
	bool bProductionTargetEnablementAccepted = false;
	bool bWouldAffectRuntimeDefaults = false;
	FString ConfigFilePath;
	FString TargetDescription;
	FString SummaryText;
	FSQLUIPersistenceSettingsProductionConfigTargetDescriptor
		ProductionTargetDescriptor;
	TArray<FSQLUIPersistenceSettingsValidationMessage> Messages;
};

struct SQLUICORE_API FSQLUIPersistenceSettingsApplyProductionTargetEnablement
{
	bool bEnableProductionTarget = false;
	FString RequestDescription;
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

	static FSQLUIPersistenceSettingsApplyConfigTargetResolution ResolveDocumentedProductionTargetStrategy();

	static FSQLUIPersistenceSettingsApplyConfigTargetResolution ResolveDocumentedProductionTargetStrategyWithEnablement(
		const FSQLUIPersistenceSettingsApplyProductionTargetEnablement& Enablement);

	static FSQLUIPersistenceSettingsApplyConfigTargetResolution ResolveFutureProjectUserConfigTarget();

	static FString GetSelectedProductionConfigTargetRelativePath();

	static FSQLUIPersistenceSettingsProductionConfigTargetDescriptor
		DescribeSelectedProductionConfigTarget();
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
