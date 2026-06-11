#include "SQLUISamplePersistenceSettingsApplyContractPanelWidget.h"

#include "UObject/UObjectGlobals.h"

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPanelWidget::
	RefreshDefaultPersistenceSettingsApplyContractPanel()
{
	EnsureApplyContractPresenter();
	if (!IsValid(ApplyContractPresenter))
	{
		FSQLUISamplePersistenceSettingsApplyContractRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings apply contract presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		ApplyContractPresenter
			->RefreshDefaultPersistenceSettingsApplyContractDisplay());
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPanelWidget::
	RefreshCurrentPersistenceSettingsApplyContractPanel()
{
	EnsureApplyContractPresenter();
	if (!IsValid(ApplyContractPresenter))
	{
		FSQLUISamplePersistenceSettingsApplyContractRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings apply contract presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		ApplyContractPresenter
			->RefreshCurrentPersistenceSettingsApplyContractDisplay());
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPanelWidget::
	BuildPersistenceSettingsApplyContractPanel(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	EnsureApplyContractPresenter();
	if (!IsValid(ApplyContractPresenter))
	{
		FSQLUISamplePersistenceSettingsApplyContractRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings apply contract presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		ApplyContractPresenter->BuildPersistenceSettingsApplyContractDisplay(
			Draft));
}

TArray<FSQLUIPersistenceSettingsApplyContractDisplayRow>
USQLUISamplePersistenceSettingsApplyContractPanelWidget::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceSettingsApplyContractPanelWidget::GetFormattedLines()
	const
{
	return FormattedLines;
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPanelWidget::GetLastRefreshResult()
	const
{
	return LastRefreshResult;
}

FString
USQLUISamplePersistenceSettingsApplyContractPanelWidget::GetSummaryText() const
{
	return SummaryText;
}

void USQLUISamplePersistenceSettingsApplyContractPanelWidget::
	EnsureApplyContractPresenter()
{
	if (!IsValid(ApplyContractPresenter))
	{
		ApplyContractPresenter =
			NewObject<USQLUISamplePersistenceSettingsApplyContractPresenter>(
				this);
	}
}

FSQLUISamplePersistenceSettingsApplyContractRefreshResult
USQLUISamplePersistenceSettingsApplyContractPanelWidget::StoreRefreshResult(
	const FSQLUISamplePersistenceSettingsApplyContractRefreshResult& InResult)
{
	LastRefreshResult = InResult;
	Rows = InResult.Rows;
	FormattedLines = InResult.FormattedLines;
	SummaryText = InResult.SummaryText;
	bCanApplyInFuture = InResult.bCanApplyInFuture;
	bActualApplyImplemented = InResult.bActualApplyImplemented;
	bCanExecuteApplyNow = InResult.bCanExecuteApplyNow;
	bIsValid = InResult.bIsValid;
	bHasChanges = InResult.bHasChanges;
	bHasErrors = InResult.bHasErrors;
	bHasWarnings = InResult.bHasWarnings;
	bRequiresRestartOrReinitialize =
		InResult.bRequiresRestartOrReinitialize;
	bWouldNeedProviderReinitialize =
		InResult.bWouldNeedProviderReinitialize;
	bWouldNeedRepositoryReopen = InResult.bWouldNeedRepositoryReopen;
	bWouldDiscardChangesOnCancel =
		InResult.bWouldDiscardChangesOnCancel;
	Availability = InResult.Availability;
	return LastRefreshResult;
}
