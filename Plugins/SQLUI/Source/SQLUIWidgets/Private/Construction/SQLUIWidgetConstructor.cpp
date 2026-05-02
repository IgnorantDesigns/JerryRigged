#include "Construction/SQLUIWidgetConstructor.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "Widgets/SQLUIBaseWidget.h"

namespace
{
void AddSQLUIWidgetConstructionError(FSQLUIWidgetConstructionResult& Result, const FString& ErrorMessage)
{
	Result.bSucceeded = false;
	Result.Errors.Add(ErrorMessage);

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = ErrorMessage;
	}
}

UWorld* ResolveSQLUIWidgetConstructionWorld(const FSQLUIWidgetConstructionRequest& Request)
{
	if (IsValid(Request.OwningPlayer.Get()))
	{
		return Request.OwningPlayer->GetWorld();
	}

	if (!IsValid(Request.WorldContextObject.Get()) || !GEngine)
	{
		return nullptr;
	}

	return GEngine->GetWorldFromContextObject(Request.WorldContextObject.Get(), EGetWorldErrorMode::ReturnNull);
}

void ValidateSQLUIWidgetConstructionRequest(
	const FSQLUIWidgetConstructionRequest& Request,
	FSQLUIWidgetConstructionResult& Result)
{
	if (!IsValid(Request.RuntimeContext.Get()))
	{
		AddSQLUIWidgetConstructionError(
			Result,
			TEXT("SQLUI widget construction requires a runtime context."));
	}
	else if (!Request.RuntimeContext->IsInitialized())
	{
		AddSQLUIWidgetConstructionError(
			Result,
			TEXT("SQLUI widget construction requires an initialized runtime context."));
	}

	if (Request.OwningPlayer && !IsValid(Request.OwningPlayer.Get()))
	{
		AddSQLUIWidgetConstructionError(
			Result,
			TEXT("SQLUI widget construction received an invalid owning player."));
	}

	if (!IsValid(Request.OwningPlayer.Get()) && !ResolveSQLUIWidgetConstructionWorld(Request))
	{
		AddSQLUIWidgetConstructionError(
			Result,
			TEXT("SQLUI widget construction requires an owning player or a resolvable world context."));
	}

	if (Request.PreparedNodes.IsEmpty())
	{
		AddSQLUIWidgetConstructionError(
			Result,
			TEXT("SQLUI widget construction requires at least one prepared widget build node."));
	}

	TSet<FString> WidgetIds;
	for (const FSQLUIPreparedWidgetBuildNode& PreparedNode : Request.PreparedNodes)
	{
		const FString& WidgetId = PreparedNode.LayoutNode.WidgetId;
		if (WidgetId.IsEmpty())
		{
			AddSQLUIWidgetConstructionError(
				Result,
				TEXT("SQLUI widget construction found a prepared node without a widget id."));
		}
		else if (WidgetIds.Contains(WidgetId))
		{
			AddSQLUIWidgetConstructionError(
				Result,
				FString::Printf(TEXT("SQLUI widget construction found duplicate widget id '%s'."), *WidgetId));
		}
		else
		{
			WidgetIds.Add(WidgetId);
		}

		UClass* WidgetClass = PreparedNode.WidgetClass.Get();
		if (!WidgetClass)
		{
			AddSQLUIWidgetConstructionError(
				Result,
				WidgetId.IsEmpty()
					? TEXT("SQLUI widget construction found a prepared node without a widget class.")
					: FString::Printf(TEXT("SQLUI widget construction node '%s' does not include a widget class."), *WidgetId));
			continue;
		}

		if (!WidgetClass->IsChildOf(USQLUIBaseWidget::StaticClass()))
		{
			AddSQLUIWidgetConstructionError(
				Result,
				WidgetId.IsEmpty()
					? FString::Printf(TEXT("SQLUI widget construction class '%s' is not a USQLUIBaseWidget."), *WidgetClass->GetPathName())
					: FString::Printf(TEXT("SQLUI widget construction node '%s' uses class '%s', which is not a USQLUIBaseWidget."), *WidgetId, *WidgetClass->GetPathName()));
		}

		if (WidgetClass->HasAnyClassFlags(CLASS_Abstract))
		{
			AddSQLUIWidgetConstructionError(
				Result,
				WidgetId.IsEmpty()
					? FString::Printf(TEXT("SQLUI widget construction class '%s' is abstract."), *WidgetClass->GetPathName())
					: FString::Printf(TEXT("SQLUI widget construction node '%s' uses abstract class '%s'."), *WidgetId, *WidgetClass->GetPathName()));
		}
	}

	if (!Request.RootWidgetId.IsEmpty() && !WidgetIds.Contains(Request.RootWidgetId))
	{
		AddSQLUIWidgetConstructionError(
			Result,
			FString::Printf(TEXT("SQLUI widget construction root widget id '%s' does not match a prepared node."), *Request.RootWidgetId));
	}
}
}

FSQLUIWidgetConstructionResult USQLUIWidgetConstructor::ConstructWidgets(const FSQLUIWidgetConstructionRequest& Request) const
{
	FSQLUIWidgetConstructionResult Result;
	ValidateSQLUIWidgetConstructionRequest(Request, Result);

	if (!Result.Errors.IsEmpty())
	{
		return Result;
	}

	UWorld* World = ResolveSQLUIWidgetConstructionWorld(Request);
	for (const FSQLUIPreparedWidgetBuildNode& PreparedNode : Request.PreparedNodes)
	{
		UClass* WidgetClass = PreparedNode.WidgetClass.Get();
		USQLUIBaseWidget* CreatedWidget = Request.OwningPlayer
			? CreateWidget<USQLUIBaseWidget>(Request.OwningPlayer.Get(), WidgetClass)
			: CreateWidget<USQLUIBaseWidget>(World, WidgetClass);

		if (!CreatedWidget)
		{
			AddSQLUIWidgetConstructionError(
				Result,
				FString::Printf(
					TEXT("SQLUI widget construction failed to create widget '%s' using class '%s'."),
					*PreparedNode.LayoutNode.WidgetId,
					*WidgetClass->GetPathName()));
			return Result;
		}

		FSQLUIWidgetInitializeParams InitializeParams;
		InitializeParams.RuntimeContext = Request.RuntimeContext;
		InitializeParams.LayoutNode = PreparedNode.LayoutNode;
		CreatedWidget->InitializeSQLUIWidget(InitializeParams);

		FSQLUICreatedWidgetRecord CreatedWidgetRecord;
		CreatedWidgetRecord.WidgetId = PreparedNode.LayoutNode.WidgetId;
		CreatedWidgetRecord.LayoutNode = PreparedNode.LayoutNode;
		CreatedWidgetRecord.WidgetClass = WidgetClass;
		CreatedWidgetRecord.Widget = CreatedWidget;

		Result.CreatedWidgets.Add(CreatedWidgetRecord);
		Result.CreatedWidgetMap.Add(CreatedWidgetRecord.WidgetId, CreatedWidget);

		if (!Request.RootWidgetId.IsEmpty() && CreatedWidgetRecord.WidgetId == Request.RootWidgetId)
		{
			Result.RootWidget = CreatedWidget;
		}
	}

	Result.bSucceeded = true;
	return Result;
}
