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
};
