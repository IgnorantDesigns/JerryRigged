#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SQLUISampleSmokeTestRunner.h"

#include "SQLUISampleVisualPipelineDemoActor.generated.h"

class USQLUIBaseWidget;

UCLASS(BlueprintType)
class SQLUISAMPLES_API ASQLUISampleVisualPipelineDemoActor : public AActor
{
	GENERATED_BODY()

public:
	ASQLUISampleVisualPipelineDemoActor();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples")
	void RunVisualPipelineDemo();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRunOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bAddToViewport = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ViewportZOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISampleSmokeTestRequest DemoRequest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "SQLUI|Samples")
	FSQLUISampleSmokeTestResult LastResult;

private:
	UPROPERTY(Transient)
	TObjectPtr<USQLUIBaseWidget> AddedRootWidget = nullptr;
};
