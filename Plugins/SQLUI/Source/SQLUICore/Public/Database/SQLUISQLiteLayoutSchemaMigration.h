#pragma once

#include "CoreMinimal.h"
#include "Database/SQLUISQLiteMigrationTypes.h"

#include "SQLUISQLiteLayoutSchemaMigration.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUISQLiteLayoutSchemaMigrationProbeResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bMigrationSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bLayoutsTableExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bLayoutRevisionsTableExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bLayoutTagsTableExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bLayoutCheckpointsTableExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bLayoutPreviewsTableExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bExpectedIndexesExist = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bDatabaseRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUISQLiteLayoutSchemaInitializationResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bDatabaseOpened = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSchemaReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bMigrationApplied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bMigrationAlreadyApplied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString ErrorMessage;
};

class SQLUICORE_API FSQLUISQLiteLayoutSchemaMigration
{
public:
	static FString GetInitialLayoutSchemaMigrationId();

	static FSQLUISQLiteMigrationStep GetInitialLayoutSchemaMigrationStep();

	static FString GetDefaultProbeDatabasePath();

	static FSQLUISQLiteLayoutSchemaMigrationProbeResult RunProbe(
		const FString& DatabasePath = FString(),
		bool bRemoveDatabaseAfterClose = true);

	static FSQLUISQLiteLayoutSchemaInitializationResult ApplyInitialSchema(
		const FString& DatabasePath,
		bool bCreateDatabaseIfMissing);

	static bool CountInitialSchemaMigrationRecords(
		const FString& DatabasePath,
		int32& OutRecordCount,
		FString& OutErrorMessage);

	// Smoke-test helper for preparing a complete-schema/missing-record edge case.
	static bool DeleteInitialSchemaMigrationRecordForSmokeTest(
		const FString& DatabasePath,
		FString& OutErrorMessage);
};
