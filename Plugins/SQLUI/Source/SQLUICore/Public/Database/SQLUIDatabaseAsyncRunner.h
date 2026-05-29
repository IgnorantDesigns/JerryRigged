#pragma once

#include "CoreMinimal.h"
#include "Database/SQLUIDatabaseAsyncTypes.h"

class SQLUICORE_API FSQLUIDatabaseAsyncRunner
{
public:
	static void RunAsync(
		const FSQLUIDatabaseAsyncRequest& Request,
		FSQLUIDatabaseAsyncCompleteDelegate Callback);
};
