#include "SQLUISamplePersistenceSettingsApplyPreviewPresenter.h"

namespace
{
const TCHAR* SQLUISamplePersistenceSettingsApplyPreviewDisplayStateToString(
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

FString FormatSQLUISamplePersistenceSettingsApplyPreviewLine(
	const FSQLUIPersistenceSettingsApplyPreviewDisplayRow& Row)
{
	FString Line = FString::Printf(
		TEXT("%s: %s [%s]"),
		*Row.Label.ToString(),
		*Row.Value.ToString(),
		SQLUISamplePersistenceSettingsApplyPreviewDisplayStateToString(
			Row.State));

	const FString Detail = Row.DetailText.ToString();
	if (!Detail.IsEmpty())
	{
		Line += FString::Printf(TEXT(" - %s"), *Detail);
	}

	return Line;
}
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPresenter::
	RefreshDefaultPersistenceSettingsApplyPreviewDisplay()
{
	return BuildPersistenceSettingsApplyPreviewDisplay(
		USQLUIPersistenceSettingsDraftLibrary::MakeDefaultPersistenceSettingsDraft());
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPresenter::
	RefreshCurrentPersistenceSettingsApplyPreviewDisplay()
{
	return BuildPersistenceSettingsApplyPreviewDisplay(
		USQLUIPersistenceSettingsDraftLibrary::MakeCurrentPersistenceSettingsDraft());
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPresenter::
	BuildPersistenceSettingsApplyPreviewDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	return StoreDisplaySummary(
		USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
			PreviewAndMakePersistenceSettingsApplyPreviewDisplay(Draft));
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPresenter::
	BuildPersistenceSettingsApplyPreviewDisplayFromPreview(
		const FSQLUIPersistenceSettingsApplyPreviewResult& PreviewResult)
{
	return StoreDisplaySummary(
		USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
			MakePersistenceSettingsApplyPreviewDisplay(PreviewResult));
}

TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow>
USQLUISamplePersistenceSettingsApplyPreviewPresenter::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceSettingsApplyPreviewPresenter::GetFormattedLines() const
{
	return FormattedLines;
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPresenter::GetLastRefreshResult()
	const
{
	return LastRefreshResult;
}

FString
USQLUISamplePersistenceSettingsApplyPreviewPresenter::GetSummaryText() const
{
	return SummaryText;
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPresenter::StoreDisplaySummary(
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary& InSummary)
{
	Rows = InSummary.Rows;
	SummaryText = InSummary.SummaryText.ToString();
	RebuildFormattedLines();

	LastRefreshResult.DisplaySummary = InSummary;
	LastRefreshResult.Rows = Rows;
	LastRefreshResult.FormattedLines = FormattedLines;
	LastRefreshResult.SummaryText = SummaryText;
	LastRefreshResult.bCanApplyInFuture = InSummary.bCanApplyInFuture;
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
	LastRefreshResult.bSucceeded = Rows.Num() > 0;
	return LastRefreshResult;
}

void USQLUISamplePersistenceSettingsApplyPreviewPresenter::RebuildFormattedLines()
{
	FormattedLines.Reset(Rows.Num());
	for (const FSQLUIPersistenceSettingsApplyPreviewDisplayRow& Row : Rows)
	{
		FormattedLines.Add(
			FormatSQLUISamplePersistenceSettingsApplyPreviewLine(Row));
	}
}
