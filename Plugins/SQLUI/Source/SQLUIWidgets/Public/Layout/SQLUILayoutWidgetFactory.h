#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "UObject/Object.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"
#include "Widgets/SQLUIBaseWidget.h"

#include "SQLUILayoutWidgetFactory.generated.h"

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetBuildRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	FSQLUILayoutDocument Document;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	TObjectPtr<USQLUIRuntimeContext> RuntimeContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	TObjectPtr<USQLUIWidgetCatalog> WidgetCatalogOverride = nullptr;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIPreparedWidgetBuildNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	FSQLUILayoutNode LayoutNode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	FSQLUIWidgetCatalogEntry CatalogEntry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	TSubclassOf<USQLUIBaseWidget> WidgetClass = nullptr;
};

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetBuildResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	FSQLUILayoutValidationResult LayoutValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	FSQLUIWidgetCatalogValidationResult CatalogValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	FString RootWidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Widget Factory")
	TArray<FSQLUIPreparedWidgetBuildNode> PreparedNodes;
};

UCLASS(BlueprintType)
class SQLUIWIDGETS_API USQLUILayoutWidgetBuilder : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Widget Factory")
	FSQLUIWidgetBuildResult PrepareBuild(const FSQLUIWidgetBuildRequest& Request) const;
};

UCLASS(BlueprintType)
class SQLUIWIDGETS_API USQLUILayoutWidgetFactory : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Widget Factory")
	FSQLUIWidgetBuildResult PrepareBuild(const FSQLUIWidgetBuildRequest& Request) const;
};
