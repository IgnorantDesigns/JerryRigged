#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"

#include "SQLUIPersistenceSettingsDraft.generated.h"

UENUM(BlueprintType)
enum class ESQLUIPersistenceSettingsValidationMessageSeverity : uint8
{
	Info,
	Warning,
	Error
};

UENUM(BlueprintType)
enum class ESQLUIPersistenceSettingsApplyAvailability : uint8
{
	NotImplemented,
	BlockedByValidation,
	NoChanges,
	PreviewOnlyReady
};

UENUM(BlueprintType)
enum class ESQLUIPersistenceSettingsApplyStatus : uint8
{
	NotImplemented,
	BlockedByValidation,
	NoChanges,
	PreviewOnly,
	Failed,
	Succeeded
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsValidationMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	ESQLUIPersistenceSettingsValidationMessageSeverity Severity =
		ESQLUIPersistenceSettingsValidationMessageSeverity::Info;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FString Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FString DetailText;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsDraft
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FSQLUILayoutRepositoryRuntimeConfig CurrentRuntimeConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FSQLUILayoutRepositoryRuntimeConfig PendingRuntimeConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bCurrentProviderAutoInitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bPendingProviderAutoInitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasBackendOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasSQLiteDatabasePathOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasProviderAutoInitializeOverride = false;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsValidationResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bIsValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeBackend = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeSQLitePath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeProviderAutoInitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bSQLitePathResolved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FString ResolvedSQLiteDatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsValidationMessage> Messages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FString SummaryText;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsApplyPreviewResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bCanApplyInFuture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bIsValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasChanges = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeBackend = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeSQLitePath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeSQLiteConfig = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeProviderAutoInitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldNeedProviderReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldNeedRepositoryReopen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FSQLUIPersistenceSettingsValidationResult ValidationResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsValidationMessage> Messages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FString SummaryText;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsApplyContractResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bCanApplyInFuture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bActualApplyImplemented = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bCanExecuteApplyNow = false;

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
	bool bWouldChangeBackend = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeSQLitePath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeSQLiteConfig = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldChangeProviderAutoInitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldNeedProviderReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldNeedRepositoryReopen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	ESQLUIPersistenceSettingsApplyAvailability Availability =
		ESQLUIPersistenceSettingsApplyAvailability::NotImplemented;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FSQLUIPersistenceSettingsApplyPreviewResult ApplyPreview;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsValidationMessage> Messages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FString SummaryText;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsCancelPreviewResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bHasPendingChanges = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldDiscardChanges = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bWouldResetPendingToCurrent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FSQLUIPersistenceSettingsDraft DraftAfterCancel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsValidationMessage> Messages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FString SummaryText;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsApplyRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FSQLUIPersistenceSettingsDraft Draft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bRequestBackendOnlyProductionApply = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FString ProductionTargetEnablementRequestDescription;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceSettingsApplyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bActualApplyImplemented = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidWriteConfig = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidChangeSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidInitializeProvider = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidInitializeRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidCreateDatabaseFiles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidCreateDirectories = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidOpenDatabaseForWriting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidRunMigrations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidCopySeedDatabase = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bDidDeleteFiles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	bool bRequiresRestartOrReinitialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	ESQLUIPersistenceSettingsApplyStatus Status =
		ESQLUIPersistenceSettingsApplyStatus::NotImplemented;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FSQLUIPersistenceSettingsApplyContractResult ApplyContract;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	TArray<FSQLUIPersistenceSettingsValidationMessage> Messages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence Settings")
	FString SummaryText;
};

/**
 * Non-mutating draft/pending settings helper for future persistence UI.
 *
 * These helpers only build and validate plain settings data. They do not write
 * config, apply settings, initialize providers or repositories, create SQLite
 * files, run migrations, copy seed databases, reset databases, or delete files.
 */
UCLASS(BlueprintType)
class SQLUICORE_API USQLUIPersistenceSettingsDraftLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsDraft MakeDefaultPersistenceSettingsDraft();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsDraft MakeCurrentPersistenceSettingsDraft();

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsDraft MakePersistenceSettingsDraftFromRuntimeConfig(
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig,
		bool bProviderAutoInitialize);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsDraft ResetDraftToCurrent(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsValidationResult ValidatePersistenceSettingsDraft(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsApplyPreviewResult PreviewPersistenceSettingsDraftApply(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsApplyContractResult BuildPersistenceSettingsApplyContract(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsCancelPreviewResult BuildPersistenceSettingsCancelPreview(
		const FSQLUIPersistenceSettingsDraft& Draft);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence Settings")
	static FSQLUIPersistenceSettingsApplyResult RequestPersistenceSettingsApply(
		const FSQLUIPersistenceSettingsApplyRequest& Request);
};
