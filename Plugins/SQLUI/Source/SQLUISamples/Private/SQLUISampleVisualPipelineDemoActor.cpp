#include "SQLUISampleVisualPipelineDemoActor.h"

#include "SQLUISamplesModule.h"
#include "Engine/World.h"
#include "Widgets/SQLUIBaseWidget.h"

namespace
{
const TCHAR* SQLUISampleVisualPipelineDemoBoolToString(const bool bValue)
{
	return bValue ? TEXT("true") : TEXT("false");
}

void LogSQLUISampleVisualPipelineDemoErrors(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample visual pipeline demo error: %s"), *Message);
	}
}

void LogSQLUISampleVisualPipelineDemoWarnings(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(LogSQLUISamples, Warning, TEXT("SQLUI sample visual pipeline demo warning: %s"), *Message);
	}
}
}

ASQLUISampleVisualPipelineDemoActor::ASQLUISampleVisualPipelineDemoActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASQLUISampleVisualPipelineDemoActor::BeginPlay()
{
	Super::BeginPlay();

	if (bRunOnBeginPlay)
	{
		RunVisualPipelineDemo();
	}
}

void ASQLUISampleVisualPipelineDemoActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(AddedRootWidget.Get()))
	{
		AddedRootWidget->RemoveFromParent();
		AddedRootWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ASQLUISampleVisualPipelineDemoActor::RunVisualPipelineDemo()
{
	if (IsValid(AddedRootWidget.Get()))
	{
		AddedRootWidget->RemoveFromParent();
		AddedRootWidget = nullptr;
	}

	FSQLUISampleSmokeTestRequest Request = DemoRequest;
	if (!IsValid(Request.OwningPlayer.Get()) && GetWorld())
	{
		Request.OwningPlayer = GetWorld()->GetFirstPlayerController();
	}

	LastResult = USQLUISampleSmokeTestRunner::RunSmokeTest(this, Request);

	if (LastResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample visual pipeline demo succeeded."));
	}
	else
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample visual pipeline demo failed."));
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample visual pipeline demo created widget count: %d"),
		LastResult.CreatedWidgetCount);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample visual pipeline demo root widget valid: %s"),
		SQLUISampleVisualPipelineDemoBoolToString(LastResult.bRootWidgetValid));

	bool bAddedToViewport = false;
	if (bAddToViewport)
	{
		if (IsValid(LastResult.RootWidget.Get()))
		{
			LastResult.RootWidget->AddToViewport(ViewportZOrder);
			AddedRootWidget = LastResult.RootWidget;
			bAddedToViewport = true;
		}
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample visual pipeline demo viewport add status: %s"),
		bAddToViewport
			? (bAddedToViewport ? TEXT("added") : TEXT("failed"))
			: TEXT("skipped"));

	LogSQLUISampleVisualPipelineDemoErrors(LastResult.Errors);
	LogSQLUISampleVisualPipelineDemoWarnings(LastResult.Warnings);
}
