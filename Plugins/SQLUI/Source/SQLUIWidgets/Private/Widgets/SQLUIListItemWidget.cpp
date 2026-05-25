#include "Widgets/SQLUIListItemWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"

namespace
{
FText ResolveSQLUIListItemDisplayText(const FSQLUIListItemData& ListItemData)
{
	if (ListItemData.DisplayText.ToString().TrimStartAndEnd().IsEmpty())
	{
		return NSLOCTEXT("SQLUIWidgets", "SQLUIListItemUntitled", "Untitled item");
	}

	return ListItemData.DisplayText;
}

void ApplySQLUIListItemBorderDefaults(UBorder& Border)
{
	Border.SetBrush(FSlateColorBrush(FLinearColor(0.045f, 0.055f, 0.075f, 0.98f)));
	Border.SetBrushColor(FLinearColor::White);
	Border.SetContentColorAndOpacity(FLinearColor::White);
	Border.SetPadding(FMargin(10.0f, 8.0f));
}

void ApplySQLUIListItemDisplayTextDefaults(UTextBlock& TextBlock)
{
	FSlateFontInfo FontInfo = TextBlock.GetFont();
	FontInfo.Size = 15;

	TextBlock.SetFont(FontInfo);
	TextBlock.SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.96f, 1.0f, 1.0f)));
	TextBlock.SetAutoWrapText(true);
}

void ApplySQLUIListItemIdTextDefaults(UTextBlock& TextBlock)
{
	FSlateFontInfo FontInfo = TextBlock.GetFont();
	FontInfo.Size = 11;

	TextBlock.SetFont(FontInfo);
	TextBlock.SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.68f, 0.78f, 1.0f)));
	TextBlock.SetAutoWrapText(true);
}
}

void USQLUIListItemWidget::SetListItemData(const FSQLUIListItemData& InListItemData)
{
	ListItemData = InListItemData;
	NativeOnListItemDataChanged();
}

FSQLUIListItemData USQLUIListItemWidget::GetListItemData() const
{
	return ListItemData;
}

void USQLUIListItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureVisualRoot();
	SyncVisualState();
}

void USQLUIListItemWidget::NativeOnSQLUIWidgetInitialized()
{
	Super::NativeOnSQLUIWidgetInitialized();
	EnsureVisualRoot();
	SyncVisualState();
}

FReply USQLUIListItemWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		NativeOnListItemClicked(ListItemData);
		OnListItemClicked.Broadcast(ListItemData);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void USQLUIListItemWidget::NativeOnListItemClicked(const FSQLUIListItemData& InListItemData)
{
}

void USQLUIListItemWidget::NativeOnListItemDataChanged()
{
	EnsureVisualRoot();
	SyncVisualState();
}

bool USQLUIListItemWidget::NativeApplySQLUIWidgetProperty(
	const FString& PropertyName,
	const FString& PropertyValue,
	FString& OutFailureMessage,
	bool& bOutUnsupportedProperty)
{
	if (PropertyName == TEXT("ItemId"))
	{
		FSQLUIListItemData UpdatedListItemData = GetListItemData();
		UpdatedListItemData.ItemId = PropertyValue;
		SetListItemData(UpdatedListItemData);
		return true;
	}

	if (PropertyName == TEXT("DisplayText"))
	{
		FSQLUIListItemData UpdatedListItemData = GetListItemData();
		UpdatedListItemData.DisplayText = FText::FromString(PropertyValue);
		SetListItemData(UpdatedListItemData);
		return true;
	}

	return Super::NativeApplySQLUIWidgetProperty(
		PropertyName,
		PropertyValue,
		OutFailureMessage,
		bOutUnsupportedProperty);
}

void USQLUIListItemWidget::EnsureVisualRoot()
{
	if (IsValid(DisplayTextBlock.Get()) || !WidgetTree)
	{
		return;
	}

	if (UTextBlock* ExistingRootTextBlock = Cast<UTextBlock>(WidgetTree->RootWidget))
	{
		DisplayTextBlock = ExistingRootTextBlock;
		ApplySQLUIListItemDisplayTextDefaults(*DisplayTextBlock);
		return;
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	ItemBorder = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("SQLUIListItemBorder"));
	ItemTextContainer = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("SQLUIListItemTextContainer"));
	DisplayTextBlock = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("SQLUIListItemDisplayText"));
	ItemIdTextBlock = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("SQLUIListItemIdText"));

	if (!IsValid(ItemBorder.Get())
		|| !IsValid(ItemTextContainer.Get())
		|| !IsValid(DisplayTextBlock.Get())
		|| !IsValid(ItemIdTextBlock.Get()))
	{
		return;
	}

	ApplySQLUIListItemBorderDefaults(*ItemBorder);
	ApplySQLUIListItemDisplayTextDefaults(*DisplayTextBlock);
	ApplySQLUIListItemIdTextDefaults(*ItemIdTextBlock);

	if (UVerticalBoxSlot* DisplayTextSlot = ItemTextContainer->AddChildToVerticalBox(DisplayTextBlock.Get()))
	{
		DisplayTextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	}
	ItemTextContainer->AddChildToVerticalBox(ItemIdTextBlock.Get());

	ItemBorder->SetContent(ItemTextContainer.Get());
	WidgetTree->RootWidget = ItemBorder.Get();
}

void USQLUIListItemWidget::SyncVisualState()
{
	if (IsValid(DisplayTextBlock.Get()))
	{
		DisplayTextBlock->SetText(ResolveSQLUIListItemDisplayText(ListItemData));
	}

	if (IsValid(ItemIdTextBlock.Get()))
	{
		ItemIdTextBlock->SetText(FText::FromString(ListItemData.ItemId));
		ItemIdTextBlock->SetVisibility(
			ListItemData.ItemId.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
	}
}
