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
	bool EnqueueResult(
		TFunction<ResultType()> Work,
		TFunction<void(const ResultType&)> Completion)
	{
		TSharedRef<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe> Queue = AsShared();

		return EnqueueTask(
			[Queue, Work = MoveTemp(Work), Completion = MoveTemp(Completion)]() mutable
			{
				if (Queue->IsShutdown())
				{
					Queue->CompleteCurrentTask();
					return;
				}

				AsyncTask(
					ENamedThreads::AnyBackgroundThreadNormalTask,
					[Queue, Work = MoveTemp(Work), Completion = MoveTemp(Completion)]() mutable
					{
						const ResultType Result = Work();

						AsyncTask(
							ENamedThreads::GameThread,
							[Queue, Completion = MoveTemp(Completion), Result]() mutable
							{
								if (Queue->ShouldDeliverCompletion())
								{
									Completion(Result);
								}
								Queue->CompleteCurrentTask();
							});
					});
			});
	}

	void ShutdownAndSuppressCallbacks();
	bool IsShutdown() const;

private:
	bool EnqueueTask(TFunction<void()>&& Task);
	bool ShouldDeliverCompletion() const;
	void CompleteCurrentTask();
	void DispatchNextTask();

	mutable FCriticalSection QueueCriticalSection;
	TArray<TFunction<void()>> PendingTasks;
	bool bTaskRunning = false;
	bool bShutdown = false;
};
