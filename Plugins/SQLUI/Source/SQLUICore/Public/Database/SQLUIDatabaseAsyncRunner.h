#pragma once

#include "Async/Async.h"
#include "CoreMinimal.h"
#include "Database/SQLUIDatabaseAsyncTypes.h"

class SQLUICORE_API FSQLUIDatabaseAsyncRunner
{
public:
	static void RunAsync(
		const FSQLUIDatabaseAsyncRequest& Request,
		FSQLUIDatabaseAsyncCompleteDelegate Callback);

	template<typename ResultType, typename WorkCallableType, typename CompletionCallableType>
	static void RunResultAsync(
		WorkCallableType Work,
		CompletionCallableType Completion)
	{
		AsyncTask(
			ENamedThreads::AnyBackgroundThreadNormalTask,
			[Work, Completion]() mutable
			{
				const ResultType Result = Work();

				AsyncTask(
					ENamedThreads::GameThread,
					[Completion, Result]() mutable
					{
						Completion(Result);
					});
			});
	}
};
