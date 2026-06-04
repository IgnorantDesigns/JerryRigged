#pragma once

#include "Async/Async.h"
#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Templates/Function.h"
#include "Templates/SharedPointer.h"

class SQLUICORE_API FSQLUIDatabaseAsyncQueue
	: public TSharedFromThis<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe>
{
public:
	template<typename ResultType>
	void EnqueueResult(
		TFunction<ResultType()> Work,
		TFunction<void(const ResultType&)> Completion)
	{
		TSharedRef<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe> Queue = AsShared();

		EnqueueTask(
			[Queue, Work = MoveTemp(Work), Completion = MoveTemp(Completion)]() mutable
			{
				AsyncTask(
					ENamedThreads::AnyBackgroundThreadNormalTask,
					[Queue, Work = MoveTemp(Work), Completion = MoveTemp(Completion)]() mutable
					{
						const ResultType Result = Work();

						AsyncTask(
							ENamedThreads::GameThread,
							[Queue, Completion = MoveTemp(Completion), Result]() mutable
							{
								Completion(Result);
								Queue->CompleteCurrentTask();
							});
					});
			});
	}

private:
	void EnqueueTask(TFunction<void()>&& Task);
	void CompleteCurrentTask();
	void DispatchNextTask();

	FCriticalSection QueueCriticalSection;
	TArray<TFunction<void()>> PendingTasks;
	bool bTaskRunning = false;
};
