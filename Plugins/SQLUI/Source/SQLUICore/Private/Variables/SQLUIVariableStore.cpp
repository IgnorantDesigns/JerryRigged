#include "Variables/SQLUIVariableStore.h"

void USQLUIVariableStore::SetVariable(const FSQLUIVariableKey& Key, const FSQLUIVariableValue& Value)
{
	if (Key.Name.IsEmpty())
	{
		return;
	}

	ValuesByName.Add(Key.Name, Value);
	OnVariableChanged.Broadcast(Key, Value);
}

bool USQLUIVariableStore::GetVariable(const FSQLUIVariableKey& Key, FSQLUIVariableValue& OutValue) const
{
	const FSQLUIVariableValue* Value = ValuesByName.Find(Key.Name);
	if (!Value)
	{
		OutValue = FSQLUIVariableValue();
		return false;
	}

	OutValue = *Value;
	return true;
}

bool USQLUIVariableStore::ContainsVariable(const FSQLUIVariableKey& Key) const
{
	return !Key.Name.IsEmpty() && ValuesByName.Contains(Key.Name);
}

bool USQLUIVariableStore::ClearVariable(const FSQLUIVariableKey& Key)
{
	if (Key.Name.IsEmpty())
	{
		return false;
	}

	const int32 RemovedCount = ValuesByName.Remove(Key.Name);
	if (RemovedCount <= 0)
	{
		return false;
	}

	OnVariableCleared.Broadcast(Key);
	return true;
}

void USQLUIVariableStore::ClearAllVariables()
{
	if (ValuesByName.IsEmpty())
	{
		return;
	}

	ValuesByName.Empty();
	OnAllVariablesCleared.Broadcast();
}

