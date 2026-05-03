#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"

#include "SQLUISampleSmokeTestCommandlet.generated.h"

UCLASS()
class SQLUISAMPLES_API USQLUISampleSmokeTestCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USQLUISampleSmokeTestCommandlet();

	virtual int32 Main(const FString& Params) override;
};
