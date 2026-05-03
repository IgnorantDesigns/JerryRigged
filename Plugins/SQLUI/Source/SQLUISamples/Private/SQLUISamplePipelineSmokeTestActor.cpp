#include "SQLUISamplePipelineSmokeTestActor.h"

#include "SQLUISamplesModule.h"
#include "Engine/World.h"

namespace
{
const TCHAR* SQLUISamplePipelineSmokeTestStepStatusToString(
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

void LogSQLUISamplePipelineSmokeTestErrors(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample pipeline smoke test error: %s"), *Message);
	}
}

void LogSQLUISamplePipelineSmokeTestWarnings(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(LogSQLUISamples, Warning, TEXT("SQLUI sample pipeline smoke test warning: %s"), *Message);
	}
}

void LogSQLUISamplePipelineSmokeTestResult(const FSQLUISampleSmokeTestResult& Result)
{
	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test root widget valid: %s"),
		Result.bRootWidgetValid ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test created widget count: %d"),
		Result.CreatedWidgetCount);

	for (const FSQLUIRuntimeWidgetPipelineStepResult& StepResult : Result.StepResults)
	{
		UE_LOG(
			LogSQLUISamples,
			Log,
			TEXT("SQLUI sample pipeline smoke test step '%s': %s"),
			*StepResult.StepName,
			SQLUISamplePipelineSmokeTestStepStatusToString(StepResult.Status));
	}

	LogSQLUISamplePipelineSmokeTestErrors(Result.Errors);
	LogSQLUISamplePipelineSmokeTestWarnings(Result.Warnings);
}
}

ASQLUISamplePipelineSmokeTestActor::ASQLUISamplePipelineSmokeTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASQLUISamplePipelineSmokeTestActor::BeginPlay()
{
	Super::BeginPlay();

	if (bRunSmokeTestOnBeginPlay)
	{
		RunSmokeTest();
	}
}

void ASQLUISamplePipelineSmokeTestActor::RunSmokeTest()
{
	FSQLUISampleSmokeTestRequest Request = SmokeTestRequest;
	if (!IsValid(Request.OwningPlayer.Get()) && GetWorld())
	{
		Request.OwningPlayer = GetWorld()->GetFirstPlayerController();
	}

	const FSQLUISampleSmokeTestResult Result =
		USQLUISampleSmokeTestRunner::RunSmokeTest(this, Request);
	if (Result.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample pipeline smoke test succeeded."));
	}
	else
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample pipeline smoke test failed."));
	}

	LogSQLUISamplePipelineSmokeTestResult(Result);
}
