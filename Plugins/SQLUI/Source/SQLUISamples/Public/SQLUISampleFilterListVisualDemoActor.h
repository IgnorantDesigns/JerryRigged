#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SQLUISampleFilterListVisualDemoActor.generated.h"

class USQLUIFilterBox;
class USQLUIListWidget;

UCLASS(BlueprintType)
class SQLUISAMPLES_API ASQLUISampleFilterListVisualDemoActor : public AActor
{
	GENERATED_BODY()

public:
	ASQLUISampleFilterListVisualDemoActor();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples")
	void RunVisualFilterListDemo();

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
	FVector2D FilterBoxViewportPosition = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FVector2D FilterBoxViewportSize = FVector2D(420.0f, 64.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FVector2D ListWidgetViewportPosition = FVector2D(24.0f, 96.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FVector2D ListWidgetViewportSize = FVector2D(420.0f, 220.0f);

private:
	void RemoveAddedWidgets();

	UPROPERTY(Transient)
	TObjectPtr<USQLUIFilterBox> AddedFilterBoxWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USQLUIListWidget> AddedListWidget = nullptr;
};
