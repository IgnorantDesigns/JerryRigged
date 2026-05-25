#pragma once

#include "CoreMinimal.h"

#include "SQLUIListTypes.generated.h"

USTRUCT(BlueprintType)
struct SQLUIWIDGETS_API FSQLUIListItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|List")
	FString ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|List")
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|List")
	TMap<FString, FString> Properties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|List")
	TMap<FString, FString> Metadata;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FSQLUIListItemClickedDelegate, const FSQLUIListItemData&);
