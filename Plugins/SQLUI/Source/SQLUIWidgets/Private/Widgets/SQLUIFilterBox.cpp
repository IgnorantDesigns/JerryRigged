#include "Widgets/SQLUIFilterBox.h"

void USQLUIFilterBox::SetFilterText(const FText& InFilterText)
{
	FilterText = InFilterText;
	NativeOnFilterTextChanged(FilterText);
}

FText USQLUIFilterBox::GetFilterText() const
{
	return FilterText;
}

void USQLUIFilterBox::ClearFilterText()
{
	SetFilterText(FText::GetEmpty());
}

void USQLUIFilterBox::NativeOnFilterTextChanged(const FText& InFilterText)
{
}

bool USQLUIFilterBox::NativeApplySQLUIWidgetProperty(
	const FString& PropertyName,
	const FString& PropertyValue,
	FString& OutFailureMessage,
	bool& bOutUnsupportedProperty)
{
	if (PropertyName == TEXT("FilterText"))
	{
		SetFilterText(FText::FromString(PropertyValue));
		return true;
	}

	return Super::NativeApplySQLUIWidgetProperty(
		PropertyName,
		PropertyValue,
		OutFailureMessage,
		bOutUnsupportedProperty);
}
