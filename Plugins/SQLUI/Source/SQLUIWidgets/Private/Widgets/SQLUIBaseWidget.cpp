#include "Widgets/SQLUIBaseWidget.h"

namespace
{
bool TryParseSQLUIWidgetBool(const FString& Value, bool& bOutValue)
{
	const FString NormalizedValue = Value.TrimStartAndEnd().ToLower();
	if (NormalizedValue == TEXT("true")
		|| NormalizedValue == TEXT("1")
		|| NormalizedValue == TEXT("yes")
		|| NormalizedValue == TEXT("on"))
	{
		bOutValue = true;
		return true;
	}

	if (NormalizedValue == TEXT("false")
		|| NormalizedValue == TEXT("0")
		|| NormalizedValue == TEXT("no")
		|| NormalizedValue == TEXT("off"))
	{
		bOutValue = false;
		return true;
	}

	return false;
}
}

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

bool USQLUIBaseWidget::ApplySQLUIWidgetProperty(
	const FString& PropertyName,
	const FString& PropertyValue,
	FString& OutFailureMessage,
	bool& bOutUnsupportedProperty)
{
	OutFailureMessage.Reset();
	bOutUnsupportedProperty = false;

	return NativeApplySQLUIWidgetProperty(
		PropertyName,
		PropertyValue,
		OutFailureMessage,
		bOutUnsupportedProperty);
}

bool USQLUIBaseWidget::CanAcceptSQLUIChildWidget(
	const USQLUIBaseWidget* ChildWidget,
	const FSQLUILayoutNode& ChildLayoutNode) const
{
	return false;
}

bool USQLUIBaseWidget::AddSQLUIChildWidget(USQLUIBaseWidget* ChildWidget)
{
	return false;
}

void USQLUIBaseWidget::NativeOnSQLUIWidgetInitialized()
{
}

bool USQLUIBaseWidget::NativeApplySQLUIWidgetProperty(
	const FString& PropertyName,
	const FString& PropertyValue,
	FString& OutFailureMessage,
	bool& bOutUnsupportedProperty)
{
	if (PropertyName == TEXT("IsEnabled"))
	{
		bool bParsedValue = false;
		if (!TryParseSQLUIWidgetBool(PropertyValue, bParsedValue))
		{
			OutFailureMessage = FString::Printf(
				TEXT("SQLUI widget property '%s' expected a boolean value but received '%s'."),
				*PropertyName,
				*PropertyValue);
			return false;
		}

		SetIsEnabled(bParsedValue);
		return true;
	}

	bOutUnsupportedProperty = true;
	OutFailureMessage = FString::Printf(
		TEXT("SQLUI widget property '%s' is not supported by widget '%s'."),
		*PropertyName,
		*GetClass()->GetName());
	return false;
}
