#pragma once

#include "CoreMinimal.h"
#include "Bindings/SQLUIBindingResolver.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "Widgets/SQLUIBaseWidget.h"

#include "SQLUIWidgetBindingTypes.generated.h"

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetBindingApplyError
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString BindingId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString TargetProperty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString SourceKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString SourcePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bUnsupportedBinding = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bPropertyApplyFailure = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bUnsupportedProperty = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetBindingApplyWarning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString BindingId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString TargetProperty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString SourceKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString SourcePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString WarningMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bUnsupportedBinding = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bPropertyApplyFailure = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bUnsupportedProperty = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIAppliedWidgetBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString BindingId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString TargetProperty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString SourceKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString SourcePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString ResolvedValueString;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bAppliedToWidget = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetBindingApplyRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	TArray<FSQLUILayoutNode> LayoutNodes;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Bindings")
	TMap<FString, USQLUIBaseWidget*> CreatedWidgetMap;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Bindings")
	TObjectPtr<USQLUIRuntimeContext> RuntimeContext = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Bindings")
	TObjectPtr<USQLUIBindingResolver> BindingResolverOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bFailOnUnsupportedBindings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bFailOnPropertyApplyFailure = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetBindingApplyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	TArray<FSQLUIWidgetBindingApplyError> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	TArray<FSQLUIWidgetBindingApplyWarning> Warnings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Bindings")
	TArray<FSQLUIAppliedWidgetBinding> AppliedBindings;
};
