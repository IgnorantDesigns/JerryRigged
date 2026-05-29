#pragma once

#include "CoreMinimal.h"

#include "SQLUISQLiteProbe.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUISQLiteProbeResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bDatabaseOpened = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bDatabaseClosed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bDatabaseRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString ErrorMessage;
};

class SQLUICORE_API FSQLUISQLiteProbe
{
public:
	static FString GetDefaultProbeDatabasePath();

	static FSQLUISQLiteProbeResult RunOpenCloseProbe(
		const FString& DatabasePath = FString(),
		bool bRemoveDatabaseAfterClose = true);
};
