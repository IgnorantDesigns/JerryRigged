#include "SQLUISamplePersistenceStatusPanelAdapter.h"

#include "UObject/UObjectGlobals.h"

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPanelAdapter::RefreshPersistenceStatusPanel(
	UObject* WorldContextObject)
{
	EnsurePresenter();
	if (!IsValid(Presenter))
	{
		FSQLUISamplePersistenceStatusRefreshResult Result;
		Result.SummaryText = TEXT("Persistence status presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(Presenter->RefreshPersistenceStatus(WorldContextObject));
}

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPanelAdapter::RefreshPersistenceStatusPanelFromRuntimeConfig(
	UObject* WorldContextObject,
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	EnsurePresenter();
	if (!IsValid(Presenter))
	{
		FSQLUISamplePersistenceStatusRefreshResult Result;
		Result.SummaryText = TEXT("Persistence status presenter was not available.");
		return StoreRefreshResult(Result);
	}

	return StoreRefreshResult(
		Presenter->RefreshPersistenceStatusFromRuntimeConfig(
			WorldContextObject,
			RuntimeConfig));
}

TArray<FSQLUIPersistenceStatusDisplayRow>
USQLUISamplePersistenceStatusPanelAdapter::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceStatusPanelAdapter::GetFormattedLines() const
{
	return FormattedLines;
}

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPanelAdapter::GetLastRefreshResult() const
{
	return LastRefreshResult;
}

FString USQLUISamplePersistenceStatusPanelAdapter::GetSummaryText() const
{
	return SummaryText;
}

void USQLUISamplePersistenceStatusPanelAdapter::EnsurePresenter()
{
	if (!IsValid(Presenter))
	{
		Presenter = NewObject<USQLUISamplePersistenceStatusPresenter>(this);
	}
}

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPanelAdapter::StoreRefreshResult(
	const FSQLUISamplePersistenceStatusRefreshResult& InResult)
{
	LastRefreshResult = InResult;
	Rows = InResult.Rows;
	FormattedLines = InResult.FormattedLines;
	SummaryText = InResult.SummaryText;
	return LastRefreshResult;
}
