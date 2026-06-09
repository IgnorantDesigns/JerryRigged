#include "SQLUISamplePersistenceSettingsDraftPresenter.h"

namespace
{
const TCHAR* SQLUISamplePersistenceSettingsDraftDisplayStateToString(
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

FString FormatSQLUISamplePersistenceSettingsDraftLine(
	const FSQLUIPersistenceSettingsValidationDisplayRow& Row)
{
	FString Line = FString::Printf(
		TEXT("%s: %s [%s]"),
		*Row.Label.ToString(),
		*Row.Value.ToString(),
		SQLUISamplePersistenceSettingsDraftDisplayStateToString(Row.State));

	const FString Detail = Row.DetailText.ToString();
	if (!Detail.IsEmpty())
	{
		Line += FString::Printf(TEXT(" - %s"), *Detail);
	}

	return Line;
}
}

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPresenter::
	RefreshDefaultPersistenceSettingsDraftDisplay()
{
	return BuildPersistenceSettingsDraftValidationDisplay(
		USQLUIPersistenceSettingsDraftLibrary::MakeDefaultPersistenceSettingsDraft());
}

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPresenter::
	RefreshCurrentPersistenceSettingsDraftDisplay()
{
	return BuildPersistenceSettingsDraftValidationDisplay(
		USQLUIPersistenceSettingsDraftLibrary::MakeCurrentPersistenceSettingsDraft());
}

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPresenter::
	BuildPersistenceSettingsDraftValidationDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	return StoreDisplaySummary(
		USQLUIPersistenceSettingsDraftDisplayLibrary::
			ValidateAndMakePersistenceSettingsValidationDisplay(Draft));
}

TArray<FSQLUIPersistenceSettingsValidationDisplayRow>
USQLUISamplePersistenceSettingsDraftPresenter::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceSettingsDraftPresenter::GetFormattedLines() const
{
	return FormattedLines;
}

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPresenter::GetLastRefreshResult() const
{
	return LastRefreshResult;
}

FString USQLUISamplePersistenceSettingsDraftPresenter::GetSummaryText() const
{
	return SummaryText;
}

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPresenter::StoreDisplaySummary(
	const FSQLUIPersistenceSettingsValidationDisplaySummary& InSummary)
{
	Rows = InSummary.Rows;
	SummaryText = InSummary.SummaryText.ToString();
	RebuildFormattedLines();

	LastRefreshResult.DisplaySummary = InSummary;
	LastRefreshResult.Rows = Rows;
	LastRefreshResult.FormattedLines = FormattedLines;
	LastRefreshResult.SummaryText = SummaryText;
	LastRefreshResult.bIsValid = InSummary.bIsValid;
	LastRefreshResult.bHasErrors = InSummary.bHasErrors;
	LastRefreshResult.bHasWarnings = InSummary.bHasWarnings;
	LastRefreshResult.bRequiresRestartOrReinitialize =
		InSummary.bRequiresRestartOrReinitialize;
	LastRefreshResult.bSucceeded = Rows.Num() > 0;
	return LastRefreshResult;
}

void USQLUISamplePersistenceSettingsDraftPresenter::RebuildFormattedLines()
{
	FormattedLines.Reset(Rows.Num());
	for (const FSQLUIPersistenceSettingsValidationDisplayRow& Row : Rows)
	{
		FormattedLines.Add(FormatSQLUISamplePersistenceSettingsDraftLine(Row));
	}
}
