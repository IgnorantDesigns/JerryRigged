#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUIPersistenceSettingsDraftDisplay.h"
#include "UObject/Object.h"

#include "SQLUISamplePersistenceSettingsDraftPresenter.generated.h"

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISamplePersistenceSettingsDraftRefreshResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bIsValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bHasErrors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bHasWarnings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUIPersistenceSettingsValidationDisplaySummary DisplaySummary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsValidationDisplayRow> Rows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FString> FormattedLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	FString SummaryText;
};

/**
 * Minimal sample/dev-facing presenter for persistence settings draft rows.
 *
 * This optional SQLUISamples helper consumes the SQLUICore validation-only
 * draft and display-row surfaces. It keeps the latest result in memory for
 * simple sample UI or Blueprint binding, but it does not apply settings, write
 * config, create directories or databases, open databases, run migrations,
 * copy seed databases, initialize providers/repositories, reset databases, or
 * delete files.
 */
UCLASS(BlueprintType)
class SQLUISAMPLES_API USQLUISamplePersistenceSettingsDraftPresenter : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsDraftRefreshResult
	RefreshDefaultPersistenceSettingsDraftDisplay();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsDraftRefreshResult
	RefreshCurrentPersistenceSettingsDraftDisplay();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsDraftRefreshResult
	BuildPersistenceSettingsDraftValidationDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsValidationDisplayRow> GetRows() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FString> GetFormattedLines() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsDraftRefreshResult GetLastRefreshResult() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	FString GetSummaryText() const;

private:
	FSQLUISamplePersistenceSettingsDraftRefreshResult StoreDisplaySummary(
		const FSQLUIPersistenceSettingsValidationDisplaySummary& InSummary);
	void RebuildFormattedLines();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceSettingsValidationDisplayRow> Rows;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceSettingsDraftRefreshResult LastRefreshResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;
};
