#pragma once

#include "CoreMinimal.h"
#include "Actions/SQLUIWidgetActionApplier.h"
#include "Assembly/SQLUIWidgetTreeAssembler.h"
#include "Bindings/SQLUIWidgetBindingApplier.h"
#include "Construction/SQLUIWidgetConstructor.h"
#include "GameFramework/PlayerController.h"
#include "Layout/SQLUILayoutWidgetFactory.h"
#include "Properties/SQLUIWidgetPropertyApplier.h"
#include "UObject/Object.h"

#include "SQLUIRuntimeWidgetPipeline.generated.h"

class USQLUIBindingResolver;
class USQLUIRuntimeContext;
class USQLUIWidgetCatalog;

UENUM(BlueprintType)
enum class ESQLUIRuntimeWidgetPipelineStepStatus : uint8
{
	NotRun,
	Skipped,
	Succeeded,
	Failed
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIRuntimeWidgetPipelineStepResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	FString StepName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	ESQLUIRuntimeWidgetPipelineStepStatus Status = ESQLUIRuntimeWidgetPipelineStepStatus::NotRun;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	TArray<FString> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	TArray<FString> Warnings;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIRuntimeWidgetPipelineRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	FSQLUILayoutDocument Document;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	TObjectPtr<USQLUIRuntimeContext> RuntimeContext = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	TObjectPtr<APlayerController> OwningPlayer = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	TObjectPtr<UObject> WorldContextObject = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	TObjectPtr<USQLUIWidgetCatalog> WidgetCatalogOverride = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	TObjectPtr<USQLUIBindingResolver> BindingResolverOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	bool bApplyProperties = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	bool bApplyBindings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	bool bApplyActions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	bool bExecuteActions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	bool bStopOnOptionalStepFailure = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIRuntimeWidgetPipelineResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	TObjectPtr<USQLUIBaseWidget> RootWidget = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	TArray<FSQLUICreatedWidgetRecord> CreatedWidgets;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	TMap<FString, USQLUIBaseWidget*> CreatedWidgetMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	TArray<FString> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	TArray<FString> Warnings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	TArray<FSQLUIRuntimeWidgetPipelineStepResult> StepResults;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	FSQLUIWidgetBuildResult PrepareResult;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	FSQLUIWidgetConstructionResult ConstructionResult;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Runtime Widget Pipeline")
	FSQLUIWidgetTreeAssemblyResult AssemblyResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	FSQLUIWidgetPropertyApplyResult PropertyApplicationResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	FSQLUIWidgetBindingApplyResult BindingApplicationResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime Widget Pipeline")
	FSQLUIWidgetActionApplyResult ActionApplicationResult;
};

UCLASS(BlueprintType)
class SQLUIWIDGETS_API USQLUIRuntimeWidgetPipeline : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Runtime Widget Pipeline")
	FSQLUIRuntimeWidgetPipelineResult RunPipeline(const FSQLUIRuntimeWidgetPipelineRequest& Request) const;
};
