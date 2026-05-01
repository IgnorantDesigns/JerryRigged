#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Layout/SQLUILayoutTypes.h"

#include "SQLUIBaseWidget.generated.h"

class USQLUIRuntimeContext;

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIWidgetInitializeParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget")
	TObjectPtr<USQLUIRuntimeContext> RuntimeContext = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Widget")
	FSQLUILayoutNode LayoutNode;
};

UCLASS(BlueprintType, Blueprintable)
class SQLUIWIDGETS_API USQLUIBaseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Widget")
	void InitializeSQLUIWidget(const FSQLUIWidgetInitializeParams& InInitializeParams);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Widget")
	bool IsSQLUIWidgetInitialized() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Widget")
	USQLUIRuntimeContext* GetSQLUIRuntimeContext() const;

	const FSQLUILayoutNode& GetSQLUILayoutNode() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Widget")
	FString GetSQLUIWidgetId() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Widget")
	FString GetSQLUIWidgetTypeKey() const;

protected:
	virtual void NativeOnSQLUIWidgetInitialized();

private:
	UPROPERTY(Transient)
	FSQLUIWidgetInitializeParams InitializeParams;

	UPROPERTY(Transient)
	bool bSQLUIWidgetInitialized = false;
};
