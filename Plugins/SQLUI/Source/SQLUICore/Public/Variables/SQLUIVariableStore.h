#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Variables/SQLUIVariableTypes.h"

#include "SQLUIVariableStore.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FSQLUIVariableChangedDelegate, const FSQLUIVariableKey&, const FSQLUIVariableValue&);
DECLARE_MULTICAST_DELEGATE_OneParam(FSQLUIVariableClearedDelegate, const FSQLUIVariableKey&);
DECLARE_MULTICAST_DELEGATE(FSQLUIAllVariablesClearedDelegate);

UCLASS(BlueprintType)
class SQLUICORE_API USQLUIVariableStore : public UObject
{
	GENERATED_BODY()

public:
	FSQLUIVariableChangedDelegate OnVariableChanged;
	FSQLUIVariableClearedDelegate OnVariableCleared;
	FSQLUIAllVariablesClearedDelegate OnAllVariablesCleared;

	void SetVariable(const FSQLUIVariableKey& Key, const FSQLUIVariableValue& Value);
	bool GetVariable(const FSQLUIVariableKey& Key, FSQLUIVariableValue& OutValue) const;
	bool ContainsVariable(const FSQLUIVariableKey& Key) const;
	bool ClearVariable(const FSQLUIVariableKey& Key);
	void ClearAllVariables();

private:
	UPROPERTY(Transient)
	TMap<FString, FSQLUIVariableValue> ValuesByName;
};

