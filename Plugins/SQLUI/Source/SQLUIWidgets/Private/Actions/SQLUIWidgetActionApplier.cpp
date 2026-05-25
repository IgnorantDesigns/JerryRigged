#include "Actions/SQLUIWidgetActionApplier.h"

#include "Actions/SQLUIActionRegistry.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "Variables/SQLUIVariableTypes.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"
#include "Widgets/SQLUIBaseWidget.h"
#include "Widgets/SQLUIListWidget.h"

namespace
{
const TCHAR* SQLUIWidgetActionTriggerRowClicked = TEXT("RowClicked");
const TCHAR* SQLUIWidgetActionTriggerOnRowClicked = TEXT("OnRowClicked");

FString GetSQLUIWidgetActionLabel(const FSQLUILayoutAction& Action)
{
	if (!Action.ActionId.IsEmpty())
	{
		return Action.ActionId;
	}

	if (!Action.ActionTypeKey.IsEmpty())
	{
		return Action.ActionTypeKey;
	}

	if (!Action.Trigger.IsEmpty())
	{
		return Action.Trigger;
	}

	return TEXT("<unnamed>");
}

FString MakeSQLUIWidgetActionMessagePrefix(
	const FString& WidgetId,
	const FSQLUILayoutAction& Action)
{
	return FString::Printf(
		TEXT("SQLUI widget action '%s' on widget '%s'"),
		*GetSQLUIWidgetActionLabel(Action),
		*WidgetId);
}

FSQLUIWidgetActionApplyError MakeSQLUIWidgetActionApplyError(
	const FString& WidgetId,
	const FSQLUILayoutAction& Action,
	const FString& ErrorMessage,
	const bool bInvalidAction = false,
	const bool bUnsupportedAction = false,
	const bool bUnregisteredAction = false,
	const bool bRegistryUnavailable = false,
	const bool bActionExecutionFailure = false)
{
	FSQLUIWidgetActionApplyError Error;
	Error.WidgetId = WidgetId;
	Error.ActionId = Action.ActionId;
	Error.Trigger = Action.Trigger;
	Error.ActionTypeKey = Action.ActionTypeKey;
	Error.ErrorMessage = ErrorMessage;
	Error.bInvalidAction = bInvalidAction;
	Error.bUnsupportedAction = bUnsupportedAction;
	Error.bUnregisteredAction = bUnregisteredAction;
	Error.bRegistryUnavailable = bRegistryUnavailable;
	Error.bActionExecutionFailure = bActionExecutionFailure;
	return Error;
}

FSQLUIWidgetActionApplyWarning MakeSQLUIWidgetActionApplyWarning(
	const FString& WidgetId,
	const FSQLUILayoutAction& Action,
	const FString& WarningMessage,
	const bool bMissingTrigger = false,
	const bool bUnsupportedAction = false,
	const bool bUnregisteredAction = false,
	const bool bRegistryUnavailable = false,
	const bool bActionExecutionFailure = false)
{
	FSQLUIWidgetActionApplyWarning Warning;
	Warning.WidgetId = WidgetId;
	Warning.ActionId = Action.ActionId;
	Warning.Trigger = Action.Trigger;
	Warning.ActionTypeKey = Action.ActionTypeKey;
	Warning.WarningMessage = WarningMessage;
	Warning.bMissingTrigger = bMissingTrigger;
	Warning.bUnsupportedAction = bUnsupportedAction;
	Warning.bUnregisteredAction = bUnregisteredAction;
	Warning.bRegistryUnavailable = bRegistryUnavailable;
	Warning.bActionExecutionFailure = bActionExecutionFailure;
	return Warning;
}

void AddSQLUIWidgetActionApplyError(
	FSQLUIWidgetActionApplyResult& Result,
	const FString& WidgetId,
	const FSQLUILayoutAction& Action,
	const FString& ErrorMessage,
	const bool bInvalidAction = false,
	const bool bUnsupportedAction = false,
	const bool bUnregisteredAction = false,
	const bool bRegistryUnavailable = false,
	const bool bActionExecutionFailure = false)
{
	Result.Errors.Add(MakeSQLUIWidgetActionApplyError(
		WidgetId,
		Action,
		ErrorMessage,
		bInvalidAction,
		bUnsupportedAction,
		bUnregisteredAction,
		bRegistryUnavailable,
		bActionExecutionFailure));

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = ErrorMessage;
	}
}

void AddSQLUIWidgetActionApplyWarning(
	FSQLUIWidgetActionApplyResult& Result,
	const FString& WidgetId,
	const FSQLUILayoutAction& Action,
	const FString& WarningMessage,
	const bool bMissingTrigger = false,
	const bool bUnsupportedAction = false,
	const bool bUnregisteredAction = false,
	const bool bRegistryUnavailable = false,
	const bool bActionExecutionFailure = false)
{
	Result.Warnings.Add(MakeSQLUIWidgetActionApplyWarning(
		WidgetId,
		Action,
		WarningMessage,
		bMissingTrigger,
		bUnsupportedAction,
		bUnregisteredAction,
		bRegistryUnavailable,
		bActionExecutionFailure));
}

void AddSQLUIWidgetActionApplyProblem(
	FSQLUIWidgetActionApplyResult& Result,
	const bool bShouldFail,
	const FString& WidgetId,
	const FSQLUILayoutAction& Action,
	const FString& Message,
	const bool bInvalidAction = false,
	const bool bUnsupportedAction = false,
	const bool bUnregisteredAction = false,
	const bool bRegistryUnavailable = false,
	const bool bActionExecutionFailure = false)
{
	if (bShouldFail)
	{
		AddSQLUIWidgetActionApplyError(
			Result,
			WidgetId,
			Action,
			Message,
			bInvalidAction,
			bUnsupportedAction,
			bUnregisteredAction,
			bRegistryUnavailable,
			bActionExecutionFailure);
		return;
	}

	AddSQLUIWidgetActionApplyWarning(
		Result,
		WidgetId,
		Action,
		Message,
		false,
		bUnsupportedAction,
		bUnregisteredAction,
		bRegistryUnavailable,
		bActionExecutionFailure);
}

USQLUIBaseWidget* FindSQLUIActionApplyWidget(
	const FSQLUIWidgetActionApplyRequest& Request,
	const FString& WidgetId)
{
	USQLUIBaseWidget* const* CreatedWidget = Request.CreatedWidgetMap.Find(WidgetId);
	return CreatedWidget ? *CreatedWidget : nullptr;
}

USQLUIActionRegistry* ResolveSQLUIActionRegistry(
	const FSQLUIWidgetActionApplyRequest& Request,
	FString& OutUnavailableMessage)
{
	OutUnavailableMessage.Reset();

	USQLUIRuntimeContext* RuntimeContext = Request.RuntimeContext.Get();
	if (!IsValid(RuntimeContext))
	{
		OutUnavailableMessage = TEXT("SQLUI widget action application requires a valid runtime context to check the action registry.");
		return nullptr;
	}

	if (!RuntimeContext->IsInitialized())
	{
		OutUnavailableMessage = TEXT("SQLUI widget action application requires an initialized runtime context to check the action registry.");
		return nullptr;
	}

	USQLUIActionRegistry* ActionRegistry = RuntimeContext->GetActionRegistry();
	if (!IsValid(ActionRegistry))
	{
		OutUnavailableMessage = TEXT("SQLUI widget action application could not find an action registry on the runtime context.");
		return nullptr;
	}

	return ActionRegistry;
}

USQLUIWidgetCatalog* ResolveSQLUIWidgetCatalog(const FSQLUIWidgetActionApplyRequest& Request)
{
	USQLUIRuntimeContext* RuntimeContext = Request.RuntimeContext.Get();
	if (!IsValid(RuntimeContext) || !RuntimeContext->IsInitialized())
	{
		return nullptr;
	}

	USQLUIWidgetCatalog* WidgetCatalog = RuntimeContext->GetWidgetCatalog();
	return IsValid(WidgetCatalog) ? WidgetCatalog : nullptr;
}

FSQLUIVariableValue MakeSQLUIActionParameterValue(const FString& ParameterValue)
{
	FSQLUIVariableValue Value;
	Value.Type = ESQLUIVariableValueType::String;
	Value.StringValue = ParameterValue;
	return Value;
}

void AddSQLUIActionRequestStringParameter(
	TMap<FString, FSQLUIVariableValue>& Parameters,
	const FString& Key,
	const FString& Value)
{
	if (!Key.IsEmpty())
	{
		Parameters.Add(Key, MakeSQLUIActionParameterValue(Value));
	}
}

FSQLUIActionRequest MakeSQLUIActionRequest(
	const FSQLUILayoutNode& LayoutNode,
	const FSQLUILayoutAction& Action)
{
	FSQLUIActionRequest ActionRequest;
	ActionRequest.ActionKey.Name = Action.ActionTypeKey;

	AddSQLUIActionRequestStringParameter(ActionRequest.Parameters, TEXT("WidgetId"), LayoutNode.WidgetId);
	AddSQLUIActionRequestStringParameter(ActionRequest.Parameters, TEXT("WidgetTypeKey"), LayoutNode.WidgetTypeKey);
	AddSQLUIActionRequestStringParameter(ActionRequest.Parameters, TEXT("ActionId"), Action.ActionId);
	AddSQLUIActionRequestStringParameter(ActionRequest.Parameters, TEXT("Trigger"), Action.Trigger);
	AddSQLUIActionRequestStringParameter(ActionRequest.Parameters, TEXT("ActionTypeKey"), Action.ActionTypeKey);

	for (const TPair<FString, FString>& ParameterPair : Action.Parameters)
	{
		ActionRequest.Parameters.Add(ParameterPair.Key, MakeSQLUIActionParameterValue(ParameterPair.Value));
	}

	return ActionRequest;
}

FSQLUIAppliedWidgetAction MakeSQLUIAppliedWidgetAction(
	const FString& WidgetId,
	const FSQLUILayoutAction& Action,
	const FSQLUIActionRequest& ActionRequest,
	const bool bPrepared)
{
	FSQLUIAppliedWidgetAction AppliedAction;
	AppliedAction.WidgetId = WidgetId;
	AppliedAction.ActionId = Action.ActionId;
	AppliedAction.Trigger = Action.Trigger;
	AppliedAction.ActionTypeKey = Action.ActionTypeKey;
	AppliedAction.ActionRequest = ActionRequest;
	AppliedAction.bPrepared = bPrepared;
	return AppliedAction;
}

bool IsSQLUIWidgetActionSupportedByCatalog(
	const FSQLUIWidgetActionApplyRequest& Request,
	const FSQLUILayoutNode& LayoutNode,
	const FSQLUILayoutAction& Action)
{
	USQLUIWidgetCatalog* WidgetCatalog = ResolveSQLUIWidgetCatalog(Request);
	if (!WidgetCatalog)
	{
		return true;
	}

	const FSQLUIWidgetCatalogEntry* CatalogEntry = WidgetCatalog->FindEntry(LayoutNode.WidgetTypeKey);
	if (!CatalogEntry || CatalogEntry->SupportedActionKeys.IsEmpty())
	{
		return true;
	}

	return CatalogEntry->SupportedActionKeys.Contains(Action.ActionTypeKey);
}

bool IsSQLUIRowClickedTrigger(const FString& Trigger)
{
	return Trigger.Equals(SQLUIWidgetActionTriggerRowClicked, ESearchCase::IgnoreCase)
		|| Trigger.Equals(SQLUIWidgetActionTriggerOnRowClicked, ESearchCase::IgnoreCase);
}

void ClearSQLUIWidgetEventActions(USQLUIBaseWidget* Widget)
{
	if (USQLUIListWidget* ListWidget = Cast<USQLUIListWidget>(Widget))
	{
		ListWidget->ClearRowClickActions();
	}
}

bool TryBindSQLUIWidgetEventAction(
	USQLUIBaseWidget* Widget,
	const FSQLUILayoutNode& LayoutNode,
	const FSQLUILayoutAction& Action,
	const FSQLUIActionRequest& ActionRequest,
	FSQLUIAppliedWidgetAction& AppliedAction,
	FSQLUIWidgetActionApplyResult& Result,
	const bool bFailOnUnsupportedActions)
{
	if (!IsSQLUIRowClickedTrigger(Action.Trigger))
	{
		return false;
	}

	USQLUIListWidget* ListWidget = Cast<USQLUIListWidget>(Widget);
	if (!IsValid(ListWidget))
	{
		AppliedAction.bUnsupportedAction = true;
		AppliedAction.bSkipped = true;
		AppliedAction.bFailed = bFailOnUnsupportedActions;
		Result.AppliedActions.Add(AppliedAction);

		AddSQLUIWidgetActionApplyProblem(
			Result,
			bFailOnUnsupportedActions,
			LayoutNode.WidgetId,
			Action,
			FString::Printf(
				TEXT("%s uses row-click trigger '%s', but widget type '%s' does not support row-click action binding."),
				*MakeSQLUIWidgetActionMessagePrefix(LayoutNode.WidgetId, Action),
				*Action.Trigger,
				*LayoutNode.WidgetTypeKey),
			false,
			true);
		return true;
	}

	ListWidget->AddRowClickAction(ActionRequest);
	AppliedAction.bBoundToEvent = true;
	AppliedAction.bSucceeded = true;
	Result.AppliedActions.Add(AppliedAction);
	return true;
}
}

FSQLUIWidgetActionApplyResult USQLUIWidgetActionApplier::ApplyActions(
	const FSQLUIWidgetActionApplyRequest& Request) const
{
	FSQLUIWidgetActionApplyResult Result;

	FString RegistryUnavailableMessage;
	USQLUIActionRegistry* ActionRegistry = ResolveSQLUIActionRegistry(Request, RegistryUnavailableMessage);

	for (int32 NodeIndex = 0; NodeIndex < Request.LayoutNodes.Num(); ++NodeIndex)
	{
		const FSQLUILayoutNode& LayoutNode = Request.LayoutNodes[NodeIndex];
		if (LayoutNode.WidgetId.IsEmpty())
		{
			AddSQLUIWidgetActionApplyError(
				Result,
				FString(),
				FSQLUILayoutAction(),
				FString::Printf(TEXT("SQLUI widget action application found a layout node without a widget id at index %d."), NodeIndex),
				true);
			continue;
		}

		USQLUIBaseWidget* Widget = FindSQLUIActionApplyWidget(Request, LayoutNode.WidgetId);
		if (!IsValid(Widget))
		{
			AddSQLUIWidgetActionApplyError(
				Result,
				LayoutNode.WidgetId,
				FSQLUILayoutAction(),
				FString::Printf(
					TEXT("SQLUI widget action application layout node '%s' does not have a valid created widget."),
					*LayoutNode.WidgetId),
				true);
			continue;
		}

		ClearSQLUIWidgetEventActions(Widget);

		for (const FSQLUILayoutAction& Action : LayoutNode.Actions)
		{
			FSQLUIActionRequest ActionRequest;
			FSQLUIAppliedWidgetAction AppliedAction = MakeSQLUIAppliedWidgetAction(
				LayoutNode.WidgetId,
				Action,
				ActionRequest,
				false);

			if (Action.ActionTypeKey.IsEmpty())
			{
				AppliedAction.bSkipped = true;
				AppliedAction.bFailed = true;
				Result.AppliedActions.Add(AppliedAction);

				AddSQLUIWidgetActionApplyError(
					Result,
					LayoutNode.WidgetId,
					Action,
					FString::Printf(
						TEXT("%s does not include an action type key."),
						*MakeSQLUIWidgetActionMessagePrefix(LayoutNode.WidgetId, Action)),
					true);
				continue;
			}

			if (Action.Trigger.IsEmpty())
			{
				AddSQLUIWidgetActionApplyWarning(
					Result,
					LayoutNode.WidgetId,
					Action,
					FString::Printf(
						TEXT("%s does not include a trigger. It will only run immediately when registered action execution is enabled."),
						*MakeSQLUIWidgetActionMessagePrefix(LayoutNode.WidgetId, Action)),
					true);
			}

			ActionRequest = MakeSQLUIActionRequest(LayoutNode, Action);
			AppliedAction = MakeSQLUIAppliedWidgetAction(
				LayoutNode.WidgetId,
				Action,
				ActionRequest,
				true);

			if (!IsSQLUIWidgetActionSupportedByCatalog(Request, LayoutNode, Action))
			{
				AppliedAction.bUnsupportedAction = true;
				AppliedAction.bSkipped = true;
				AppliedAction.bFailed = Request.bFailOnUnsupportedActions;
				Result.AppliedActions.Add(AppliedAction);

				AddSQLUIWidgetActionApplyProblem(
					Result,
					Request.bFailOnUnsupportedActions,
					LayoutNode.WidgetId,
					Action,
					FString::Printf(
						TEXT("%s uses unsupported action type key '%s' for widget type '%s'."),
						*MakeSQLUIWidgetActionMessagePrefix(LayoutNode.WidgetId, Action),
						*Action.ActionTypeKey,
						*LayoutNode.WidgetTypeKey),
					false,
					true);
				continue;
			}

			if (!IsValid(ActionRegistry))
			{
				AppliedAction.bRegistryUnavailable = true;
				AppliedAction.bSkipped = true;
				AppliedAction.bFailed = Request.bFailOnUnregisteredActions;
				Result.AppliedActions.Add(AppliedAction);

				AddSQLUIWidgetActionApplyProblem(
					Result,
					Request.bFailOnUnregisteredActions,
					LayoutNode.WidgetId,
					Action,
					RegistryUnavailableMessage.IsEmpty()
						? FString::Printf(
							TEXT("%s could not check action registration because the action registry is unavailable."),
							*MakeSQLUIWidgetActionMessagePrefix(LayoutNode.WidgetId, Action))
						: RegistryUnavailableMessage,
					false,
					false,
					false,
					true);
				continue;
			}

			const FSQLUIActionKey ActionKey = ActionRequest.ActionKey;
			AppliedAction.bRegistered = ActionRegistry->IsActionRegistered(ActionKey);
			if (!AppliedAction.bRegistered)
			{
				AppliedAction.bUnregisteredAction = true;
				AppliedAction.bSkipped = true;
				AppliedAction.bFailed = Request.bFailOnUnregisteredActions;
				Result.AppliedActions.Add(AppliedAction);

				AddSQLUIWidgetActionApplyProblem(
					Result,
					Request.bFailOnUnregisteredActions,
					LayoutNode.WidgetId,
					Action,
					FString::Printf(
						TEXT("%s prepared action type key '%s', but no action handler is registered yet."),
						*MakeSQLUIWidgetActionMessagePrefix(LayoutNode.WidgetId, Action),
						*Action.ActionTypeKey),
					false,
					false,
					true);
				continue;
			}

			if (TryBindSQLUIWidgetEventAction(
				Widget,
				LayoutNode,
				Action,
				ActionRequest,
				AppliedAction,
				Result,
				Request.bFailOnUnsupportedActions))
			{
				continue;
			}

			if (!Action.Trigger.IsEmpty())
			{
				AppliedAction.bSkipped = true;
				Result.AppliedActions.Add(AppliedAction);
				AddSQLUIWidgetActionApplyWarning(
					Result,
					LayoutNode.WidgetId,
					Action,
					FString::Printf(
						TEXT("%s uses trigger '%s', but this trigger is not wired to a runtime widget event yet."),
						*MakeSQLUIWidgetActionMessagePrefix(LayoutNode.WidgetId, Action),
						*Action.Trigger));
				continue;
			}

			if (!Request.bExecuteRegisteredActions)
			{
				AppliedAction.bSkipped = true;
				Result.AppliedActions.Add(AppliedAction);
				continue;
			}

			AppliedAction.bExecuted = true;
			AppliedAction.ActionResult = ActionRegistry->ExecuteAction(ActionRequest);
			AppliedAction.bSucceeded = AppliedAction.ActionResult.bSucceeded;
			AppliedAction.bFailed = !AppliedAction.ActionResult.bSucceeded;
			Result.AppliedActions.Add(AppliedAction);

			if (!AppliedAction.ActionResult.bSucceeded)
			{
				const FString ExecutionMessage = AppliedAction.ActionResult.ErrorMessage.IsEmpty()
					? FString::Printf(
						TEXT("%s executed action type key '%s', but the action reported failure."),
						*MakeSQLUIWidgetActionMessagePrefix(LayoutNode.WidgetId, Action),
						*Action.ActionTypeKey)
					: AppliedAction.ActionResult.ErrorMessage;

				AddSQLUIWidgetActionApplyProblem(
					Result,
					Request.bFailOnActionExecutionFailure,
					LayoutNode.WidgetId,
					Action,
					ExecutionMessage,
					false,
					false,
					false,
					false,
					true);
			}
		}
	}

	Result.bSucceeded = Result.Errors.IsEmpty();
	return Result;
}
