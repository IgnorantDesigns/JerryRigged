#include "SQLUISampleVisualListDemoActor.h"

#include "SQLUISamplesModule.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SQLUIListTypes.h"
#include "Widgets/SQLUIListWidget.h"

namespace
{
FSQLUIListItemData MakeSQLUISampleVisualListItem(
	const FString& ItemId,
	const FText& DisplayText)
{
	FSQLUIListItemData ItemData;
	ItemData.ItemId = ItemId;
	ItemData.DisplayText = DisplayText;
	return ItemData;
}

TArray<FSQLUIListItemData> MakeSQLUISampleVisualListItems()
{
	TArray<FSQLUIListItemData> Items;
	Items.Reserve(3);
	Items.Add(MakeSQLUISampleVisualListItem(
		TEXT("SQLUI.Sample.List.Row.001"),
		NSLOCTEXT("SQLUISamples", "VisualListDemoFirstRow", "Sample row: Alpha assembly")));
	Items.Add(MakeSQLUISampleVisualListItem(
		TEXT("SQLUI.Sample.List.Row.002"),
		NSLOCTEXT("SQLUISamples", "VisualListDemoSecondRow", "Sample row: Bravo wiring")));
	Items.Add(MakeSQLUISampleVisualListItem(
		TEXT("SQLUI.Sample.List.Row.003"),
		NSLOCTEXT("SQLUISamples", "VisualListDemoThirdRow", "Sample row: Charlie verification")));
	return Items;
}
}

ASQLUISampleVisualListDemoActor::ASQLUISampleVisualListDemoActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASQLUISampleVisualListDemoActor::BeginPlay()
{
	Super::BeginPlay();

	if (bRunOnBeginPlay)
	{
		RunVisualListDemo();
	}
}

void ASQLUISampleVisualListDemoActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(AddedListWidget.Get()))
	{
		AddedListWidget->RemoveFromParent();
		AddedListWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ASQLUISampleVisualListDemoActor::RunVisualListDemo()
{
	if (IsValid(AddedListWidget.Get()))
	{
		AddedListWidget->RemoveFromParent();
		AddedListWidget = nullptr;
	}

	UWorld* World = GetWorld();
	APlayerController* OwningPlayer = World ? World->GetFirstPlayerController() : nullptr;

	USQLUIListWidget* ListWidget = nullptr;
	if (IsValid(OwningPlayer))
	{
		ListWidget = CreateWidget<USQLUIListWidget>(
			OwningPlayer,
			USQLUIListWidget::StaticClass());
	}
	else if (World)
	{
		ListWidget = CreateWidget<USQLUIListWidget>(
			World,
			USQLUIListWidget::StaticClass());
	}

	if (!IsValid(ListWidget))
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample visual list demo failed: could not create list widget."));
		return;
	}

	const TArray<FSQLUIListItemData> Items = MakeSQLUISampleVisualListItems();
	ListWidget->SetItems(Items);
	AddedListWidget = ListWidget;

	bool bAddedToViewport = false;
	if (bAddToViewport)
	{
		ListWidget->AddToViewport(ViewportZOrder);
		ListWidget->SetPositionInViewport(ViewportPosition, true);
		if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
		{
			ListWidget->SetDesiredSizeInViewport(ViewportSize);
		}

		bAddedToViewport = true;
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample visual list demo succeeded. Row count: %d"),
		Items.Num());

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample visual list demo viewport add status: %s"),
		bAddToViewport
			? (bAddedToViewport ? TEXT("added") : TEXT("failed"))
			: TEXT("skipped"));
}
