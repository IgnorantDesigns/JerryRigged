#pragma once

#include "CoreMinimal.h"
#include "Variables/SQLUIVariableTypes.h"

#include "SQLUIActionTypes.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIActionKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Actions")
	FString Name;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIActionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Actions")
	FSQLUIActionKey ActionKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Actions")
	TMap<FString, FSQLUIVariableValue> Parameters;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIActionResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Actions")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Actions")
	bool bActionUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Actions")
	bool bNotImplemented = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Actions")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Actions")
	TMap<FString, FSQLUIVariableValue> OutputValues;
};

DECLARE_DELEGATE_RetVal_OneParam(FSQLUIActionResult, FSQLUIActionHandler, const FSQLUIActionRequest&);

