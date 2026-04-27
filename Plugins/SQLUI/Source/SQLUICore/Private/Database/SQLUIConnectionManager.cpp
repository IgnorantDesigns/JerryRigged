#include "Database/SQLUIConnectionManager.h"

#include "Database/SQLUIAsyncService.h"
#include "Database/SQLUISettings.h"
#include "Database/SQLUIStubBackendService.h"

void USQLUIConnectionManager::Deinitialize()
{
	ShutdownAll();
	Super::Deinitialize();
}

USQLUIAsyncService* USQLUIConnectionManager::GetOrCreate(const FString& DbKey, const FString& DbPath)
{
	if (DbKey.IsEmpty())
	{
		return nullptr;
	}

	if (TObjectPtr<USQLUIAsyncService>* ExistingService = Services.Find(DbKey))
	{
		if (IsValid(*ExistingService))
		{
			return ExistingService->Get();
		}
	}

	const USQLUISettings* Settings = GetDefault<USQLUISettings>();
	FString UnavailableReason = TEXT("SQLUI SQLite backend is unavailable: no concrete backend is implemented yet.");
	if (Settings && !Settings->bEnableSQLite)
	{
		UnavailableReason = TEXT("SQLUI SQLite backend is disabled by config.");
	}
	else if (Settings && Settings->bEnableSQLite && !Settings->bUseStubBackendWhenSQLiteUnavailable)
	{
		UnavailableReason = TEXT("SQLUI SQLite backend is enabled, but no concrete backend is implemented yet.");
	}

	TSharedPtr<ISQLUIBackendService, ESPMode::ThreadSafe> Backend =
		MakeShared<FSQLUIStubBackendService, ESPMode::ThreadSafe>(UnavailableReason);

	USQLUIAsyncService* NewService = NewObject<USQLUIAsyncService>(this);
	NewService->Initialize(DbKey, DbPath, Backend);
	Services.Add(DbKey, NewService);

	return NewService;
}

void USQLUIConnectionManager::ShutdownAll()
{
	for (const TPair<FString, TObjectPtr<USQLUIAsyncService>>& ServicePair : Services)
	{
		if (IsValid(ServicePair.Value))
		{
			ServicePair.Value->Shutdown();
		}
	}

	Services.Empty();
}
