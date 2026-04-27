#pragma once

#include "CoreMinimal.h"
#include "Database/ISQLUIBackendService.h"
#include "UObject/Object.h"

#include "SQLUIAsyncService.generated.h"

UCLASS()
class SQLUICORE_API USQLUIAsyncService : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(
		const FString& InDbKey,
		const FString& InDatabasePath,
		TSharedPtr<ISQLUIBackendService, ESPMode::ThreadSafe> InBackend);

	void Shutdown();
	void ExecuteAsync(const FSQLUIQueryRequest& Request, FSQLUIQueryCompleteDelegate Callback);

	bool IsInitialized() const;
	bool IsShuttingDown() const;
	const FString& GetDbKey() const;
	const FString& GetDatabasePath() const;

protected:
	virtual void BeginDestroy() override;

private:
	FString DbKey;
	FString DatabasePath;
	TSharedPtr<ISQLUIBackendService, ESPMode::ThreadSafe> Backend;
	bool bIsShuttingDown = false;
};
