#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSQLUISamples, Log, All);

class FSQLUISamplesModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RunPackagedRuntimeSQLiteSmoke();
	void RunPackagedRuntimeProviderStartupSmoke();
	void RunPackagedRuntimeProviderSubsystemSmoke();
	void RunPackagedRuntimePersistenceWorkflowSmoke();

	FDelegateHandle PackagedRuntimeSQLiteSmokeDelegateHandle;
	FDelegateHandle PackagedRuntimeProviderStartupSmokeDelegateHandle;
	FDelegateHandle PackagedRuntimeProviderSubsystemSmokeDelegateHandle;
	FDelegateHandle PackagedRuntimePersistenceWorkflowSmokeDelegateHandle;
};
