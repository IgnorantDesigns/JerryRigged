#include "Pipeline/SQLUIRuntimeWidgetPipeline.h"

#include "Bindings/SQLUIBindingResolver.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"

namespace
{
const TCHAR* SQLUIRuntimeWidgetPipelinePrepareStepName = TEXT("PrepareBuild");
const TCHAR* SQLUIRuntimeWidgetPipelineConstructStepName = TEXT("ConstructWidgets");
const TCHAR* SQLUIRuntimeWidgetPipelineAssembleStepName = TEXT("AssembleWidgetTree");
const TCHAR* SQLUIRuntimeWidgetPipelinePropertiesStepName = TEXT("ApplyProperties");
const TCHAR* SQLUIRuntimeWidgetPipelineBindingsStepName = TEXT("ApplyBindings");
const TCHAR* SQLUIRuntimeWidgetPipelineActionsStepName = TEXT("ApplyActions");

template <typename ServiceType>
ServiceType* NewSQLUIRuntimeWidgetPipelineService(const USQLUIRuntimeWidgetPipeline* Pipeline)
{
	return NewObject<ServiceType>(const_cast<USQLUIRuntimeWidgetPipeline*>(Pipeline));
}

void AddSQLUIRuntimeWidgetPipelineMessage(TArray<FString>& Messages, const FString& Message)
{
	if (!Message.IsEmpty())
	{
		Messages.Add(Message);
	}
}

void AddSQLUIRuntimeWidgetPipelineError(
	FSQLUIRuntimeWidgetPipelineResult& Result,
	const FString& ErrorMessage)
{
	AddSQLUIRuntimeWidgetPipelineMessage(Result.Errors, ErrorMessage);

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = ErrorMessage;
	}
}

void AddSQLUIRuntimeWidgetPipelineWarning(
	FSQLUIRuntimeWidgetPipelineResult& Result,
	const FString& WarningMessage)
{
	AddSQLUIRuntimeWidgetPipelineMessage(Result.Warnings, WarningMessage);
}

void AddSQLUIRuntimeWidgetPipelineStepError(
	FSQLUIRuntimeWidgetPipelineStepResult& StepResult,
	const FString& ErrorMessage)
{
	AddSQLUIRuntimeWidgetPipelineMessage(StepResult.Errors, ErrorMessage);

	if (StepResult.ErrorMessage.IsEmpty())
	{
		StepResult.ErrorMessage = ErrorMessage;
	}
}

void AddSQLUIRuntimeWidgetPipelineStepWarning(
	FSQLUIRuntimeWidgetPipelineStepResult& StepResult,
	const FString& WarningMessage)
{
	AddSQLUIRuntimeWidgetPipelineMessage(StepResult.Warnings, WarningMessage);
}

FSQLUIRuntimeWidgetPipelineStepResult MakeSQLUIRuntimeWidgetPipelineStepResult(
	const TCHAR* StepName,
	const ESQLUIRuntimeWidgetPipelineStepStatus Status)
{
	FSQLUIRuntimeWidgetPipelineStepResult StepResult;
	StepResult.StepName = StepName;
	StepResult.Status = Status;
	return StepResult;
}

void AppendSQLUIRuntimeWidgetPipelineStepResult(
	FSQLUIRuntimeWidgetPipelineResult& Result,
	const FSQLUIRuntimeWidgetPipelineStepResult& StepResult)
{
	Result.StepResults.Add(StepResult);

	for (const FString& Error : StepResult.Errors)
	{
		AddSQLUIRuntimeWidgetPipelineError(Result, Error);
	}

	for (const FString& Warning : StepResult.Warnings)
	{
		AddSQLUIRuntimeWidgetPipelineWarning(Result, Warning);
	}
}

FString MakeSQLUIRuntimeWidgetPipelineStepServiceError(const TCHAR* StepName)
{
	return FString::Printf(TEXT("SQLUI runtime widget pipeline could not create service for step '%s'."), StepName);
}

FSQLUIRuntimeWidgetPipelineStepResult MakeSQLUIRuntimeWidgetPipelineServiceFailureStep(
	const TCHAR* StepName)
{
	FSQLUIRuntimeWidgetPipelineStepResult StepResult = MakeSQLUIRuntimeWidgetPipelineStepResult(
		StepName,
		ESQLUIRuntimeWidgetPipelineStepStatus::Failed);
	AddSQLUIRuntimeWidgetPipelineStepError(StepResult, MakeSQLUIRuntimeWidgetPipelineStepServiceError(StepName));
	return StepResult;
}

TArray<FSQLUILayoutNode> ExtractSQLUIRuntimeWidgetPipelineLayoutNodes(
	const TArray<FSQLUIPreparedWidgetBuildNode>& PreparedNodes)
{
	TArray<FSQLUILayoutNode> LayoutNodes;
	LayoutNodes.Reserve(PreparedNodes.Num());

	for (const FSQLUIPreparedWidgetBuildNode& PreparedNode : PreparedNodes)
	{
		LayoutNodes.Add(PreparedNode.LayoutNode);
	}

	return LayoutNodes;
}

TMap<FString, TObjectPtr<USQLUIBaseWidget>> MakeSQLUIRuntimeWidgetPipelineAssemblyWidgetMap(
	const TMap<FString, USQLUIBaseWidget*>& CreatedWidgetMap)
{
	TMap<FString, TObjectPtr<USQLUIBaseWidget>> AssemblyWidgetMap;

	for (const TPair<FString, USQLUIBaseWidget*>& CreatedWidgetPair : CreatedWidgetMap)
	{
		AssemblyWidgetMap.Add(CreatedWidgetPair.Key, CreatedWidgetPair.Value);
	}

	return AssemblyWidgetMap;
}

FString DescribeSQLUIWidgetPropertyApplyError(const FSQLUIWidgetPropertyApplyError& Error)
{
	return FString::Printf(
		TEXT("Widget '%s' property '%s': %s"),
		*Error.WidgetId,
		*Error.PropertyName,
		*Error.ErrorMessage);
}

FString DescribeSQLUIWidgetBindingApplyError(const FSQLUIWidgetBindingApplyError& Error)
{
	return FString::Printf(
		TEXT("Widget '%s' binding '%s' target '%s' source '%s/%s': %s"),
		*Error.WidgetId,
		*Error.BindingId,
		*Error.TargetProperty,
		*Error.SourceKey,
		*Error.SourcePath,
		*Error.ErrorMessage);
}

FString DescribeSQLUIWidgetBindingApplyWarning(const FSQLUIWidgetBindingApplyWarning& Warning)
{
	return FString::Printf(
		TEXT("Widget '%s' binding '%s' target '%s' source '%s/%s': %s"),
		*Warning.WidgetId,
		*Warning.BindingId,
		*Warning.TargetProperty,
		*Warning.SourceKey,
		*Warning.SourcePath,
		*Warning.WarningMessage);
}

FString DescribeSQLUIWidgetActionApplyError(const FSQLUIWidgetActionApplyError& Error)
{
	return FString::Printf(
		TEXT("Widget '%s' action '%s' trigger '%s' action type '%s': %s"),
		*Error.WidgetId,
		*Error.ActionId,
		*Error.Trigger,
		*Error.ActionTypeKey,
		*Error.ErrorMessage);
}

FString DescribeSQLUIWidgetActionApplyWarning(const FSQLUIWidgetActionApplyWarning& Warning)
{
	return FString::Printf(
		TEXT("Widget '%s' action '%s' trigger '%s' action type '%s': %s"),
		*Warning.WidgetId,
		*Warning.ActionId,
		*Warning.Trigger,
		*Warning.ActionTypeKey,
		*Warning.WarningMessage);
}

FSQLUIRuntimeWidgetPipelineStepResult MakeSQLUIRuntimeWidgetPipelinePrepareStepResult(
	const FSQLUIWidgetBuildResult& PrepareResult)
{
	FSQLUIRuntimeWidgetPipelineStepResult StepResult = MakeSQLUIRuntimeWidgetPipelineStepResult(
		SQLUIRuntimeWidgetPipelinePrepareStepName,
		PrepareResult.bSucceeded
			? ESQLUIRuntimeWidgetPipelineStepStatus::Succeeded
			: ESQLUIRuntimeWidgetPipelineStepStatus::Failed);

	AddSQLUIRuntimeWidgetPipelineStepError(StepResult, PrepareResult.ErrorMessage);

	for (const FString& Error : PrepareResult.LayoutValidation.Errors)
	{
		AddSQLUIRuntimeWidgetPipelineStepError(StepResult, Error);
	}

	for (const FString& Error : PrepareResult.CatalogValidation.Errors)
	{
		AddSQLUIRuntimeWidgetPipelineStepError(StepResult, Error);
	}

	for (const FString& Warning : PrepareResult.LayoutValidation.Warnings)
	{
		AddSQLUIRuntimeWidgetPipelineStepWarning(StepResult, Warning);
	}

	for (const FString& Warning : PrepareResult.CatalogValidation.Warnings)
	{
		AddSQLUIRuntimeWidgetPipelineStepWarning(StepResult, Warning);
	}

	return StepResult;
}

FSQLUIRuntimeWidgetPipelineStepResult MakeSQLUIRuntimeWidgetPipelineConstructionStepResult(
	const FSQLUIWidgetConstructionResult& ConstructionResult)
{
	FSQLUIRuntimeWidgetPipelineStepResult StepResult = MakeSQLUIRuntimeWidgetPipelineStepResult(
		SQLUIRuntimeWidgetPipelineConstructStepName,
		ConstructionResult.bSucceeded
			? ESQLUIRuntimeWidgetPipelineStepStatus::Succeeded
			: ESQLUIRuntimeWidgetPipelineStepStatus::Failed);

	AddSQLUIRuntimeWidgetPipelineStepError(StepResult, ConstructionResult.ErrorMessage);

	for (const FString& Error : ConstructionResult.Errors)
	{
		AddSQLUIRuntimeWidgetPipelineStepError(StepResult, Error);
	}

	return StepResult;
}

FSQLUIRuntimeWidgetPipelineStepResult MakeSQLUIRuntimeWidgetPipelineAssemblyStepResult(
	const FSQLUIWidgetTreeAssemblyResult& AssemblyResult)
{
	FSQLUIRuntimeWidgetPipelineStepResult StepResult = MakeSQLUIRuntimeWidgetPipelineStepResult(
		SQLUIRuntimeWidgetPipelineAssembleStepName,
		AssemblyResult.bSucceeded
			? ESQLUIRuntimeWidgetPipelineStepStatus::Succeeded
			: ESQLUIRuntimeWidgetPipelineStepStatus::Failed);

	AddSQLUIRuntimeWidgetPipelineStepError(StepResult, AssemblyResult.ErrorMessage);

	for (const FString& Error : AssemblyResult.Errors)
	{
		AddSQLUIRuntimeWidgetPipelineStepError(StepResult, Error);
	}

	for (const FString& Warning : AssemblyResult.Warnings)
	{
		AddSQLUIRuntimeWidgetPipelineStepWarning(StepResult, Warning);
	}

	return StepResult;
}

FSQLUIRuntimeWidgetPipelineStepResult MakeSQLUIRuntimeWidgetPipelinePropertyStepResult(
	const FSQLUIWidgetPropertyApplyResult& PropertyResult)
{
	FSQLUIRuntimeWidgetPipelineStepResult StepResult = MakeSQLUIRuntimeWidgetPipelineStepResult(
		SQLUIRuntimeWidgetPipelinePropertiesStepName,
		PropertyResult.bSucceeded
			? ESQLUIRuntimeWidgetPipelineStepStatus::Succeeded
			: ESQLUIRuntimeWidgetPipelineStepStatus::Failed);

	AddSQLUIRuntimeWidgetPipelineStepError(StepResult, PropertyResult.ErrorMessage);

	for (const FSQLUIWidgetPropertyApplyError& Error : PropertyResult.Errors)
	{
		AddSQLUIRuntimeWidgetPipelineStepError(StepResult, DescribeSQLUIWidgetPropertyApplyError(Error));
	}

	for (const FString& Warning : PropertyResult.Warnings)
	{
		AddSQLUIRuntimeWidgetPipelineStepWarning(StepResult, Warning);
	}

	return StepResult;
}

FSQLUIRuntimeWidgetPipelineStepResult MakeSQLUIRuntimeWidgetPipelineBindingStepResult(
	const FSQLUIWidgetBindingApplyResult& BindingResult)
{
	FSQLUIRuntimeWidgetPipelineStepResult StepResult = MakeSQLUIRuntimeWidgetPipelineStepResult(
		SQLUIRuntimeWidgetPipelineBindingsStepName,
		BindingResult.bSucceeded
			? ESQLUIRuntimeWidgetPipelineStepStatus::Succeeded
			: ESQLUIRuntimeWidgetPipelineStepStatus::Failed);

	AddSQLUIRuntimeWidgetPipelineStepError(StepResult, BindingResult.ErrorMessage);

	for (const FSQLUIWidgetBindingApplyError& Error : BindingResult.Errors)
	{
		AddSQLUIRuntimeWidgetPipelineStepError(StepResult, DescribeSQLUIWidgetBindingApplyError(Error));
	}

	for (const FSQLUIWidgetBindingApplyWarning& Warning : BindingResult.Warnings)
	{
		AddSQLUIRuntimeWidgetPipelineStepWarning(StepResult, DescribeSQLUIWidgetBindingApplyWarning(Warning));
	}

	return StepResult;
}

FSQLUIRuntimeWidgetPipelineStepResult MakeSQLUIRuntimeWidgetPipelineActionStepResult(
	const FSQLUIWidgetActionApplyResult& ActionResult)
{
	FSQLUIRuntimeWidgetPipelineStepResult StepResult = MakeSQLUIRuntimeWidgetPipelineStepResult(
		SQLUIRuntimeWidgetPipelineActionsStepName,
		ActionResult.bSucceeded
			? ESQLUIRuntimeWidgetPipelineStepStatus::Succeeded
			: ESQLUIRuntimeWidgetPipelineStepStatus::Failed);

	AddSQLUIRuntimeWidgetPipelineStepError(StepResult, ActionResult.ErrorMessage);

	for (const FSQLUIWidgetActionApplyError& Error : ActionResult.Errors)
	{
		AddSQLUIRuntimeWidgetPipelineStepError(StepResult, DescribeSQLUIWidgetActionApplyError(Error));
	}

	for (const FSQLUIWidgetActionApplyWarning& Warning : ActionResult.Warnings)
	{
		AddSQLUIRuntimeWidgetPipelineStepWarning(StepResult, DescribeSQLUIWidgetActionApplyWarning(Warning));
	}

	return StepResult;
}

FSQLUIRuntimeWidgetPipelineStepResult MakeSQLUIRuntimeWidgetPipelineSkippedStep(
	const TCHAR* StepName)
{
	return MakeSQLUIRuntimeWidgetPipelineStepResult(
		StepName,
		ESQLUIRuntimeWidgetPipelineStepStatus::Skipped);
}

bool DidSQLUIRuntimeWidgetPipelineOptionalStepFail(
	const FSQLUIRuntimeWidgetPipelineStepResult& StepResult)
{
	return StepResult.Status == ESQLUIRuntimeWidgetPipelineStepStatus::Failed;
}

bool ShouldStopSQLUIRuntimeWidgetPipelineAfterOptionalStep(
	const FSQLUIRuntimeWidgetPipelineRequest& Request,
	const FSQLUIRuntimeWidgetPipelineStepResult& StepResult)
{
	return Request.bStopOnOptionalStepFailure && DidSQLUIRuntimeWidgetPipelineOptionalStepFail(StepResult);
}

bool DidSQLUIRuntimeWidgetPipelineSucceed(
	const FSQLUIRuntimeWidgetPipelineResult& Result)
{
	for (const FSQLUIRuntimeWidgetPipelineStepResult& StepResult : Result.StepResults)
	{
		if (StepResult.Status == ESQLUIRuntimeWidgetPipelineStepStatus::Failed)
		{
			return false;
		}
	}

	return Result.Errors.IsEmpty();
}
}

FSQLUIRuntimeWidgetPipelineResult USQLUIRuntimeWidgetPipeline::RunPipeline(
	const FSQLUIRuntimeWidgetPipelineRequest& Request) const
{
	FSQLUIRuntimeWidgetPipelineResult Result;

	USQLUILayoutWidgetFactory* LayoutWidgetFactory =
		NewSQLUIRuntimeWidgetPipelineService<USQLUILayoutWidgetFactory>(this);
	if (!IsValid(LayoutWidgetFactory))
	{
		AppendSQLUIRuntimeWidgetPipelineStepResult(
			Result,
			MakeSQLUIRuntimeWidgetPipelineServiceFailureStep(SQLUIRuntimeWidgetPipelinePrepareStepName));
		return Result;
	}

	FSQLUIWidgetBuildRequest BuildRequest;
	BuildRequest.Document = Request.Document;
	BuildRequest.RuntimeContext = Request.RuntimeContext;
	BuildRequest.WidgetCatalogOverride = Request.WidgetCatalogOverride;

	Result.PrepareResult = LayoutWidgetFactory->PrepareBuild(BuildRequest);
	AppendSQLUIRuntimeWidgetPipelineStepResult(
		Result,
		MakeSQLUIRuntimeWidgetPipelinePrepareStepResult(Result.PrepareResult));

	if (!Result.PrepareResult.bSucceeded)
	{
		return Result;
	}

	const TArray<FSQLUILayoutNode> LayoutNodes =
		ExtractSQLUIRuntimeWidgetPipelineLayoutNodes(Result.PrepareResult.PreparedNodes);

	USQLUIWidgetConstructor* WidgetConstructor =
		NewSQLUIRuntimeWidgetPipelineService<USQLUIWidgetConstructor>(this);
	if (!IsValid(WidgetConstructor))
	{
		AppendSQLUIRuntimeWidgetPipelineStepResult(
			Result,
			MakeSQLUIRuntimeWidgetPipelineServiceFailureStep(SQLUIRuntimeWidgetPipelineConstructStepName));
		return Result;
	}

	FSQLUIWidgetConstructionRequest ConstructionRequest;
	ConstructionRequest.PreparedNodes = Result.PrepareResult.PreparedNodes;
	ConstructionRequest.RootWidgetId = Result.PrepareResult.RootWidgetId;
	ConstructionRequest.RuntimeContext = Request.RuntimeContext;
	ConstructionRequest.OwningPlayer = Request.OwningPlayer;
	ConstructionRequest.WorldContextObject = Request.WorldContextObject;

	Result.ConstructionResult = WidgetConstructor->ConstructWidgets(ConstructionRequest);
	Result.RootWidget = Result.ConstructionResult.RootWidget;
	Result.CreatedWidgets = Result.ConstructionResult.CreatedWidgets;
	Result.CreatedWidgetMap = Result.ConstructionResult.CreatedWidgetMap;

	AppendSQLUIRuntimeWidgetPipelineStepResult(
		Result,
		MakeSQLUIRuntimeWidgetPipelineConstructionStepResult(Result.ConstructionResult));

	if (!Result.ConstructionResult.bSucceeded)
	{
		return Result;
	}

	USQLUIWidgetTreeAssembler* WidgetTreeAssembler =
		NewSQLUIRuntimeWidgetPipelineService<USQLUIWidgetTreeAssembler>(this);
	if (!IsValid(WidgetTreeAssembler))
	{
		AppendSQLUIRuntimeWidgetPipelineStepResult(
			Result,
			MakeSQLUIRuntimeWidgetPipelineServiceFailureStep(SQLUIRuntimeWidgetPipelineAssembleStepName));
		return Result;
	}

	FSQLUIWidgetTreeAssemblyRequest AssemblyRequest;
	AssemblyRequest.RootWidgetId = Result.PrepareResult.RootWidgetId;
	AssemblyRequest.LayoutNodes = LayoutNodes;
	AssemblyRequest.CreatedWidgetMap = MakeSQLUIRuntimeWidgetPipelineAssemblyWidgetMap(Result.CreatedWidgetMap);

	Result.AssemblyResult = WidgetTreeAssembler->AssembleWidgetTree(AssemblyRequest);
	Result.RootWidget = Result.AssemblyResult.RootWidget;

	AppendSQLUIRuntimeWidgetPipelineStepResult(
		Result,
		MakeSQLUIRuntimeWidgetPipelineAssemblyStepResult(Result.AssemblyResult));

	if (!Result.AssemblyResult.bSucceeded)
	{
		return Result;
	}

	if (!Request.bApplyProperties)
	{
		AppendSQLUIRuntimeWidgetPipelineStepResult(
			Result,
			MakeSQLUIRuntimeWidgetPipelineSkippedStep(SQLUIRuntimeWidgetPipelinePropertiesStepName));
	}
	else
	{
		USQLUIWidgetPropertyApplier* WidgetPropertyApplier =
			NewSQLUIRuntimeWidgetPipelineService<USQLUIWidgetPropertyApplier>(this);
		if (!IsValid(WidgetPropertyApplier))
		{
			const FSQLUIRuntimeWidgetPipelineStepResult PropertyStepResult =
				MakeSQLUIRuntimeWidgetPipelineServiceFailureStep(SQLUIRuntimeWidgetPipelinePropertiesStepName);
			AppendSQLUIRuntimeWidgetPipelineStepResult(Result, PropertyStepResult);
			if (ShouldStopSQLUIRuntimeWidgetPipelineAfterOptionalStep(Request, PropertyStepResult))
			{
				return Result;
			}
		}
		else
		{
			FSQLUIWidgetPropertyApplyRequest PropertyRequest;
			PropertyRequest.LayoutNodes = LayoutNodes;
			PropertyRequest.CreatedWidgetMap = Result.CreatedWidgetMap;

			Result.PropertyApplicationResult = WidgetPropertyApplier->ApplyProperties(PropertyRequest);
			const FSQLUIRuntimeWidgetPipelineStepResult PropertyStepResult =
				MakeSQLUIRuntimeWidgetPipelinePropertyStepResult(Result.PropertyApplicationResult);
			AppendSQLUIRuntimeWidgetPipelineStepResult(Result, PropertyStepResult);
			if (ShouldStopSQLUIRuntimeWidgetPipelineAfterOptionalStep(Request, PropertyStepResult))
			{
				return Result;
			}
		}
	}

	if (!Request.bApplyBindings)
	{
		AppendSQLUIRuntimeWidgetPipelineStepResult(
			Result,
			MakeSQLUIRuntimeWidgetPipelineSkippedStep(SQLUIRuntimeWidgetPipelineBindingsStepName));
	}
	else
	{
		USQLUIWidgetBindingApplier* WidgetBindingApplier =
			NewSQLUIRuntimeWidgetPipelineService<USQLUIWidgetBindingApplier>(this);
		if (!IsValid(WidgetBindingApplier))
		{
			const FSQLUIRuntimeWidgetPipelineStepResult BindingStepResult =
				MakeSQLUIRuntimeWidgetPipelineServiceFailureStep(SQLUIRuntimeWidgetPipelineBindingsStepName);
			AppendSQLUIRuntimeWidgetPipelineStepResult(Result, BindingStepResult);
			if (ShouldStopSQLUIRuntimeWidgetPipelineAfterOptionalStep(Request, BindingStepResult))
			{
				return Result;
			}
		}
		else
		{
			FSQLUIWidgetBindingApplyRequest BindingRequest;
			BindingRequest.LayoutNodes = LayoutNodes;
			BindingRequest.CreatedWidgetMap = Result.CreatedWidgetMap;
			BindingRequest.RuntimeContext = Request.RuntimeContext;
			BindingRequest.BindingResolverOverride = Request.BindingResolverOverride;

			Result.BindingApplicationResult = WidgetBindingApplier->ApplyBindings(BindingRequest);
			const FSQLUIRuntimeWidgetPipelineStepResult BindingStepResult =
				MakeSQLUIRuntimeWidgetPipelineBindingStepResult(Result.BindingApplicationResult);
			AppendSQLUIRuntimeWidgetPipelineStepResult(Result, BindingStepResult);
			if (ShouldStopSQLUIRuntimeWidgetPipelineAfterOptionalStep(Request, BindingStepResult))
			{
				return Result;
			}
		}
	}

	if (!Request.bApplyActions)
	{
		AppendSQLUIRuntimeWidgetPipelineStepResult(
			Result,
			MakeSQLUIRuntimeWidgetPipelineSkippedStep(SQLUIRuntimeWidgetPipelineActionsStepName));
	}
	else
	{
		USQLUIWidgetActionApplier* WidgetActionApplier =
			NewSQLUIRuntimeWidgetPipelineService<USQLUIWidgetActionApplier>(this);
		if (!IsValid(WidgetActionApplier))
		{
			const FSQLUIRuntimeWidgetPipelineStepResult ActionStepResult =
				MakeSQLUIRuntimeWidgetPipelineServiceFailureStep(SQLUIRuntimeWidgetPipelineActionsStepName);
			AppendSQLUIRuntimeWidgetPipelineStepResult(Result, ActionStepResult);
			if (ShouldStopSQLUIRuntimeWidgetPipelineAfterOptionalStep(Request, ActionStepResult))
			{
				return Result;
			}
		}
		else
		{
			FSQLUIWidgetActionApplyRequest ActionRequest;
			ActionRequest.LayoutNodes = LayoutNodes;
			ActionRequest.CreatedWidgetMap = Result.CreatedWidgetMap;
			ActionRequest.RuntimeContext = Request.RuntimeContext;
			ActionRequest.bExecuteRegisteredActions = Request.bExecuteActions;

			Result.ActionApplicationResult = WidgetActionApplier->ApplyActions(ActionRequest);
			const FSQLUIRuntimeWidgetPipelineStepResult ActionStepResult =
				MakeSQLUIRuntimeWidgetPipelineActionStepResult(Result.ActionApplicationResult);
			AppendSQLUIRuntimeWidgetPipelineStepResult(Result, ActionStepResult);
			if (ShouldStopSQLUIRuntimeWidgetPipelineAfterOptionalStep(Request, ActionStepResult))
			{
				return Result;
			}
		}
	}

	Result.bSucceeded = DidSQLUIRuntimeWidgetPipelineSucceed(Result);
	return Result;
}
