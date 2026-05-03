#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SQLUISampleSmokeTestRunner.h"

#include "SQLUISamplePipelineSmokeTestActor.generated.h"

UCLASS(BlueprintType)
class SQLUISAMPLES_API ASQLUISamplePipelineSmokeTestActor : public AActor
{
	GENERATED_BODY()

public:
	ASQLUISamplePipelineSmokeTestActor();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples")
	void RunSmokeTest();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRunSmokeTestOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISampleSmokeTestRequest SmokeTestRequest;
};
