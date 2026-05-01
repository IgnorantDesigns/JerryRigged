#include "Widgets/SQLUIBaseWidget.h"

void USQLUIBaseWidget::InitializeSQLUIWidget(const FSQLUIWidgetInitializeParams& InInitializeParams)
{
	InitializeParams = InInitializeParams;
	bSQLUIWidgetInitialized = true;

	NativeOnSQLUIWidgetInitialized();
}

bool USQLUIBaseWidget::IsSQLUIWidgetInitialized() const
{
	return bSQLUIWidgetInitialized;
}

USQLUIRuntimeContext* USQLUIBaseWidget::GetSQLUIRuntimeContext() const
{
	return InitializeParams.RuntimeContext.Get();
}

const FSQLUILayoutNode& USQLUIBaseWidget::GetSQLUILayoutNode() const
{
	return InitializeParams.LayoutNode;
}

FString USQLUIBaseWidget::GetSQLUIWidgetId() const
{
	return InitializeParams.LayoutNode.WidgetId;
}

FString USQLUIBaseWidget::GetSQLUIWidgetTypeKey() const
{
	return InitializeParams.LayoutNode.WidgetTypeKey;
}

void USQLUIBaseWidget::NativeOnSQLUIWidgetInitialized()
{
}
