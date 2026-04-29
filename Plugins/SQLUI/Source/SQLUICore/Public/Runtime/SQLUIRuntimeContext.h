#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "SQLUIRuntimeContext.generated.h"

class USQLUIActionRegistry;
class USQLUILayoutFactory;
class USQLUIVariableStore;
class USQLUIWidgetCatalog;

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIRuntimeContextSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime")
	TObjectPtr<USQLUIVariableStore> VariableStore = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime")
	TObjectPtr<USQLUIActionRegistry> ActionRegistry = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime")
	TObjectPtr<USQLUIWidgetCatalog> WidgetCatalog = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Runtime")
	TObjectPtr<USQLUILayoutFactory> LayoutFactory = nullptr;
};

UCLASS(BlueprintType)
class SQLUICORE_API USQLUIRuntimeContext : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FSQLUIRuntimeContextSettings& InSettings);

	bool IsInitialized() const;

	USQLUIVariableStore* GetVariableStore() const;
	USQLUIActionRegistry* GetActionRegistry() const;
	USQLUIWidgetCatalog* GetWidgetCatalog() const;
	USQLUILayoutFactory* GetLayoutFactory() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<USQLUIVariableStore> VariableStore = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USQLUIActionRegistry> ActionRegistry = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USQLUIWidgetCatalog> WidgetCatalog = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USQLUILayoutFactory> LayoutFactory = nullptr;

	bool bInitialized = false;
};
