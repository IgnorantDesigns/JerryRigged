#include "SQLUISamplePersistenceSettingsApplyResultPresenter.h"

namespace
{
const TCHAR* SQLUISamplePersistenceSettingsApplyResultDisplayStateToString(
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

FString FormatSQLUISamplePersistenceSettingsApplyResultLine(
	const FSQLUIPersistenceSettingsApplyResultDisplayRow& Row)
{
	FString Line = FString::Printf(
		TEXT("%s: %s [%s]"),
		*Row.Label.ToString(),
		*Row.Value.ToString(),
		SQLUISamplePersistenceSettingsApplyResultDisplayStateToString(
			Row.State));

	const FString Detail = Row.DetailText.ToString();
	if (!Detail.IsEmpty())
	{
		Line += FString::Printf(TEXT(" - %s"), *Detail);
	}

	return Line;
}
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPresenter::
	RefreshDefaultPersistenceSettingsApplyResultDisplay()
{
	return BuildPersistenceSettingsApplyResultDisplay(
		USQLUIPersistenceSettingsDraftLibrary::MakeDefaultPersistenceSettingsDraft());
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPresenter::
	RefreshCurrentPersistenceSettingsApplyResultDisplay()
{
	return BuildPersistenceSettingsApplyResultDisplay(
		USQLUIPersistenceSettingsDraftLibrary::MakeCurrentPersistenceSettingsDraft());
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPresenter::
	BuildPersistenceSettingsApplyResultDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft)
{
	return StoreDisplaySummary(
		USQLUIPersistenceSettingsApplyResultDisplayLibrary::
			RequestAndMakePersistenceSettingsApplyResultDisplay(Draft));
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPresenter::
	BuildPersistenceSettingsApplyResultDisplayFromResult(
		const FSQLUIPersistenceSettingsApplyResult& ApplyResult)
{
	return StoreDisplaySummary(
		USQLUIPersistenceSettingsApplyResultDisplayLibrary::
			MakePersistenceSettingsApplyResultDisplay(ApplyResult));
}

TArray<FSQLUIPersistenceSettingsApplyResultDisplayRow>
USQLUISamplePersistenceSettingsApplyResultPresenter::GetRows() const
{
	return Rows;
}

TArray<FString>
USQLUISamplePersistenceSettingsApplyResultPresenter::GetFormattedLines() const
{
	return FormattedLines;
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPresenter::GetLastRefreshResult()
	const
{
	return LastRefreshResult;
}

FString
USQLUISamplePersistenceSettingsApplyResultPresenter::GetSummaryText() const
{
	return SummaryText;
}

FSQLUISamplePersistenceSettingsApplyResultRefreshResult
USQLUISamplePersistenceSettingsApplyResultPresenter::StoreDisplaySummary(
	const FSQLUIPersistenceSettingsApplyResultDisplaySummary& InSummary)
{
	Rows = InSummary.Rows;
	SummaryText = InSummary.SummaryText.ToString();
	RebuildFormattedLines();

	LastRefreshResult.DisplaySummary = InSummary;
	LastRefreshResult.Rows = Rows;
	LastRefreshResult.FormattedLines = FormattedLines;
	LastRefreshResult.SummaryText = SummaryText;
	LastRefreshResult.bApplyResultSucceeded = InSummary.bSucceeded;
	LastRefreshResult.bActualApplyImplemented =
		InSummary.bActualApplyImplemented;
	LastRefreshResult.bDidWriteConfig = InSummary.bDidWriteConfig;
	LastRefreshResult.bDidChangeSettings = InSummary.bDidChangeSettings;
	LastRefreshResult.bDidInitializeProvider =
		InSummary.bDidInitializeProvider;
	LastRefreshResult.bDidInitializeRepository =
		InSummary.bDidInitializeRepository;
	LastRefreshResult.bDidCreateDatabaseFiles =
		InSummary.bDidCreateDatabaseFiles;
	LastRefreshResult.bDidCreateDirectories = InSummary.bDidCreateDirectories;
	LastRefreshResult.bDidOpenDatabaseForWriting =
		InSummary.bDidOpenDatabaseForWriting;
	LastRefreshResult.bDidRunMigrations = InSummary.bDidRunMigrations;
	LastRefreshResult.bDidCopySeedDatabase = InSummary.bDidCopySeedDatabase;
	LastRefreshResult.bDidDeleteFiles = InSummary.bDidDeleteFiles;
	LastRefreshResult.bRequiresRestartOrReinitialize =
		InSummary.bRequiresRestartOrReinitialize;
	LastRefreshResult.bHasErrors = InSummary.bHasErrors;
	LastRefreshResult.bHasWarnings = InSummary.bHasWarnings;
	LastRefreshResult.Status = InSummary.Status;
	LastRefreshResult.bSucceeded = Rows.Num() > 0;
	return LastRefreshResult;
}

void USQLUISamplePersistenceSettingsApplyResultPresenter::
	RebuildFormattedLines()
{
	FormattedLines.Reset(Rows.Num());
	for (const FSQLUIPersistenceSettingsApplyResultDisplayRow& Row : Rows)
	{
		FormattedLines.Add(
			FormatSQLUISamplePersistenceSettingsApplyResultLine(Row));
	}
}
