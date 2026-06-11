#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SQLUISamplePersistenceSettingsApplyContractPresenter.h"

#include "SQLUISamplePersistenceSettingsApplyContractPanelWidget.generated.h"

/**
 * Optional C++ UMG shell for future persistence settings apply/cancel
 * contract panels.
 *
 * This shell creates no visual layout, adds no widget blueprint asset, and is
 * never wired into startup by SQLUISamples. Refresh/build is caller-invoked
 * only and delegates to the sample presenter, which delegates to the
 * SQLUICore non-mutating apply/cancel contract display-row path.
 */
UCLASS(BlueprintType, Blueprintable)
class SQLUISAMPLES_API USQLUISamplePersistenceSettingsApplyContractPanelWidget
	: public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult
	RefreshDefaultPersistenceSettingsApplyContractPanel();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult
	RefreshCurrentPersistenceSettingsApplyContractPanel();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult
	BuildPersistenceSettingsApplyContractPanel(
		const FSQLUIPersistenceSettingsDraft& Draft);

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
	void EnsureApplyContractPresenter();
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult StoreRefreshResult(
		const FSQLUISamplePersistenceSettingsApplyContractRefreshResult& InResult);

	UPROPERTY(Transient)
	TObjectPtr<USQLUISamplePersistenceSettingsApplyContractPresenter>
		ApplyContractPresenter = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceSettingsApplyContractDisplayRow> Rows;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceSettingsApplyContractRefreshResult LastRefreshResult;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bCanApplyInFuture = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bActualApplyImplemented = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bCanExecuteApplyNow = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bIsValid = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bHasChanges = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bHasErrors = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bHasWarnings = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bWouldNeedProviderReinitialize = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bWouldNeedRepositoryReopen = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bWouldDiscardChangesOnCancel = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	ESQLUIPersistenceSettingsApplyAvailability Availability =
		ESQLUIPersistenceSettingsApplyAvailability::NotImplemented;
};
