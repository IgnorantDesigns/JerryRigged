#pragma once

#include "CoreMinimal.h"
#include "Widgets/SQLUIBaseWidget.h"

#include "SQLUIVerticalBoxWidget.generated.h"

class UVerticalBox;

UCLASS(BlueprintType, Blueprintable)
class SQLUIWIDGETS_API USQLUIVerticalBoxWidget : public USQLUIBaseWidget
{
	GENERATED_BODY()

public:
	virtual bool CanAcceptSQLUIChildWidget(
		const USQLUIBaseWidget* ChildWidget,
		const FSQLUILayoutNode& ChildLayoutNode) const override;

	virtual bool AddSQLUIChildWidget(USQLUIBaseWidget* ChildWidget) override;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Vertical Box")
	void ClearSQLUIChildWidgets();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnSQLUIWidgetInitialized() override;

private:
	void EnsureVerticalBoxRoot();

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ChildContainer = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USQLUIBaseWidget>> ChildWidgets;
};
