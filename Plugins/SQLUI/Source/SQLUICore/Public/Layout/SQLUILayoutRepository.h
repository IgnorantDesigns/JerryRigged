#pragma once

#include "CoreMinimal.h"
#include "Layout/ISQLUILayoutRepository.h"
#include "UObject/Object.h"

#include "SQLUILayoutRepository.generated.h"

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
