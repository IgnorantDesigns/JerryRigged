#pragma once

#include "CoreMinimal.h"
#include "Widgets/SQLUIBaseWidget.h"
#include "Widgets/SQLUIListTypes.h"

#include "SQLUIListItemWidget.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;

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
	virtual void NativeOnInitialized() override;
	virtual void NativeOnSQLUIWidgetInitialized() override;
	virtual void NativeOnListItemDataChanged();
	virtual bool NativeApplySQLUIWidgetProperty(
		const FString& PropertyName,
		const FString& PropertyValue,
		FString& OutFailureMessage,
		bool& bOutUnsupportedProperty) override;

private:
	void EnsureVisualRoot();
	void SyncVisualState();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SQLUI|List", meta = (AllowPrivateAccess = "true"))
	FSQLUIListItemData ListItemData;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ItemBorder = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ItemTextContainer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DisplayTextBlock = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ItemIdTextBlock = nullptr;
};
