#include "Widgets/SQLUIFilterBox.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"

namespace
{
bool AreSQLUIFilterBoxTextsEqual(const FText& Left, const FText& Right)
{
	return Left.ToString() == Right.ToString();
}

void ApplySQLUIFilterBoxVisualDefaults(UEditableTextBox& FilterTextBox)
{
	const FLinearColor TextColor(0.95f, 0.97f, 1.0f, 1.0f);
	const FLinearColor DisabledTextColor(0.72f, 0.76f, 0.82f, 1.0f);
	const FLinearColor BackgroundColor(0.035f, 0.045f, 0.06f, 0.98f);
	const FLinearColor HoveredBackgroundColor(0.055f, 0.07f, 0.095f, 1.0f);
	const FLinearColor FocusedBackgroundColor(0.07f, 0.085f, 0.115f, 1.0f);

	FEditableTextBoxStyle TextBoxStyle = FilterTextBox.GetWidgetStyle();
	TextBoxStyle
		.SetBackgroundImageNormal(FSlateColorBrush(BackgroundColor))
		.SetBackgroundImageHovered(FSlateColorBrush(HoveredBackgroundColor))
		.SetBackgroundImageFocused(FSlateColorBrush(FocusedBackgroundColor))
		.SetBackgroundImageReadOnly(FSlateColorBrush(BackgroundColor))
		.SetBackgroundColor(FSlateColor(FLinearColor::White))
		.SetForegroundColor(FSlateColor(TextColor))
		.SetFocusedForegroundColor(FSlateColor(TextColor))
		.SetReadOnlyForegroundColor(FSlateColor(DisabledTextColor))
		.SetPadding(FMargin(10.0f, 6.0f));

	FilterTextBox.SetWidgetStyle(TextBoxStyle);
	FilterTextBox.SetForegroundColor(TextColor);
	FilterTextBox.SetMinDesiredWidth(280.0f);
}

void ApplySQLUIFilterBoxBorderVisualDefaults(UBorder& Border)
{
	Border.SetBrush(FSlateColorBrush(FLinearColor(0.015f, 0.02f, 0.03f, 0.92f)));
	Border.SetBrushColor(FLinearColor::White);
	Border.SetContentColorAndOpacity(FLinearColor::White);
	Border.SetPadding(FMargin(8.0f));
}
}

void USQLUIFilterBox::SetFilterText(const FText& InFilterText)
{
	if (AreSQLUIFilterBoxTextsEqual(FilterText, InFilterText))
	{
		return;
	}

	FilterText = InFilterText;
	SyncFilterTextBoxText();
	NativeOnFilterTextChanged(FilterText);
	OnFilterTextChanged.Broadcast(FilterText);
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
		UBorder* FilterBorder = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("SQLUIFilterBorder"));
		FilterTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(
			UEditableTextBox::StaticClass(),
			TEXT("SQLUIFilterTextBox"));

		if (IsValid(FilterBorder) && IsValid(FilterTextBox.Get()))
		{
			ApplySQLUIFilterBoxBorderVisualDefaults(*FilterBorder);
			FilterBorder->SetContent(FilterTextBox.Get());
			WidgetTree->RootWidget = FilterBorder;
		}
	}

	if (!IsValid(FilterTextBox.Get()))
	{
		return;
	}

	ApplySQLUIFilterBoxVisualDefaults(*FilterTextBox);

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
