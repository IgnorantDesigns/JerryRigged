#include "SQLUISamplePersistenceSettingsApplyContractPresenter.h"

namespace
{
const TCHAR* SQLUISamplePersistenceSettingsApplyContractDisplayStateToString(
	const ESQLUIPersistenceSettingsValidationDisplayState State)
{
	switch (State)
	{
	case ESQLUIPersistenceSettingsValidationDisplayState::Normal:
		return TEXT("Normal");
	case ESQLUIPersistenceSettingsValidationDisplayState::Good:
		return TEXT("Good");
	case ESQLUIPersistenceSettingsValidationDisplayState::Warning:
		return TEXT("Warning");
	case ESQLUIPersistenceSettingsValidationDisplayState::Error:
		return TEXT("Error");
	default:
		return TEXT("Unknown");
	}
}

FString FormatSQLUISamplePersistenceSettingsApplyContractLine(
	const FSQLUIPersistenceSettingsApplyContractDisplayRow& Row)
{
	FString Line = FString::Printf(
		TEXT("%s: %s [%s]"),
		*Row.Label.ToString(),
		*Row.Value.ToString(),
		SQLUISamplePersistenceSettingsApplyContractDisplayStateToString(
			Row.State));

	const FString Detail = Row.DetailText.ToString();
	if (!Detail.IsEmpty())
	{
		Line += FString::Printf(TEXT(" - %s"), *Detail);
	}

	return Line;
}
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPresenter::
	RefreshDefaultPersistenceSettingsApplyContractDisplay()
{
	return BuildPersistenceSettingsApplyContractDisplay(
		USQLUIPersistenceSettingsDraftLibrary::MakeDefaultPersistenceSettingsDraft());
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPresenter::
	RefreshCurrentPersistenceSettingsApplyContractDisplay()
{
	return BuildPersistenceSettingsApplyContractDisplay(
		USQLUIPersistenceSettingsDraftLibrary::MakeCurrentPersistenceSettingsDraft());
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPresenter::
	BuildPersistenceSettingsApplyContractDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	return StoreDisplaySummary(
		USQLUIPersistenceSettingsApplyContractDisplayLibrary::
			BuildAndMakePersistenceSettingsApplyContractDisplay(Draft));
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPresenter::
	BuildPersistenceSettingsApplyContractDisplayFromContract(
		const FSQLUIPersistenceSettingsApplyContractResult& ContractResult,
		const FSQLUIPersistenceSettingsCancelPreviewResult& CancelPreviewResult)
{
	return StoreDisplaySummary(
		USQLUIPersistenceSettingsApplyContractDisplayLibrary::
			MakePersistenceSettingsApplyContractDisplay(
				ContractResult,
				CancelPreviewResult));
}

TArray<FSQLUIPersistenceSettingsApplyContractDisplayRow>
USQLUISamplePersistenceSettingsApplyContractPresenter::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceSettingsApplyContractPresenter::GetFormattedLines() const
{
	return FormattedLines;
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPresenter::GetLastRefreshResult()
	const
{
	return LastRefreshResult;
}

FString
USQLUISamplePersistenceSettingsApplyContractPresenter::GetSummaryText() const
{
	return SummaryText;
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPresenter::StoreDisplaySummary(
	const FSQLUIPersistenceSettingsApplyContractDisplaySummary& InSummary)
{
	Rows = InSummary.Rows;
	SummaryText = InSummary.SummaryText.ToString();
	RebuildFormattedLines();

	LastRefreshResult.DisplaySummary = InSummary;
	LastRefreshResult.Rows = Rows;
	LastRefreshResult.FormattedLines = FormattedLines;
	LastRefreshResult.SummaryText = SummaryText;
	LastRefreshResult.bCanApplyInFuture = InSummary.bCanApplyInFuture;
	LastRefreshResult.bActualApplyImplemented =
		InSummary.bActualApplyImplemented;
	LastRefreshResult.bCanExecuteApplyNow = InSummary.bCanExecuteApplyNow;
	LastRefreshResult.bIsValid = InSummary.bIsValid;
	LastRefreshResult.bHasChanges = InSummary.bHasChanges;
	LastRefreshResult.bHasErrors = InSummary.bHasErrors;
	LastRefreshResult.bHasWarnings = InSummary.bHasWarnings;
	LastRefreshResult.bRequiresRestartOrReinitialize =
		InSummary.bRequiresRestartOrReinitialize;
	LastRefreshResult.bWouldNeedProviderReinitialize =
		InSummary.bWouldNeedProviderReinitialize;
	LastRefreshResult.bWouldNeedRepositoryReopen =
		InSummary.bWouldNeedRepositoryReopen;
	LastRefreshResult.bWouldDiscardChangesOnCancel =
		InSummary.bWouldDiscardChangesOnCancel;
	LastRefreshResult.Availability = InSummary.Availability;
	LastRefreshResult.bSucceeded = Rows.Num() > 0;
	return LastRefreshResult;
}

void USQLUISamplePersistenceSettingsApplyContractPresenter::
	RebuildFormattedLines()
{
	FormattedLines.Reset(Rows.Num());
	for (const FSQLUIPersistenceSettingsApplyContractDisplayRow& Row : Rows)
	{
		FormattedLines.Add(
			FormatSQLUISamplePersistenceSettingsApplyContractLine(Row));
	}
}
