#include "Layout/SQLUIPersistenceSettingsDraft.h"

#include "Database/SQLUISQLiteAvailability.h"
#include "Layout/SQLUILayoutRepositoryRuntimeSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

namespace
{
FString SQLUIPersistenceSettingsBackendName(
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

bool IsSQLUIPersistenceSettingsSelectableBackend(
	const ESQLUILayoutRepositoryBackend Backend)
{
	switch (Backend)
	{
	case ESQLUILayoutRepositoryBackend::InMemory:
	case ESQLUILayoutRepositoryBackend::JsonFile:
	case ESQLUILayoutRepositoryBackend::SQLite:
		return true;
	case ESQLUILayoutRepositoryBackend::Unavailable:
	default:
		return false;
	}
}

bool DoesSQLUIPersistenceSettingsPathContainControlCharacters(
	const FString& Path)
{
	for (int32 Index = 0; Index < Path.Len(); ++Index)
	{
		const TCHAR Character = Path[Index];
		if (Character == TEXT('\r')
			|| Character == TEXT('\n')
			|| Character == TEXT('\t'))
		{
			return true;
		}
	}

	return false;
}

FString NormalizeSQLUIPersistenceSettingsComparablePath(const FString& Path)
{
	return FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(Path);
}

FString TrimSQLUIPersistenceSettingsString(FString Value)
{
	Value.TrimStartAndEndInline();
	return Value;
}

void AddSQLUIPersistenceSettingsValidationMessage(
	FSQLUIPersistenceSettingsValidationResult& Result,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity,
	const FString& Message,
	const FString& DetailText = FString())
{
	FSQLUIPersistenceSettingsValidationMessage ValidationMessage;
	ValidationMessage.Severity = Severity;
	ValidationMessage.Message = Message;
	ValidationMessage.DetailText = DetailText;
	Result.Messages.Add(MoveTemp(ValidationMessage));

	if (Severity == ESQLUIPersistenceSettingsValidationMessageSeverity::Error)
	{
		Result.bIsValid = false;
	}
}

void AddSQLUIPersistenceSettingsApplyPreviewMessage(
	FSQLUIPersistenceSettingsApplyPreviewResult& Result,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity,
	const FString& Message,
	const FString& DetailText = FString())
{
	FSQLUIPersistenceSettingsValidationMessage PreviewMessage;
	PreviewMessage.Severity = Severity;
	PreviewMessage.Message = Message;
	PreviewMessage.DetailText = DetailText;
	Result.Messages.Add(MoveTemp(PreviewMessage));
}

void AddSQLUIPersistenceSettingsContractMessage(
	TArray<FSQLUIPersistenceSettingsValidationMessage>& Messages,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity,
	const FString& Message,
	const FString& DetailText = FString())
{
	FSQLUIPersistenceSettingsValidationMessage ContractMessage;
	ContractMessage.Severity = Severity;
	ContractMessage.Message = Message;
	ContractMessage.DetailText = DetailText;
	Messages.Add(MoveTemp(ContractMessage));
}

void AddSQLUIPersistenceSettingsApplyResultMessage(
	FSQLUIPersistenceSettingsApplyResult& Result,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity,
	const FString& Message,
	const FString& DetailText = FString())
{
	FSQLUIPersistenceSettingsValidationMessage ApplyMessage;
	ApplyMessage.Severity = Severity;
	ApplyMessage.Message = Message;
	ApplyMessage.DetailText = DetailText;
	Result.Messages.Add(MoveTemp(ApplyMessage));
}

bool DoesSQLUIPersistenceSettingsMessageListHaveSeverity(
	const TArray<FSQLUIPersistenceSettingsValidationMessage>& Messages,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity)
{
	for (const FSQLUIPersistenceSettingsValidationMessage& Message : Messages)
	{
		if (Message.Severity == Severity)
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUIPersistenceSettingsValidationHaveErrors(
	const FSQLUIPersistenceSettingsValidationResult& Result)
{
	return DoesSQLUIPersistenceSettingsMessageListHaveSeverity(
		Result.Messages,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Error);
}

bool AreSQLUIPersistenceSettingsStringsEquivalent(
	const FString& First,
	const FString& Second)
{
	return TrimSQLUIPersistenceSettingsString(First)
		.Equals(TrimSQLUIPersistenceSettingsString(Second), ESearchCase::CaseSensitive);
}

bool AreSQLUIPersistenceSettingsPathsEquivalent(
	const FString& First,
	const FString& Second)
{
	return NormalizeSQLUIPersistenceSettingsComparablePath(First)
		.Equals(
			NormalizeSQLUIPersistenceSettingsComparablePath(Second),
			ESearchCase::CaseSensitive);
}

bool AreSQLUIPersistenceSettingsSeedPathsEquivalent(
	const FString& First,
	const FString& Second)
{
	return FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteSeedDatabasePath(First)
		.Equals(
			FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteSeedDatabasePath(Second),
			ESearchCase::CaseSensitive);
}

bool WouldSQLUIPersistenceSettingsChangeJsonFileConfig(
	const FSQLUILayoutRepositoryRuntimeConfig& Current,
	const FSQLUILayoutRepositoryRuntimeConfig& Pending)
{
	return !AreSQLUIPersistenceSettingsStringsEquivalent(
		Current.JsonFileBaseDirectory,
		Pending.JsonFileBaseDirectory);
}

bool WouldSQLUIPersistenceSettingsChangeSQLiteConfig(
	const FSQLUILayoutRepositoryRuntimeConfig& Current,
	const FSQLUILayoutRepositoryRuntimeConfig& Pending)
{
	return !AreSQLUIPersistenceSettingsPathsEquivalent(
			Current.SQLiteDatabasePath,
			Pending.SQLiteDatabasePath)
		|| !AreSQLUIPersistenceSettingsSeedPathsEquivalent(
			Current.SQLiteSeedDatabasePath,
			Pending.SQLiteSeedDatabasePath)
		|| Current.bSQLiteReadOnly != Pending.bSQLiteReadOnly
		|| Current.bSQLiteInitializeSchemaIfMissing
			!= Pending.bSQLiteInitializeSchemaIfMissing
		|| Current.bSQLiteCreateDatabaseIfMissing
			!= Pending.bSQLiteCreateDatabaseIfMissing
		|| Current.bSQLiteRunCallbackOperationsAsync
			!= Pending.bSQLiteRunCallbackOperationsAsync
		|| Current.bSQLiteCopySeedIfMissing
			!= Pending.bSQLiteCopySeedIfMissing
		|| Current.bSQLiteOverwriteDatabaseFromSeed
			!= Pending.bSQLiteOverwriteDatabaseFromSeed;
}

bool DoesSQLUIPersistenceSettingsDraftHavePendingChanges(
	const FSQLUIPersistenceSettingsDraft& Draft)
{
	return Draft.CurrentRuntimeConfig.Backend != Draft.PendingRuntimeConfig.Backend
		|| WouldSQLUIPersistenceSettingsChangeJsonFileConfig(
			Draft.CurrentRuntimeConfig,
			Draft.PendingRuntimeConfig)
		|| WouldSQLUIPersistenceSettingsChangeSQLiteConfig(
			Draft.CurrentRuntimeConfig,
			Draft.PendingRuntimeConfig)
		|| Draft.bCurrentProviderAutoInitialize
			!= Draft.bPendingProviderAutoInitialize;
}

void ValidateSQLUIPersistenceSettingsBackend(
	const FSQLUIPersistenceSettingsDraft& Draft,
	FSQLUIPersistenceSettingsValidationResult& Result)
{
	const ESQLUILayoutRepositoryBackend Backend =
		Draft.PendingRuntimeConfig.Backend;

	if (!IsSQLUIPersistenceSettingsSelectableBackend(Backend))
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			FString::Printf(
				TEXT("'%s' is not a selectable SQLUI persistence backend."),
				*SQLUIPersistenceSettingsBackendName(Backend)),
			TEXT("Use InMemory, JsonFile, or SQLite. Draft validation does not fall back silently."));
		return;
	}

	if (Backend == ESQLUILayoutRepositoryBackend::InMemory)
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("InMemory is the safe default persistence backend."),
			TEXT("No SQLite path, database creation, migration, seed copy, or provider initialization is required."));
		return;
	}

	if (Backend == ESQLUILayoutRepositoryBackend::JsonFile)
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("JsonFile can be represented as a pending backend."),
			TEXT("Draft validation does not create JSON repository directories or write layout files."));
		return;
	}

	if (!FSQLUISQLiteAvailability::IsSQLiteCoreCompiledIn())
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("SQLite was selected but SQLiteCore is not available in this build."),
			TEXT("Draft validation does not create a repository or attempt a fallback backend."));
		return;
	}

	AddSQLUIPersistenceSettingsValidationMessage(
		Result,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
		TEXT("SQLite can be represented as a pending backend."),
		TEXT("Selecting SQLite in a draft does not create databases, run migrations, copy seeds, or initialize repositories."));
}

void ValidateSQLUIPersistenceSettingsSQLitePath(
	const FSQLUIPersistenceSettingsDraft& Draft,
	FSQLUIPersistenceSettingsValidationResult& Result)
{
	if (Draft.PendingRuntimeConfig.Backend != ESQLUILayoutRepositoryBackend::SQLite)
	{
		return;
	}

	const FString PendingPath =
		TrimSQLUIPersistenceSettingsString(Draft.PendingRuntimeConfig.SQLiteDatabasePath);
	if (PendingPath.IsEmpty())
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("SQLite requires a database path before settings can be applied."),
			TEXT("Draft validation leaves the database path empty and does not create a default database file."));
		return;
	}

	if (DoesSQLUIPersistenceSettingsPathContainControlCharacters(PendingPath))
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("SQLite database path contains unsupported control characters."),
			TEXT("Draft validation checks path text only and does not touch the filesystem."));
		return;
	}

	Result.ResolvedSQLiteDatabasePath =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(PendingPath);
	Result.bSQLitePathResolved = !Result.ResolvedSQLiteDatabasePath.IsEmpty();
	if (!Result.bSQLitePathResolved)
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("SQLite database path could not be resolved."),
			TEXT("Draft validation does not create directories or files."));
		return;
	}

	if (FPaths::IsRelative(PendingPath)
		|| PendingPath.Equals(
			FSQLUILayoutRepositoryRuntimeConfigResolver::DefaultSQLiteDatabaseToken,
			ESearchCase::IgnoreCase))
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("SQLite path resolves through the SQLUICore runtime path policy."),
			Result.ResolvedSQLiteDatabasePath);
	}
	else
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
			TEXT("SQLite path is absolute and will be preserved by the runtime resolver."),
			TEXT("Review product policy before applying absolute paths. Draft validation does not open or create the file."));
	}
}

void ValidateSQLUIPersistenceSettingsSQLiteMutationFlags(
	const FSQLUIPersistenceSettingsDraft& Draft,
	FSQLUIPersistenceSettingsValidationResult& Result)
{
	if (Draft.PendingRuntimeConfig.Backend != ESQLUILayoutRepositoryBackend::SQLite)
	{
		return;
	}

	if (Draft.PendingRuntimeConfig.bSQLiteInitializeSchemaIfMissing
		|| Draft.PendingRuntimeConfig.bSQLiteCreateDatabaseIfMissing)
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
			TEXT("SQLite schema initialization or database creation is represented only as pending policy."),
			TEXT("Draft validation does not initialize schema, create database files, or run migrations."));
	}

	if (Draft.PendingRuntimeConfig.bSQLiteCopySeedIfMissing
		|| Draft.PendingRuntimeConfig.bSQLiteOverwriteDatabaseFromSeed)
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
			TEXT("SQLite seed-copy options are represented only as pending policy."),
			TEXT("Draft validation does not copy seed databases or overwrite runtime databases."));
	}
}

void ValidateSQLUIPersistenceSettingsProviderAutoInitialize(
	const FSQLUIPersistenceSettingsDraft& Draft,
	FSQLUIPersistenceSettingsValidationResult& Result)
{
	if (!Draft.bPendingProviderAutoInitialize)
	{
		AddSQLUIPersistenceSettingsValidationMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("Provider auto-init remains disabled in the pending draft."),
			TEXT("This preserves the current safe startup policy unless Apply is implemented in a future PR."));
		return;
	}

	AddSQLUIPersistenceSettingsValidationMessage(
		Result,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
		TEXT("Provider auto-init is enabled only in the pending draft."),
		TEXT("Draft validation does not write config, initialize the provider, or change startup behavior."));
}
}

FSQLUIPersistenceSettingsDraft
USQLUIPersistenceSettingsDraftLibrary::MakeDefaultPersistenceSettingsDraft()
{
	const FSQLUILayoutRepositoryRuntimeConfig DefaultConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
	return MakePersistenceSettingsDraftFromRuntimeConfig(DefaultConfig, false);
}

FSQLUIPersistenceSettingsDraft
USQLUIPersistenceSettingsDraftLibrary::MakeCurrentPersistenceSettingsDraft()
{
	const USQLUILayoutRepositoryRuntimeSettings* RuntimeSettings =
		GetDefault<USQLUILayoutRepositoryRuntimeSettings>();
	const FSQLUILayoutRepositoryRuntimeConfig RuntimeConfig =
		FSQLUILayoutRepositoryRuntimeSettingsPolicy::
			MakeRuntimeConfigFromSettingsAndCommandLine(
				RuntimeSettings,
				FCommandLine::Get());
	const bool bProviderAutoInitialize =
		FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
			RuntimeSettings,
			FCommandLine::Get());

	return MakePersistenceSettingsDraftFromRuntimeConfig(
		RuntimeConfig,
		bProviderAutoInitialize);
}

FSQLUIPersistenceSettingsDraft
USQLUIPersistenceSettingsDraftLibrary::MakePersistenceSettingsDraftFromRuntimeConfig(
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig,
	const bool bProviderAutoInitialize)
{
	FSQLUIPersistenceSettingsDraft Draft;
	Draft.CurrentRuntimeConfig = RuntimeConfig;
	Draft.PendingRuntimeConfig = RuntimeConfig;
	Draft.bCurrentProviderAutoInitialize = bProviderAutoInitialize;
	Draft.bPendingProviderAutoInitialize = bProviderAutoInitialize;
	return Draft;
}

FSQLUIPersistenceSettingsDraft
USQLUIPersistenceSettingsDraftLibrary::ResetDraftToCurrent(
	const FSQLUIPersistenceSettingsDraft& Draft)
{
	FSQLUIPersistenceSettingsDraft ResetDraft = Draft;
	ResetDraft.PendingRuntimeConfig = Draft.CurrentRuntimeConfig;
	ResetDraft.bPendingProviderAutoInitialize =
		Draft.bCurrentProviderAutoInitialize;
	ResetDraft.bHasBackendOverride = false;
	ResetDraft.bHasSQLiteDatabasePathOverride = false;
	ResetDraft.bHasProviderAutoInitializeOverride = false;
	return ResetDraft;
}

FSQLUIPersistenceSettingsValidationResult
USQLUIPersistenceSettingsDraftLibrary::ValidatePersistenceSettingsDraft(
	const FSQLUIPersistenceSettingsDraft& Draft)
{
	FSQLUIPersistenceSettingsValidationResult Result;
	Result.bIsValid = true;
	Result.bWouldChangeBackend =
		Draft.CurrentRuntimeConfig.Backend != Draft.PendingRuntimeConfig.Backend;
	Result.bWouldChangeSQLitePath =
		NormalizeSQLUIPersistenceSettingsComparablePath(
			Draft.CurrentRuntimeConfig.SQLiteDatabasePath)
		!= NormalizeSQLUIPersistenceSettingsComparablePath(
			Draft.PendingRuntimeConfig.SQLiteDatabasePath);
	Result.bWouldChangeProviderAutoInitialize =
		Draft.bCurrentProviderAutoInitialize
		!= Draft.bPendingProviderAutoInitialize;
	Result.bRequiresRestartOrReinitialize =
		Result.bWouldChangeBackend
		|| Result.bWouldChangeSQLitePath
		|| Result.bWouldChangeProviderAutoInitialize;

	ValidateSQLUIPersistenceSettingsBackend(Draft, Result);
	ValidateSQLUIPersistenceSettingsSQLitePath(Draft, Result);
	ValidateSQLUIPersistenceSettingsSQLiteMutationFlags(Draft, Result);
	ValidateSQLUIPersistenceSettingsProviderAutoInitialize(Draft, Result);

	if (DoesSQLUIPersistenceSettingsValidationHaveErrors(Result))
	{
		Result.bIsValid = false;
		Result.SummaryText =
			TEXT("SQLUI persistence settings draft has validation errors. No settings were applied.");
	}
	else if (Result.bRequiresRestartOrReinitialize)
	{
		Result.SummaryText =
			TEXT("SQLUI persistence settings draft is valid and would require an explicit apply plus restart or provider reinitialization.");
	}
	else
	{
		Result.SummaryText =
			TEXT("SQLUI persistence settings draft is valid and matches the current settings.");
	}

	return Result;
}

FSQLUIPersistenceSettingsApplyPreviewResult
USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
	const FSQLUIPersistenceSettingsDraft& Draft)
{
	FSQLUIPersistenceSettingsApplyPreviewResult Result;
	Result.ValidationResult = ValidatePersistenceSettingsDraft(Draft);
	Result.bIsValid = Result.ValidationResult.bIsValid;
	Result.bWouldChangeBackend =
		Result.ValidationResult.bWouldChangeBackend;
	Result.bWouldChangeSQLitePath =
		Result.ValidationResult.bWouldChangeSQLitePath;
	Result.bWouldChangeSQLiteConfig =
		WouldSQLUIPersistenceSettingsChangeSQLiteConfig(
			Draft.CurrentRuntimeConfig,
			Draft.PendingRuntimeConfig);
	Result.bWouldChangeProviderAutoInitialize =
		Result.ValidationResult.bWouldChangeProviderAutoInitialize;

	const bool bWouldChangeJsonFileConfig =
		WouldSQLUIPersistenceSettingsChangeJsonFileConfig(
			Draft.CurrentRuntimeConfig,
			Draft.PendingRuntimeConfig);

	Result.bWouldNeedRepositoryReopen =
		Result.bWouldChangeBackend
		|| bWouldChangeJsonFileConfig
		|| Result.bWouldChangeSQLiteConfig;
	Result.bWouldNeedProviderReinitialize =
		Result.bWouldNeedRepositoryReopen
		|| Result.bWouldChangeProviderAutoInitialize;
	Result.bHasChanges =
		Result.bWouldNeedRepositoryReopen
		|| Result.bWouldChangeProviderAutoInitialize;
	Result.bRequiresRestartOrReinitialize =
		Result.bHasChanges
		&& (Result.ValidationResult.bRequiresRestartOrReinitialize
			|| Result.bWouldNeedRepositoryReopen
			|| Result.bWouldNeedProviderReinitialize);
	Result.bCanApplyInFuture =
		Result.bIsValid
		&& Result.bHasChanges;
	Result.Messages = Result.ValidationResult.Messages;

	if (!Result.bIsValid)
	{
		AddSQLUIPersistenceSettingsApplyPreviewMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("Future Apply would be blocked by validation errors."),
			TEXT("This is a dry-run preview only. No settings were applied or saved."));
		Result.SummaryText =
			TEXT("SQLUI persistence settings apply preview found validation errors. Not applied.");
	}
	else if (!Result.bHasChanges)
	{
		AddSQLUIPersistenceSettingsApplyPreviewMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("No changes to apply."),
			TEXT("The pending draft matches the current settings. Not applied."));
		Result.SummaryText =
			TEXT("SQLUI persistence settings apply preview found no changes to apply. Not applied.");
	}
	else
	{
		AddSQLUIPersistenceSettingsApplyPreviewMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
			TEXT("Future Apply would change SQLUI persistence settings."),
			TEXT("This preview does not write config, initialize providers, reopen repositories, create databases, or run migrations."));

		if (Result.bWouldChangeBackend)
		{
			AddSQLUIPersistenceSettingsApplyPreviewMessage(
				Result,
				ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
				TEXT("Future Apply would change the persistence backend."),
				TEXT("A repository reopen or provider reinitialization would be required."));
		}

		if (Result.bWouldChangeSQLitePath || Result.bWouldChangeSQLiteConfig)
		{
			AddSQLUIPersistenceSettingsApplyPreviewMessage(
				Result,
				ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
				TEXT("Future Apply would change SQLite path or policy settings."),
				TEXT("The preview resolves text only and does not create directories, open databases for writing, copy seeds, or run migrations."));
		}

		if (Result.bWouldChangeProviderAutoInitialize)
		{
			AddSQLUIPersistenceSettingsApplyPreviewMessage(
				Result,
				ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
				TEXT("Future Apply would change provider auto-init policy."),
				TEXT("Startup behavior is unchanged by this preview."));
		}

		Result.SummaryText =
			TEXT("SQLUI persistence settings apply preview is valid and has pending changes. Not applied.");
	}

	return Result;
}

FSQLUIPersistenceSettingsApplyContractResult
USQLUIPersistenceSettingsDraftLibrary::BuildPersistenceSettingsApplyContract(
	const FSQLUIPersistenceSettingsDraft& Draft)
{
	FSQLUIPersistenceSettingsApplyContractResult Result;
	Result.ApplyPreview = PreviewPersistenceSettingsDraftApply(Draft);
	Result.bCanApplyInFuture = Result.ApplyPreview.bCanApplyInFuture;
	Result.bActualApplyImplemented = false;
	Result.bCanExecuteApplyNow = false;
	Result.bIsValid = Result.ApplyPreview.bIsValid;
	Result.bHasChanges = Result.ApplyPreview.bHasChanges;
	Result.bRequiresRestartOrReinitialize =
		Result.ApplyPreview.bRequiresRestartOrReinitialize;
	Result.bWouldChangeBackend = Result.ApplyPreview.bWouldChangeBackend;
	Result.bWouldChangeSQLitePath = Result.ApplyPreview.bWouldChangeSQLitePath;
	Result.bWouldChangeSQLiteConfig = Result.ApplyPreview.bWouldChangeSQLiteConfig;
	Result.bWouldChangeProviderAutoInitialize =
		Result.ApplyPreview.bWouldChangeProviderAutoInitialize;
	Result.bWouldNeedProviderReinitialize =
		Result.ApplyPreview.bWouldNeedProviderReinitialize;
	Result.bWouldNeedRepositoryReopen =
		Result.ApplyPreview.bWouldNeedRepositoryReopen;
	Result.Messages = Result.ApplyPreview.Messages;

	if (!Result.bIsValid)
	{
		Result.Availability =
			ESQLUIPersistenceSettingsApplyAvailability::BlockedByValidation;
	}
	else if (!Result.bHasChanges)
	{
		Result.Availability =
			ESQLUIPersistenceSettingsApplyAvailability::NoChanges;
	}
	else if (Result.bCanApplyInFuture)
	{
		Result.Availability =
			ESQLUIPersistenceSettingsApplyAvailability::PreviewOnlyReady;
	}
	else
	{
		Result.Availability =
			ESQLUIPersistenceSettingsApplyAvailability::NotImplemented;
	}

	AddSQLUIPersistenceSettingsContractMessage(
		Result.Messages,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
		TEXT("Actual Apply execution is not implemented."),
		TEXT("This contract is preview-only. No settings were applied or saved."));

	Result.bHasErrors =
		!Result.bIsValid
		|| DoesSQLUIPersistenceSettingsMessageListHaveSeverity(
			Result.Messages,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error);
	Result.bHasWarnings =
		DoesSQLUIPersistenceSettingsMessageListHaveSeverity(
			Result.Messages,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Warning);

	if (!Result.bIsValid)
	{
		Result.SummaryText =
			TEXT("SQLUI persistence settings apply contract is blocked by validation. Actual Apply is not implemented. Not applied.");
	}
	else if (!Result.bHasChanges)
	{
		Result.SummaryText =
			TEXT("SQLUI persistence settings apply contract found no changes to apply. Actual Apply is not implemented. Not saved.");
	}
	else
	{
		Result.SummaryText =
			TEXT("SQLUI persistence settings apply contract is ready for a future Apply, but actual Apply is not implemented. Not applied or saved.");
	}

	return Result;
}

FSQLUIPersistenceSettingsCancelPreviewResult
USQLUIPersistenceSettingsDraftLibrary::BuildPersistenceSettingsCancelPreview(
	const FSQLUIPersistenceSettingsDraft& Draft)
{
	FSQLUIPersistenceSettingsCancelPreviewResult Result;
	Result.bHasPendingChanges =
		DoesSQLUIPersistenceSettingsDraftHavePendingChanges(Draft);
	Result.bWouldDiscardChanges = Result.bHasPendingChanges;
	Result.bWouldResetPendingToCurrent = Result.bHasPendingChanges;
	Result.DraftAfterCancel = Draft;
	Result.DraftAfterCancel.PendingRuntimeConfig = Draft.CurrentRuntimeConfig;
	Result.DraftAfterCancel.bPendingProviderAutoInitialize =
		Draft.bCurrentProviderAutoInitialize;
	Result.DraftAfterCancel.bHasBackendOverride = false;
	Result.DraftAfterCancel.bHasSQLiteDatabasePathOverride = false;
	Result.DraftAfterCancel.bHasProviderAutoInitializeOverride = false;

	if (Result.bHasPendingChanges)
	{
		AddSQLUIPersistenceSettingsContractMessage(
			Result.Messages,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("Cancel would discard pending SQLUI persistence settings changes."),
			TEXT("This is a value preview only. No settings were applied, saved, or reset live."));
		Result.SummaryText =
			TEXT("SQLUI persistence settings cancel preview would discard pending changes and restore pending values to current settings. Not applied or saved.");
	}
	else
	{
		AddSQLUIPersistenceSettingsContractMessage(
			Result.Messages,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("No pending SQLUI persistence settings changes to discard."),
			TEXT("This is a value preview only. No settings were applied, saved, or reset live."));
		Result.SummaryText =
			TEXT("SQLUI persistence settings cancel preview found no pending changes to discard. Not applied or saved.");
	}

	return Result;
}

FSQLUIPersistenceSettingsApplyResult
USQLUIPersistenceSettingsDraftLibrary::RequestPersistenceSettingsApply(
	const FSQLUIPersistenceSettingsApplyRequest& Request)
{
	FSQLUIPersistenceSettingsApplyResult Result;
	Result.ApplyContract = BuildPersistenceSettingsApplyContract(Request.Draft);
	Result.bActualApplyImplemented = false;
	Result.bSucceeded = false;
	Result.bDidWriteConfig = false;
	Result.bDidChangeSettings = false;
	Result.bDidInitializeProvider = false;
	Result.bDidInitializeRepository = false;
	Result.bDidCreateDatabaseFiles = false;
	Result.bDidCreateDirectories = false;
	Result.bDidOpenDatabaseForWriting = false;
	Result.bDidRunMigrations = false;
	Result.bDidCopySeedDatabase = false;
	Result.bDidDeleteFiles = false;
	Result.bRequiresRestartOrReinitialize =
		Result.ApplyContract.bRequiresRestartOrReinitialize;
	Result.Messages = Result.ApplyContract.Messages;

	if (!Result.ApplyContract.bIsValid)
	{
		Result.Status =
			ESQLUIPersistenceSettingsApplyStatus::BlockedByValidation;
		AddSQLUIPersistenceSettingsApplyResultMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error,
			TEXT("SQLUI persistence settings Apply request was blocked by validation."),
			TEXT("Actual Apply is not implemented. No settings were applied, saved, or written to config."));
		Result.SummaryText =
			TEXT("SQLUI persistence settings Apply is blocked by validation. Actual Apply is not implemented. Not applied or saved.");
		return Result;
	}

	if (!Result.ApplyContract.bHasChanges)
	{
		Result.Status = ESQLUIPersistenceSettingsApplyStatus::NoChanges;
		AddSQLUIPersistenceSettingsApplyResultMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Info,
			TEXT("SQLUI persistence settings Apply request found no changes."),
			TEXT("Actual Apply is not implemented. No config was written and no settings changed."));
		Result.SummaryText =
			TEXT("SQLUI persistence settings Apply found no changes. Actual Apply is not implemented. Not saved.");
		return Result;
	}

	if (Result.ApplyContract.bCanApplyInFuture)
	{
		Result.Status = ESQLUIPersistenceSettingsApplyStatus::PreviewOnly;
		AddSQLUIPersistenceSettingsApplyResultMessage(
			Result,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
			TEXT("SQLUI persistence settings Apply request is preview-only."),
			TEXT("The draft is valid for a future Apply, but actual Apply is not implemented. No config was written, no provider or repository was initialized, and no database files were created."));
		Result.SummaryText =
			TEXT("SQLUI persistence settings Apply is preview-only. Actual Apply is not implemented. Not applied or saved.");
		return Result;
	}

	Result.Status = ESQLUIPersistenceSettingsApplyStatus::NotImplemented;
	AddSQLUIPersistenceSettingsApplyResultMessage(
		Result,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Warning,
		TEXT("SQLUI persistence settings Apply execution is unavailable."),
		TEXT("Actual Apply is not implemented. No config, settings, provider, repository, database, migration, seed-copy, or file-delete side effects were performed."));
	Result.SummaryText =
		TEXT("SQLUI persistence settings Apply execution is unavailable. Actual Apply is not implemented. Not applied or saved.");
	return Result;
}
