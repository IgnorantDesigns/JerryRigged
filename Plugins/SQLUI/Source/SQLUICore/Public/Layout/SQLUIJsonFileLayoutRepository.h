#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepository.h"

#include "SQLUIJsonFileLayoutRepository.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIJsonFileLayoutRepositorySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString BaseDirectory;
};

UCLASS(BlueprintType)
class SQLUICORE_API USQLUIJsonFileLayoutRepository : public USQLUILayoutRepository
{
	GENERATED_BODY()

public:
	virtual void LoadLayout(const FString& LayoutId, FSQLUILayoutLoadCompleteDelegate Callback) override;
	virtual void SaveLayout(const FSQLUILayoutDocument& Document, FSQLUILayoutSaveCompleteDelegate Callback) override;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	void Configure(const FSQLUIJsonFileLayoutRepositorySettings& InSettings);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUIJsonFileLayoutRepositorySettings GetSettings() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FString GetResolvedBaseDirectory() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutLoadResult LoadLayoutById(const FString& LayoutId) const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryRemoveResult RemoveLayout(const FString& LayoutId);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	virtual FSQLUILayoutRepositoryListResult ListLayouts() const override;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryClearResult ClearLayouts();

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository", meta = (AllowPrivateAccess = "true"))
	FSQLUIJsonFileLayoutRepositorySettings Settings;

	FString ResolveBaseDirectory() const;
	bool TryResolveLayoutFilePath(const FString& LayoutId, FString& OutFilePath, FString& OutErrorMessage) const;
};
