#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "SQLUIConnectionManager.generated.h"

class USQLUIAsyncService;

UCLASS()
class SQLUICORE_API USQLUIConnectionManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	USQLUIAsyncService* GetOrCreate(const FString& DbKey, const FString& DbPath);
	void ShutdownAll();

private:
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<USQLUIAsyncService>> Services;
};
