#include "SQLUISamplePipelineSmokeTestActor.h"

#include "SQLUISamplesModule.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Pipeline/SQLUIRuntimeWidgetPipeline.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "Variables/SQLUIVariableStore.h"
#include "Variables/SQLUIVariableTypes.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"
#include "WidgetCatalog/SQLUIWidgetCatalogRegistrar.h"

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

FSQLUILayoutDocument MakeSQLUISamplePipelineSmokeTestDocument()
{
	FSQLUILayoutDocument Document;
	Document.Version.SchemaVersion = 1;
	Document.Version.Revision = 1;
	Document.Version.Label = TEXT("SQLUI Sample Pipeline Smoke Test");
	Document.Metadata.LayoutId = TEXT("sqlui.sample.pipeline.smoke-test");
	Document.Metadata.DisplayName = TEXT("SQLUI Sample Pipeline Smoke Test");
	Document.Metadata.Description = TEXT("Minimal in-memory SQLUI runtime widget pipeline smoke test layout.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.RootWidgetId = TEXT("SQLUI.Sample.FilterRoot");

	FSQLUILayoutBinding FilterTextBinding;
	FilterTextBinding.BindingId = TEXT("SQLUI.Sample.FilterTextBinding");
	FilterTextBinding.TargetProperty = TEXT("FilterText");
	FilterTextBinding.SourceKey = TEXT("Variable");
	FilterTextBinding.SourcePath = TEXT("Sample.FilterText");

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = Document.RootWidgetId;
	RootNode.WidgetTypeKey = FSQLUIWidgetTypeKeys::FilterBox().Value;
	RootNode.Properties.Add(TEXT("FilterText"), TEXT("Smoke test"));
	RootNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));
	RootNode.Bindings.Add(FilterTextBinding);

	Document.Nodes.Add(RootNode);
	return Document;
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

void LogSQLUISamplePipelineSmokeTestResult(const FSQLUIRuntimeWidgetPipelineResult& Result)
{
	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test root widget valid: %s"),
		IsValid(Result.RootWidget.Get()) ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample pipeline smoke test created widget count: %d"),
		Result.CreatedWidgets.Num());

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
	USQLUIWidgetCatalog* WidgetCatalog = NewObject<USQLUIWidgetCatalog>(this);
	if (!IsValid(WidgetCatalog))
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample pipeline smoke test failed: could not create widget catalog."));
		return;
	}

	if (!USQLUIWidgetCatalogRegistrar::RegisterDefaultSQLUIWidgets(WidgetCatalog))
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample pipeline smoke test failed: could not register default widget catalog entries."));
		return;
	}

	USQLUIVariableStore* VariableStore = NewObject<USQLUIVariableStore>(this);
	if (!IsValid(VariableStore))
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample pipeline smoke test failed: could not create variable store."));
		return;
	}

	FSQLUIVariableKey FilterTextKey;
	FilterTextKey.Name = TEXT("Sample.FilterText");

	FSQLUIVariableValue FilterTextValue;
	FilterTextValue.Type = ESQLUIVariableValueType::String;
	FilterTextValue.StringValue = TEXT("Smoke test variable");
	VariableStore->SetVariable(FilterTextKey, FilterTextValue);

	USQLUIRuntimeContext* RuntimeContext = NewObject<USQLUIRuntimeContext>(this);
	if (!IsValid(RuntimeContext))
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample pipeline smoke test failed: could not create runtime context."));
		return;
	}

	FSQLUIRuntimeContextSettings RuntimeContextSettings;
	RuntimeContextSettings.VariableStore = VariableStore;
	RuntimeContextSettings.WidgetCatalog = WidgetCatalog;
	RuntimeContext->Initialize(RuntimeContextSettings);

	if (!RuntimeContext->IsInitialized())
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample pipeline smoke test failed: runtime context did not initialize."));
		return;
	}

	USQLUIRuntimeWidgetPipeline* Pipeline = NewObject<USQLUIRuntimeWidgetPipeline>(this);
	if (!IsValid(Pipeline))
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample pipeline smoke test failed: could not create runtime widget pipeline."));
		return;
	}

	FSQLUIRuntimeWidgetPipelineRequest Request;
	Request.Document = MakeSQLUISamplePipelineSmokeTestDocument();
	Request.RuntimeContext = RuntimeContext;
	Request.WidgetCatalogOverride = WidgetCatalog;
	Request.WorldContextObject = this;
	Request.OwningPlayer = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	Request.bExecuteActions = false;

	const FSQLUIRuntimeWidgetPipelineResult Result = Pipeline->RunPipeline(Request);
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
