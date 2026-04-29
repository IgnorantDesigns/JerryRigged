#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Variables/SQLUIVariableTypes.h"

#include "SQLUIBindingTypes.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIBindingResolveRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Bindings")
	FSQLUILayoutBinding Binding;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIBindingResolveResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Bindings")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Bindings")
	bool bBindingUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Bindings")
	bool bNotImplemented = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Bindings")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Bindings")
	FSQLUIVariableValue ResolvedValue;
};
