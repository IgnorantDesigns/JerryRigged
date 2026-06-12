#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SQLUISamplePersistenceSettingsApplyResultPresenter.h"

#include "SQLUISamplePersistenceSettingsApplyResultPanelWidget.generated.h"

/**
 * Optional C++ UMG shell for future persistence settings apply-result panels.
 *
 * This shell creates no visual layout, adds no widget blueprint asset, and is
 * never wired into startup by SQLUISamples. Refresh/build is caller-invoked
 * only and delegates to the sample presenter, which delegates to the
 * SQLUICore unavailable/non-mutating apply-result display-row path.
 */
UCLASS(BlueprintType, Blueprintable)
class SQLUISAMPLES_API
	USQLUISamplePersistenceSettingsApplyResultPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult
	RefreshDefaultPersistenceSettingsApplyResultPanel();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult
	RefreshCurrentPersistenceSettingsApplyResultPanel();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult
	BuildPersistenceSettingsApplyResultPanel(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsApplyResultDisplayRow> GetRows() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FString> GetFormattedLines() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult
	GetLastRefreshResult() const;

	UFUNCTION(BlueprintPure, Category = "SQLUI|Samples|Persistence Settings")
	FString GetSummaryText() const;

private:
	void EnsureApplyResultPresenter();
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult StoreRefreshResult(
		const FSQLUISamplePersistenceSettingsApplyResultRefreshResult& InResult);

	UPROPERTY(Transient)
	TObjectPtr<USQLUISamplePersistenceSettingsApplyResultPresenter>
		ApplyResultPresenter = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceSettingsApplyResultDisplayRow> Rows;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult LastRefreshResult;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bSucceeded = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bApplyResultSucceeded = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bActualApplyImplemented = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidWriteConfig = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidChangeSettings = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidInitializeProvider = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidInitializeRepository = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidCreateDatabaseFiles = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidCreateDirectories = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidOpenDatabaseForWriting = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidRunMigrations = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidCopySeedDatabase = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bDidDeleteFiles = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bHasErrors = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	bool bHasWarnings = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	ESQLUIPersistenceSettingsApplyStatus Status =
		ESQLUIPersistenceSettingsApplyStatus::NotImplemented;
};
