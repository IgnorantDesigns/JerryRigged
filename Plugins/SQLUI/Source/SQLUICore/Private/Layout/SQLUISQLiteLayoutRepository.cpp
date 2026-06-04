#include "Layout/SQLUISQLiteLayoutRepository.h"

#include "Database/SQLUIDatabaseAsyncQueue.h"
#include "Layout/SQLUISQLiteLayoutRepositoryWorker.h"

namespace
{
FSQLUISQLiteLayoutRepositoryWorkerSettings MakeSQLUISQLiteLayoutRepositoryWorkerSettings(
	const FSQLUISQLiteLayoutRepositorySettings& Settings)
{
	FSQLUISQLiteLayoutRepositoryWorkerSettings WorkerSettings;
	WorkerSettings.DatabasePath = Settings.DatabasePath;
	WorkerSettings.bInitializeSchemaIfMissing = Settings.bInitializeSchemaIfMissing;
	WorkerSettings.bCreateDatabaseIfMissing = Settings.bCreateDatabaseIfMissing;
	WorkerSettings.bAllowSchemaInitializationWrites = !Settings.bReadOnly;
	return WorkerSettings;
}
}

void USQLUISQLiteLayoutRepository::LoadLayout(
	const FString& LayoutId,
	FSQLUILayoutLoadCompleteDelegate Callback)
{
	if (Settings.bRunCallbackOperationsAsync)
	{
		const FSQLUISQLiteLayoutRepositoryWorkerSettings WorkerSettings =
			MakeSQLUISQLiteLayoutRepositoryWorkerSettings(Settings);
		const FString LayoutIdCopy = LayoutId;

		TSharedRef<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe> Queue =
			GetOrCreateAsyncQueue();
		Queue->EnqueueResult<FSQLUILayoutLoadResult>(
			[WorkerSettings, LayoutIdCopy]()
			{
				return FSQLUISQLiteLayoutRepositoryWorker::LoadLayoutById(
					WorkerSettings,
					LayoutIdCopy);
			},
			[Callback](const FSQLUILayoutLoadResult& Result) mutable
			{
				Callback.ExecuteIfBound(Result);
			});
		return;
	}

	Callback.ExecuteIfBound(LoadLayoutById(LayoutId));
}

void USQLUISQLiteLayoutRepository::SaveLayout(
	const FSQLUILayoutDocument& Document,
	FSQLUILayoutSaveCompleteDelegate Callback)
{
	if (Settings.bRunCallbackOperationsAsync)
	{
		const FSQLUISQLiteLayoutRepositoryWorkerSettings WorkerSettings =
			MakeSQLUISQLiteLayoutRepositoryWorkerSettings(Settings);
		const FSQLUILayoutDocument DocumentCopy = Document;
		const bool bReadOnly = Settings.bReadOnly;

		TSharedRef<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe> Queue =
			GetOrCreateAsyncQueue();
		Queue->EnqueueResult<FSQLUILayoutSaveResult>(
			[WorkerSettings, DocumentCopy, bReadOnly]()
			{
				if (bReadOnly)
				{
					return FSQLUISQLiteLayoutRepositoryWorker::MakeReadOnlySaveFailure(
						DocumentCopy.Metadata.LayoutId);
				}

				return FSQLUISQLiteLayoutRepositoryWorker::SaveLayout(
					WorkerSettings,
					DocumentCopy);
			},
			[Callback](const FSQLUILayoutSaveResult& Result) mutable
			{
				Callback.ExecuteIfBound(Result);
			});
		return;
	}

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
	if (!Settings.bRunCallbackOperationsAsync)
	{
		AsyncQueue.Reset();
	}
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

TSharedRef<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe>
USQLUISQLiteLayoutRepository::GetOrCreateAsyncQueue()
{
	if (!AsyncQueue.IsValid())
	{
		AsyncQueue = MakeShared<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe>();
	}

	return AsyncQueue.ToSharedRef();
}
