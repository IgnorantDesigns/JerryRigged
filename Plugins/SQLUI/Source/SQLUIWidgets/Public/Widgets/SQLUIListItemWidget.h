#pragma once

#include "CoreMinimal.h"
#include "Widgets/SQLUIBaseWidget.h"
#include "Widgets/SQLUIListTypes.h"

#include "SQLUIListItemWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SQLUIWIDGETS_API USQLUIListItemWidget : public USQLUIBaseWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|List")
	void SetListItemData(const FSQLUIListItemData& InListItemData);

	UFUNCTION(BlueprintPure, Category = "SQLUI|List")
	FSQLUIListItemData GetListItemData() const;

protected:
	virtual void NativeOnListItemDataChanged();
	virtual bool NativeApplySQLUIWidgetProperty(
		const FString& PropertyName,
		const FString& PropertyValue,
		FString& OutFailureMessage,
		bool& bOutUnsupportedProperty) override;

private:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SQLUI|List", meta = (AllowPrivateAccess = "true"))
	FSQLUIListItemData ListItemData;
};
