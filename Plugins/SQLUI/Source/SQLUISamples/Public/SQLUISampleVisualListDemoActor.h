#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SQLUISampleVisualListDemoActor.generated.h"

class USQLUIListWidget;

UCLASS(BlueprintType)
class SQLUISAMPLES_API ASQLUISampleVisualListDemoActor : public AActor
{
	GENERATED_BODY()

public:
	ASQLUISampleVisualListDemoActor();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples")
	void RunVisualListDemo();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRunOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bAddToViewport = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FVector2D ViewportPosition = FVector2D(24.0f, 96.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FVector2D ViewportSize = FVector2D(420.0f, 220.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ViewportZOrder = 0;

private:
	UPROPERTY(Transient)
	TObjectPtr<USQLUIListWidget> AddedListWidget = nullptr;
};
