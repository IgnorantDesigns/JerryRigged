#include "Widgets/SQLUIVerticalBoxWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
void ApplySQLUIVerticalBoxSlotDefaults(UVerticalBoxSlot& Slot)
{
	Slot.SetHorizontalAlignment(HAlign_Fill);
	Slot.SetVerticalAlignment(VAlign_Top);
	Slot.SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
}
}

bool USQLUIVerticalBoxWidget::CanAcceptSQLUIChildWidget(
	const USQLUIBaseWidget* ChildWidget,
	const FSQLUILayoutNode& ChildLayoutNode) const
{
	return IsValid(ChildWidget) && ChildWidget != this;
}

bool USQLUIVerticalBoxWidget::AddSQLUIChildWidget(USQLUIBaseWidget* ChildWidget)
{
	if (!CanAcceptSQLUIChildWidget(
		ChildWidget,
		IsValid(ChildWidget) ? ChildWidget->GetSQLUILayoutNode() : FSQLUILayoutNode()))
	{
		return false;
	}

	EnsureVerticalBoxRoot();
	if (!IsValid(ChildContainer.Get()))
	{
		return false;
	}

	if (ChildWidget->GetParent() == ChildContainer.Get())
	{
		ChildWidgets.AddUnique(ChildWidget);
		return true;
	}

	if (ChildWidget->GetParent())
	{
		return false;
	}

	UVerticalBoxSlot* ChildSlot = ChildContainer->AddChildToVerticalBox(ChildWidget);
	if (!ChildSlot)
	{
		return false;
	}

	ApplySQLUIVerticalBoxSlotDefaults(*ChildSlot);
	ChildWidgets.AddUnique(ChildWidget);
	return true;
}

void USQLUIVerticalBoxWidget::ClearSQLUIChildWidgets()
{
	EnsureVerticalBoxRoot();

	if (IsValid(ChildContainer.Get()))
	{
		ChildContainer->ClearChildren();
	}

	ChildWidgets.Reset();
}

void USQLUIVerticalBoxWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureVerticalBoxRoot();
}

void USQLUIVerticalBoxWidget::NativeOnSQLUIWidgetInitialized()
{
	Super::NativeOnSQLUIWidgetInitialized();
	EnsureVerticalBoxRoot();
}

void USQLUIVerticalBoxWidget::EnsureVerticalBoxRoot()
{
	if (IsValid(ChildContainer.Get()) || !WidgetTree)
	{
		return;
	}

	if (UVerticalBox* ExistingRootVerticalBox = Cast<UVerticalBox>(WidgetTree->RootWidget))
	{
		ChildContainer = ExistingRootVerticalBox;
		return;
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	ChildContainer = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("SQLUIVerticalBox"));

	if (IsValid(ChildContainer.Get()))
	{
		WidgetTree->RootWidget = ChildContainer.Get();
	}
}
