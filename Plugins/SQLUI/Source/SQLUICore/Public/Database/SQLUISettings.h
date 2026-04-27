#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "SQLUISettings.generated.h"

UCLASS(Config = Game, DefaultConfig)
class SQLUICORE_API USQLUISettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "SQLUI|Database")
	bool bEnableSQLite = false;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "SQLUI|Database")
	bool bUseStubBackendWhenSQLiteUnavailable = true;
};
