#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"
#include "UObject/Object.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"

#include "SQLUILayoutFactory.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutFactoryRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Factory")
	FSQLUILayoutDocument Document;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Factory")
	TObjectPtr<USQLUIWidgetCatalog> WidgetCatalog = nullptr;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutFactoryResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Factory")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Factory")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Factory")
	FSQLUILayoutValidationResult LayoutValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Factory")
	FSQLUIWidgetCatalogValidationResult CatalogValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Factory")
	FSQLUILayoutDocument PreparedDocument;
};

UCLASS(BlueprintType)
class SQLUICORE_API USQLUILayoutFactory : public UObject
{
	GENERATED_BODY()

public:
	FSQLUILayoutFactoryResult PrepareLayout(const FSQLUILayoutFactoryRequest& Request) const;
};
