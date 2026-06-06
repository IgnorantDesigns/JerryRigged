#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUIPersistenceStatusDisplay.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"
#include "UObject/Object.h"

#include "SQLUISamplePersistenceStatusPresenter.generated.h"

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISamplePersistenceStatusRefreshResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence")
	TArray<FSQLUIPersistenceStatusDisplayRow> Rows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence")
	TArray<FString> FormattedLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence")
	FString SummaryText;
};

/**
 * Minimal sample/dev-facing presenter for read-only persistence status rows.
 *
 * This SQLUISamples helper consumes SQLUICore display rows and stores stable
 * strings for simple UI, Blueprint, or commandlet presentation. Refresh is an
 * explicit caller-invoked re-query of the SQLUICore status/display surfaces. It
 * does not probe files directly, initialize providers, create repositories,
 * create databases, run migrations, copy seed databases, reset databases, or
 * delete files.
 */
UCLASS(BlueprintType)
class SQLUISAMPLES_API USQLUISamplePersistenceStatusPresenter : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence", meta = (WorldContext = "WorldContextObject"))
	void Refresh(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence", meta = (WorldContext = "WorldContextObject"))
	void RefreshFromRuntimeConfig(
		UObject* WorldContextObject,
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence", meta = (WorldContext = "WorldContextObject"))
	FSQLUISamplePersistenceStatusRefreshResult RefreshPersistenceStatus(
		UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence", meta = (WorldContext = "WorldContextObject"))
	FSQLUISamplePersistenceStatusRefreshResult RefreshPersistenceStatusFromRuntimeConfig(
		UObject* WorldContextObject,
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence")
	TArray<FSQLUIPersistenceStatusDisplayRow> GetRows() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence")
	TArray<FString> GetFormattedLines() const;

private:
	FSQLUISamplePersistenceStatusRefreshResult SetRows(
		TArray<FSQLUIPersistenceStatusDisplayRow>&& InRows);
	void RebuildFormattedLines();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceStatusDisplayRow> Rows;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;
};
