#pragma once

#include "CoreMinimal.h"

#include "SQLUISQLiteLayoutReadProbe.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUISQLiteLayoutReadProbeResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSchemaMigrationSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSeedInserted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bListSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bListedMetadataFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bLoadSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bLoadedDocumentValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bDatabaseRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	int32 ListedLayoutCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString SeedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString LoadedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString ErrorMessage;
};

class SQLUICORE_API FSQLUISQLiteLayoutReadProbe
{
public:
	static FString GetDefaultProbeDatabasePath();

	static FSQLUISQLiteLayoutReadProbeResult RunProbe(
		const FString& DatabasePath = FString(),
		bool bRemoveDatabaseAfterClose = true);

	static bool CountLayoutRevisions(
		const FString& DatabasePath,
		const FString& LayoutId,
		int32& OutRevisionCount,
		FString& OutErrorMessage);
};
