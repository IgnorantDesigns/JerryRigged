#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepository.h"

#include "SQLUIInMemoryLayoutRepository.generated.h"

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
class SQLUICORE_API USQLUIInMemoryLayoutRepository : public USQLUILayoutRepository
{
	GENERATED_BODY()

public:
	virtual void LoadLayout(const FString& LayoutId, FSQLUILayoutLoadCompleteDelegate Callback) override;
	virtual void SaveLayout(const FSQLUILayoutDocument& Document, FSQLUILayoutSaveCompleteDelegate Callback) override;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutLoadResult LoadLayoutById(const FString& LayoutId) const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutLoadResult LoadLayoutByDisplayName(const FString& DisplayName) const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryRemoveResult RemoveLayout(const FString& LayoutId);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryListResult ListLayouts() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryClearResult ClearLayouts();

private:
	UPROPERTY(Transient)
	TMap<FString, FSQLUILayoutDocument> LayoutsById;
};
