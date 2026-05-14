#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pipeline/SQLUIRuntimeWidgetPipeline.h"

#include "SQLUISampleLayoutDrivenFilterListDemoActor.generated.h"

class USQLUIBaseWidget;

UCLASS(BlueprintType)
class SQLUISAMPLES_API ASQLUISampleLayoutDrivenFilterListDemoActor : public AActor
{
	GENERATED_BODY()

public:
	ASQLUISampleLayoutDrivenFilterListDemoActor();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples")
	void RunLayoutDrivenFilterListDemo();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRunOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bAddToViewport = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FVector2D ViewportPosition = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FVector2D ViewportSize = FVector2D(420.0f, 320.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ViewportZOrder = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "SQLUI|Samples")
	FSQLUIRuntimeWidgetPipelineResult LastPipelineResult;

private:
	void RemoveAddedRootWidget();

	UPROPERTY(Transient)
	TObjectPtr<USQLUIBaseWidget> AddedRootWidget = nullptr;
};
