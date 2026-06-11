#include "Layout/SQLUIPersistenceSettingsDraftDisplay.h"

namespace
{
FText SQLUIPersistenceSettingsDisplayText(const TCHAR* Text)
{
	return FText::FromString(Text);
}

FText SQLUIPersistenceSettingsDisplayString(const FString& Text)
{
	return Text.IsEmpty()
		? SQLUIPersistenceSettingsDisplayText(TEXT("Unavailable"))
		: FText::FromString(Text);
}

FText SQLUIPersistenceSettingsDisplayYesNo(const bool bValue)
{
	return SQLUIPersistenceSettingsDisplayText(bValue ? TEXT("Yes") : TEXT("No"));
}

FString SQLUIPersistenceSettingsDisplayBackendName(
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

FString TrimSQLUIPersistenceSettingsDisplayString(FString Value)
{
	Value.TrimStartAndEndInline();
	return Value;
}

bool DoesSQLUIPersistenceSettingsDisplayPathContainControlCharacters(
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

ESQLUIPersistenceSettingsValidationDisplayState
MakeSQLUIPersistenceSettingsDisplayStateForSeverity(
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity)
{
	switch (Severity)
	{
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Error:
		return ESQLUIPersistenceSettingsValidationDisplayState::Error;
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Warning:
		return ESQLUIPersistenceSettingsValidationDisplayState::Warning;
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Info:
	default:
		return ESQLUIPersistenceSettingsValidationDisplayState::Normal;
	}
}

const TCHAR* MakeSQLUIPersistenceSettingsDisplayLabelForSeverity(
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity)
{
	switch (Severity)
	{
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Error:
		return TEXT("Validation error");
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Warning:
		return TEXT("Validation warning");
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Info:
	default:
		return TEXT("Validation info");
	}
}

const TCHAR* MakeSQLUIPersistenceSettingsApplyPreviewDisplayLabelForSeverity(
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity)
{
	switch (Severity)
	{
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Error:
		return TEXT("Apply preview error");
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Warning:
		return TEXT("Apply preview warning");
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Info:
	default:
		return TEXT("Apply preview info");
	}
}

const TCHAR* MakeSQLUIPersistenceSettingsApplyContractDisplayLabelForSeverity(
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity)
{
	switch (Severity)
	{
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Error:
		return TEXT("Apply contract error");
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Warning:
		return TEXT("Apply contract warning");
	case ESQLUIPersistenceSettingsValidationMessageSeverity::Info:
	default:
		return TEXT("Apply contract info");
	}
}

bool DoesSQLUIPersistenceSettingsValidationHaveSeverity(
	const FSQLUIPersistenceSettingsValidationResult& ValidationResult,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity)
{
	for (const FSQLUIPersistenceSettingsValidationMessage& Message : ValidationResult.Messages)
	{
		if (Message.Severity == Severity)
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUIPersistenceSettingsApplyPreviewHaveSeverity(
	const FSQLUIPersistenceSettingsApplyPreviewResult& PreviewResult,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity)
{
	for (const FSQLUIPersistenceSettingsValidationMessage& Message :
		PreviewResult.Messages)
	{
		if (Message.Severity == Severity)
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUIPersistenceSettingsApplyContractHaveSeverity(
	const FSQLUIPersistenceSettingsApplyContractResult& ContractResult,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity)
{
	for (const FSQLUIPersistenceSettingsValidationMessage& Message :
		ContractResult.Messages)
	{
		if (Message.Severity == Severity)
		{
			return true;
		}
	}

	return false;
}

ESQLUIPersistenceSettingsValidationDisplayState
MakeSQLUIPersistenceSettingsDisplaySummaryState(
	const FSQLUIPersistenceSettingsValidationResult& ValidationResult)
{
	if (DoesSQLUIPersistenceSettingsValidationHaveSeverity(
		ValidationResult,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Error))
	{
		return ESQLUIPersistenceSettingsValidationDisplayState::Error;
	}

	if (DoesSQLUIPersistenceSettingsValidationHaveSeverity(
			ValidationResult,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Warning)
		|| ValidationResult.bRequiresRestartOrReinitialize)
	{
		return ESQLUIPersistenceSettingsValidationDisplayState::Warning;
	}

	return ValidationResult.bIsValid
		? ESQLUIPersistenceSettingsValidationDisplayState::Good
		: ESQLUIPersistenceSettingsValidationDisplayState::Normal;
}

ESQLUIPersistenceSettingsValidationDisplayState
MakeSQLUIPersistenceSettingsApplyPreviewDisplaySummaryState(
	const FSQLUIPersistenceSettingsApplyPreviewResult& PreviewResult)
{
	if (DoesSQLUIPersistenceSettingsApplyPreviewHaveSeverity(
		PreviewResult,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Error))
	{
		return ESQLUIPersistenceSettingsValidationDisplayState::Error;
	}

	if (DoesSQLUIPersistenceSettingsApplyPreviewHaveSeverity(
			PreviewResult,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Warning)
		|| PreviewResult.bRequiresRestartOrReinitialize)
	{
		return ESQLUIPersistenceSettingsValidationDisplayState::Warning;
	}

	return PreviewResult.bIsValid
		? ESQLUIPersistenceSettingsValidationDisplayState::Good
		: ESQLUIPersistenceSettingsValidationDisplayState::Normal;
}

ESQLUIPersistenceSettingsValidationDisplayState
MakeSQLUIPersistenceSettingsApplyContractDisplaySummaryState(
	const FSQLUIPersistenceSettingsApplyContractResult& ContractResult)
{
	if (ContractResult.bHasErrors
		|| DoesSQLUIPersistenceSettingsApplyContractHaveSeverity(
			ContractResult,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error))
	{
		return ESQLUIPersistenceSettingsValidationDisplayState::Error;
	}

	if (ContractResult.bHasWarnings
		|| ContractResult.bRequiresRestartOrReinitialize
		|| ContractResult.Availability
			== ESQLUIPersistenceSettingsApplyAvailability::NotImplemented
		|| ContractResult.Availability
			== ESQLUIPersistenceSettingsApplyAvailability::PreviewOnlyReady)
	{
		return ESQLUIPersistenceSettingsValidationDisplayState::Warning;
	}

	return ContractResult.bIsValid
		? ESQLUIPersistenceSettingsValidationDisplayState::Good
		: ESQLUIPersistenceSettingsValidationDisplayState::Normal;
}

void AddSQLUIPersistenceSettingsValidationDisplayRow(
	TArray<FSQLUIPersistenceSettingsValidationDisplayRow>& Rows,
	const FName FieldKey,
	const TCHAR* Label,
	const FText& Value,
	const ESQLUIPersistenceSettingsValidationDisplayState State =
		ESQLUIPersistenceSettingsValidationDisplayState::Normal,
	const TCHAR* DetailText = TEXT(""))
{
	FSQLUIPersistenceSettingsValidationDisplayRow Row;
	Row.FieldKey = FieldKey;
	Row.Label = SQLUIPersistenceSettingsDisplayText(Label);
	Row.Value = Value;
	Row.State = State;
	Row.DetailText = SQLUIPersistenceSettingsDisplayText(DetailText);
	Rows.Add(Row);
}

void AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
	TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow>& Rows,
	const FName FieldKey,
	const TCHAR* Label,
	const FText& Value,
	const ESQLUIPersistenceSettingsValidationDisplayState State =
		ESQLUIPersistenceSettingsValidationDisplayState::Normal,
	const TCHAR* DetailText = TEXT(""))
{
	FSQLUIPersistenceSettingsApplyPreviewDisplayRow Row;
	Row.FieldKey = FieldKey;
	Row.Label = SQLUIPersistenceSettingsDisplayText(Label);
	Row.Value = Value;
	Row.State = State;
	Row.DetailText = SQLUIPersistenceSettingsDisplayText(DetailText);
	Rows.Add(Row);
}

void AddSQLUIPersistenceSettingsApplyContractDisplayRow(
	TArray<FSQLUIPersistenceSettingsApplyContractDisplayRow>& Rows,
	const FName FieldKey,
	const TCHAR* Label,
	const FText& Value,
	const ESQLUIPersistenceSettingsValidationDisplayState State =
		ESQLUIPersistenceSettingsValidationDisplayState::Normal,
	const TCHAR* DetailText = TEXT(""))
{
	FSQLUIPersistenceSettingsApplyContractDisplayRow Row;
	Row.FieldKey = FieldKey;
	Row.Label = SQLUIPersistenceSettingsDisplayText(Label);
	Row.Value = Value;
	Row.State = State;
	Row.DetailText = SQLUIPersistenceSettingsDisplayText(DetailText);
	Rows.Add(Row);
}

FText MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldChangeValue(
	const bool bWouldChange)
{
	return SQLUIPersistenceSettingsDisplayText(
		bWouldChange ? TEXT("Would change") : TEXT("No change"));
}

FText MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldRequireValue(
	const bool bWouldRequire,
	const TCHAR* RequiredText)
{
	return SQLUIPersistenceSettingsDisplayText(
		bWouldRequire ? RequiredText : TEXT("Not required"));
}

ESQLUIPersistenceSettingsValidationDisplayState
MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
	const bool bWouldChange)
{
	return bWouldChange
		? ESQLUIPersistenceSettingsValidationDisplayState::Warning
		: ESQLUIPersistenceSettingsValidationDisplayState::Good;
}

FText MakeSQLUIPersistenceSettingsApplyContractDisplayAvailabilityValue(
	const ESQLUIPersistenceSettingsApplyAvailability Availability)
{
	switch (Availability)
	{
	case ESQLUIPersistenceSettingsApplyAvailability::BlockedByValidation:
		return SQLUIPersistenceSettingsDisplayText(TEXT("Blocked by validation"));
	case ESQLUIPersistenceSettingsApplyAvailability::NoChanges:
		return SQLUIPersistenceSettingsDisplayText(TEXT("No changes to apply"));
	case ESQLUIPersistenceSettingsApplyAvailability::PreviewOnlyReady:
		return SQLUIPersistenceSettingsDisplayText(
			TEXT("Preview only, ready for future apply"));
	case ESQLUIPersistenceSettingsApplyAvailability::NotImplemented:
	default:
		return SQLUIPersistenceSettingsDisplayText(TEXT("Apply not implemented"));
	}
}

ESQLUIPersistenceSettingsValidationDisplayState
MakeSQLUIPersistenceSettingsApplyContractDisplayAvailabilityState(
	const ESQLUIPersistenceSettingsApplyAvailability Availability)
{
	switch (Availability)
	{
	case ESQLUIPersistenceSettingsApplyAvailability::BlockedByValidation:
		return ESQLUIPersistenceSettingsValidationDisplayState::Error;
	case ESQLUIPersistenceSettingsApplyAvailability::NoChanges:
		return ESQLUIPersistenceSettingsValidationDisplayState::Good;
	case ESQLUIPersistenceSettingsApplyAvailability::PreviewOnlyReady:
	case ESQLUIPersistenceSettingsApplyAvailability::NotImplemented:
	default:
		return ESQLUIPersistenceSettingsValidationDisplayState::Warning;
	}
}

FText MakeSQLUIPersistenceSettingsApplyContractDisplayCancelValue(
	const FSQLUIPersistenceSettingsCancelPreviewResult& CancelPreviewResult)
{
	if (CancelPreviewResult.bWouldDiscardChanges)
	{
		return SQLUIPersistenceSettingsDisplayText(
			TEXT("Would discard pending changes"));
	}

	return SQLUIPersistenceSettingsDisplayText(
		TEXT("No pending changes to discard"));
}

bool IsSQLUIPersistenceSettingsDisplaySQLite(
	const FSQLUIPersistenceSettingsDraft& Draft)
{
	return Draft.PendingRuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::SQLite;
}

FText MakeSQLUIPersistenceSettingsDisplaySQLitePathValue(
	const FSQLUIPersistenceSettingsDraft& Draft,
	const FSQLUIPersistenceSettingsValidationResult& ValidationResult)
{
	if (!IsSQLUIPersistenceSettingsDisplaySQLite(Draft))
	{
		return SQLUIPersistenceSettingsDisplayText(TEXT("Not applicable"));
	}

	if (ValidationResult.bSQLitePathResolved)
	{
		return FText::FromString(ValidationResult.ResolvedSQLiteDatabasePath);
	}

	const FString PendingPath =
		TrimSQLUIPersistenceSettingsDisplayString(Draft.PendingRuntimeConfig.SQLiteDatabasePath);
	return PendingPath.IsEmpty()
		? SQLUIPersistenceSettingsDisplayText(TEXT("Not configured"))
		: FText::FromString(PendingPath);
}

ESQLUIPersistenceSettingsValidationDisplayState
MakeSQLUIPersistenceSettingsDisplaySQLitePathState(
	const FSQLUIPersistenceSettingsDraft& Draft,
	const FSQLUIPersistenceSettingsValidationResult& ValidationResult)
{
	if (!IsSQLUIPersistenceSettingsDisplaySQLite(Draft))
	{
		return ESQLUIPersistenceSettingsValidationDisplayState::Normal;
	}

	const FString PendingPath =
		TrimSQLUIPersistenceSettingsDisplayString(Draft.PendingRuntimeConfig.SQLiteDatabasePath);
	if (PendingPath.IsEmpty()
		|| DoesSQLUIPersistenceSettingsDisplayPathContainControlCharacters(PendingPath)
		|| !ValidationResult.bSQLitePathResolved)
	{
		return ESQLUIPersistenceSettingsValidationDisplayState::Error;
	}

	return ESQLUIPersistenceSettingsValidationDisplayState::Normal;
}

FText MakeSQLUIPersistenceSettingsDisplayProviderAutoInitValue(
	const FSQLUIPersistenceSettingsDraft& Draft)
{
	if (Draft.bPendingProviderAutoInitialize)
	{
		return SQLUIPersistenceSettingsDisplayText(TEXT("Enabled (pending)"));
	}

	return SQLUIPersistenceSettingsDisplayText(TEXT("Disabled"));
}
}

FSQLUIPersistenceSettingsValidationDisplaySummary
USQLUIPersistenceSettingsDraftDisplayLibrary::
	ValidateAndMakePersistenceSettingsValidationDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	return MakePersistenceSettingsValidationDisplay(
		Draft,
		USQLUIPersistenceSettingsDraftLibrary::ValidatePersistenceSettingsDraft(Draft));
}

FSQLUIPersistenceSettingsValidationDisplaySummary
USQLUIPersistenceSettingsDraftDisplayLibrary::
	MakePersistenceSettingsValidationDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft,
		const FSQLUIPersistenceSettingsValidationResult& ValidationResult)
{
	FSQLUIPersistenceSettingsValidationDisplaySummary Summary;
	Summary.bIsValid = ValidationResult.bIsValid;
	Summary.bHasErrors = DoesSQLUIPersistenceSettingsValidationHaveSeverity(
		ValidationResult,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Error);
	Summary.bHasWarnings = DoesSQLUIPersistenceSettingsValidationHaveSeverity(
		ValidationResult,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Warning);
	Summary.bRequiresRestartOrReinitialize =
		ValidationResult.bRequiresRestartOrReinitialize;
	Summary.SummaryText = SQLUIPersistenceSettingsDisplayString(
		ValidationResult.SummaryText);
	Summary.Rows = MakePersistenceSettingsValidationDisplayRows(
		Draft,
		ValidationResult);
	return Summary;
}

TArray<FSQLUIPersistenceSettingsValidationDisplayRow>
USQLUIPersistenceSettingsDraftDisplayLibrary::
	MakePersistenceSettingsValidationDisplayRows(
		const FSQLUIPersistenceSettingsDraft& Draft,
		const FSQLUIPersistenceSettingsValidationResult& ValidationResult)
{
	TArray<FSQLUIPersistenceSettingsValidationDisplayRow> Rows;

	AddSQLUIPersistenceSettingsValidationDisplayRow(
		Rows,
		TEXT("Summary"),
		TEXT("Validation summary"),
		SQLUIPersistenceSettingsDisplayString(ValidationResult.SummaryText),
		MakeSQLUIPersistenceSettingsDisplaySummaryState(ValidationResult),
		TEXT("Validation-only summary. No settings have been applied."));

	AddSQLUIPersistenceSettingsValidationDisplayRow(
		Rows,
		TEXT("Backend"),
		TEXT("Backend"),
		FText::FromString(SQLUIPersistenceSettingsDisplayBackendName(
			Draft.PendingRuntimeConfig.Backend)),
		Draft.PendingRuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::InMemory
			? ESQLUIPersistenceSettingsValidationDisplayState::Good
			: ESQLUIPersistenceSettingsValidationDisplayState::Normal,
		TEXT("Pending backend value only. This row does not switch repositories."));

	AddSQLUIPersistenceSettingsValidationDisplayRow(
		Rows,
		TEXT("BackendChange"),
		TEXT("Backend change"),
		SQLUIPersistenceSettingsDisplayYesNo(ValidationResult.bWouldChangeBackend),
		ValidationResult.bWouldChangeBackend
			? ESQLUIPersistenceSettingsValidationDisplayState::Warning
			: ESQLUIPersistenceSettingsValidationDisplayState::Good,
		TEXT("Yes means an explicit future Apply step would be required."));

	AddSQLUIPersistenceSettingsValidationDisplayRow(
		Rows,
		TEXT("SQLitePath"),
		TEXT("SQLite path"),
		MakeSQLUIPersistenceSettingsDisplaySQLitePathValue(Draft, ValidationResult),
		MakeSQLUIPersistenceSettingsDisplaySQLitePathState(Draft, ValidationResult),
		TEXT("Resolved for display only. No directories or database files are created."));

	AddSQLUIPersistenceSettingsValidationDisplayRow(
		Rows,
		TEXT("ProviderAutoInit"),
		TEXT("Provider auto-init"),
		MakeSQLUIPersistenceSettingsDisplayProviderAutoInitValue(Draft),
		Draft.bPendingProviderAutoInitialize
			? ESQLUIPersistenceSettingsValidationDisplayState::Warning
			: ESQLUIPersistenceSettingsValidationDisplayState::Good,
		TEXT("Pending startup policy only. This row does not initialize providers or write config."));

	AddSQLUIPersistenceSettingsValidationDisplayRow(
		Rows,
		TEXT("RestartOrReinitialize"),
		TEXT("Restart/reinitialize required"),
		SQLUIPersistenceSettingsDisplayYesNo(
			ValidationResult.bRequiresRestartOrReinitialize),
		ValidationResult.bRequiresRestartOrReinitialize
			? ESQLUIPersistenceSettingsValidationDisplayState::Warning
			: ESQLUIPersistenceSettingsValidationDisplayState::Good,
		TEXT("Informational only. No lifecycle action is taken by display generation."));

	for (const FSQLUIPersistenceSettingsValidationMessage& Message :
		ValidationResult.Messages)
	{
		AddSQLUIPersistenceSettingsValidationDisplayRow(
			Rows,
			TEXT("ValidationMessage"),
			MakeSQLUIPersistenceSettingsDisplayLabelForSeverity(Message.Severity),
			SQLUIPersistenceSettingsDisplayString(Message.Message),
			MakeSQLUIPersistenceSettingsDisplayStateForSeverity(Message.Severity),
			*Message.DetailText);
	}

	return Rows;
}

FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
	PreviewAndMakePersistenceSettingsApplyPreviewDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	return MakePersistenceSettingsApplyPreviewDisplay(
		USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
			Draft));
}

FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
	MakePersistenceSettingsApplyPreviewDisplay(
		const FSQLUIPersistenceSettingsApplyPreviewResult& PreviewResult)
{
	FSQLUIPersistenceSettingsApplyPreviewDisplaySummary Summary;
	Summary.bCanApplyInFuture = PreviewResult.bCanApplyInFuture;
	Summary.bIsValid = PreviewResult.bIsValid;
	Summary.bHasChanges = PreviewResult.bHasChanges;
	Summary.bHasErrors = DoesSQLUIPersistenceSettingsApplyPreviewHaveSeverity(
		PreviewResult,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Error);
	Summary.bHasWarnings = DoesSQLUIPersistenceSettingsApplyPreviewHaveSeverity(
		PreviewResult,
		ESQLUIPersistenceSettingsValidationMessageSeverity::Warning);
	Summary.bRequiresRestartOrReinitialize =
		PreviewResult.bRequiresRestartOrReinitialize;
	Summary.bWouldNeedProviderReinitialize =
		PreviewResult.bWouldNeedProviderReinitialize;
	Summary.bWouldNeedRepositoryReopen = PreviewResult.bWouldNeedRepositoryReopen;
	Summary.SummaryText = SQLUIPersistenceSettingsDisplayString(
		PreviewResult.SummaryText);
	Summary.Rows = MakePersistenceSettingsApplyPreviewDisplayRows(PreviewResult);
	return Summary;
}

TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow>
USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
	MakePersistenceSettingsApplyPreviewDisplayRows(
		const FSQLUIPersistenceSettingsApplyPreviewResult& PreviewResult)
{
	TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow> Rows;

	AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
		Rows,
		TEXT("ApplyPreviewSummary"),
		TEXT("Apply preview summary"),
		SQLUIPersistenceSettingsDisplayString(PreviewResult.SummaryText),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplaySummaryState(
			PreviewResult),
		TEXT("Dry-run display only. No settings were applied or saved."));

	AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
		Rows,
		TEXT("CanApplyInFuture"),
		TEXT("Can apply in future"),
		SQLUIPersistenceSettingsDisplayYesNo(PreviewResult.bCanApplyInFuture),
		PreviewResult.bIsValid
			? ESQLUIPersistenceSettingsValidationDisplayState::Good
			: ESQLUIPersistenceSettingsValidationDisplayState::Error,
		TEXT("Informational only. This row does not run Apply."));

	AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
		Rows,
		TEXT("HasChanges"),
		TEXT("Has changes"),
		SQLUIPersistenceSettingsDisplayYesNo(PreviewResult.bHasChanges),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			PreviewResult.bHasChanges),
		TEXT("Future Apply would be needed only when pending changes exist."));

	AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
		Rows,
		TEXT("BackendChange"),
		TEXT("Backend change"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldChangeValue(
			PreviewResult.bWouldChangeBackend),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			PreviewResult.bWouldChangeBackend),
		TEXT("Preview only. This row does not switch repositories."));

	AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
		Rows,
		TEXT("SQLitePathOrPolicyChange"),
		TEXT("SQLite path/policy change"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldChangeValue(
			PreviewResult.bWouldChangeSQLitePath
			|| PreviewResult.bWouldChangeSQLiteConfig),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			PreviewResult.bWouldChangeSQLitePath
			|| PreviewResult.bWouldChangeSQLiteConfig),
		TEXT("SQLite remains opt-in. This preview does not create directories or databases, open databases for writing, copy seeds, or run migrations."));

	AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
		Rows,
		TEXT("ProviderAutoInitChange"),
		TEXT("Provider auto-init policy change"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldChangeValue(
			PreviewResult.bWouldChangeProviderAutoInitialize),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			PreviewResult.bWouldChangeProviderAutoInitialize),
		TEXT("Startup behavior is unchanged by this preview."));

	AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
		Rows,
		TEXT("RepositoryReopen"),
		TEXT("Repository reopen"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldRequireValue(
			PreviewResult.bWouldNeedRepositoryReopen,
			TEXT("Would require reopen")),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			PreviewResult.bWouldNeedRepositoryReopen),
		TEXT("Preview only. No repository is opened or reopened."));

	AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
		Rows,
		TEXT("ProviderReinitialize"),
		TEXT("Provider reinitialize"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldRequireValue(
			PreviewResult.bWouldNeedProviderReinitialize,
			TEXT("Would require reinitialize")),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			PreviewResult.bWouldNeedProviderReinitialize),
		TEXT("Preview only. No provider is initialized or reinitialized."));

	AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
		Rows,
		TEXT("RestartOrReinitialize"),
		TEXT("Restart/reinitialize required"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldRequireValue(
			PreviewResult.bRequiresRestartOrReinitialize,
			TEXT("Would require restart or reinitialize")),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			PreviewResult.bRequiresRestartOrReinitialize),
		TEXT("Informational only. No lifecycle action is taken by display generation."));

	for (const FSQLUIPersistenceSettingsValidationMessage& Message :
		PreviewResult.Messages)
	{
		AddSQLUIPersistenceSettingsApplyPreviewDisplayRow(
			Rows,
			TEXT("ApplyPreviewMessage"),
			MakeSQLUIPersistenceSettingsApplyPreviewDisplayLabelForSeverity(
				Message.Severity),
			SQLUIPersistenceSettingsDisplayString(Message.Message),
			MakeSQLUIPersistenceSettingsDisplayStateForSeverity(Message.Severity),
			*Message.DetailText);
	}

	return Rows;
}

FSQLUIPersistenceSettingsApplyContractDisplaySummary
USQLUIPersistenceSettingsApplyContractDisplayLibrary::
	BuildAndMakePersistenceSettingsApplyContractDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	return MakePersistenceSettingsApplyContractDisplay(
		USQLUIPersistenceSettingsDraftLibrary::BuildPersistenceSettingsApplyContract(
			Draft),
		USQLUIPersistenceSettingsDraftLibrary::BuildPersistenceSettingsCancelPreview(
			Draft));
}

FSQLUIPersistenceSettingsApplyContractDisplaySummary
USQLUIPersistenceSettingsApplyContractDisplayLibrary::
	MakePersistenceSettingsApplyContractDisplay(
		const FSQLUIPersistenceSettingsApplyContractResult& ContractResult,
		const FSQLUIPersistenceSettingsCancelPreviewResult& CancelPreviewResult)
{
	FSQLUIPersistenceSettingsApplyContractDisplaySummary Summary;
	Summary.bCanApplyInFuture = ContractResult.bCanApplyInFuture;
	Summary.bActualApplyImplemented = ContractResult.bActualApplyImplemented;
	Summary.bCanExecuteApplyNow = ContractResult.bCanExecuteApplyNow;
	Summary.bIsValid = ContractResult.bIsValid;
	Summary.bHasChanges = ContractResult.bHasChanges;
	Summary.bHasErrors = ContractResult.bHasErrors;
	Summary.bHasWarnings = ContractResult.bHasWarnings;
	Summary.bRequiresRestartOrReinitialize =
		ContractResult.bRequiresRestartOrReinitialize;
	Summary.bWouldNeedProviderReinitialize =
		ContractResult.bWouldNeedProviderReinitialize;
	Summary.bWouldNeedRepositoryReopen =
		ContractResult.bWouldNeedRepositoryReopen;
	Summary.bWouldDiscardChangesOnCancel =
		CancelPreviewResult.bWouldDiscardChanges;
	Summary.Availability = ContractResult.Availability;
	Summary.SummaryText = SQLUIPersistenceSettingsDisplayString(
		ContractResult.SummaryText);
	Summary.Rows = MakePersistenceSettingsApplyContractDisplayRows(
		ContractResult,
		CancelPreviewResult);
	return Summary;
}

TArray<FSQLUIPersistenceSettingsApplyContractDisplayRow>
USQLUIPersistenceSettingsApplyContractDisplayLibrary::
	MakePersistenceSettingsApplyContractDisplayRows(
		const FSQLUIPersistenceSettingsApplyContractResult& ContractResult,
		const FSQLUIPersistenceSettingsCancelPreviewResult& CancelPreviewResult)
{
	TArray<FSQLUIPersistenceSettingsApplyContractDisplayRow> Rows;

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("ApplyContractSummary"),
		TEXT("Apply/cancel contract summary"),
		SQLUIPersistenceSettingsDisplayString(ContractResult.SummaryText),
		MakeSQLUIPersistenceSettingsApplyContractDisplaySummaryState(
			ContractResult),
		TEXT("Display only. No settings were applied, saved, or written to config."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("ApplyAvailability"),
		TEXT("Apply availability"),
		MakeSQLUIPersistenceSettingsApplyContractDisplayAvailabilityValue(
			ContractResult.Availability),
		MakeSQLUIPersistenceSettingsApplyContractDisplayAvailabilityState(
			ContractResult.Availability),
		TEXT("Readiness only. This row does not run Apply."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("ActualApplyImplemented"),
		TEXT("Actual apply implemented"),
		SQLUIPersistenceSettingsDisplayYesNo(
			ContractResult.bActualApplyImplemented),
		ContractResult.bActualApplyImplemented
			? ESQLUIPersistenceSettingsValidationDisplayState::Good
			: ESQLUIPersistenceSettingsValidationDisplayState::Warning,
		TEXT("Actual Apply execution remains unavailable in this implementation slice."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("CanExecuteApplyNow"),
		TEXT("Can execute apply now"),
		SQLUIPersistenceSettingsDisplayYesNo(ContractResult.bCanExecuteApplyNow),
		ContractResult.bCanExecuteApplyNow
			? ESQLUIPersistenceSettingsValidationDisplayState::Good
			: ESQLUIPersistenceSettingsValidationDisplayState::Warning,
		TEXT("No config write, repository lifecycle action, or provider initialization is performed."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("CanApplyInFuture"),
		TEXT("Can apply in future"),
		SQLUIPersistenceSettingsDisplayYesNo(ContractResult.bCanApplyInFuture),
		ContractResult.bCanApplyInFuture
			? ESQLUIPersistenceSettingsValidationDisplayState::Good
			: MakeSQLUIPersistenceSettingsApplyContractDisplayAvailabilityState(
				ContractResult.Availability),
		TEXT("A future Apply implementation would still need an explicit non-preview execution path."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("HasChanges"),
		TEXT("Has changes"),
		SQLUIPersistenceSettingsDisplayYesNo(ContractResult.bHasChanges),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			ContractResult.bHasChanges),
		TEXT("Pending-value comparison only. No live settings are changed."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("BackendChange"),
		TEXT("Backend change"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldChangeValue(
			ContractResult.bWouldChangeBackend),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			ContractResult.bWouldChangeBackend),
		TEXT("Preview only. This row does not switch repositories or enable SQLite."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("SQLitePathOrPolicyChange"),
		TEXT("SQLite path/policy change"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldChangeValue(
			ContractResult.bWouldChangeSQLitePath
			|| ContractResult.bWouldChangeSQLiteConfig),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			ContractResult.bWouldChangeSQLitePath
			|| ContractResult.bWouldChangeSQLiteConfig),
		TEXT("SQLite remains opt-in. This display does not create directories or databases, open databases for writing, copy seeds, or run migrations."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("ProviderAutoInitChange"),
		TEXT("Provider auto-init policy change"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldChangeValue(
			ContractResult.bWouldChangeProviderAutoInitialize),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			ContractResult.bWouldChangeProviderAutoInitialize),
		TEXT("Startup behavior is unchanged by this display."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("RepositoryReopen"),
		TEXT("Repository reopen"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldRequireValue(
			ContractResult.bWouldNeedRepositoryReopen,
			TEXT("Would require reopen")),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			ContractResult.bWouldNeedRepositoryReopen),
		TEXT("Preview only. No repository is opened or reopened."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("ProviderReinitialize"),
		TEXT("Provider reinitialize"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldRequireValue(
			ContractResult.bWouldNeedProviderReinitialize,
			TEXT("Would require reinitialize")),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			ContractResult.bWouldNeedProviderReinitialize),
		TEXT("Preview only. No provider is initialized or reinitialized."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("RestartOrReinitialize"),
		TEXT("Restart/reinitialize required"),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayWouldRequireValue(
			ContractResult.bRequiresRestartOrReinitialize,
			TEXT("Would require restart or reinitialize")),
		MakeSQLUIPersistenceSettingsApplyPreviewDisplayChangeState(
			ContractResult.bRequiresRestartOrReinitialize),
		TEXT("Informational only. No lifecycle action is taken by display generation."));

	AddSQLUIPersistenceSettingsApplyContractDisplayRow(
		Rows,
		TEXT("CancelPreview"),
		TEXT("Cancel/discard preview"),
		MakeSQLUIPersistenceSettingsApplyContractDisplayCancelValue(
			CancelPreviewResult),
		CancelPreviewResult.bWouldDiscardChanges
			? ESQLUIPersistenceSettingsValidationDisplayState::Warning
			: ESQLUIPersistenceSettingsValidationDisplayState::Good,
		TEXT("Value preview only. No live draft, config, provider, repository, or file state is reset or deleted."));

	for (const FSQLUIPersistenceSettingsValidationMessage& Message :
		ContractResult.Messages)
	{
		AddSQLUIPersistenceSettingsApplyContractDisplayRow(
			Rows,
			TEXT("ApplyContractMessage"),
			MakeSQLUIPersistenceSettingsApplyContractDisplayLabelForSeverity(
				Message.Severity),
			SQLUIPersistenceSettingsDisplayString(Message.Message),
			MakeSQLUIPersistenceSettingsDisplayStateForSeverity(Message.Severity),
			*Message.DetailText);
	}

	for (const FSQLUIPersistenceSettingsValidationMessage& Message :
		CancelPreviewResult.Messages)
	{
		AddSQLUIPersistenceSettingsApplyContractDisplayRow(
			Rows,
			TEXT("CancelPreviewMessage"),
			MakeSQLUIPersistenceSettingsApplyContractDisplayLabelForSeverity(
				Message.Severity),
			SQLUIPersistenceSettingsDisplayString(Message.Message),
			MakeSQLUIPersistenceSettingsDisplayStateForSeverity(Message.Severity),
			*Message.DetailText);
	}

	return Rows;
}
