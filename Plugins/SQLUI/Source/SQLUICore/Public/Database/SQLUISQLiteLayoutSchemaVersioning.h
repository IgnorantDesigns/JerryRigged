#pragma once

#include "CoreMinimal.h"

#include "SQLUISQLiteLayoutSchemaVersioning.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUISQLiteLayoutSchemaVersionStatus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bDatabaseOpened = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bMigrationTableExists = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bInitialSchemaRecorded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSchemaObjectsReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bHasPendingMigrations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bHasFailedMigration = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	int32 AppliedMigrationCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	int32 KnownMigrationCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString LatestAppliedMigrationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString LatestKnownMigrationId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	TArray<FString> AppliedMigrationIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	TArray<FString> PendingMigrationIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString ErrorMessage;
};

class SQLUICORE_API FSQLUISQLiteLayoutSchemaVersioning
{
public:
	static TArray<FString> GetKnownLayoutSchemaMigrationIds();

	static FString GetLatestKnownLayoutSchemaMigrationId();

	static FSQLUISQLiteLayoutSchemaVersionStatus GetLayoutSchemaVersionStatus(
		const FString& DatabasePath);

	static FSQLUISQLiteLayoutSchemaVersionStatus GetMigrationVersionStatus(
		const FString& DatabasePath,
		const TArray<FString>& KnownMigrationIds,
		bool bVerifyLayoutSchemaObjects);

	static FSQLUISQLiteLayoutSchemaVersionStatus ApplyKnownLayoutSchemaMigrations(
		const FString& DatabasePath,
		bool bCreateDatabaseIfMissing);
};
