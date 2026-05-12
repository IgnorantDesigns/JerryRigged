#pragma once

#include "CoreMinimal.h"
#include "Widgets/SQLUIBaseWidget.h"
#include "Widgets/SQLUIListTypes.h"

#include "SQLUIListWidget.generated.h"

class UBorder;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class USQLUIListItemWidget;

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

	UFUNCTION(BlueprintCallable, Category = "SQLUI|List")
	void SetEmptyText(const FText& InEmptyText);

	UFUNCTION(BlueprintPure, Category = "SQLUI|List")
	FText GetEmptyText() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnSQLUIWidgetInitialized() override;
	virtual void NativeOnItemsChanged();
	virtual bool NativeApplySQLUIWidgetProperty(
		const FString& PropertyName,
		const FString& PropertyValue,
		FString& OutFailureMessage,
		bool& bOutUnsupportedProperty) override;

private:
	void EnsureVisualRoot();
	void RebuildListItems();
	void AddEmptyStateRow();
	void AddListItemRow(const FSQLUIListItemData& ItemData);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SQLUI|List", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIListItemData> Items;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SQLUI|List", meta = (AllowPrivateAccess = "true"))
	FText EmptyText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ListBorder = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> ListScrollBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ItemContainer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EmptyTextBlock = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USQLUIListItemWidget>> ItemWidgets;
};
