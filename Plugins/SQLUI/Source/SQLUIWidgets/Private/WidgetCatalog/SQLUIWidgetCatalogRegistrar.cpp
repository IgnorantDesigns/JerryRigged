#include "WidgetCatalog/SQLUIWidgetCatalogRegistrar.h"

#include "Widgets/SQLUIFilterBox.h"
#include "Widgets/SQLUIListItemWidget.h"
#include "Widgets/SQLUIListWidget.h"
#include "Widgets/SQLUIVerticalBoxWidget.h"

namespace
{
FSQLUIWidgetTypeKey MakeSQLUIWidgetTypeKey(const TCHAR* Value)
{
	FSQLUIWidgetTypeKey Key;
	Key.Value = Value;
	return Key;
}

TArray<FString> MakeSQLUIVariableBindingKeys()
{
	return {
		TEXT("Variable"),
		TEXT("VariableStore")
	};
}
}

FSQLUIWidgetTypeKey FSQLUIWidgetTypeKeys::VerticalBox()
{
	return MakeSQLUIWidgetTypeKey(TEXT("SQLUI.VerticalBox"));
}

FSQLUIWidgetTypeKey FSQLUIWidgetTypeKeys::FilterBox()
{
	return MakeSQLUIWidgetTypeKey(TEXT("SQLUI.FilterBox"));
}

FSQLUIWidgetTypeKey FSQLUIWidgetTypeKeys::ListWidget()
{
	return MakeSQLUIWidgetTypeKey(TEXT("SQLUI.ListWidget"));
}

FSQLUIWidgetTypeKey FSQLUIWidgetTypeKeys::ListItemWidget()
{
	return MakeSQLUIWidgetTypeKey(TEXT("SQLUI.ListItemWidget"));
}

bool USQLUIWidgetCatalogRegistrar::RegisterDefaultSQLUIWidgets(USQLUIWidgetCatalog* WidgetCatalog)
{
	if (!WidgetCatalog)
	{
		return false;
	}

	bool bRegisteredAll = true;
	bRegisteredAll &= RegisterVerticalBox(WidgetCatalog);
	bRegisteredAll &= RegisterFilterBox(WidgetCatalog);
	bRegisteredAll &= RegisterListWidget(WidgetCatalog);
	bRegisteredAll &= RegisterListItemWidget(WidgetCatalog);
	return bRegisteredAll;
}

bool USQLUIWidgetCatalogRegistrar::RegisterVerticalBox(USQLUIWidgetCatalog* WidgetCatalog)
{
	if (!WidgetCatalog)
	{
		return false;
	}

	FSQLUIWidgetCatalogEntry Entry;
	Entry.WidgetTypeKey = FSQLUIWidgetTypeKeys::VerticalBox();
	Entry.DisplayName = TEXT("Vertical Box");
	Entry.Description = TEXT("SQLUI vertical container widget metadata.");
	Entry.WidgetClassPath = USQLUIVerticalBoxWidget::StaticClass()->GetPathName();
	Entry.SupportedPropertyKeys = {
		TEXT("IsEnabled")
	};
	Entry.SupportedBindingKeys = MakeSQLUIVariableBindingKeys();
	Entry.SupportedActionKeys = {};

	return WidgetCatalog->RegisterEntry(Entry);
}

bool USQLUIWidgetCatalogRegistrar::RegisterFilterBox(USQLUIWidgetCatalog* WidgetCatalog)
{
	if (!WidgetCatalog)
	{
		return false;
	}

	FSQLUIWidgetCatalogEntry Entry;
	Entry.WidgetTypeKey = FSQLUIWidgetTypeKeys::FilterBox();
	Entry.DisplayName = TEXT("Filter Box");
	Entry.Description = TEXT("SQLUI filter input widget metadata.");
	Entry.WidgetClassPath = USQLUIFilterBox::StaticClass()->GetPathName();
	Entry.SupportedPropertyKeys = {
		TEXT("FilterText"),
		TEXT("PlaceholderText"),
		TEXT("IsEnabled")
	};
	Entry.SupportedBindingKeys = MakeSQLUIVariableBindingKeys();
	Entry.SupportedActionKeys = {};

	return WidgetCatalog->RegisterEntry(Entry);
}

bool USQLUIWidgetCatalogRegistrar::RegisterListWidget(USQLUIWidgetCatalog* WidgetCatalog)
{
	if (!WidgetCatalog)
	{
		return false;
	}

	FSQLUIWidgetCatalogEntry Entry;
	Entry.WidgetTypeKey = FSQLUIWidgetTypeKeys::ListWidget();
	Entry.DisplayName = TEXT("List Widget");
	Entry.Description = TEXT("SQLUI list widget metadata.");
	Entry.WidgetClassPath = USQLUIListWidget::StaticClass()->GetPathName();
	Entry.SupportedPropertyKeys = {
		TEXT("Items"),
		TEXT("EmptyText"),
		TEXT("SelectionMode"),
		TEXT("IsEnabled")
	};
	Entry.SupportedBindingKeys = MakeSQLUIVariableBindingKeys();
	Entry.SupportedActionKeys = {};

	return WidgetCatalog->RegisterEntry(Entry);
}

bool USQLUIWidgetCatalogRegistrar::RegisterListItemWidget(USQLUIWidgetCatalog* WidgetCatalog)
{
	if (!WidgetCatalog)
	{
		return false;
	}

	FSQLUIWidgetCatalogEntry Entry;
	Entry.WidgetTypeKey = FSQLUIWidgetTypeKeys::ListItemWidget();
	Entry.DisplayName = TEXT("List Item Widget");
	Entry.Description = TEXT("SQLUI list item widget metadata.");
	Entry.WidgetClassPath = USQLUIListItemWidget::StaticClass()->GetPathName();
	Entry.SupportedPropertyKeys = {
		TEXT("ItemId"),
		TEXT("DisplayText"),
		TEXT("Properties"),
		TEXT("Metadata"),
		TEXT("IsSelected")
	};
	Entry.SupportedBindingKeys = MakeSQLUIVariableBindingKeys();
	Entry.SupportedActionKeys = {};

	return WidgetCatalog->RegisterEntry(Entry);
}
