#pragma once

#include "CoreMinimal.h"
#include "Properties/SQLUIWidgetPropertyTypes.h"
#include "UObject/Object.h"

#include "SQLUIWidgetPropertyApplier.generated.h"

UCLASS(BlueprintType)
class SQLUIWIDGETS_API USQLUIWidgetPropertyApplier : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Widget Properties")
	FSQLUIWidgetPropertyApplyResult ApplyProperties(const FSQLUIWidgetPropertyApplyRequest& Request) const;
};
