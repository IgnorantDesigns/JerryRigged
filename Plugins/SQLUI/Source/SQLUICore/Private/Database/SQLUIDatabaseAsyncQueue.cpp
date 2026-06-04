#include "Database/SQLUIDatabaseAsyncQueue.h"

#include "Misc/ScopeLock.h"

bool FSQLUIDatabaseAsyncQueue::EnqueueTask(TFunction<void()>&& Task)
{
	bool bShouldDispatch = false;
	{
		FScopeLock Lock(&QueueCriticalSection);
		if (bShutdown)
		{
			return false;
		}

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

	return true;
}

void FSQLUIDatabaseAsyncQueue::ShutdownAndSuppressCallbacks()
{
	FScopeLock Lock(&QueueCriticalSection);
	bShutdown = true;
	PendingTasks.Empty();
}

bool FSQLUIDatabaseAsyncQueue::IsShutdown() const
{
	FScopeLock Lock(&QueueCriticalSection);
	return bShutdown;
}

bool FSQLUIDatabaseAsyncQueue::ShouldDeliverCompletion() const
{
	FScopeLock Lock(&QueueCriticalSection);
	return !bShutdown;
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
		if (bShutdown)
		{
			PendingTasks.Empty();
			bTaskRunning = false;
			return;
		}

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
