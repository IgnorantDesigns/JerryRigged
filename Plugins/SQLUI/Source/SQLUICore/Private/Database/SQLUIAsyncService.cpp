#include "Database/SQLUIAsyncService.h"

#include "Async/Async.h"

namespace
{
FSQLUIQueryResult MakeSQLUIUnavailableResult(const FString& ErrorMessage)
{
	FSQLUIQueryResult Result;
	Result.bSucceeded = false;
	Result.bBackendUnavailable = true;
	Result.ErrorMessage = ErrorMessage;
	return Result;
}
}

void USQLUIAsyncService::Initialize(
	const FString& InDbKey,
	const FString& InDatabasePath,
	TSharedPtr<ISQLUIBackendService, ESPMode::ThreadSafe> InBackend)
{
	DbKey = InDbKey;
	DatabasePath = InDatabasePath;
	Backend = MoveTemp(InBackend);
	bIsShuttingDown = false;

	if (Backend.IsValid())
	{
		FString OpenErrorMessage;
		Backend->Open(DatabasePath, OpenErrorMessage);
	}
}

void USQLUIAsyncService::Shutdown()
{
	bIsShuttingDown = true;

	if (Backend.IsValid())
	{
		Backend->Close();
		Backend.Reset();
	}
}

void USQLUIAsyncService::ExecuteAsync(const FSQLUIQueryRequest& Request, FSQLUIQueryCompleteDelegate Callback)
{
	FSQLUIQueryRequest RequestCopy = Request;
	if (RequestCopy.DbKey.IsEmpty())
	{
		RequestCopy.DbKey = DbKey;
	}
	if (RequestCopy.DatabasePath.IsEmpty())
	{
		RequestCopy.DatabasePath = DatabasePath;
	}

	TWeakObjectPtr<USQLUIAsyncService> WeakThis(this);
	TSharedPtr<ISQLUIBackendService, ESPMode::ThreadSafe> BackendCopy = Backend;

	if (bIsShuttingDown)
	{
		const FSQLUIQueryResult Result = MakeSQLUIUnavailableResult(TEXT("SQLUI async service is shutting down."));
		AsyncTask(ENamedThreads::GameThread, [WeakThis, Callback, Result]()
		{
			if (WeakThis.IsValid())
			{
				Callback.ExecuteIfBound(Result);
			}
		});
		return;
	}

	if (!BackendCopy.IsValid())
	{
		const FSQLUIQueryResult Result = MakeSQLUIUnavailableResult(TEXT("SQLUI async service has no backend."));
		AsyncTask(ENamedThreads::GameThread, [WeakThis, Callback, Result]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsShuttingDown())
			{
				Callback.ExecuteIfBound(Result);
			}
		});
		return;
	}

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, BackendCopy, RequestCopy, Callback]()
	{
		const FSQLUIQueryResult Result = BackendCopy->Execute(RequestCopy);

		AsyncTask(ENamedThreads::GameThread, [WeakThis, Callback, Result]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsShuttingDown())
			{
				Callback.ExecuteIfBound(Result);
			}
		});
	});
}

bool USQLUIAsyncService::IsInitialized() const
{
	return Backend.IsValid();
}

bool USQLUIAsyncService::IsShuttingDown() const
{
	return bIsShuttingDown;
}

const FString& USQLUIAsyncService::GetDbKey() const
{
	return DbKey;
}

const FString& USQLUIAsyncService::GetDatabasePath() const
{
	return DatabasePath;
}

void USQLUIAsyncService::BeginDestroy()
{
	Shutdown();
	Super::BeginDestroy();
}
