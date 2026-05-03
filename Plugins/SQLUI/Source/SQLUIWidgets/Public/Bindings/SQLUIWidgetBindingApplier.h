#pragma once

#include "Bindings/SQLUIWidgetBindingTypes.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "SQLUIWidgetBindingApplier.generated.h"

UCLASS(BlueprintType)
class SQLUIWIDGETS_API USQLUIWidgetBindingApplier : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Widget Bindings")
	FSQLUIWidgetBindingApplyResult ApplyBindings(const FSQLUIWidgetBindingApplyRequest& Request) const;
};
