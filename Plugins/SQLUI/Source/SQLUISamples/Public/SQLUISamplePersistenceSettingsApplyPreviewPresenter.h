#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUIPersistenceSettingsDraftDisplay.h"
#include "UObject/Object.h"

#include "SQLUISamplePersistenceSettingsApplyPreviewPresenter.generated.h"

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bCanApplyInFuture = false;

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
	FSQLUIPersistenceSettingsApplyPreviewDisplaySummary DisplaySummary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow> Rows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FString> FormattedLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	FString SummaryText;
};

/**
 * Minimal sample/dev-facing presenter for persistence settings apply-preview rows.
 *
 * This optional SQLUISamples helper consumes the SQLUICore dry-run apply preview
 * and apply-preview display-row surfaces. It keeps the latest result in memory
 * for simple sample UI or Blueprint binding, but it does not apply settings,
 * write config, create directories or databases, open databases, run migrations,
 * copy seed databases, initialize providers/repositories, reset databases, or
 * delete files.
 */
UCLASS(BlueprintType)
class SQLUISAMPLES_API USQLUISamplePersistenceSettingsApplyPreviewPresenter
	: public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
	RefreshDefaultPersistenceSettingsApplyPreviewDisplay();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
	RefreshCurrentPersistenceSettingsApplyPreviewDisplay();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
	BuildPersistenceSettingsApplyPreviewDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
	BuildPersistenceSettingsApplyPreviewDisplayFromPreview(
		const FSQLUIPersistenceSettingsApplyPreviewResult& PreviewResult);

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
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult
	StoreDisplaySummary(
		const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary& InSummary);
	void RebuildFormattedLines();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow> Rows;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceSettingsApplyPreviewRefreshResult LastRefreshResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;
};
