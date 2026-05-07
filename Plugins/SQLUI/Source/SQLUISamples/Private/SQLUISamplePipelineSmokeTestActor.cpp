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

void LogSQLUISamplePipelineSmokeTestJsonFixtureMessages(
	const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample pipeline smoke test JSON fixture validation error: %s"),
			*Error);
	}

	for (const FString& Warning : Validation.Warnings)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample pipeline smoke test JSON fixture validation warning: %s"),
			*Warning);
	}
}

void LogSQLUISamplePipelineSmokeTestJsonFixtureResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedJsonLayoutFixture)
	{
		return;
	}

	UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample pipeline smoke test JSON fixture selected."));
	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test JSON fixture parse %s."),
		Result.bJsonLayoutFixtureParseSucceeded ? TEXT("succeeded") : TEXT("failed"));
	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test JSON fixture validation %s."),
		Result.bJsonLayoutFixtureValidationSucceeded ? TEXT("succeeded") : TEXT("failed"));

	LogSQLUISamplePipelineSmokeTestJsonFixtureMessages(Result.JsonLayoutFixtureValidation);
}

void LogSQLUISamplePipelineSmokeTestRepositoryValidationMessages(
	const TCHAR* OperationName,
	const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample pipeline smoke test in-memory layout repository %s validation error: %s"),
			OperationName,
			*Error);
	}

	for (const FString& Warning : Validation.Warnings)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample pipeline smoke test in-memory layout repository %s validation warning: %s"),
			OperationName,
			*Warning);
	}
}

void LogSQLUISamplePipelineSmokeTestRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedInMemoryLayoutRepository)
	{
		return;
	}

	UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample pipeline smoke test in-memory layout repository selected."));
	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test in-memory layout repository save %s. LayoutId='%s'"),
		Result.bRepositorySaveSucceeded ? TEXT("succeeded") : TEXT("failed"),
		*Result.SavedLayoutId);

	if (!Result.RepositorySaveErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample pipeline smoke test in-memory layout repository save error: %s"),
			*Result.RepositorySaveErrorMessage);
	}

	LogSQLUISamplePipelineSmokeTestRepositoryValidationMessages(
		TEXT("save"),
		Result.RepositorySaveValidation);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test in-memory layout repository load %s. LayoutId='%s'"),
		Result.bRepositoryLoadSucceeded ? TEXT("succeeded") : TEXT("failed"),
		*Result.LoadedLayoutId);

	if (!Result.RepositoryLoadErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample pipeline smoke test in-memory layout repository load error: %s"),
			*Result.RepositoryLoadErrorMessage);
	}

	LogSQLUISamplePipelineSmokeTestRepositoryValidationMessages(
		TEXT("load"),
		Result.RepositoryLoadValidation);
}

void LogSQLUISamplePipelineSmokeTestJsonFileRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedJsonFileLayoutRepository)
	{
		return;
	}

	UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample pipeline smoke test JSON file layout repository selected."));
	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test JSON file layout repository save %s. LayoutId='%s'"),
		Result.bJsonFileRepositorySaveSucceeded ? TEXT("succeeded") : TEXT("failed"),
		*Result.JsonFileRepositorySavedLayoutId);

	if (!Result.JsonFileRepositorySaveErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample pipeline smoke test JSON file layout repository save error: %s"),
			*Result.JsonFileRepositorySaveErrorMessage);
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test JSON file layout repository load %s. LayoutId='%s'"),
		Result.bJsonFileRepositoryLoadSucceeded ? TEXT("succeeded") : TEXT("failed"),
		*Result.JsonFileRepositoryLoadedLayoutId);

	if (!Result.JsonFileRepositoryLoadErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample pipeline smoke test JSON file layout repository load error: %s"),
			*Result.JsonFileRepositoryLoadErrorMessage);
	}
}

void LogSQLUISamplePipelineSmokeTestResult(const FSQLUISampleSmokeTestResult& Result)
{
	LogSQLUISamplePipelineSmokeTestJsonFixtureResult(Result);
	LogSQLUISamplePipelineSmokeTestRepositoryResult(Result);
	LogSQLUISamplePipelineSmokeTestJsonFileRepositoryResult(Result);

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
