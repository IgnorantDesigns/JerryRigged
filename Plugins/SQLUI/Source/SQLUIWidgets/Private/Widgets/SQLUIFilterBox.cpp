#include "Widgets/SQLUIFilterBox.h"

#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"

void USQLUIFilterBox::SetFilterText(const FText& InFilterText)
{
	FilterText = InFilterText;
	SyncFilterTextBoxText();
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

void USQLUIFilterBox::SetPlaceholderText(const FText& InPlaceholderText)
{
	PlaceholderText = InPlaceholderText;
	SyncFilterTextBoxPlaceholderText();
}

FText USQLUIFilterBox::GetPlaceholderText() const
{
	return PlaceholderText;
}

void USQLUIFilterBox::NativeOnSQLUIWidgetInitialized()
{
	Super::NativeOnSQLUIWidgetInitialized();
	EnsureFilterTextBox();
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

	if (PropertyName == TEXT("PlaceholderText"))
	{
		SetPlaceholderText(FText::FromString(PropertyValue));
		return true;
	}

	return Super::NativeApplySQLUIWidgetProperty(
		PropertyName,
		PropertyValue,
		OutFailureMessage,
		bOutUnsupportedProperty);
}

void USQLUIFilterBox::HandleFilterTextBoxTextChanged(const FText& InText)
{
	if (bSyncingFilterTextToTextBox)
	{
		return;
	}

	SetFilterText(InText);
}

void USQLUIFilterBox::EnsureFilterTextBox()
{
	if (IsValid(FilterTextBox.Get()) || !WidgetTree)
	{
		return;
	}

	if (UEditableTextBox* ExistingRootTextBox = Cast<UEditableTextBox>(WidgetTree->RootWidget))
	{
		FilterTextBox = ExistingRootTextBox;
	}
	else if (!WidgetTree->RootWidget)
	{
		FilterTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(
			UEditableTextBox::StaticClass(),
			TEXT("SQLUIFilterTextBox"));
		WidgetTree->RootWidget = FilterTextBox.Get();
	}

	if (!IsValid(FilterTextBox.Get()))
	{
		return;
	}

	FilterTextBox->OnTextChanged.RemoveDynamic(this, &USQLUIFilterBox::HandleFilterTextBoxTextChanged);
	FilterTextBox->OnTextChanged.AddDynamic(this, &USQLUIFilterBox::HandleFilterTextBoxTextChanged);

	SyncFilterTextBoxText();
	SyncFilterTextBoxPlaceholderText();
}

void USQLUIFilterBox::SyncFilterTextBoxText()
{
	if (!IsValid(FilterTextBox.Get()))
	{
		return;
	}

	if (FilterTextBox->GetText().ToString() == FilterText.ToString())
	{
		return;
	}

	bSyncingFilterTextToTextBox = true;
	FilterTextBox->SetText(FilterText);
	bSyncingFilterTextToTextBox = false;
}

void USQLUIFilterBox::SyncFilterTextBoxPlaceholderText()
{
	if (IsValid(FilterTextBox.Get()))
	{
		FilterTextBox->SetHintText(PlaceholderText);
	}
}
