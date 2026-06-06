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
	SetRows(
		USQLUIPersistenceStatusDisplayLibrary::GetPersistenceStatusDisplayRows(
			WorldContextObject));
}

void USQLUISamplePersistenceStatusPresenter::RefreshFromRuntimeConfig(
	UObject* WorldContextObject,
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	SetRows(
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

void USQLUISamplePersistenceStatusPresenter::SetRows(
	TArray<FSQLUIPersistenceStatusDisplayRow>&& InRows)
{
	Rows = MoveTemp(InRows);
	RebuildFormattedLines();
}

void USQLUISamplePersistenceStatusPresenter::RebuildFormattedLines()
{
	FormattedLines.Reset(Rows.Num());
	for (const FSQLUIPersistenceStatusDisplayRow& Row : Rows)
	{
		FormattedLines.Add(FormatSQLUISamplePersistenceStatusLine(Row));
	}
}
