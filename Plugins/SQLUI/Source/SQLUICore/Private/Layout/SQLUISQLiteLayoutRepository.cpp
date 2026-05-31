#include "Layout/SQLUISQLiteLayoutRepository.h"

#include "Layout/SQLUISQLiteLayoutRepositoryWorker.h"

namespace
{
FSQLUISQLiteLayoutRepositoryWorkerSettings MakeSQLUISQLiteLayoutRepositoryWorkerSettings(
	const FSQLUISQLiteLayoutRepositorySettings& Settings)
{
	FSQLUISQLiteLayoutRepositoryWorkerSettings WorkerSettings;
	WorkerSettings.DatabasePath = Settings.DatabasePath;
	return WorkerSettings;
}
}

void USQLUISQLiteLayoutRepository::LoadLayout(
	const FString& LayoutId,
	FSQLUILayoutLoadCompleteDelegate Callback)
{
	Callback.ExecuteIfBound(LoadLayoutById(LayoutId));
}

void USQLUISQLiteLayoutRepository::SaveLayout(
	const FSQLUILayoutDocument& Document,
	FSQLUILayoutSaveCompleteDelegate Callback)
{
	if (Settings.bReadOnly)
	{
		Callback.ExecuteIfBound(
			FSQLUISQLiteLayoutRepositoryWorker::MakeReadOnlySaveFailure(
				Document.Metadata.LayoutId));
		return;
	}

	Callback.ExecuteIfBound(
		FSQLUISQLiteLayoutRepositoryWorker::SaveLayout(
			MakeSQLUISQLiteLayoutRepositoryWorkerSettings(Settings),
			Document));
}

void USQLUISQLiteLayoutRepository::Configure(
	const FSQLUISQLiteLayoutRepositorySettings& InSettings)
{
	Settings = InSettings;
}

FSQLUISQLiteLayoutRepositorySettings USQLUISQLiteLayoutRepository::GetSettings() const
{
	return Settings;
}

FString USQLUISQLiteLayoutRepository::GetResolvedDatabasePath() const
{
	return FSQLUISQLiteLayoutRepositoryWorker::ResolveDatabasePath(Settings.DatabasePath);
}

FSQLUILayoutLoadResult USQLUISQLiteLayoutRepository::LoadLayoutById(
	const FString& LayoutId) const
{
	return FSQLUISQLiteLayoutRepositoryWorker::LoadLayoutById(
		MakeSQLUISQLiteLayoutRepositoryWorkerSettings(Settings),
		LayoutId);
}

FSQLUILayoutRepositoryListResult USQLUISQLiteLayoutRepository::ListLayouts() const
{
	return FSQLUISQLiteLayoutRepositoryWorker::ListLayouts(
		MakeSQLUISQLiteLayoutRepositoryWorkerSettings(Settings));
}

FSQLUILayoutRepositoryRemoveResult USQLUISQLiteLayoutRepository::RemoveLayout(
	const FString& LayoutId)
{
	if (Settings.bReadOnly)
	{
		return FSQLUISQLiteLayoutRepositoryWorker::MakeReadOnlyRemoveFailure(LayoutId);
	}

	return FSQLUISQLiteLayoutRepositoryWorker::RemoveLayout(
		MakeSQLUISQLiteLayoutRepositoryWorkerSettings(Settings),
		LayoutId);
}

FSQLUILayoutRepositoryClearResult USQLUISQLiteLayoutRepository::ClearLayouts()
{
	if (Settings.bReadOnly)
	{
		return FSQLUISQLiteLayoutRepositoryWorker::MakeReadOnlyClearFailure();
	}

	return FSQLUISQLiteLayoutRepositoryWorker::ClearLayouts(
		MakeSQLUISQLiteLayoutRepositoryWorkerSettings(Settings));
}
