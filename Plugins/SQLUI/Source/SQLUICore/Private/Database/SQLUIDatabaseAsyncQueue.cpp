#include "Database/SQLUIDatabaseAsyncQueue.h"

#include "Misc/ScopeLock.h"

void FSQLUIDatabaseAsyncQueue::EnqueueTask(TFunction<void()>&& Task)
{
	bool bShouldDispatch = false;
	{
		FScopeLock Lock(&QueueCriticalSection);
		PendingTasks.Add(MoveTemp(Task));
		if (!bTaskRunning)
		{
			bTaskRunning = true;
			bShouldDispatch = true;
		}
	}

	if (bShouldDispatch)
	{
		DispatchNextTask();
	}
}

void FSQLUIDatabaseAsyncQueue::CompleteCurrentTask()
{
	DispatchNextTask();
}

void FSQLUIDatabaseAsyncQueue::DispatchNextTask()
{
	TFunction<void()> NextTask;
	{
		FScopeLock Lock(&QueueCriticalSection);
		if (PendingTasks.Num() == 0)
		{
			bTaskRunning = false;
			return;
		}

		NextTask = MoveTemp(PendingTasks[0]);
		PendingTasks.RemoveAt(0);
	}

	NextTask();
}
