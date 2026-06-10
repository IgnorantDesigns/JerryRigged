#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SQLUISamplePersistenceSettingsApplyPreviewPresenter.h"

#include "SQLUISamplePersistenceSettingsApplyPreviewPanelWidget.generated.h"

/**
 * Optional C++ UMG shell for future persistence settings apply-preview panels.
 *
 * This shell creates no visual layout, adds no widget blueprint asset, and is
 * never wired into startup by SQLUISamples. Refresh/build is caller-invoked
 * only and delegates to the sample presenter, which delegates to the
 * SQLUICore dry-run apply-preview/display-row path.
 */
UCLASS(BlueprintType, Blueprintable)
class SQLUISAMPLES_API USQLUISamplePersistenceSettingsApplyPreviewPanelWidget
	: public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
	RefreshDefaultPersistenceSettingsApplyPreviewPanel();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
	RefreshCurrentPersistenceSettingsApplyPreviewPanel();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
	BuildPersistenceSettingsApplyPreviewPanel(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow> GetRows() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FString> GetFormattedLines() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
	GetLastRefreshResult() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	FString GetSummaryText() const;

private:
	void EnsureApplyPreviewPresenter();
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult StoreRefreshResult(
		const FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult& InResult);

	UPROPERTY(Transient)
	TObjectPtr<USQLUISamplePersistenceSettingsApplyPreviewPresenter>
		ApplyPreviewPresenter = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow> Rows;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult LastRefreshResult;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bCanApplyInFuture = false;

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
};
