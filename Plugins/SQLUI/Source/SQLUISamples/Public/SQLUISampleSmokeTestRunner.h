#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Pipeline/SQLUIRuntimeWidgetPipeline.h"
#include "UObject/Object.h"

#include "SQLUISampleSmokeTestRunner.generated.h"

class APlayerController;
class USQLUIBaseWidget;

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISampleSmokeTestRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Samples")
	TObjectPtr<APlayerController> OwningPlayer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bApplyProperties = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bApplyBindings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bApplyActions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bExecuteActions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bStopOnOptionalStepFailure = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseJsonLayoutFixture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseInMemoryLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseJsonFileLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString SampleFilterText = TEXT("Smoke test");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString SampleFilterVariableValue = TEXT("Smoke test variable");
};

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISampleSmokeTestResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedJsonLayoutFixture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bJsonLayoutFixtureParseSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bJsonLayoutFixtureValidationSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUILayoutValidationResult JsonLayoutFixtureValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedInMemoryLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRepositorySaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRepositoryLoadSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString SavedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString LoadedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString RepositorySaveErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString RepositoryLoadErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUILayoutValidationResult RepositorySaveValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUILayoutValidationResult RepositoryLoadValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedJsonFileLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bJsonFileRepositorySaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bJsonFileRepositoryLoadSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString JsonFileRepositorySavedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString JsonFileRepositoryLoadedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString JsonFileRepositorySaveErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString JsonFileRepositoryLoadErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	TArray<FString> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	TArray<FString> Warnings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	TArray<FSQLUIRuntimeWidgetPipelineStepResult> StepResults;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 CreatedWidgetCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRootWidgetValid = false;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Samples")
	TObjectPtr<USQLUIBaseWidget> RootWidget = nullptr;
};

UCLASS(BlueprintType)
class SQLUISAMPLES_API USQLUISampleSmokeTestRunner : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples", meta = (WorldContext = "WorldContextObject"))
	static FSQLUISampleSmokeTestResult RunSmokeTest(
		UObject* WorldContextObject,
		const FSQLUISampleSmokeTestRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples", meta = (WorldContext = "WorldContextObject"))
	static FSQLUISampleSmokeTestResult RunJsonLayoutSmokeTest(
		UObject* WorldContextObject,
		const FSQLUISampleSmokeTestRequest& Request);
};
