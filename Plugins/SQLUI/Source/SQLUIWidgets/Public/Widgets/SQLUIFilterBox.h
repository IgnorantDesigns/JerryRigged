#pragma once

#include "CoreMinimal.h"
#include "Widgets/SQLUIBaseWidget.h"

#include "SQLUIFilterBox.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SQLUIWIDGETS_API USQLUIFilterBox : public USQLUIBaseWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Filter")
	void SetFilterText(const FText& InFilterText);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Filter")
	FText GetFilterText() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Filter")
	void ClearFilterText();

protected:
	virtual void NativeOnFilterTextChanged(const FText& InFilterText);

private:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SQLUI|Filter", meta = (AllowPrivateAccess = "true"))
	FText FilterText;
};
