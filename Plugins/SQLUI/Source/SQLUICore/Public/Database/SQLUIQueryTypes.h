#pragma once

#include "CoreMinimal.h"

#include "SQLUIQueryTypes.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIQueryParameter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString Value;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIQueryRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	TMap<FString, FString> Columns;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIQueryResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	bool bBackendUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	TArray<FSQLUIQueryRow> Rows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	int32 RowsAffected = 0;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIQueryRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString DbKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	FString Sql;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Database")
	TArray<FSQLUIQueryParameter> Parameters;
};

DECLARE_DELEGATE_OneParam(FSQLUIQueryCompleteDelegate, const FSQLUIQueryResult&);
