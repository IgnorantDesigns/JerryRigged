#include "Widgets/SQLUIListWidget.h"

void USQLUIListWidget::SetItems(const TArray<FSQLUIListItemData>& InItems)
{
	Items = InItems;
	NativeOnItemsChanged();
}

TArray<FSQLUIListItemData> USQLUIListWidget::GetItems() const
{
	return Items;
}

void USQLUIListWidget::ClearItems()
{
	Items.Reset();
	NativeOnItemsChanged();
}

void USQLUIListWidget::NativeOnItemsChanged()
{
}
