#include "SQLUISamplesModule.h"

#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "SQLUISamplePackagedRuntimeSQLiteSmoke.h"

DEFINE_LOG_CATEGORY(LogSQLUISamples);

void FSQLUISamplesModule::StartupModule()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("SQLUIPackagedRuntimeSQLiteSmoke")))
	{
		PackagedRuntimeSQLiteSmokeDelegateHandle =
			FCoreDelegates::OnFEngineLoopInitComplete.AddRaw(
				this,
				&FSQLUISamplesModule::RunPackagedRuntimeSQLiteSmoke);
	}
}

void FSQLUISamplesModule::ShutdownModule()
{
	if (PackagedRuntimeSQLiteSmokeDelegateHandle.IsValid())
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(PackagedRuntimeSQLiteSmokeDelegateHandle);
		PackagedRuntimeSQLiteSmokeDelegateHandle.Reset();
	}
}

void FSQLUISamplesModule::RunPackagedRuntimeSQLiteSmoke()
{
	if (PackagedRuntimeSQLiteSmokeDelegateHandle.IsValid())
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(PackagedRuntimeSQLiteSmokeDelegateHandle);
		PackagedRuntimeSQLiteSmokeDelegateHandle.Reset();
	}

	FSQLUISamplePackagedRuntimeSQLiteSmoke::RunAndRequestExit();
}

IMPLEMENT_MODULE(FSQLUISamplesModule, SQLUISamples)
