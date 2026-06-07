#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Layout/SQLUIPersistenceStatusDisplay.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"
#include "SQLUISamplePersistenceStatusPanelAdapter.h"

#include "SQLUISamplePersistenceStatusPanelWidget.generated.h"

/**
 * Optional read-only C++ UMG shell for future persistence status panels.
 *
 * This shell creates no visual layout, adds no widget blueprint asset, and is
 * never wired into startup by SQLUISamples. Refresh is caller-invoked only and
 * delegates to the panel adapter, which delegates to the existing presenter and
 * SQLUICore display-row path.
 */
UCLASS(BlueprintType, Blueprintable)
class SQLUISAMPLES_API USQLUISamplePersistenceStatusPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence")
	FSQLUISamplePersistenceStatusRefreshResult RefreshPersistenceStatusPanel();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence")
	FSQLUISamplePersistenceStatusRefreshResult RefreshPersistenceStatusPanelFromRuntimeConfig(
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
	void EnsurePanelAdapter();
	FSQLUISamplePersistenceStatusRefreshResult StoreRefreshResult(
		const FSQLUISamplePersistenceStatusRefreshResult& InResult);

	UPROPERTY(Transient)
	TObjectPtr<USQLUISamplePersistenceStatusPanelAdapter> PanelAdapter = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceStatusDisplayRow> Rows;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceStatusRefreshResult LastRefreshResult;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;
};
