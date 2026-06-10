#include "SQLUISamplePersistenceSettingsApplyPreviewPanelWidget.h"

#include "UObject/UObjectGlobals.h"

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPanelWidget::
	RefreshDefaultPersistenceSettingsApplyPreviewPanel()
{
	EnsureApplyPreviewPresenter();
	if (!IsValid(ApplyPreviewPresenter))
	{
		FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings apply preview presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		ApplyPreviewPresenter
			->RefreshDefaultPersistenceSettingsApplyPreviewDisplay());
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPanelWidget::
	RefreshCurrentPersistenceSettingsApplyPreviewPanel()
{
	EnsureApplyPreviewPresenter();
	if (!IsValid(ApplyPreviewPresenter))
	{
		FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings apply preview presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		ApplyPreviewPresenter
			->RefreshCurrentPersistenceSettingsApplyPreviewDisplay());
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPanelWidget::
	BuildPersistenceSettingsApplyPreviewPanel(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	EnsureApplyPreviewPresenter();
	if (!IsValid(ApplyPreviewPresenter))
	{
		FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings apply preview presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		ApplyPreviewPresenter->BuildPersistenceSettingsApplyPreviewDisplay(
			Draft));
}

TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow>
USQLUISamplePersistenceSettingsApplyPreviewPanelWidget::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceSettingsApplyPreviewPanelWidget::GetFormattedLines()
	const
{
	return FormattedLines;
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPanelWidget::GetLastRefreshResult()
	const
{
	return LastRefreshResult;
}

FString
USQLUISamplePersistenceSettingsApplyPreviewPanelWidget::GetSummaryText() const
{
	return SummaryText;
}

void USQLUISamplePersistenceSettingsApplyPreviewPanelWidget::
	EnsureApplyPreviewPresenter()
{
	if (!IsValid(ApplyPreviewPresenter))
	{
		ApplyPreviewPresenter =
			NewObject<USQLUISamplePersistenceSettingsApplyPreviewPresenter>(
				this);
	}
}

FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
USQLUISamplePersistenceSettingsApplyPreviewPanelWidget::StoreRefreshResult(
	const FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult& InResult)
{
	LastRefreshResult = InResult;
	Rows = InResult.Rows;
	FormattedLines = InResult.FormattedLines;
	SummaryText = InResult.SummaryText;
	bCanApplyInFuture = InResult.bCanApplyInFuture;
	bIsValid = InResult.bIsValid;
	bHasChanges = InResult.bHasChanges;
	bHasErrors = InResult.bHasErrors;
	bHasWarnings = InResult.bHasWarnings;
	bRequiresRestartOrReinitialize =
		InResult.bRequiresRestartOrReinitialize;
	bWouldNeedProviderReinitialize =
		InResult.bWouldNeedProviderReinitialize;
	bWouldNeedRepositoryReopen = InResult.bWouldNeedRepositoryReopen;
	return LastRefreshResult;
}
