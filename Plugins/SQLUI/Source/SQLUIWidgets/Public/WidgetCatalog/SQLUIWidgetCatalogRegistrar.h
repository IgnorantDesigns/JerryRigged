#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"

#include "SQLUIWidgetCatalogRegistrar.generated.h"

struct SQLUIWIDGETS_API FSQLUIWidgetTypeKeys
{
	static FSQLUIWidgetTypeKey VerticalBox();
	static FSQLUIWidgetTypeKey FilterBox();
	static FSQLUIWidgetTypeKey ListWidget();
	static FSQLUIWidgetTypeKey ListItemWidget();
};

UCLASS()
class SQLUIWIDGETS_API USQLUIWidgetCatalogRegistrar : public UObject
{
	GENERATED_BODY()

public:
	static bool RegisterDefaultSQLUIWidgets(USQLUIWidgetCatalog* WidgetCatalog);
	static bool RegisterVerticalBox(USQLUIWidgetCatalog* WidgetCatalog);
	static bool RegisterFilterBox(USQLUIWidgetCatalog* WidgetCatalog);
	static bool RegisterListWidget(USQLUIWidgetCatalog* WidgetCatalog);
	static bool RegisterListItemWidget(USQLUIWidgetCatalog* WidgetCatalog);
};
