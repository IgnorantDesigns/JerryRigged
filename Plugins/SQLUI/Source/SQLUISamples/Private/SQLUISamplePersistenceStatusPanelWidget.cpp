#include "SQLUISamplePersistenceStatusPanelWidget.h"

#include "UObject/UObjectGlobals.h"

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPanelWidget::RefreshPersistenceStatusPanel()
{
	EnsurePanelAdapter();
	if (!IsValid(PanelAdapter))
	{
		FSQLUISamplePersistenceStatusRefreshResult Result;
		Result.SummaryText = TEXT("Persistence status panel adapter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(PanelAdapter->RefreshPersistenceStatusPanel(this));
}

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPanelWidget::RefreshPersistenceStatusPanelFromRuntimeConfig(
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	EnsurePanelAdapter();
	if (!IsValid(PanelAdapter))
	{
		FSQLUISamplePersistenceStatusRefreshResult Result;
		Result.SummaryText = TEXT("Persistence status panel adapter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		PanelAdapter->RefreshPersistenceStatusPanelFromRuntimeConfig(
			this,
			RuntimeConfig));
}

TArray<FSQLUIPersistenceStatusDisplayRow>
USQLUISamplePersistenceStatusPanelWidget::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceStatusPanelWidget::GetFormattedLines() const
{
	return FormattedLines;
}

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPanelWidget::GetLastRefreshResult() const
{
	return LastRefreshResult;
}

FString USQLUISamplePersistenceStatusPanelWidget::GetSummaryText() const
{
	return SummaryText;
}

void USQLUISamplePersistenceStatusPanelWidget::EnsurePanelAdapter()
{
	if (!IsValid(PanelAdapter))
	{
		PanelAdapter = NewObject<USQLUISamplePersistenceStatusPanelAdapter>(this);
	}
}

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPanelWidget::StoreRefreshResult(
	const FSQLUISamplePersistenceStatusRefreshResult& InResult)
{
	LastRefreshResult = InResult;
	Rows = InResult.Rows;
	FormattedLines = InResult.FormattedLines;
	SummaryText = InResult.SummaryText;
	return LastRefreshResult;
}
