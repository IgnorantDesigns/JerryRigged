#pragma once

#include "Actions/SQLUIActionTypes.h"
#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "Widgets/SQLUIBaseWidget.h"

#include "SQLUIWidgetActionTypes.generated.h"

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetActionApplyError
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString ActionTypeKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bInvalidAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bUnsupportedAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bUnregisteredAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bRegistryUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bActionExecutionFailure = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetActionApplyWarning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString ActionTypeKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString WarningMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bMissingTrigger = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bUnsupportedAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bUnregisteredAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bRegistryUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bActionExecutionFailure = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIAppliedWidgetAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString ActionTypeKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FSQLUIActionRequest ActionRequest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FSQLUIActionResult ActionResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bPrepared = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bRegistered = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bBoundToEvent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bExecuted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bFailed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bSkipped = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bUnsupportedAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bUnregisteredAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bRegistryUnavailable = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetActionApplyRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	TArray<FSQLUILayoutNode> LayoutNodes;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Actions")
	TMap<FString, USQLUIBaseWidget*> CreatedWidgetMap;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Actions")
	TObjectPtr<USQLUIRuntimeContext> RuntimeContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bExecuteRegisteredActions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bFailOnUnsupportedActions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bFailOnUnregisteredActions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bFailOnActionExecutionFailure = true;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetActionApplyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	TArray<FSQLUIWidgetActionApplyError> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	TArray<FSQLUIWidgetActionApplyWarning> Warnings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Actions")
	TArray<FSQLUIAppliedWidgetAction> AppliedActions;
};
