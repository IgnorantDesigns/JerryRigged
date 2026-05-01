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
