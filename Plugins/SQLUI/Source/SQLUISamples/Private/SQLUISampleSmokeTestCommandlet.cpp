#include "SQLUISampleSmokeTestCommandlet.h"

#include "SQLUISamplesModule.h"
#include "SQLUISampleSmokeTestRunner.h"
#include "Engine/World.h"
#include "Misc/Parse.h"

namespace
{
const TCHAR* SQLUISampleSmokeTestCommandletWorldName = TEXT("SQLUISampleSmokeTestCommandletWorld");

const TCHAR* SQLUISampleSmokeTestStepStatusToString(
	const ESQLUIRuntimeWidgetPipelineStepStatus Status)
{
	switch (Status)
	{
	case ESQLUIRuntimeWidgetPipelineStepStatus::NotRun:
		return TEXT("NotRun");
	case ESQLUIRuntimeWidgetPipelineStepStatus::Skipped:
		return TEXT("Skipped");
	case ESQLUIRuntimeWidgetPipelineStepStatus::Succeeded:
		return TEXT("Succeeded");
	case ESQLUIRuntimeWidgetPipelineStepStatus::Failed:
		return TEXT("Failed");
	default:
		return TEXT("Unknown");
	}
}

void LogSQLUISampleSmokeTestErrors(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test error: %s"),
			*Message);
	}
}

void LogSQLUISampleSmokeTestWarnings(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample smoke test warning: %s"),
			*Message);
	}
}

void LogSQLUISampleSmokeTestJsonFixtureMessages(
	const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test JSON fixture validation error: %s"),
			*Error);
	}

	for (const FString& Warning : Validation.Warnings)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample smoke test JSON fixture validation warning: %s"),
			*Warning);
	}
}

void LogSQLUISampleSmokeTestJsonFixtureResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedJsonLayoutFixture)
	{
		return;
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test JSON fixture parse %s."),
		Result.bJsonLayoutFixtureParseSucceeded ? TEXT("succeeded") : TEXT("failed"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test JSON fixture validation %s."),
		Result.bJsonLayoutFixtureValidationSucceeded ? TEXT("succeeded") : TEXT("failed"));

	LogSQLUISampleSmokeTestJsonFixtureMessages(Result.JsonLayoutFixtureValidation);
}

void LogSQLUISampleSmokeTestStepErrors(
	const TCHAR* StepName,
	const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test step '%s' error: %s"),
			StepName,
			*Message);
	}
}

void LogSQLUISampleSmokeTestStepWarnings(
	const TCHAR* StepName,
	const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample smoke test step '%s' warning: %s"),
			StepName,
			*Message);
	}
}

void LogSQLUISampleSmokeTestResult(const FSQLUISampleSmokeTestResult& Result)
{
	LogSQLUISampleSmokeTestJsonFixtureResult(Result);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test root widget valid: %s"),
		Result.bRootWidgetValid ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test created widget count: %d"),
		Result.CreatedWidgetCount);

	LogSQLUISampleSmokeTestErrors(Result.Errors);
	LogSQLUISampleSmokeTestWarnings(Result.Warnings);

	for (const FSQLUIRuntimeWidgetPipelineStepResult& StepResult : Result.StepResults)
	{
		UE_LOG(
			LogSQLUISamples,
			Log,
			TEXT("SQLUI sample smoke test step '%s': %s"),
			*StepResult.StepName,
			SQLUISampleSmokeTestStepStatusToString(StepResult.Status));

		LogSQLUISampleSmokeTestStepErrors(
			*StepResult.StepName,
			StepResult.Errors);

		LogSQLUISampleSmokeTestStepWarnings(
			*StepResult.StepName,
			StepResult.Warnings);
	}
}

UWorld* CreateSQLUISampleSmokeTestCommandletWorld()
{
	return UWorld::CreateWorld(
		EWorldType::Game,
		false,
		SQLUISampleSmokeTestCommandletWorldName);
}

void DestroySQLUISampleSmokeTestCommandletWorld(UWorld* World)
{
	if (World)
	{
		World->DestroyWorld(false);
	}
}
}

USQLUISampleSmokeTestCommandlet::USQLUISampleSmokeTestCommandlet()
{
	IsClient = false;
	IsEditor = false;
	IsServer = false;
	LogToConsole = true;
}

int32 USQLUISampleSmokeTestCommandlet::Main(const FString& Params)
{
	UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample smoke test commandlet started."));
	const bool bUseJsonLayoutFixture =
		FParse::Param(*Params, TEXT("UseJsonLayoutFixture"))
		|| FParse::Param(*Params, TEXT("JsonLayoutFixture"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test JSON fixture selected: %s"),
		bUseJsonLayoutFixture ? TEXT("true") : TEXT("false"));

	UWorld* CommandletWorld = CreateSQLUISampleSmokeTestCommandletWorld();
	if (!CommandletWorld)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test commandlet could not create a transient world context."));
		return 1;
	}

	FSQLUISampleSmokeTestResult Result;
	{
		FSQLUISampleSmokeTestRequest Request;
		Request.bUseJsonLayoutFixture = bUseJsonLayoutFixture;
		Result = USQLUISampleSmokeTestRunner::RunSmokeTest(CommandletWorld, Request);
	}

	if (Result.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample smoke test commandlet succeeded."));
	}
	else
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample smoke test commandlet failed."));
	}

	LogSQLUISampleSmokeTestResult(Result);
	DestroySQLUISampleSmokeTestCommandletWorld(CommandletWorld);

	return Result.bSucceeded ? 0 : 1;
}
