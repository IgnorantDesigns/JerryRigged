#include "Widgets/SQLUIListItemWidget.h"

void USQLUIListItemWidget::SetListItemData(const FSQLUIListItemData& InListItemData)
{
	ListItemData = InListItemData;
	NativeOnListItemDataChanged();
}

FSQLUIListItemData USQLUIListItemWidget::GetListItemData() const
{
	return ListItemData;
}

void USQLUIListItemWidget::NativeOnListItemDataChanged()
{
}

bool USQLUIListItemWidget::NativeApplySQLUIWidgetProperty(
	const FString& PropertyName,
	const FString& PropertyValue,
	FString& OutFailureMessage,
	bool& bOutUnsupportedProperty)
{
	if (PropertyName == TEXT("ItemId"))
	{
		FSQLUIListItemData UpdatedListItemData = GetListItemData();
		UpdatedListItemData.ItemId = PropertyValue;
		SetListItemData(UpdatedListItemData);
		return true;
	}

	if (PropertyName == TEXT("DisplayText"))
	{
		FSQLUIListItemData UpdatedListItemData = GetListItemData();
		UpdatedListItemData.DisplayText = FText::FromString(PropertyValue);
		SetListItemData(UpdatedListItemData);
		return true;
	}

	return Super::NativeApplySQLUIWidgetProperty(
		PropertyName,
		PropertyValue,
		OutFailureMessage,
		bOutUnsupportedProperty);
}
