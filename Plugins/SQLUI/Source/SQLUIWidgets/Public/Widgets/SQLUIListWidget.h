#pragma once

#include "CoreMinimal.h"
#include "Widgets/SQLUIBaseWidget.h"
#include "Widgets/SQLUIListTypes.h"

#include "SQLUIListWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SQLUIWIDGETS_API USQLUIListWidget : public USQLUIBaseWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|List")
	void SetItems(const TArray<FSQLUIListItemData>& InItems);

	UFUNCTION(BlueprintPure, Category = "SQLUI|List")
	TArray<FSQLUIListItemData> GetItems() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|List")
	void ClearItems();

protected:
	virtual void NativeOnItemsChanged();

private:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SQLUI|List", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIListItemData> Items;
};
