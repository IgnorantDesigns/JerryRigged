#pragma once

#include "CoreMinimal.h"

class SQLUICORE_API FSQLUISQLiteAvailability
{
public:
	static bool IsSQLiteCoreCompiledIn();
	static FString GetSQLiteCoreAvailabilitySummary();
};
