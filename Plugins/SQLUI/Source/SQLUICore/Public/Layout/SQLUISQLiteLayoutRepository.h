#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepository.h"

#include "SQLUISQLiteLayoutRepository.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUISQLiteLayoutRepositorySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bReadOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	bool bRunCallbackOperationsAsync = false;
};

UCLASS(BlueprintType)
class SQLUICORE_API USQLUISQLiteLayoutRepository : public USQLUILayoutRepository
{
	GENERATED_BODY()

public:
	virtual void LoadLayout(const FString& LayoutId, FSQLUILayoutLoadCompleteDelegate Callback) override;
	virtual void SaveLayout(const FSQLUILayoutDocument& Document, FSQLUILayoutSaveCompleteDelegate Callback) override;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	void Configure(const FSQLUISQLiteLayoutRepositorySettings& InSettings);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUISQLiteLayoutRepositorySettings GetSettings() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FString GetResolvedDatabasePath() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutLoadResult LoadLayoutById(const FString& LayoutId) const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryListResult ListLayouts() const;

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryRemoveResult RemoveLayout(const FString& LayoutId);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	FSQLUILayoutRepositoryClearResult ClearLayouts();

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository", meta = (AllowPrivateAccess = "true"))
	FSQLUISQLiteLayoutRepositorySettings Settings;
};
