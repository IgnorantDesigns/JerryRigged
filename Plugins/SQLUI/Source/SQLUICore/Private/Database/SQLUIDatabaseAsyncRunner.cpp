#include "Database/SQLUIDatabaseAsyncRunner.h"

#include "Async/Async.h"
#include "HAL/PlatformTLS.h"

namespace
{
FString MakeSQLUIDatabaseAsyncDefaultErrorMessage(const FSQLUIDatabaseAsyncRequest& Request)
{
	if (!Request.ErrorMessage.IsEmpty())
	{
		return Request.ErrorMessage;
	}

	return FString::Printf(
		TEXT("SQLUI database async request '%s' failed by simulation."),
		*Request.RequestId);
}
}

void FSQLUIDatabaseAsyncRunner::RunAsync(
	const FSQLUIDatabaseAsyncRequest& Request,
	FSQLUIDatabaseAsyncCompleteDelegate Callback)
{
	const FSQLUIDatabaseAsyncRequest RequestCopy = Request;

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [RequestCopy, Callback]()
	{
		FSQLUIDatabaseAsyncResult Result;
		Result.RequestId = RequestCopy.RequestId;
		Result.DebugName = RequestCopy.DebugName;
		Result.bBackgroundWorkCompleted = true;
		Result.bRanOnBackgroundThread = !IsInGameThread();
		Result.WorkerThreadId = static_cast<int32>(FPlatformTLS::GetCurrentThreadId());
		Result.bSucceeded = RequestCopy.bSimulateSuccess;

		if (!Result.bSucceeded)
		{
			Result.ErrorMessage = MakeSQLUIDatabaseAsyncDefaultErrorMessage(RequestCopy);
		}

		AsyncTask(ENamedThreads::GameThread, [Callback, Result]()
		{
			FSQLUIDatabaseAsyncResult DeliveredResult = Result;
			DeliveredResult.bDeliveredOnGameThread = IsInGameThread();
			Callback.ExecuteIfBound(DeliveredResult);
		});
	});
}
