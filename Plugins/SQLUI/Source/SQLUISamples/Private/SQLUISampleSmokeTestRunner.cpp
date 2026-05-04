#include "SQLUISampleSmokeTestRunner.h"

#include "Layout/SQLUILayoutJson.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "Variables/SQLUIVariableStore.h"
#include "Variables/SQLUIVariableTypes.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"
#include "WidgetCatalog/SQLUIWidgetCatalogRegistrar.h"
#include "UObject/Package.h"

namespace
{
const TCHAR* SQLUISampleSmokeTestFilterVariableName = TEXT("Sample.FilterText");

struct FSQLUISampleLayoutJsonFixture
{
	static FString GetJsonString()
	{
		return TEXT(R"SQLUIJSON(
{
	"Version": {
		"SchemaVersion": 1,
		"Revision": 1,
		"Label": "SQLUI Sample JSON Layout Fixture"
	},
	"Metadata": {
		"LayoutId": "sqlui.sample.json-layout-fixture",
		"DisplayName": "SQLUI Sample JSON Layout Fixture",
		"Description": "Minimal SQLUISamples JSON layout fixture for the runtime widget pipeline smoke test.",
		"CreatedBy": "SQLUISamples",
		"CreatedAtUtc": "",
		"UpdatedAtUtc": "",
		"Tags": [],
		"SearchMetadata": {}
	},
	"RootWidgetId": "SQLUI.Sample.Json.FilterRoot",
	"Nodes": [
		{
			"WidgetId": "SQLUI.Sample.Json.FilterRoot",
			"ParentWidgetId": "",
			"WidgetTypeKey": "SQLUI.FilterBox",
			"ChildWidgetIds": [],
			"Properties": {
				"FilterText": "Smoke test JSON fixture",
				"IsEnabled": "true"
			},
			"Bindings": [
				{
					"BindingId": "SQLUI.Sample.Json.FilterTextBinding",
					"TargetProperty": "FilterText",
					"SourceKey": "Variable",
					"SourcePath": "Sample.FilterText",
					"TransformKey": "",
					"Options": {}
				}
			],
			"Actions": [],
			"Tags": [],
			"SearchMetadata": {}
		}
	],
	"Properties": {}
}
)SQLUIJSON");
	}
};

struct FSQLUISampleSmokeTestLayoutResult
{
	bool bSucceeded = true;
	bool bUsedJsonLayoutFixture = false;
	bool bJsonLayoutFixtureParseSucceeded = false;
	bool bJsonLayoutFixtureValidationSucceeded = false;
	FSQLUILayoutValidationResult JsonLayoutFixtureValidation;
	FSQLUILayoutDocument Document;
};

void AddSQLUISampleSmokeTestError(
	FSQLUISampleSmokeTestResult& Result,
	const FString& ErrorMessage)
{
	if (!ErrorMessage.IsEmpty())
	{
		Result.Errors.Add(ErrorMessage);
	}

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = ErrorMessage;
	}
}

UObject* ResolveSQLUISampleSmokeTestOuter(
	UObject* WorldContextObject,
	const FSQLUISampleSmokeTestRequest& Request)
{
	if (IsValid(WorldContextObject))
	{
		return WorldContextObject;
	}

	if (IsValid(Request.OwningPlayer.Get()))
	{
		return Request.OwningPlayer.Get();
	}

	return GetTransientPackage();
}

FSQLUILayoutDocument MakeSQLUISampleSmokeTestDocument(
	const FSQLUISampleSmokeTestRequest& Request)
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
	FilterTextBinding.SourcePath = SQLUISampleSmokeTestFilterVariableName;

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = Document.RootWidgetId;
	RootNode.WidgetTypeKey = FSQLUIWidgetTypeKeys::FilterBox().Value;
	RootNode.Properties.Add(TEXT("FilterText"), Request.SampleFilterText);
	RootNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));
	RootNode.Bindings.Add(FilterTextBinding);

	Document.Nodes.Add(RootNode);
	return Document;
}

bool IsSQLUISampleLayoutJsonParseFailureMessage(const FString& ErrorMessage)
{
	return ErrorMessage.Contains(TEXT("deserialize"), ESearchCase::IgnoreCase);
}

bool DidSQLUISampleLayoutJsonParseFail(const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		if (IsSQLUISampleLayoutJsonParseFailureMessage(Error))
		{
			return true;
		}
	}

	return false;
}

FSQLUISampleSmokeTestLayoutResult ResolveSQLUISampleSmokeTestLayoutDocument(
	const FSQLUISampleSmokeTestRequest& Request)
{
	FSQLUISampleSmokeTestLayoutResult Result;
	Result.bUsedJsonLayoutFixture = Request.bUseJsonLayoutFixture;

	if (!Request.bUseJsonLayoutFixture)
	{
		Result.Document = MakeSQLUISampleSmokeTestDocument(Request);
		return Result;
	}

	const bool bParsedAndValidated = FSQLUILayoutJson::FromJsonString(
		FSQLUISampleLayoutJsonFixture::GetJsonString(),
		Result.Document,
		Result.JsonLayoutFixtureValidation);

	Result.bJsonLayoutFixtureParseSucceeded =
		!DidSQLUISampleLayoutJsonParseFail(Result.JsonLayoutFixtureValidation);
	Result.bJsonLayoutFixtureValidationSucceeded =
		Result.bJsonLayoutFixtureParseSucceeded && Result.JsonLayoutFixtureValidation.bIsValid;
	Result.bSucceeded = bParsedAndValidated;
	return Result;
}

void ApplySQLUISampleLayoutResult(
	FSQLUISampleSmokeTestResult& Result,
	const FSQLUISampleSmokeTestLayoutResult& LayoutResult)
{
	Result.bUsedJsonLayoutFixture = LayoutResult.bUsedJsonLayoutFixture;
	Result.bJsonLayoutFixtureParseSucceeded = LayoutResult.bJsonLayoutFixtureParseSucceeded;
	Result.bJsonLayoutFixtureValidationSucceeded = LayoutResult.bJsonLayoutFixtureValidationSucceeded;
	Result.JsonLayoutFixtureValidation = LayoutResult.JsonLayoutFixtureValidation;
}

void AddSQLUISampleLayoutValidationMessages(
	FSQLUISampleSmokeTestResult& Result,
	const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		AddSQLUISampleSmokeTestError(Result, Error);
	}

	for (const FString& Warning : Validation.Warnings)
	{
		Result.Warnings.Add(Warning);
	}
}

void SeedSQLUISampleSmokeTestVariables(
	USQLUIVariableStore* VariableStore,
	const FSQLUISampleSmokeTestRequest& Request)
{
	if (!IsValid(VariableStore))
	{
		return;
	}

	FSQLUIVariableKey FilterTextKey;
	FilterTextKey.Name = SQLUISampleSmokeTestFilterVariableName;

	FSQLUIVariableValue FilterTextValue;
	FilterTextValue.Type = ESQLUIVariableValueType::String;
	FilterTextValue.StringValue = Request.SampleFilterVariableValue;
	VariableStore->SetVariable(FilterTextKey, FilterTextValue);
}

FSQLUISampleSmokeTestResult MakeSQLUISampleSmokeTestResult(
	const FSQLUIRuntimeWidgetPipelineResult& PipelineResult,
	const FSQLUISampleSmokeTestLayoutResult& LayoutResult)
{
	FSQLUISampleSmokeTestResult Result;
	ApplySQLUISampleLayoutResult(Result, LayoutResult);
	Result.bSucceeded = PipelineResult.bSucceeded;
	Result.ErrorMessage = PipelineResult.ErrorMessage;
	Result.Errors = PipelineResult.Errors;
	Result.Warnings = PipelineResult.Warnings;
	Result.StepResults = PipelineResult.StepResults;
	Result.CreatedWidgetCount = PipelineResult.CreatedWidgets.Num();
	Result.RootWidget = PipelineResult.RootWidget;
	Result.bRootWidgetValid = IsValid(Result.RootWidget.Get());
	return Result;
}
}

FSQLUISampleSmokeTestResult USQLUISampleSmokeTestRunner::RunSmokeTest(
	UObject* WorldContextObject,
	const FSQLUISampleSmokeTestRequest& Request)
{
	FSQLUISampleSmokeTestResult Result;
	UObject* Outer = ResolveSQLUISampleSmokeTestOuter(WorldContextObject, Request);

	const FSQLUISampleSmokeTestLayoutResult LayoutResult =
		ResolveSQLUISampleSmokeTestLayoutDocument(Request);
	ApplySQLUISampleLayoutResult(Result, LayoutResult);
	if (!LayoutResult.bSucceeded)
	{
		AddSQLUISampleSmokeTestError(
			Result,
			LayoutResult.bJsonLayoutFixtureParseSucceeded
				? TEXT("SQLUI sample pipeline smoke test failed: JSON layout fixture validation failed.")
				: TEXT("SQLUI sample pipeline smoke test failed: JSON layout fixture parse failed."));
		AddSQLUISampleLayoutValidationMessages(Result, LayoutResult.JsonLayoutFixtureValidation);
		return Result;
	}

	USQLUIWidgetCatalog* WidgetCatalog = NewObject<USQLUIWidgetCatalog>(Outer);
	if (!IsValid(WidgetCatalog))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not create widget catalog."));
		return Result;
	}

	if (!USQLUIWidgetCatalogRegistrar::RegisterDefaultSQLUIWidgets(WidgetCatalog))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not register default widget catalog entries."));
		return Result;
	}

	USQLUIVariableStore* VariableStore = NewObject<USQLUIVariableStore>(Outer);
	if (!IsValid(VariableStore))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not create variable store."));
		return Result;
	}

	SeedSQLUISampleSmokeTestVariables(VariableStore, Request);

	USQLUIRuntimeContext* RuntimeContext = NewObject<USQLUIRuntimeContext>(Outer);
	if (!IsValid(RuntimeContext))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not create runtime context."));
		return Result;
	}

	FSQLUIRuntimeContextSettings RuntimeContextSettings;
	RuntimeContextSettings.VariableStore = VariableStore;
	RuntimeContextSettings.WidgetCatalog = WidgetCatalog;
	RuntimeContext->Initialize(RuntimeContextSettings);

	if (!RuntimeContext->IsInitialized())
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: runtime context did not initialize."));
		return Result;
	}

	USQLUIRuntimeWidgetPipeline* Pipeline = NewObject<USQLUIRuntimeWidgetPipeline>(Outer);
	if (!IsValid(Pipeline))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not create runtime widget pipeline."));
		return Result;
	}

	FSQLUIRuntimeWidgetPipelineRequest PipelineRequest;
	PipelineRequest.Document = LayoutResult.Document;
	PipelineRequest.RuntimeContext = RuntimeContext;
	PipelineRequest.WidgetCatalogOverride = WidgetCatalog;
	PipelineRequest.WorldContextObject = WorldContextObject;
	PipelineRequest.OwningPlayer = Request.OwningPlayer;
	PipelineRequest.bApplyProperties = Request.bApplyProperties;
	PipelineRequest.bApplyBindings = Request.bApplyBindings;
	PipelineRequest.bApplyActions = Request.bApplyActions;
	PipelineRequest.bExecuteActions = Request.bExecuteActions;
	PipelineRequest.bStopOnOptionalStepFailure = Request.bStopOnOptionalStepFailure;

	return MakeSQLUISampleSmokeTestResult(Pipeline->RunPipeline(PipelineRequest), LayoutResult);
}

FSQLUISampleSmokeTestResult USQLUISampleSmokeTestRunner::RunJsonLayoutSmokeTest(
	UObject* WorldContextObject,
	const FSQLUISampleSmokeTestRequest& Request)
{
	FSQLUISampleSmokeTestRequest JsonRequest = Request;
	JsonRequest.bUseJsonLayoutFixture = true;
	return RunSmokeTest(WorldContextObject, JsonRequest);
}
