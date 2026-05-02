#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Layout/SQLUILayoutWidgetFactory.h"
#include "UObject/Object.h"

#include "SQLUIWidgetConstructor.generated.h"

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetConstructionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Construction")
	TArray<FSQLUIPreparedWidgetBuildNode> PreparedNodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Construction")
	FString RootWidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SQLUI|Widget Construction")
	TObjectPtr<USQLUIRuntimeContext> RuntimeContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SQLUI|Widget Construction")
	TObjectPtr<APlayerController> OwningPlayer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SQLUI|Widget Construction")
	TObjectPtr<UObject> WorldContextObject = nullptr;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUICreatedWidgetRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Construction")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Construction")
	FSQLUILayoutNode LayoutNode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Construction")
	TSubclassOf<USQLUIBaseWidget> WidgetClass = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Construction")
	TObjectPtr<USQLUIBaseWidget> Widget = nullptr;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetConstructionResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Construction")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Construction")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Construction")
	TArray<FString> Errors;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Construction")
	TObjectPtr<USQLUIBaseWidget> RootWidget = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Construction")
	TArray<FSQLUICreatedWidgetRecord> CreatedWidgets;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget Construction")
	TMap<FString, USQLUIBaseWidget*> CreatedWidgetMap;
};

UCLASS(BlueprintType)
class SQLUIWIDGETS_API USQLUIWidgetConstructor : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Widget Construction")
	FSQLUIWidgetConstructionResult ConstructWidgets(const FSQLUIWidgetConstructionRequest& Request) const;
};
