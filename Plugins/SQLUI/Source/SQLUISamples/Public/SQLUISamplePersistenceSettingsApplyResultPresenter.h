#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUIPersistenceSettingsDraftDisplay.h"
#include "UObject/Object.h"

#include "SQLUISamplePersistenceSettingsApplyResultPresenter.generated.h"

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISamplePersistenceSettingsApplyResultRefreshResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bApplyResultSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bActualApplyImplemented = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidWriteConfig = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidChangeSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidInitializeProvider = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidInitializeRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidCreateDatabaseFiles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidCreateDirectories = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidOpenDatabaseForWriting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidRunMigrations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidCopySeedDatabase = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bDidDeleteFiles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bHasErrors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	bool bHasWarnings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	ESQLUIPersistenceSettingsApplyStatus Status =
		ESQLUIPersistenceSettingsApplyStatus::NotImplemented;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUIPersistenceSettingsApplyResultDisplaySummary DisplaySummary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsApplyResultDisplayRow> Rows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	TArray<FString> FormattedLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples|Persistence Settings")
	FString SummaryText;
};

/**
 * Minimal sample/dev-facing presenter for persistence settings apply-result rows.
 *
 * This optional SQLUISamples helper consumes the SQLUICore non-mutating apply
 * entrypoint skeleton and apply-result display-row surfaces. It keeps the
 * latest result in memory for simple sample UI or Blueprint binding, but it
 * does not apply settings, write config, create directories or databases, open
 * databases, run migrations, copy seed databases, initialize providers or
 * repositories, reset databases, or delete files.
 */
UCLASS(BlueprintType)
class SQLUISAMPLES_API USQLUISamplePersistenceSettingsApplyResultPresenter
	: public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult
	RefreshDefaultPersistenceSettingsApplyResultDisplay();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult
	RefreshCurrentPersistenceSettingsApplyResultDisplay();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult
	BuildPersistenceSettingsApplyResultDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples|Persistence Settings")
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult
	BuildPersistenceSettingsApplyResultDisplayFromResult(
		const FSQLUIPersistenceSettingsApplyResult& ApplyResult);

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
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult
	StoreDisplaySummary(
		const FSQLUIPersistenceSettingsApplyResultDisplaySummary& InSummary);
	void RebuildFormattedLines();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FSQLUIPersistenceSettingsApplyResultDisplayRow> Rows;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FString> FormattedLines;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FSQLUISamplePersistenceSettingsApplyResultRefreshResult LastRefreshResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SQLUI|Samples|Persistence Settings", meta = (AllowPrivateAccess = "true"))
	FString SummaryText;
};
