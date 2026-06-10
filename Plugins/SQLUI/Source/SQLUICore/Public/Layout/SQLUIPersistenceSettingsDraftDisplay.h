#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Layout/SQLUIPersistenceSettingsDraft.h"

#include "SQLUIPersistenceSettingsDraftDisplay.generated.h"

UENUM(BlueprintType)
enum class ESQLUIPersistenceSettingsValidationDisplayState : uint8
{
	Normal,
	Good,
	Warning,
	Error
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsValidationDisplayRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FName FieldKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FText Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	ESQLUIPersistenceSettingsValidationDisplayState State =
		ESQLUIPersistenceSettingsValidationDisplayState::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FText DetailText;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsValidationDisplaySummary
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bIsValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasErrors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasWarnings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FText SummaryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsValidationDisplayRow> Rows;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsApplyPreviewDisplayRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FName FieldKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FText Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	ESQLUIPersistenceSettingsValidationDisplayState State =
		ESQLUIPersistenceSettingsValidationDisplayState::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FText DetailText;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bCanApplyInFuture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bIsValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasChanges = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasErrors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasWarnings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldNeedProviderReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldNeedRepositoryReopen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FText SummaryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow> Rows;
};

/**
 * UI-safe formatter for validation-only persistence settings drafts.
 *
 * These helpers may call the non-mutating draft validation path, but they do
 * not apply settings, write config, initialize providers or repositories,
 * create directories or database files, open databases, run migrations, copy
 * seed databases, reset databases, or delete files.
 */
UCLASS(BlueprintType)
class SQLUICORE_API USQLUIPersistenceSettingsDraftDisplayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsValidationDisplaySummary
	ValidateAndMakePersistenceSettingsValidationDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsValidationDisplaySummary
	MakePersistenceSettingsValidationDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft,
		const FSQLUIPersistenceSettingsValidationResult& ValidationResult);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static TArray<FSQLUIPersistenceSettingsValidationDisplayRow>
	MakePersistenceSettingsValidationDisplayRows(
		const FSQLUIPersistenceSettingsDraft& Draft,
		const FSQLUIPersistenceSettingsValidationResult& ValidationResult);
};

/**
 * UI-safe formatter for dry-run persistence settings apply-intent previews.
 *
 * These helpers may call the non-mutating draft apply preview path, but they do
 * not apply settings, write config, initialize providers or repositories,
 * create directories or database files, open databases, run migrations, copy
 * seed databases, reset databases, or delete files.
 */
UCLASS(BlueprintType)
class SQLUICORE_API USQLUIPersistenceSettingsApplyPreviewDisplayLibrary
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
	PreviewAndMakePersistenceSettingsApplyPreviewDisplay(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
	MakePersistenceSettingsApplyPreviewDisplay(
		const FSQLUIPersistenceSettingsApplyPreviewResult& PreviewResult);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static TArray<FSQLUIPersistenceSettingsApplyPreviewDisplayRow>
	MakePersistenceSettingsApplyPreviewDisplayRows(
		const FSQLUIPersistenceSettingsApplyPreviewResult& PreviewResult);
};
