#pragma once

#include "CoreMinimal.h"
#include "Widgets/SQLUIBaseWidget.h"

#include "SQLUIFilterBox.generated.h"

class UEditableTextBox;

DECLARE_MULTICAST_DELEGATE_OneParam(FSQLUIFilterTextChangedDelegate, const FText&);

UCLASS(BlueprintType, Blueprintable)
class SQLUIWIDGETS_API USQLUIFilterBox : public USQLUIBaseWidget
{
	GENERATED_BODY()

public:
	FSQLUIFilterTextChangedDelegate OnFilterTextChanged;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Filter")
	void SetFilterText(const FText& InFilterText);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Filter")
	FText GetFilterText() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Filter")
	void ClearFilterText();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Filter")
	void SetPlaceholderText(const FText& InPlaceholderText);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Filter")
	FText GetPlaceholderText() const;

protected:
	virtual void NativeOnSQLUIWidgetInitialized() override;
	virtual void NativeOnFilterTextChanged(const FText& InFilterText);
	virtual bool NativeApplySQLUIWidgetProperty(
		const FString& PropertyName,
		const FString& PropertyValue,
		FString& OutFailureMessage,
		bool& bOutUnsupportedProperty) override;

private:
	UFUNCTION()
	void HandleFilterTextBoxTextChanged(const FText& InText);

	void EnsureFilterTextBox();
	void SyncFilterTextBoxText();
	void SyncFilterTextBoxPlaceholderText();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SQLUI|Filter", meta = (AllowPrivateAccess = "true"))
	FText FilterText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SQLUI|Filter", meta = (AllowPrivateAccess = "true"))
	FText PlaceholderText;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> FilterTextBox = nullptr;

	bool bSyncingFilterTextToTextBox = false;
};
