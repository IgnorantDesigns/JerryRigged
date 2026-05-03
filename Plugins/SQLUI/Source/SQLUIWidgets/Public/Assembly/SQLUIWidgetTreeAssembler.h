#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"
#include "UObject/Object.h"
#include "Widgets/SQLUIBaseWidget.h"

#include "SQLUIWidgetTreeAssembler.generated.h"

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetTreeAssemblyRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	FString RootWidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	TArray<FSQLUILayoutNode> LayoutNodes;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Tree Assembly")
	TMap<FString, TObjectPtr<USQLUIBaseWidget>> CreatedWidgetMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	bool bFailOnUnsupportedContainers = false;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIAssembledWidgetNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	FString ParentWidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	FSQLUILayoutNode LayoutNode;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Tree Assembly")
	TObjectPtr<USQLUIBaseWidget> Widget = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Tree Assembly")
	TObjectPtr<USQLUIBaseWidget> ParentWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	TArray<FString> ChildWidgetIds;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Tree Assembly")
	TArray<TObjectPtr<USQLUIBaseWidget>> ChildWidgets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	bool bHasChildren = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	bool bCanAcceptAllChildren = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	TArray<FString> UnsupportedChildWidgetIds;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetTreeAssemblyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	TArray<FString> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	TArray<FString> Warnings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Tree Assembly")
	FString RootWidgetId;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Tree Assembly")
	TObjectPtr<USQLUIBaseWidget> RootWidget = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Tree Assembly")
	TArray<FSQLUIAssembledWidgetNode> AssembledNodes;
};

UCLASS(BlueprintType)
class SQLUIWIDGETS_API USQLUIWidgetTreeAssembler : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Widget Tree Assembly")
	FSQLUIWidgetTreeAssemblyResult AssembleWidgetTree(const FSQLUIWidgetTreeAssemblyRequest& Request) const;
};
