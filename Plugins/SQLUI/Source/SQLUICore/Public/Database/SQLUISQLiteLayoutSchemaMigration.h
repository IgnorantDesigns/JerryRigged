#pragma once

#include "CoreMinimal.h"

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

class SQLUICORE_API FSQLUISQLiteLayoutSchemaMigration
{
public:
	static FString GetDefaultProbeDatabasePath();

	static FSQLUISQLiteLayoutSchemaMigrationProbeResult RunProbe(
		const FString& DatabasePath = FString(),
		bool bRemoveDatabaseAfterClose = true);
};
