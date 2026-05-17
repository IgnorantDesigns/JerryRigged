#include "Widgets/SQLUIListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Widgets/SQLUIListItemWidget.h"

namespace
{
FText ResolveSQLUIListEmptyText(const FText& EmptyText)
{
	if (EmptyText.ToString().TrimStartAndEnd().IsEmpty())
	{
		return NSLOCTEXT("SQLUIWidgets", "SQLUIListEmptyState", "No items");
	}

	return EmptyText;
}

bool DoesSQLUIListItemMatchFilter(
	const FSQLUIListItemData& ItemData,
	const FText& FilterText)
{
	const FString TrimmedFilterText = FilterText.ToString().TrimStartAndEnd();
	if (TrimmedFilterText.IsEmpty())
	{
		return true;
	}

	return ItemData.DisplayText.ToString().Contains(TrimmedFilterText, ESearchCase::IgnoreCase)
		|| ItemData.ItemId.Contains(TrimmedFilterText, ESearchCase::IgnoreCase);
}

bool TryParseSQLUIListItemsProperty(
	const FString& PropertyValue,
	TArray<FSQLUIListItemData>& OutItems,
	FString& OutFailureMessage)
{
	OutItems.Reset();
	OutFailureMessage.Reset();

	if (PropertyValue.TrimStartAndEnd().IsEmpty())
	{
		return true;
	}

	TArray<FString> RowValues;
	PropertyValue.ParseIntoArray(RowValues, TEXT(";"), false);

	for (int32 RowIndex = 0; RowIndex < RowValues.Num(); ++RowIndex)
	{
		const FString RowValue = RowValues[RowIndex].TrimStartAndEnd();
		if (RowValue.IsEmpty())
		{
			OutFailureMessage = FString::Printf(
				TEXT("SQLUI list widget property 'Items' contains an empty row at index %d."),
				RowIndex);
			return false;
		}

		FString ItemId;
		FString DisplayText;
		if (!RowValue.Split(TEXT("|"), &ItemId, &DisplayText, ESearchCase::CaseSensitive, ESearchDir::FromStart))
		{
			OutFailureMessage = FString::Printf(
				TEXT("SQLUI list widget property 'Items' row %d must use the format 'ItemId|Display text'."),
				RowIndex);
			return false;
		}

		ItemId = ItemId.TrimStartAndEnd();
		DisplayText = DisplayText.TrimStartAndEnd();
		if (DisplayText.IsEmpty())
		{
			OutFailureMessage = FString::Printf(
				TEXT("SQLUI list widget property 'Items' row %d must include non-empty display text."),
				RowIndex);
			return false;
		}

		FSQLUIListItemData ItemData;
		ItemData.ItemId = ItemId;
		ItemData.DisplayText = FText::FromString(DisplayText);
		OutItems.Add(ItemData);
	}

	return true;
}

void ApplySQLUIListBorderDefaults(UBorder& Border)
{
	Border.SetBrush(FSlateColorBrush(FLinearColor(0.018f, 0.024f, 0.034f, 0.96f)));
	Border.SetBrushColor(FLinearColor::White);
	Border.SetContentColorAndOpacity(FLinearColor::White);
	Border.SetPadding(FMargin(8.0f));
}

void ApplySQLUIListEmptyTextDefaults(UTextBlock& TextBlock)
{
	FSlateFontInfo FontInfo = TextBlock.GetFont();
	FontInfo.Size = 14;

	TextBlock.SetFont(FontInfo);
	TextBlock.SetColorAndOpacity(FSlateColor(FLinearColor(0.66f, 0.72f, 0.82f, 1.0f)));
	TextBlock.SetAutoWrapText(true);
}
}

void USQLUIListWidget::SetItems(const TArray<FSQLUIListItemData>& InItems)
{
	Items = InItems;
	NativeOnItemsChanged();
}

TArray<FSQLUIListItemData> USQLUIListWidget::GetItems() const
{
	return Items;
}

void USQLUIListWidget::ClearItems()
{
	Items.Reset();
	NativeOnItemsChanged();
}

void USQLUIListWidget::SetFilterText(const FText& InFilterText)
{
	if (FilterText.ToString() == InFilterText.ToString())
	{
		return;
	}

	FilterText = InFilterText;
	RebuildListItems();
}

FText USQLUIListWidget::GetFilterText() const
{
	return FilterText;
}

void USQLUIListWidget::ClearFilterText()
{
	SetFilterText(FText::GetEmpty());
}

void USQLUIListWidget::SetEmptyText(const FText& InEmptyText)
{
	EmptyText = InEmptyText;
	RebuildListItems();
}

FText USQLUIListWidget::GetEmptyText() const
{
	return ResolveSQLUIListEmptyText(EmptyText);
}

void USQLUIListWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureVisualRoot();
	RebuildListItems();
}

void USQLUIListWidget::NativeOnSQLUIWidgetInitialized()
{
	Super::NativeOnSQLUIWidgetInitialized();
	EnsureVisualRoot();
	RebuildListItems();
}

void USQLUIListWidget::NativeOnItemsChanged()
{
	RebuildListItems();
}

bool USQLUIListWidget::NativeApplySQLUIWidgetProperty(
	const FString& PropertyName,
	const FString& PropertyValue,
	FString& OutFailureMessage,
	bool& bOutUnsupportedProperty)
{
	if (PropertyName == TEXT("Items"))
	{
		TArray<FSQLUIListItemData> ParsedItems;
		if (!TryParseSQLUIListItemsProperty(PropertyValue, ParsedItems, OutFailureMessage))
		{
			return false;
		}

		if (ParsedItems.IsEmpty())
		{
			ClearItems();
		}
		else
		{
			SetItems(ParsedItems);
		}

		return true;
	}

	if (PropertyName == TEXT("EmptyText"))
	{
		SetEmptyText(FText::FromString(PropertyValue));
		return true;
	}

	return Super::NativeApplySQLUIWidgetProperty(
		PropertyName,
		PropertyValue,
		OutFailureMessage,
		bOutUnsupportedProperty);
}

void USQLUIListWidget::EnsureVisualRoot()
{
	if (IsValid(ItemContainer.Get()) || !WidgetTree)
	{
		return;
	}

	if (UVerticalBox* ExistingRootVerticalBox = Cast<UVerticalBox>(WidgetTree->RootWidget))
	{
		ItemContainer = ExistingRootVerticalBox;
		return;
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	ListBorder = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("SQLUIListBorder"));
	ListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(),
		TEXT("SQLUIListScrollBox"));
	ItemContainer = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("SQLUIListItemContainer"));

	if (!IsValid(ListBorder.Get()) || !IsValid(ListScrollBox.Get()) || !IsValid(ItemContainer.Get()))
	{
		return;
	}

	ApplySQLUIListBorderDefaults(*ListBorder);
	ListScrollBox->AddChild(ItemContainer.Get());
	ListBorder->SetContent(ListScrollBox.Get());
	WidgetTree->RootWidget = ListBorder.Get();
}

void USQLUIListWidget::RebuildListItems()
{
	EnsureVisualRoot();

	if (!IsValid(ItemContainer.Get()))
	{
		return;
	}

	ItemContainer->ClearChildren();
	ItemWidgets.Reset();
	EmptyTextBlock = nullptr;

	if (Items.IsEmpty())
	{
		AddEmptyStateRow();
		return;
	}

	ItemWidgets.Reserve(Items.Num());
	int32 DisplayedItemCount = 0;
	for (const FSQLUIListItemData& ItemData : Items)
	{
		if (!ShouldDisplayItem(ItemData))
		{
			continue;
		}

		AddListItemRow(ItemData);
		++DisplayedItemCount;
	}

	if (DisplayedItemCount == 0)
	{
		AddEmptyStateRow();
	}
}

bool USQLUIListWidget::ShouldDisplayItem(const FSQLUIListItemData& ItemData) const
{
	return DoesSQLUIListItemMatchFilter(ItemData, FilterText);
}

void USQLUIListWidget::AddEmptyStateRow()
{
	if (!IsValid(ItemContainer.Get()) || !WidgetTree)
	{
		return;
	}

	EmptyTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (!IsValid(EmptyTextBlock.Get()))
	{
		return;
	}

	ApplySQLUIListEmptyTextDefaults(*EmptyTextBlock);
	EmptyTextBlock->SetText(GetEmptyText());

	if (UVerticalBoxSlot* EmptyTextSlot = ItemContainer->AddChildToVerticalBox(EmptyTextBlock.Get()))
	{
		EmptyTextSlot->SetPadding(FMargin(2.0f, 4.0f));
	}
}

void USQLUIListWidget::AddListItemRow(const FSQLUIListItemData& ItemData)
{
	if (!IsValid(ItemContainer.Get()) || !WidgetTree)
	{
		return;
	}

	USQLUIListItemWidget* ItemWidget = WidgetTree->ConstructWidget<USQLUIListItemWidget>(
		USQLUIListItemWidget::StaticClass());
	if (!IsValid(ItemWidget))
	{
		return;
	}

	ItemWidget->SetListItemData(ItemData);
	ItemWidgets.Add(ItemWidget);

	if (UVerticalBoxSlot* ItemSlot = ItemContainer->AddChildToVerticalBox(ItemWidget))
	{
		ItemSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}
}
