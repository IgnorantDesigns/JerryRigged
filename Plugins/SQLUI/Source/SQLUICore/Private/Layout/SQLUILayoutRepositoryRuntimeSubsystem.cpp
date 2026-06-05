#include "Layout/SQLUILayoutRepositoryRuntimeSubsystem.h"

#include "Misc/CommandLine.h"
#include "Layout/SQLUILayoutRepositoryRuntimeSettings.h"

void USQLUILayoutRepositoryRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const USQLUILayoutRepositoryRuntimeSettings* RuntimeSettings =
		GetDefault<USQLUILayoutRepositoryRuntimeSettings>();
	bAutoInitializationRequested =
		FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
			RuntimeSettings,
			FCommandLine::Get());
	bAutoInitializationSuccessful = false;

	if (bAutoInitializationRequested)
	{
		USQLUILayoutRepositoryRuntimeProvider* RuntimeProvider = GetOrCreateProvider();
		if (IsValid(RuntimeProvider))
		{
			const FSQLUILayoutRepositoryRuntimeIntegrationRequest Request =
				FSQLUILayoutRepositoryRuntimeSettingsPolicy::
					MakeRuntimeIntegrationRequestFromSettingsAndCommandLine(
						RuntimeSettings,
						FCommandLine::Get());
			bAutoInitializationSuccessful =
				RuntimeProvider->InitializeRepository(Request);
		}
	}
}

void USQLUILayoutRepositoryRuntimeSubsystem::Deinitialize()
{
	ResetRepository();
	Provider = nullptr;
	bAutoInitializationRequested = false;
	bAutoInitializationSuccessful = false;
	Super::Deinitialize();
}

USQLUILayoutRepositoryRuntimeProvider*
USQLUILayoutRepositoryRuntimeSubsystem::GetProvider() const
{
	return Provider.Get();
}

USQLUILayoutRepositoryRuntimeProvider*
USQLUILayoutRepositoryRuntimeSubsystem::GetOrCreateProvider()
{
	if (!IsValid(Provider.Get()))
	{
		Provider = NewObject<USQLUILayoutRepositoryRuntimeProvider>(this);
	}

	return Provider.Get();
}

USQLUILayoutRepository* USQLUILayoutRepositoryRuntimeSubsystem::GetRepository() const
{
	return IsValid(Provider.Get()) ? Provider->GetRepository() : nullptr;
}

bool USQLUILayoutRepositoryRuntimeSubsystem::HasRepository() const
{
	return IsValid(Provider.Get()) && Provider->HasRepository();
}

bool USQLUILayoutRepositoryRuntimeSubsystem::WasAutoInitializationRequested() const
{
	return bAutoInitializationRequested;
}

bool USQLUILayoutRepositoryRuntimeSubsystem::WasAutoInitializationSuccessful() const
{
	return bAutoInitializationSuccessful;
}

bool USQLUILayoutRepositoryRuntimeSubsystem::InitializeRepositoryFromRuntimeConfig(
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	USQLUILayoutRepositoryRuntimeProvider* RuntimeProvider = GetOrCreateProvider();
	return IsValid(RuntimeProvider)
		&& RuntimeProvider->InitializeRepositoryFromRuntimeConfig(RuntimeConfig);
}

bool USQLUILayoutRepositoryRuntimeSubsystem::InitializeRepositoryFromCommandLine(
	const FString& CommandLine,
	const FSQLUILayoutRepositoryRuntimeConfig& Defaults)
{
	USQLUILayoutRepositoryRuntimeProvider* RuntimeProvider = GetOrCreateProvider();
	return IsValid(RuntimeProvider)
		&& RuntimeProvider->InitializeRepositoryFromCommandLine(CommandLine, Defaults);
}

void USQLUILayoutRepositoryRuntimeSubsystem::ResetRepository()
{
	if (IsValid(Provider.Get()))
	{
		Provider->ResetRepository();
	}

	bAutoInitializationSuccessful = false;
}

ESQLUILayoutRepositoryBackend
USQLUILayoutRepositoryRuntimeSubsystem::GetActiveBackend() const
{
	return IsValid(Provider.Get())
		? Provider->GetActiveBackend()
		: ESQLUILayoutRepositoryBackend::Unavailable;
}

FSQLUILayoutRepositoryRuntimeIntegrationResult
USQLUILayoutRepositoryRuntimeSubsystem::GetLastIntegrationResult() const
{
	return IsValid(Provider.Get())
		? Provider->GetLastIntegrationResult()
		: FSQLUILayoutRepositoryRuntimeIntegrationResult();
}
