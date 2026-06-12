#include "SQLUISamplePersistenceSettingsApplyResultPanelWidget.h"

#include "UObject/UObjectGlobals.h"

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPanelWidget::
	RefreshDefaultPersistenceSettingsApplyResultPanel()
{
	EnsureApplyResultPresenter();
	if (!IsValid(ApplyResultPresenter))
	{
		FSQLUISamplePersistenceSettingsApplyResultRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings apply result presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		ApplyResultPresenter
			->RefreshDefaultPersistenceSettingsApplyResultDisplay());
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPanelWidget::
	RefreshCurrentPersistenceSettingsApplyResultPanel()
{
	EnsureApplyResultPresenter();
	if (!IsValid(ApplyResultPresenter))
	{
		FSQLUISamplePersistenceSettingsApplyResultRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings apply result presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		ApplyResultPresenter
			->RefreshCurrentPersistenceSettingsApplyResultDisplay());
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPanelWidget::
	BuildPersistenceSettingsApplyResultPanel(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	EnsureApplyResultPresenter();
	if (!IsValid(ApplyResultPresenter))
	{
		FSQLUISamplePersistenceSettingsApplyResultRefreshResult Result;
		Result.SummaryText =
			TEXT("Persistence settings apply result presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		ApplyResultPresenter->BuildPersistenceSettingsApplyResultDisplay(
			Draft));
}

TArray<FSQLUIPersistenceSettingsApplyResultDisplayRow>
USQLUISamplePersistenceSettingsApplyResultPanelWidget::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceSettingsApplyResultPanelWidget::GetFormattedLines()
	const
{
	return FormattedLines;
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPanelWidget::GetLastRefreshResult()
	const
{
	return LastRefreshResult;
}

FString
USQLUISamplePersistenceSettingsApplyResultPanelWidget::GetSummaryText() const
{
	return SummaryText;
}

void USQLUISamplePersistenceSettingsApplyResultPanelWidget::
	EnsureApplyResultPresenter()
{
	if (!IsValid(ApplyResultPresenter))
	{
		ApplyResultPresenter =
			NewObject<USQLUISamplePersistenceSettingsApplyResultPresenter>(
				this);
	}
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPanelWidget::StoreRefreshResult(
	const FSQLUISamplePersistenceSettingsApplyResultRefreshResult& InResult)
{
	LastRefreshResult = InResult;
	Rows = InResult.Rows;
	FormattedLines = InResult.FormattedLines;
	SummaryText = InResult.SummaryText;
	bSucceeded = InResult.bSucceeded;
	bApplyResultSucceeded = InResult.bApplyResultSucceeded;
	bActualApplyImplemented = InResult.bActualApplyImplemented;
	bDidWriteConfig = InResult.bDidWriteConfig;
	bDidChangeSettings = InResult.bDidChangeSettings;
	bDidInitializeProvider = InResult.bDidInitializeProvider;
	bDidInitializeRepository = InResult.bDidInitializeRepository;
	bDidCreateDatabaseFiles = InResult.bDidCreateDatabaseFiles;
	bDidCreateDirectories = InResult.bDidCreateDirectories;
	bDidOpenDatabaseForWriting = InResult.bDidOpenDatabaseForWriting;
	bDidRunMigrations = InResult.bDidRunMigrations;
	bDidCopySeedDatabase = InResult.bDidCopySeedDatabase;
	bDidDeleteFiles = InResult.bDidDeleteFiles;
	bRequiresRestartOrReinitialize =
		InResult.bRequiresRestartOrReinitialize;
	bHasErrors = InResult.bHasErrors;
	bHasWarnings = InResult.bHasWarnings;
	Status = InResult.Status;
	return LastRefreshResult;
}
