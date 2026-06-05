#include "Layout/SQLUILayoutRepositoryRuntimeProvider.h"

bool USQLUILayoutRepositoryRuntimeProvider::InitializeRepository(
	const FSQLUILayoutRepositoryRuntimeIntegrationRequest& Request)
{
	ActiveRepository = nullptr;
	LastIntegrationResult = FSQLUILayoutRepositoryRuntimeIntegrationResult();
	LastIntegrationResult =
		FSQLUILayoutRepositoryRuntimeIntegration::CreateRepository(this, Request);
	ActiveRepository = LastIntegrationResult.Repository;
	return LastIntegrationResult.bSucceeded;
}

bool USQLUILayoutRepositoryRuntimeProvider::InitializeRepositoryFromRuntimeConfig(
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	FSQLUILayoutRepositoryRuntimeIntegrationRequest Request;
	Request.RuntimeConfig = RuntimeConfig;
	return InitializeRepository(Request);
}

bool USQLUILayoutRepositoryRuntimeProvider::InitializeRepositoryFromCommandLine(
	const FString& CommandLine,
	const FSQLUILayoutRepositoryRuntimeConfig& Defaults)
{
	FSQLUILayoutRepositoryRuntimeIntegrationRequest Request;
	Request.RuntimeConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			*CommandLine,
			Defaults);
	return InitializeRepository(Request);
}

USQLUILayoutRepository* USQLUILayoutRepositoryRuntimeProvider::GetRepository() const
{
	return ActiveRepository.Get();
}

bool USQLUILayoutRepositoryRuntimeProvider::HasRepository() const
{
	return IsValid(ActiveRepository.Get());
}

bool USQLUILayoutRepositoryRuntimeProvider::WasLastInitializationSuccessful() const
{
	return LastIntegrationResult.bSucceeded;
}

ESQLUILayoutRepositoryBackend USQLUILayoutRepositoryRuntimeProvider::GetActiveBackend() const
{
	return LastIntegrationResult.Backend;
}

FSQLUILayoutRepositoryRuntimeIntegrationResult
USQLUILayoutRepositoryRuntimeProvider::GetLastIntegrationResult() const
{
	return LastIntegrationResult;
}

void USQLUILayoutRepositoryRuntimeProvider::ResetRepository()
{
	ActiveRepository = nullptr;
	LastIntegrationResult = FSQLUILayoutRepositoryRuntimeIntegrationResult();
}
