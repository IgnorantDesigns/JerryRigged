#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUIPersistenceSettingsDraftDisplay.h"
#include "UObject/Object.h"

#include "SQLUISamplePersistenceSettingsApplyContractPresenter.generated.h"

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISamplePersistenceSettingsApplyContractRefreshResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bCanApplyInFuture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bActualApplyImplemented = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bCanExecuteApplyNow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bIsValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bHasChanges = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bHasErrors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bHasWarnings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bWouldNeedProviderReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bWouldNeedRepositoryReopen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bWouldDiscardChangesOnCancel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	ESQLUIPersistenceSettingsApplyAvailability Availability =
		ESQLUIPersistenceSettingsApplyAvailability::NotImplemented;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUIPersistenceSettingsApplyContractDisplaySummary DisplaySummary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsApplyContractDisplayRow> Rows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FString> FormattedLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	FString SummaryText;
};

/**
 * Minimal sample/dev-facing presenter for persistence settings apply/cancel
 * contract rows.
 *
 * This optional SQLUISamples helper consumes the SQLUICore non-mutating
 * apply/cancel contract and contract display-row surfaces. It keeps the latest
 * result in memory for simple sample UI or Blueprint binding, but it does not
 * apply settings, write config, create directories or databases, open
 * databases, run migrations, copy seed databases, initialize providers or
 * repositories, reset databases, or delete files.
 */
UCLASS(BlueprintType)
class SQLUISAMPLES_API USQLUISamplePersistenceSettingsApplyContractPresenter
	: public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult
	RefreshDefaultPersistenceSettingsApplyContractDisplay();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult
	RefreshCurrentPersistenceSettingsApplyContractDisplay();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult
	BuildPersistenceSettingsApplyContractDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult
	BuildPersistenceSettingsApplyContractDisplayFromContract(
		const FSQLUIPersistenceSettingsApplyContractResult& ContractResult,
		const FSQLUIPersistenceSettingsCancelPreviewResult& CancelPreviewResult);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsApplyContractDisplayRow> GetRows() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FString> GetFormattedLines() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult
	GetLastRefreshResult() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	FString GetSummaryText() const;

private:
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult
	StoreDisplaySummary(
		const FSQLUIPersistenceSettingsApplyContractDisplaySummary& InSummary);
	void RebuildFormattedLines();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceSettingsApplyContractDisplayRow> Rows;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult LastRefreshResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;
};
