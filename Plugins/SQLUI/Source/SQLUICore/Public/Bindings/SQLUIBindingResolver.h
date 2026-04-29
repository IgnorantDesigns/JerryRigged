#pragma once

#include "Bindings/SQLUIBindingTypes.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "SQLUIBindingResolver.generated.h"

class USQLUIRuntimeContext;

UCLASS(BlueprintType)
class SQLUICORE_API USQLUIBindingResolver : public UObject
{
	GENERATED_BODY()

public:
	FSQLUIBindingResolveResult ResolveBinding(
		const FSQLUIBindingResolveRequest& Request,
		const USQLUIRuntimeContext* RuntimeContext) const;

	FSQLUIBindingResolveResult ResolveLayoutBinding(
		const FSQLUILayoutBinding& Binding,
		const USQLUIRuntimeContext* RuntimeContext) const;
};
