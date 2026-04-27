#pragma once

#include "Actions/SQLUIActionTypes.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "SQLUIActionRegistry.generated.h"

UCLASS(BlueprintType)
class SQLUICORE_API USQLUIActionRegistry : public UObject
{
	GENERATED_BODY()

public:
	bool RegisterAction(const FSQLUIActionKey& Key, FSQLUIActionHandler Handler);
	bool UnregisterAction(const FSQLUIActionKey& Key);
	bool IsActionRegistered(const FSQLUIActionKey& Key) const;
	FSQLUIActionResult ExecuteAction(const FSQLUIActionRequest& Request) const;
	void ClearActions();

private:
	TMap<FString, FSQLUIActionHandler> HandlersByName;
};

