#include "SQLUISamplesModule.h"

#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "SQLUISamplePackagedRuntimePersistenceWorkflowSmoke.h"
#include "SQLUISamplePackagedRuntimeProviderSubsystemSmoke.h"
#include "SQLUISamplePackagedRuntimeProviderStartupSmoke.h"
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

	if (FParse::Param(FCommandLine::Get(), TEXT("SQLUIRuntimeProviderStartupSmoke")))
	{
		PackagedRuntimeProviderStartupSmokeDelegateHandle =
			FCoreDelegates::OnFEngineLoopInitComplete.AddRaw(
				this,
				&FSQLUISamplesModule::RunPackagedRuntimeProviderStartupSmoke);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("SQLUIRuntimeProviderSubsystemSmoke")))
	{
		PackagedRuntimeProviderSubsystemSmokeDelegateHandle =
			FCoreDelegates::OnFEngineLoopInitComplete.AddRaw(
				this,
				&FSQLUISamplesModule::RunPackagedRuntimeProviderSubsystemSmoke);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("SQLUIRuntimePersistenceWorkflowSmoke")))
	{
		PackagedRuntimePersistenceWorkflowSmokeDelegateHandle =
			FCoreDelegates::OnFEngineLoopInitComplete.AddRaw(
				this,
				&FSQLUISamplesModule::RunPackagedRuntimePersistenceWorkflowSmoke);
	}
}

void FSQLUISamplesModule::ShutdownModule()
{
	if (PackagedRuntimeSQLiteSmokeDelegateHandle.IsValid())
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(PackagedRuntimeSQLiteSmokeDelegateHandle);
		PackagedRuntimeSQLiteSmokeDelegateHandle.Reset();
	}

	if (PackagedRuntimeProviderStartupSmokeDelegateHandle.IsValid())
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(
			PackagedRuntimeProviderStartupSmokeDelegateHandle);
		PackagedRuntimeProviderStartupSmokeDelegateHandle.Reset();
	}

	if (PackagedRuntimeProviderSubsystemSmokeDelegateHandle.IsValid())
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(
			PackagedRuntimeProviderSubsystemSmokeDelegateHandle);
		PackagedRuntimeProviderSubsystemSmokeDelegateHandle.Reset();
	}

	if (PackagedRuntimePersistenceWorkflowSmokeDelegateHandle.IsValid())
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(
			PackagedRuntimePersistenceWorkflowSmokeDelegateHandle);
		PackagedRuntimePersistenceWorkflowSmokeDelegateHandle.Reset();
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

void FSQLUISamplesModule::RunPackagedRuntimeProviderStartupSmoke()
{
	if (PackagedRuntimeProviderStartupSmokeDelegateHandle.IsValid())
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(
			PackagedRuntimeProviderStartupSmokeDelegateHandle);
		PackagedRuntimeProviderStartupSmokeDelegateHandle.Reset();
	}

	FSQLUISamplePackagedRuntimeProviderStartupSmoke::RunAndRequestExit();
}

void FSQLUISamplesModule::RunPackagedRuntimeProviderSubsystemSmoke()
{
	if (PackagedRuntimeProviderSubsystemSmokeDelegateHandle.IsValid())
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(
			PackagedRuntimeProviderSubsystemSmokeDelegateHandle);
		PackagedRuntimeProviderSubsystemSmokeDelegateHandle.Reset();
	}

	FSQLUISamplePackagedRuntimeProviderSubsystemSmoke::RunAndRequestExit();
}

void FSQLUISamplesModule::RunPackagedRuntimePersistenceWorkflowSmoke()
{
	if (PackagedRuntimePersistenceWorkflowSmokeDelegateHandle.IsValid())
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(
			PackagedRuntimePersistenceWorkflowSmokeDelegateHandle);
		PackagedRuntimePersistenceWorkflowSmokeDelegateHandle.Reset();
	}

	FSQLUISamplePackagedRuntimePersistenceWorkflowSmoke::RunAndRequestExit();
}

IMPLEMENT_MODULE(FSQLUISamplesModule, SQLUISamples)
