#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepositoryFactory.h"

#include "SQLUILayoutRepositoryRuntimeConfig.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryRuntimeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	ESQLUILayoutRepositoryBackend Backend = ESQLUILayoutRepositoryBackend::InMemory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString JsonFileBaseDirectory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString SQLiteDatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSQLiteReadOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSQLiteInitializeSchemaIfMissing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSQLiteCreateDatabaseIfMissing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSQLiteRunCallbackOperationsAsync = false;
};

/**
 * SQLUICore-owned storage selection policy for explicit runtime settings.
 *
 * This helper only maps plain config/command-line data to repository factory
 * settings. It does not create repositories, directories, database files, or
 * schema, and it does not make SQLite the default backend.
 */
class SQLUICORE_API FSQLUILayoutRepositoryRuntimeConfigResolver
{
public:
	static const TCHAR* DefaultSQLiteDatabaseToken;
	static const TCHAR* DefaultSQLiteDatabaseFilename;

	static FSQLUILayoutRepositoryRuntimeConfig MakeDefault();

	static FSQLUILayoutRepositoryFactorySettings ToFactorySettings(
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig);

	static FSQLUILayoutRepositoryRuntimeConfig FromCommandLine(
		const TCHAR* CommandLine,
		const FSQLUILayoutRepositoryRuntimeConfig& Defaults);

	static FString ResolveSQLiteDatabasePath(const FString& InputPath);

	static bool TryParseBackend(
		const FString& BackendText,
		ESQLUILayoutRepositoryBackend& OutBackend);
};
