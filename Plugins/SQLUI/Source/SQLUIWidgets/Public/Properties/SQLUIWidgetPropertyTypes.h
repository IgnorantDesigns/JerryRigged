#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Widgets/SQLUIBaseWidget.h"

#include "SQLUIWidgetPropertyTypes.generated.h"

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetPropertyApplyError
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	FString PropertyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	bool bUnsupportedProperty = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetPropertyApplyRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	TArray<FSQLUILayoutNode> LayoutNodes;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Properties")
	TMap<FString, USQLUIBaseWidget*> CreatedWidgetMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	bool bFailOnUnsupportedProperties = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetPropertyApplyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	TArray<FSQLUIWidgetPropertyApplyError> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	TArray<FString> Warnings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Properties")
	TArray<FString> AppliedPropertyNames;
};
