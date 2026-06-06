#include "SQLUISamplePersistenceStatusPresenter.h"

namespace
{
const TCHAR* SQLUISamplePersistenceStatusDisplayStateToString(
	const ESQLUIPersistenceStatusDisplayState State)
{
	switch (State)
	{
	case ESQLUIPersistenceStatusDisplayState::Normal:
		return TEXT("Normal");
	case ESQLUIPersistenceStatusDisplayState::Good:
		return TEXT("Good");
	case ESQLUIPersistenceStatusDisplayState::Warning:
		return TEXT("Warning");
	case ESQLUIPersistenceStatusDisplayState::Unavailable:
		return TEXT("Unavailable");
	default:
		return TEXT("Unknown");
	}
}

FString FormatSQLUISamplePersistenceStatusLine(
	const FSQLUIPersistenceStatusDisplayRow& Row)
{
	FString Line = FString::Printf(
		TEXT("%s: %s [%s]"),
		*Row.Label.ToString(),
		*Row.Value.ToString(),
		SQLUISamplePersistenceStatusDisplayStateToString(Row.State));

	const FString Detail = Row.DetailText.ToString();
	if (!Detail.IsEmpty())
	{
		Line += FString::Printf(TEXT(" - %s"), *Detail);
	}

	return Line;
}
}

void USQLUISamplePersistenceStatusPresenter::Refresh(UObject* WorldContextObject)
{
	RefreshPersistenceStatus(WorldContextObject);
}

void USQLUISamplePersistenceStatusPresenter::RefreshFromRuntimeConfig(
	UObject* WorldContextObject,
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	RefreshPersistenceStatusFromRuntimeConfig(WorldContextObject, RuntimeConfig);
}

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPresenter::RefreshPersistenceStatus(
	UObject* WorldContextObject)
{
	return SetRows(
		USQLUIPersistenceStatusDisplayLibrary::GetPersistenceStatusDisplayRows(
			WorldContextObject));
}

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPresenter::RefreshPersistenceStatusFromRuntimeConfig(
	UObject* WorldContextObject,
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	return SetRows(
		USQLUIPersistenceStatusDisplayLibrary::
			GetPersistenceStatusDisplayRowsFromRuntimeConfig(
				WorldContextObject,
				RuntimeConfig));
}

TArray<FSQLUIPersistenceStatusDisplayRow>
USQLUISamplePersistenceStatusPresenter::GetRows() const
{
	return Rows;
}

TArray<FString> USQLUISamplePersistenceStatusPresenter::GetFormattedLines() const
{
	return FormattedLines;
}

FSQLUISamplePersistenceStatusRefreshResult
USQLUISamplePersistenceStatusPresenter::SetRows(
	TArray<FSQLUIPersistenceStatusDisplayRow>&& InRows)
{
	Rows = MoveTemp(InRows);
	RebuildFormattedLines();

	FSQLUISamplePersistenceStatusRefreshResult Result;
	Result.Rows = Rows;
	Result.FormattedLines = FormattedLines;
	Result.bSucceeded = Rows.Num() > 0;
	Result.SummaryText = FString::Printf(
		TEXT("Refreshed %d persistence status row(s)."),
		Rows.Num());
	return Result;
}

void USQLUISamplePersistenceStatusPresenter::RebuildFormattedLines()
{
	FormattedLines.Reset(Rows.Num());
	for (const FSQLUIPersistenceStatusDisplayRow& Row : Rows)
	{
		FormattedLines.Add(FormatSQLUISamplePersistenceStatusLine(Row));
	}
}
