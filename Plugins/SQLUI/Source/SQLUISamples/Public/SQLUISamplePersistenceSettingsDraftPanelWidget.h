#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SQLUISamplePersistenceSettingsDraftPresenter.h"

#include "SQLUISamplePersistenceSettingsDraftPanelWidget.generated.h"

/**
 * Optional C++ UMG shell for future persistence settings draft panels.
 *
 * This shell creates no visual layout, adds no widget blueprint asset, and is
 * never wired into startup by SQLUISamples. Refresh/build is caller-invoked
 * only and delegates to the sample presenter, which delegates to the
 * SQLUICore validation-only draft/display-row path.
 */
UCLASS(BlueprintType, Blueprintable)
class SQLUISAMPLES_API USQLUISamplePersistenceSettingsDraftPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsDraftRefreshResult
	RefreshDefaultPersistenceSettingsDraftPanel();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsDraftRefreshResult
	RefreshCurrentPersistenceSettingsDraftPanel();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsDraftRefreshResult
	BuildPersistenceSettingsDraftPanel(
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
	void EnsureDraftPresenter();
	FSQLUISamplePersistenceSettingsDraftRefreshResult StoreRefreshResult(
		const FSQLUISamplePersistenceSettingsDraftRefreshResult& InResult);

	UPROPERTY(Transient)
	TObjectPtr<USQLUISamplePersistenceSettingsDraftPresenter> DraftPresenter = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceSettingsValidationDisplayRow> Rows;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceSettingsDraftRefreshResult LastRefreshResult;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bIsValid = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bHasErrors = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bHasWarnings = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bRequiresRestartOrReinitialize = false;
};
