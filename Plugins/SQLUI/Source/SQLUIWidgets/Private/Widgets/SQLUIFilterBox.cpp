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
