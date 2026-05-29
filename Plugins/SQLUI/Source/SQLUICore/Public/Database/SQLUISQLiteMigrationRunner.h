#pragma once

#include "CoreMinimal.h"
#include "Database/SQLUISQLiteMigrationTypes.h"

class SQLUICORE_API FSQLUISQLiteMigrationRunner
{
public:
	static FString GetDefaultProbeDatabasePath();

	static FSQLUISQLiteMigrationResult RunMigrationProbe(
		const FString& DatabasePath = FString(),
		bool bRemoveDatabaseAfterClose = true);

	static FSQLUISQLiteMigrationResult RunMigrations(
		const FString& DatabasePath,
		const TArray<FSQLUISQLiteMigrationStep>& MigrationSteps,
		bool bRemoveDatabaseAfterClose = false);
};
