#include "Layout/SQLUISQLiteLayoutRepository.h"

#include "Database/SQLUIDatabaseAsyncQueue.h"
#include "Layout/SQLUISQLiteLayoutRepositoryWorker.h"

namespace
{
const TCHAR* SQLUISQLiteAsyncQueueShutdownMessage =
	TEXT("SQLite layout repository async queue is shut down.");

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

bool ShouldShutdownSQLUISQLiteAsyncQueueForSettings(
	const FSQLUISQLiteLayoutRepositorySettings& CurrentSettings,
	const FSQLUISQLiteLayoutRepositorySettings& NewSettings)
{
	return !NewSettings.bRunCallbackOperationsAsync
		|| CurrentSettings.DatabasePath != NewSettings.DatabasePath
		|| CurrentSettings.bReadOnly != NewSettings.bReadOnly
		|| CurrentSettings.bRunCallbackOperationsAsync != NewSettings.bRunCallbackOperationsAsync
		|| CurrentSettings.bInitializeSchemaIfMissing != NewSettings.bInitializeSchemaIfMissing
		|| CurrentSettings.bCreateDatabaseIfMissing != NewSettings.bCreateDatabaseIfMissing;
}

FSQLUILayoutLoadResult MakeSQLUISQLiteAsyncQueueShutdownLoadFailure(
	const FString& LayoutId)
{
	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = false;
	Result.Document.Metadata.LayoutId = LayoutId;
	Result.ErrorMessage = SQLUISQLiteAsyncQueueShutdownMessage;
	return Result;
}

FSQLUILayoutSaveResult MakeSQLUISQLiteAsyncQueueShutdownSaveFailure(
	const FString& LayoutId)
{
	FSQLUILayoutSaveResult Result;
	Result.bSucceeded = false;
	Result.SavedLayoutId = LayoutId;
	Result.ErrorMessage = SQLUISQLiteAsyncQueueShutdownMessage;
	return Result;
}
}

void USQLUISQLiteLayoutRepository::BeginDestroy()
{
	if (AsyncQueue.IsValid())
	{
		AsyncQueue->ShutdownAndSuppressCallbacks();
		AsyncQueue.Reset();
	}

	Super::BeginDestroy();
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
		const bool bEnqueued = Queue->EnqueueResult<FSQLUILayoutLoadResult>(
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
		if (!bEnqueued)
		{
			Callback.ExecuteIfBound(
				MakeSQLUISQLiteAsyncQueueShutdownLoadFailure(LayoutIdCopy));
		}
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
		const bool bEnqueued = Queue->EnqueueResult<FSQLUILayoutSaveResult>(
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
		if (!bEnqueued)
		{
			Callback.ExecuteIfBound(
				MakeSQLUISQLiteAsyncQueueShutdownSaveFailure(
					DocumentCopy.Metadata.LayoutId));
		}
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
	if (AsyncQueue.IsValid()
		&& ShouldShutdownSQLUISQLiteAsyncQueueForSettings(Settings, InSettings))
	{
		AsyncQueue->ShutdownAndSuppressCallbacks();
		AsyncQueue.Reset();
	}

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

TSharedRef<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe>
USQLUISQLiteLayoutRepository::GetOrCreateAsyncQueue()
{
	if (!AsyncQueue.IsValid())
	{
		AsyncQueue = MakeShared<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe>();
	}

	return AsyncQueue.ToSharedRef();
}
