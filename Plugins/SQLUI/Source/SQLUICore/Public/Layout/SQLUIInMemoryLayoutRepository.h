#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepository.h"

#include "SQLUIInMemoryLayoutRepository.generated.h"

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
	virtual FSQLUILayoutRepositoryListResult ListLayouts() const override;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryClearResult ClearLayouts();

private:
	UPROPERTY(Transient)
	TMap<FString, FSQLUILayoutDocument> LayoutsById;
};
