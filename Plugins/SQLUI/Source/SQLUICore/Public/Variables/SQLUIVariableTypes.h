#pragma once

#include "CoreMinimal.h"

#include "SQLUIVariableTypes.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIVariableKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Variables")
	FString Name;
};

UENUM(BlueprintType)
enum class ESQLUIVariableValueType : uint8
{
	None,
	String,
	Number,
	Boolean
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIVariableValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Variables")
	ESQLUIVariableValueType Type = ESQLUIVariableValueType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Variables")
	FString StringValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Variables")
	double NumberValue = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Variables")
	bool bBooleanValue = false;
};

