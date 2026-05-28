#include "Layout/SQLUILayoutRepositoryFactory.h"

#include "Layout/SQLUIInMemoryLayoutRepository.h"
#include "Layout/SQLUIJsonFileLayoutRepository.h"
#include "UObject/Package.h"

USQLUILayoutRepository* USQLUILayoutRepositoryFactory::CreateLayoutRepository(
	UObject* Outer,
	const FSQLUILayoutRepositoryFactorySettings& Settings)
{
	UObject* ResolvedOuter = IsValid(Outer) ? Outer : GetTransientPackage();

	switch (Settings.Backend)
	{
	case ESQLUILayoutRepositoryBackend::InMemory:
		return NewObject<USQLUIInMemoryLayoutRepository>(ResolvedOuter);

	case ESQLUILayoutRepositoryBackend::JsonFile:
	{
		USQLUIJsonFileLayoutRepository* Repository =
			NewObject<USQLUIJsonFileLayoutRepository>(ResolvedOuter);
		if (IsValid(Repository) && !Settings.JsonFileBaseDirectory.IsEmpty())
		{
			FSQLUIJsonFileLayoutRepositorySettings JsonFileSettings;
			JsonFileSettings.BaseDirectory = Settings.JsonFileBaseDirectory;
			Repository->Configure(JsonFileSettings);
		}
		return Repository;
	}

	case ESQLUILayoutRepositoryBackend::Unavailable:
	default:
		return NewObject<USQLUILayoutRepository>(ResolvedOuter);
	}
}
