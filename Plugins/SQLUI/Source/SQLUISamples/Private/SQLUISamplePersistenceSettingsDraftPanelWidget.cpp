#include "SQLUISamplePersistenceSettingsDraftPanelWidget.h"

#include "UObject/UObjectGlobals.h"

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPanelWidget::
	RefreshDefaultPersistenceSettingsDraftPanel()
{
	EnsureDraftPresenter();
	if (!IsValid(DraftPresenter))
	{
		FSQLUISamplePersistenceSettingsDraftRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings draft presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		DraftPresenter->RefreshDefaultPersistenceSettingsDraftDisplay());
}

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPanelWidget::
	RefreshCurrentPersistenceSettingsDraftPanel()
{
	EnsureDraftPresenter();
	if (!IsValid(DraftPresenter))
	{
		FSQLUISamplePersistenceSettingsDraftRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings draft presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		DraftPresenter->RefreshCurrentPersistenceSettingsDraftDisplay());
}

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPanelWidget::
	BuildPersistenceSettingsDraftPanel(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	EnsureDraftPresenter();
	if (!IsValid(DraftPresenter))
	{
		FSQLUISamplePersistenceSettingsDraftRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings draft presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		DraftPresenter->BuildPersistenceSettingsDraftValidationDisplay(Draft));
}

TArray<FSQLUIPersistenceSettingsValidationDisplayRow>
USQLUISamplePersistenceSettingsDraftPanelWidget::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceSettingsDraftPanelWidget::GetFormattedLines() const
{
	return FormattedLines;
}

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPanelWidget::GetLastRefreshResult() const
{
	return LastRefreshResult;
}

FString USQLUISamplePersistenceSettingsDraftPanelWidget::GetSummaryText() const
{
	return SummaryText;
}

void USQLUISamplePersistenceSettingsDraftPanelWidget::EnsureDraftPresenter()
{
	if (!IsValid(DraftPresenter))
	{
		DraftPresenter =
			NewObject<USQLUISamplePersistenceSettingsDraftPresenter>(this);
	}
}

FSQLUISamplePersistenceSettingsDraftRefreshResult
USQLUISamplePersistenceSettingsDraftPanelWidget::StoreRefreshResult(
	const FSQLUISamplePersistenceSettingsDraftRefreshResult& InResult)
{
	LastRefreshResult = InResult;
	Rows = InResult.Rows;
	FormattedLines = InResult.FormattedLines;
	SummaryText = InResult.SummaryText;
	bIsValid = InResult.bIsValid;
	bHasErrors = InResult.bHasErrors;
	bHasWarnings = InResult.bHasWarnings;
	bRequiresRestartOrReinitialize = InResult.bRequiresRestartOrReinitialize;
	return LastRefreshResult;
}
