#pragma once

#include "Actions/SQLUIWidgetActionTypes.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "SQLUIWidgetActionApplier.generated.h"

UCLASS(BlueprintType)
class SQLUIWIDGETS_API USQLUIWidgetActionApplier : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Widget Actions")
	FSQLUIWidgetActionApplyResult ApplyActions(const FSQLUIWidgetActionApplyRequest& Request) const;
};
