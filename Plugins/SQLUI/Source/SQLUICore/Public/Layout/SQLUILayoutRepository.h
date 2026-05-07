#pragma once

#include "CoreMinimal.h"
#include "Layout/ISQLUILayoutRepository.h"
#include "UObject/Object.h"

#include "SQLUILayoutRepository.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryListResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	TArray<FSQLUILayoutMetadata> Layouts;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryRemoveResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString RemovedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bRemoved = false;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryClearResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	int32 RemovedCount = 0;
};

UCLASS(BlueprintType)
class SQLUICORE_API USQLUILayoutRepository : public UObject, public ISQLUILayoutRepository
{
	GENERATED_BODY()

public:
	virtual void LoadLayout(const FString& LayoutId, FSQLUILayoutLoadCompleteDelegate Callback) override;
	virtual void SaveLayout(const FSQLUILayoutDocument& Document, FSQLUILayoutSaveCompleteDelegate Callback) override;

protected:
	static FSQLUILayoutLoadResult MakeBackendUnavailableLoadResult(const FString& LayoutId);
	static FSQLUILayoutSaveResult MakeBackendUnavailableSaveResult(const FSQLUILayoutDocument& Document);
};
