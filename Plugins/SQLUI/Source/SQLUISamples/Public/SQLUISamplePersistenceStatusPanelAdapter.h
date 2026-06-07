#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUIPersistenceStatusDisplay.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"
#include "SQLUISamplePersistenceStatusPresenter.h"
#include "UObject/Object.h"

#include "SQLUISamplePersistenceStatusPanelAdapter.generated.h"

/**
 * Tiny sample/dev-facing adapter for future read-only persistence status panels.
 *
 * This adapter is intentionally not a widget. It delegates refresh work to the
 * existing SQLUISamples presenter so future Blueprint/UMG panels can bind to a
 * panel-named object without duplicating persistence probing. Refresh remains
 * caller-invoked and read-only.
 */
UCLASS(BlueprintType)
class SQLUISAMPLES_API USQLUISamplePersistenceStatusPanelAdapter : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence", meta = (WorldContext = "WorldContextObject"))
	FSQLUISamplePersistenceStatusRefreshResult RefreshPersistenceStatusPanel(
		UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence", meta = (WorldContext = "WorldContextObject"))
	FSQLUISamplePersistenceStatusRefreshResult RefreshPersistenceStatusPanelFromRuntimeConfig(
		UObject* WorldContextObject,
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence")
	TArray<FSQLUIPersistenceStatusDisplayRow> GetRows() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence")
	TArray<FString> GetFormattedLines() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence")
	FSQLUISamplePersistenceStatusRefreshResult GetLastRefreshResult() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence")
	FString GetSummaryText() const;

private:
	void EnsurePresenter();
	FSQLUISamplePersistenceStatusRefreshResult StoreRefreshResult(
		const FSQLUISamplePersistenceStatusRefreshResult& InResult);

	UPROPERTY(Transient)
	TObjectPtr<USQLUISamplePersistenceStatusPresenter> Presenter = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceStatusDisplayRow> Rows;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceStatusRefreshResult LastRefreshResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;
};
