#include "SQLUISampleFilterListVisualDemoActor.h"

#include "SQLUISamplesModule.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "WidgetCatalog/SQLUIWidgetCatalogRegistrar.h"
#include "Widgets/SQLUIFilterBox.h"
#include "Widgets/SQLUIListTypes.h"
#include "Widgets/SQLUIListWidget.h"

namespace
{
template<typename WidgetType>
WidgetType* CreateSQLUISampleFilterListWidget(
	UWorld* World,
	APlayerController* OwningPlayer)
{
	if (IsValid(OwningPlayer))
	{
		return CreateWidget<WidgetType>(
			OwningPlayer,
			WidgetType::StaticClass());
	}

	if (World)
	{
		return CreateWidget<WidgetType>(
			World,
			WidgetType::StaticClass());
	}

	return nullptr;
}

FSQLUIWidgetInitializeParams MakeSQLUISampleFilterListInitializeParams(
	const FString& WidgetId,
	const FSQLUIWidgetTypeKey& WidgetTypeKey)
{
	FSQLUIWidgetInitializeParams InitializeParams;
	InitializeParams.LayoutNode.WidgetId = WidgetId;
	InitializeParams.LayoutNode.WidgetTypeKey = WidgetTypeKey.Value;
	return InitializeParams;
}

FSQLUIListItemData MakeSQLUISampleFilterListItem(
	const FString& ItemId,
	const FText& DisplayText)
{
	FSQLUIListItemData ItemData;
	ItemData.ItemId = ItemId;
	ItemData.DisplayText = DisplayText;
	return ItemData;
}

TArray<FSQLUIListItemData> MakeSQLUISampleFilterListItems()
{
	TArray<FSQLUIListItemData> Items;
	Items.Reserve(3);
	Items.Add(MakeSQLUISampleFilterListItem(
		TEXT("SQLUI.Sample.FilterList.Row.001"),
		NSLOCTEXT("SQLUISamples", "FilterListVisualDemoFirstRow", "Sample row: Alpha assembly")));
	Items.Add(MakeSQLUISampleFilterListItem(
		TEXT("SQLUI.Sample.FilterList.Row.002"),
		NSLOCTEXT("SQLUISamples", "FilterListVisualDemoSecondRow", "Sample row: Bravo wiring")));
	Items.Add(MakeSQLUISampleFilterListItem(
		TEXT("SQLUI.Sample.FilterList.Row.003"),
		NSLOCTEXT("SQLUISamples", "FilterListVisualDemoThirdRow", "Sample row: Charlie verification")));
	return Items;
}

void ApplySQLUISampleFilterListViewportPlacement(
	UUserWidget& Widget,
	const FVector2D& ViewportPosition,
	const FVector2D& ViewportSize)
{
	Widget.SetPositionInViewport(ViewportPosition, true);
	if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
	{
		Widget.SetDesiredSizeInViewport(ViewportSize);
	}
}
}

ASQLUISampleFilterListVisualDemoActor::ASQLUISampleFilterListVisualDemoActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASQLUISampleFilterListVisualDemoActor::BeginPlay()
{
	Super::BeginPlay();

	if (bRunOnBeginPlay)
	{
		RunVisualFilterListDemo();
	}
}

void ASQLUISampleFilterListVisualDemoActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveAddedWidgets();

	Super::EndPlay(EndPlayReason);
}

void ASQLUISampleFilterListVisualDemoActor::RunVisualFilterListDemo()
{
	RemoveAddedWidgets();

	UWorld* World = GetWorld();
	APlayerController* OwningPlayer = World ? World->GetFirstPlayerController() : nullptr;

	USQLUIFilterBox* FilterBox = CreateSQLUISampleFilterListWidget<USQLUIFilterBox>(
		World,
		OwningPlayer);
	if (!IsValid(FilterBox))
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample filter/list visual demo failed: could not create filter box widget."));
		return;
	}

	USQLUIListWidget* ListWidget = CreateSQLUISampleFilterListWidget<USQLUIListWidget>(
		World,
		OwningPlayer);
	if (!IsValid(ListWidget))
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample filter/list visual demo failed: could not create list widget."));
		return;
	}

	FilterBox->InitializeSQLUIWidget(MakeSQLUISampleFilterListInitializeParams(
		TEXT("SQLUI.Sample.FilterList.FilterBox"),
		FSQLUIWidgetTypeKeys::FilterBox()));
	ListWidget->InitializeSQLUIWidget(MakeSQLUISampleFilterListInitializeParams(
		TEXT("SQLUI.Sample.FilterList.ListWidget"),
		FSQLUIWidgetTypeKeys::ListWidget()));

	FilterBox->SetPlaceholderText(NSLOCTEXT(
		"SQLUISamples",
		"FilterListVisualDemoFilterPlaceholder",
		"Filter sample rows"));
	FilterBox->ClearFilterText();

	const TArray<FSQLUIListItemData> Items = MakeSQLUISampleFilterListItems();
	ListWidget->SetItems(Items);

	AddedFilterBoxWidget = FilterBox;
	AddedListWidget = ListWidget;

	bool bAddedFilterBoxToViewport = false;
	bool bAddedListWidgetToViewport = false;
	if (bAddToViewport)
	{
		FilterBox->AddToViewport(ViewportZOrder);
		ApplySQLUISampleFilterListViewportPlacement(
			*FilterBox,
			FilterBoxViewportPosition,
			FilterBoxViewportSize);
		bAddedFilterBoxToViewport = true;

		ListWidget->AddToViewport(ViewportZOrder);
		ApplySQLUISampleFilterListViewportPlacement(
			*ListWidget,
			ListWidgetViewportPosition,
			ListWidgetViewportSize);
		bAddedListWidgetToViewport = true;
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample filter/list visual demo succeeded. Row count: %d"),
		Items.Num());

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample filter/list visual demo viewport add status: %s"),
		bAddToViewport
			? ((bAddedFilterBoxToViewport && bAddedListWidgetToViewport) ? TEXT("added") : TEXT("failed"))
			: TEXT("skipped"));
}

void ASQLUISampleFilterListVisualDemoActor::RemoveAddedWidgets()
{
	if (IsValid(AddedFilterBoxWidget.Get()))
	{
		AddedFilterBoxWidget->RemoveFromParent();
	}
	AddedFilterBoxWidget = nullptr;

	if (IsValid(AddedListWidget.Get()))
	{
		AddedListWidget->RemoveFromParent();
	}
	AddedListWidget = nullptr;
}
