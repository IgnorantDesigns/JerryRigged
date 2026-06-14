#include "Layout/SQLUIPersistenceSettingsApplyConfigTarget.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
FString NormalizeSQLUIPersistenceSettingsApplyConfigPath(const FString& Path)
{
	FString NormalizedPath = Path;
	FPaths::NormalizeFilename(NormalizedPath);
	return FPaths::ConvertRelativePathToFull(NormalizedPath);
}

FString GetSQLUIPersistenceSettingsApplySmokeRoot()
{
	FString SmokeRoot = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"));
	FPaths::NormalizeFilename(SmokeRoot);
	SmokeRoot = FPaths::ConvertRelativePathToFull(SmokeRoot);
	if (!SmokeRoot.EndsWith(TEXT("/")))
	{
		SmokeRoot += TEXT("/");
	}
	return SmokeRoot;
}

FString SQLUIPersistenceSettingsBoolText(const bool bValue)
{
	return bValue ? TEXT("true") : TEXT("false");
}

FString SQLUIPersistenceSettingsBackendText(
	const ESQLUILayoutRepositoryBackend Backend)
{
	switch (Backend)
	{
	case ESQLUILayoutRepositoryBackend::Unavailable:
		return TEXT("Unavailable");
	case ESQLUILayoutRepositoryBackend::InMemory:
		return TEXT("InMemory");
	case ESQLUILayoutRepositoryBackend::JsonFile:
		return TEXT("JsonFile");
	case ESQLUILayoutRepositoryBackend::SQLite:
		return TEXT("SQLite");
	default:
		return TEXT("Unknown");
	}
}

FString SanitizeSQLUIPersistenceSettingsConfigValue(FString Value)
{
	Value.ReplaceInline(TEXT("\r"), TEXT(" "));
	Value.ReplaceInline(TEXT("\n"), TEXT(" "));
	return Value;
}

void AddSQLUIPersistenceSettingsConfigWriteMessage(
	FSQLUIPersistenceSettingsApplyConfigWriteResult& Result,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity,
	const FString& Message,
	const FString& DetailText = FString())
{
	FSQLUIPersistenceSettingsValidationMessage ConfigWriteMessage;
	ConfigWriteMessage.Severity = Severity;
	ConfigWriteMessage.Message = Message;
	ConfigWriteMessage.DetailText = DetailText;
	Result.Messages.Add(MoveTemp(ConfigWriteMessage));
}

void AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
	FSQLUIPersistenceSettingsApplyConfigTargetResolution& Result,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity,
	const FString& Message,
	const FString& DetailText = FString())
{
	FSQLUIPersistenceSettingsValidationMessage PolicyMessage;
	PolicyMessage.Severity = Severity;
	PolicyMessage.Message = Message;
	PolicyMessage.DetailText = DetailText;
	Result.Messages.Add(MoveTemp(PolicyMessage));
}

bool IsSQLUIPersistenceSettingsSmokeConfigTargetPath(
	const FString& NormalizedConfigPath)
{
	if (NormalizedConfigPath.IsEmpty())
	{
		return false;
	}

	const FString SmokeRoot = GetSQLUIPersistenceSettingsApplySmokeRoot();
	return NormalizedConfigPath.StartsWith(SmokeRoot, ESearchCase::IgnoreCase)
		&& FPaths::GetExtension(NormalizedConfigPath, false)
			.Equals(TEXT("ini"), ESearchCase::IgnoreCase);
}

bool ValidateSQLUIPersistenceSettingsApplyConfigTarget(
	const FSQLUIPersistenceSettingsApplyConfigTarget& Target,
	FSQLUIPersistenceSettingsApplyConfigWriteResult& Result)
{
	const FSQLUIPersistenceSettingsApplyConfigTargetResolution Resolution =
		FSQLUIPersistenceSettingsApplyConfigTargetPolicy::ResolveExplicitTarget(
			Target);
	Result.TargetDescription = Resolution.TargetDescription;
	Result.ConfigFilePath = Resolution.ConfigFilePath;
	Result.bUsedSmokeOwnedTarget = Resolution.bIsSmokeOwnedTarget;
	Result.bWouldAffectRuntimeDefaults = Resolution.bWouldAffectRuntimeDefaults;
	Result.Messages.Append(Resolution.Messages);

	if (!Resolution.bCanWrite)
	{
		Result.SummaryText = Resolution.SummaryText;
		return false;
	}

	return true;
}

FString GetSQLUIPersistenceSettingsSelectedProductionRelativePath()
{
	return TEXT("Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini");
}

FString MakeSQLUIPersistenceSettingsSmokeConfigText(
	const FSQLUIPersistenceSettingsDraft& Draft,
	const FSQLUIPersistenceSettingsApplyPreviewResult& ApplyPreview,
	const FSQLUIPersistenceSettingsApplyConfigTarget& Target)
{
	const FSQLUILayoutRepositoryRuntimeConfig& Config =
		Draft.PendingRuntimeConfig;

	FString Text;
	Text += TEXT("[SQLUI.PersistenceSettingsApplySmokeTarget]\n");
	Text += FString::Printf(
		TEXT("TargetDescription=%s\n"),
		*SanitizeSQLUIPersistenceSettingsConfigValue(Target.TargetDescription));
	Text += TEXT("bSmokeOwnedTarget=true\n");
	Text += TEXT("bRuntimeDefaultsUntouched=true\n");
	Text += TEXT("bProviderLifecycleUntouched=true\n");
	Text += TEXT("bRepositoryLifecycleUntouched=true\n");
	Text += TEXT("bDidCreateDatabaseFiles=false\n");
	Text += TEXT("bDidOpenDatabaseForWriting=false\n");
	Text += TEXT("bDidRunMigrations=false\n");
	Text += TEXT("bDidCopySeedDatabase=false\n");
	Text += TEXT("bDidDeleteFiles=false\n");
	Text += FString::Printf(
		TEXT("bRequiresRestartOrReinitialize=%s\n"),
		*SQLUIPersistenceSettingsBoolText(
			ApplyPreview.bRequiresRestartOrReinitialize));
	Text += FString::Printf(
		TEXT("Backend=%s\n"),
		*SQLUIPersistenceSettingsBackendText(Config.Backend));
	Text += FString::Printf(
		TEXT("JsonFileBaseDirectory=%s\n"),
		*SanitizeSQLUIPersistenceSettingsConfigValue(
			Config.JsonFileBaseDirectory));
	Text += FString::Printf(
		TEXT("SQLiteDatabasePath=%s\n"),
		*SanitizeSQLUIPersistenceSettingsConfigValue(
			Config.SQLiteDatabasePath));
	Text += FString::Printf(
		TEXT("SQLiteSeedDatabasePath=%s\n"),
		*SanitizeSQLUIPersistenceSettingsConfigValue(
			Config.SQLiteSeedDatabasePath));
	Text += FString::Printf(
		TEXT("bSQLiteReadOnly=%s\n"),
		*SQLUIPersistenceSettingsBoolText(Config.bSQLiteReadOnly));
	Text += FString::Printf(
		TEXT("bSQLiteInitializeSchemaIfMissing=%s\n"),
		*SQLUIPersistenceSettingsBoolText(
			Config.bSQLiteInitializeSchemaIfMissing));
	Text += FString::Printf(
		TEXT("bSQLiteCreateDatabaseIfMissing=%s\n"),
		*SQLUIPersistenceSettingsBoolText(
			Config.bSQLiteCreateDatabaseIfMissing));
	Text += FString::Printf(
		TEXT("bSQLiteRunCallbackOperationsAsync=%s\n"),
		*SQLUIPersistenceSettingsBoolText(
			Config.bSQLiteRunCallbackOperationsAsync));
	Text += FString::Printf(
		TEXT("bSQLiteCopySeedIfMissing=%s\n"),
		*SQLUIPersistenceSettingsBoolText(Config.bSQLiteCopySeedIfMissing));
	Text += FString::Printf(
		TEXT("bSQLiteOverwriteDatabaseFromSeed=%s\n"),
		*SQLUIPersistenceSettingsBoolText(
			Config.bSQLiteOverwriteDatabaseFromSeed));
	Text += FString::Printf(
		TEXT("bProviderAutoInitialize=%s\n"),
		*SQLUIPersistenceSettingsBoolText(
			Draft.bPendingProviderAutoInitialize));
	return Text;
}
}

FSQLUIPersistenceSettingsApplyConfigTargetResolution
FSQLUIPersistenceSettingsApplyConfigTargetPolicy::ResolveDefaultRuntimeTarget()
{
	FSQLUIPersistenceSettingsApplyConfigTargetResolution Result;
	Result.TargetKind =
		ESQLUIPersistenceSettingsApplyConfigTargetKind::Unavailable;
	Result.bCanWrite = false;
	Result.bIsProductionRuntimeTarget = true;
	Result.bIsSmokeOwnedTarget = false;
	Result.bRequiresExplicitTarget = true;
	Result.bProductionApplyEnabled = false;
	Result.bWouldAffectRuntimeDefaults = false;
	Result.TargetDescription =
		TEXT("Default/runtime apply target");
	Result.SummaryText =
		TEXT("SQLUI persistence settings default/runtime Apply target is unavailable. No config can be written.");
	AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
		Result,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
		TEXT("Production persistence settings Apply target is not enabled yet."),
		TEXT("The resolver intentionally does not infer DefaultEngine.ini, Saved/Config, user editor settings, or any other real writable config target."));
	return Result;
}

FSQLUIPersistenceSettingsApplyConfigTargetResolution
FSQLUIPersistenceSettingsApplyConfigTargetPolicy::ResolveExplicitTarget(
	const FSQLUIPersistenceSettingsApplyConfigTarget& Target)
{
	if (!Target.bIsExplicitTestTarget)
	{
		FSQLUIPersistenceSettingsApplyConfigTargetResolution Result =
			ResolveDefaultRuntimeTarget();
		if (!Target.TargetDescription.IsEmpty())
		{
			Result.TargetDescription = Target.TargetDescription;
		}
		return Result;
	}

	FSQLUIPersistenceSettingsApplyConfigTargetResolution Result;
	Result.TargetDescription = Target.TargetDescription;
	Result.ConfigFilePath =
		NormalizeSQLUIPersistenceSettingsApplyConfigPath(Target.ConfigFilePath);
	Result.bRequiresExplicitTarget = true;
	Result.bProductionApplyEnabled = false;

	if (!Target.bAllowWrites)
	{
		Result.TargetKind =
			ESQLUIPersistenceSettingsApplyConfigTargetKind::Invalid;
		Result.bCanWrite = false;
		Result.bIsProductionRuntimeTarget = false;
		Result.bIsSmokeOwnedTarget = false;
		Result.bWouldAffectRuntimeDefaults = false;
		Result.SummaryText =
			TEXT("SQLUI persistence settings smoke-owned Apply target is invalid. No config can be written.");
		AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("Explicit smoke-owned config target did not allow writes."),
			TEXT("Smoke/test code must opt into the temporary target explicitly."));
		return Result;
	}

	if (!IsSQLUIPersistenceSettingsSmokeConfigTargetPath(Result.ConfigFilePath))
	{
		Result.TargetKind =
			ESQLUIPersistenceSettingsApplyConfigTargetKind::Invalid;
		Result.bCanWrite = false;
		Result.bIsProductionRuntimeTarget = false;
		Result.bIsSmokeOwnedTarget = false;
		Result.bWouldAffectRuntimeDefaults = true;
		Result.SummaryText =
			TEXT("SQLUI persistence settings explicit Apply target was rejected. No config can be written.");
		AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("Explicit config target is not inside Saved/SQLUI/SmokeTests or is not an ini file."),
			Result.ConfigFilePath);
		return Result;
	}

	Result.TargetKind =
		ESQLUIPersistenceSettingsApplyConfigTargetKind::SmokeOwned;
	Result.bCanWrite = true;
	Result.bIsProductionRuntimeTarget = false;
	Result.bIsSmokeOwnedTarget = true;
	Result.bWouldAffectRuntimeDefaults = false;
	Result.SummaryText =
		TEXT("SQLUI persistence settings Apply target resolved to an explicit smoke-owned target.");
	AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
		Result,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
		TEXT("Explicit smoke-owned config target can be used for isolated smoke coverage only."),
		TEXT("This is not a production/user settings Apply target and does not enable runtime config writes."));
	return Result;
}

FString
FSQLUIPersistenceSettingsApplyConfigTargetPolicy::GetSelectedProductionConfigTargetRelativePath()
{
	return GetSQLUIPersistenceSettingsSelectedProductionRelativePath();
}

FSQLUIPersistenceSettingsProductionConfigTargetDescriptor
FSQLUIPersistenceSettingsApplyConfigTargetPolicy::DescribeSelectedProductionConfigTarget()
{
	FSQLUIPersistenceSettingsProductionConfigTargetDescriptor Descriptor;
	Descriptor.SymbolicTargetName =
		TEXT("SQLUI.PersistenceSettings.RuntimeSettings");
	Descriptor.RelativeConfigPath =
		GetSelectedProductionConfigTargetRelativePath();
	Descriptor.TargetOwnership = TEXT("SQLUICore/plugin-managed");
	Descriptor.bIsProductionTarget = true;
	Descriptor.bIsSmokeOwnedTarget = false;
	Descriptor.bCanWrite = false;
	Descriptor.bIsImplemented = false;
	Descriptor.bWriteEnabled = false;
	Descriptor.bWouldUseCommittedConfig = false;
	Descriptor.bWouldUseDefaultEngineIni = false;
	Descriptor.bWouldUseSavedConfig = false;
	Descriptor.bWouldUseUserGlobalEditorSettings = false;
	Descriptor.SummaryText =
		TEXT("Selected SQLUI persistence settings production target is descriptor-only and write-disabled.");
	return Descriptor;
}

FSQLUIPersistenceSettingsApplyConfigTargetResolution
FSQLUIPersistenceSettingsApplyConfigTargetPolicy::ResolveDocumentedProductionTargetStrategy()
{
	FSQLUIPersistenceSettingsApplyConfigTargetResolution Result;
	Result.TargetKind =
		ESQLUIPersistenceSettingsApplyConfigTargetKind::FutureProjectUserConfig;
	Result.bCanWrite = false;
	Result.bIsProductionRuntimeTarget = true;
	Result.bIsSmokeOwnedTarget = false;
	Result.bRequiresExplicitTarget = true;
	Result.bProductionApplyEnabled = false;
	Result.bWouldAffectRuntimeDefaults = false;
	Result.TargetDescription =
		TEXT("Documented production config target strategy");
	Result.ProductionTargetDescriptor =
		DescribeSelectedProductionConfigTarget();
	Result.SummaryText = FString::Printf(
		TEXT("SQLUI persistence settings documented production Apply target strategy selects %s as a descriptor only. It remains unavailable for writes until a future implementation. No config can be written."),
		*Result.ProductionTargetDescriptor.RelativeConfigPath);
	AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
		Result,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
		TEXT("Documented production persistence settings config target descriptor is not write-enabled."),
		Result.ProductionTargetDescriptor.RelativeConfigPath);
	AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
		Result,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
		TEXT("Selected production persistence settings config target remains unavailable."),
		TEXT("The descriptor does not create directories, write files, use DefaultEngine.ini, use Saved/Config, use user/global editor settings, or enable production Apply."));
	return Result;
}

FSQLUIPersistenceSettingsApplyConfigTargetResolution
FSQLUIPersistenceSettingsApplyConfigTargetPolicy::ResolveDocumentedProductionTargetStrategyWithEnablement(
	const FSQLUIPersistenceSettingsApplyProductionTargetEnablement& Enablement)
{
	FSQLUIPersistenceSettingsApplyConfigTargetResolution Result =
		ResolveDocumentedProductionTargetStrategy();
	Result.bProductionTargetEnablementRequested =
		Enablement.bEnableProductionTarget;
	Result.bProductionTargetEnablementAccepted = false;

	if (!Enablement.bEnableProductionTarget)
	{
		AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("Production persistence settings config target enablement was not requested."),
			TEXT("The documented production target remains unavailable and non-writable."));
		return Result;
	}

	Result.SummaryText =
		TEXT("SQLUI persistence settings documented production Apply target enablement was requested for the selected descriptor, but it remains write-disabled and unimplemented. No config can be written.");
	AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
		Result,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
		TEXT("Production persistence settings config target enablement remains blocked."),
		TEXT("This policy result records the explicit request only; it does not create directories, write files, or enable production Apply."));
	if (!Enablement.RequestDescription.IsEmpty())
	{
		AddSQLUIPersistenceSettingsConfigTargetPolicyMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("Production persistence settings config target enablement request description."),
			Enablement.RequestDescription);
	}
	return Result;
}

FSQLUIPersistenceSettingsApplyConfigTargetResolution
FSQLUIPersistenceSettingsApplyConfigTargetPolicy::ResolveFutureProjectUserConfigTarget()
{
	return ResolveDocumentedProductionTargetStrategy();
}

FSQLUIPersistenceSettingsApplyConfigTarget
FSQLUIPersistenceSettingsApplyConfigTargetWriter::MakeUnavailableRuntimeTarget()
{
	FSQLUIPersistenceSettingsApplyConfigTarget Target;
	Target.TargetDescription =
		TEXT("Default/runtime apply target is unavailable in this implementation slice.");
	return Target;
}

FSQLUIPersistenceSettingsApplyConfigTarget
FSQLUIPersistenceSettingsApplyConfigTargetWriter::MakeExplicitSmokeTestTarget(
	const FString& ConfigFilePath,
	const FString& TargetDescription)
{
	FSQLUIPersistenceSettingsApplyConfigTarget Target;
	Target.bIsExplicitTestTarget = true;
	Target.bAllowWrites = true;
	Target.ConfigFilePath = ConfigFilePath;
	Target.TargetDescription = TargetDescription;
	return Target;
}

FSQLUIPersistenceSettingsApplyConfigWriteResult
FSQLUIPersistenceSettingsApplyConfigTargetWriter::WriteToConfigTarget(
	const FSQLUIPersistenceSettingsApplyRequest& Request,
	const FSQLUIPersistenceSettingsApplyConfigTarget& Target)
{
	FSQLUIPersistenceSettingsApplyConfigWriteResult Result;
	if (!ValidateSQLUIPersistenceSettingsApplyConfigTarget(Target, Result))
	{
		return Result;
	}

	const FSQLUIPersistenceSettingsApplyPreviewResult ApplyPreview =
		USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
			Request.Draft);
	Result.Messages.Append(ApplyPreview.Messages);
	Result.bRequiresRestartOrReinitialize =
		ApplyPreview.bRequiresRestartOrReinitialize;

	if (!ApplyPreview.bIsValid)
	{
		Result.SummaryText =
			TEXT("SQLUI persistence settings config write was blocked by validation. No config was written.");
		AddSQLUIPersistenceSettingsConfigWriteMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("Draft validation failed before the smoke-owned config target was written."),
			TEXT("The smoke-owned target was left untouched."));
		return Result;
	}

	if (!ApplyPreview.bHasChanges)
	{
		Result.bSucceeded = true;
		Result.SummaryText =
			TEXT("SQLUI persistence settings config write found no changes. No config was written.");
		AddSQLUIPersistenceSettingsConfigWriteMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("No pending persistence settings changes were written."),
			TEXT("The smoke-owned target was left untouched."));
		return Result;
	}

	const FString Directory = FPaths::GetPath(Result.ConfigFilePath);
	const bool bDirectoryExisted = IFileManager::Get().DirectoryExists(*Directory);
	if (!bDirectoryExisted
		&& !IFileManager::Get().MakeDirectory(*Directory, true))
	{
		Result.SummaryText =
			TEXT("SQLUI persistence settings config write could not create the smoke-owned target directory.");
		AddSQLUIPersistenceSettingsConfigWriteMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("Could not create the smoke-owned config target directory."),
			Directory);
		return Result;
	}
	Result.bDidCreateDirectories = !bDirectoryExisted;

	const FString ConfigText =
		MakeSQLUIPersistenceSettingsSmokeConfigText(
			Request.Draft,
			ApplyPreview,
			Target);
	if (!FFileHelper::SaveStringToFile(
			ConfigText,
			*Result.ConfigFilePath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		Result.SummaryText =
			TEXT("SQLUI persistence settings config write failed while writing the smoke-owned target.");
		AddSQLUIPersistenceSettingsConfigWriteMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("Could not write the smoke-owned config target file."),
			Result.ConfigFilePath);
		return Result;
	}

	Result.bSucceeded = true;
	Result.bDidWriteConfig = true;
	Result.bDidChangeSettings = true;
	Result.SummaryText =
		TEXT("SQLUI persistence settings were written only to an explicit smoke-owned config target.");
	AddSQLUIPersistenceSettingsConfigWriteMessage(
		Result,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
		TEXT("Smoke-owned persistence settings config target was written."),
		TEXT("Runtime/default config, provider lifecycle, repositories, databases, migrations, seed copy, and destructive actions were not touched."));
	return Result;
}
