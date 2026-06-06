#include "Layout/SQLUIPersistenceStatus.h"

#include "Database/SQLUISQLiteLayoutSchemaVersioning.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Layout/SQLUILayoutRepositoryDatabaseManagement.h"
#include "Layout/SQLUILayoutRepositoryRuntimeProvider.h"
#include "Layout/SQLUILayoutRepositoryRuntimeSettings.h"
#include "Layout/SQLUILayoutRepositoryRuntimeSubsystem.h"
#include "Misc/CommandLine.h"

namespace
{
FString SQLUIPersistenceStatusBackendName(
	const ESQLUILayoutRepositoryBackend Backend)
{
	switch (Backend)
	{
	case ESQLUILayoutRepositoryBackend::Unavailable:
		return TEXT("Unavailable");
	case ESQLUILayoutRepositoryBackend::InMemory:
		return TEXT("InMemory");
	case ESQLUILayoutRepositoryBackend::JsonFile:
		return TEXT("JsonFile");
	case ESQLUILayoutRepositoryBackend::SQLite:
		return TEXT("SQLite");
	default:
		return TEXT("Unknown");
	}
}

void AppendSQLUIPersistenceStatusText(
	FString& Target,
	const FString& Text)
{
	if (Text.IsEmpty())
	{
		return;
	}

	if (!Target.IsEmpty())
	{
		Target += TEXT(" ");
	}

	Target += Text;
}

USQLUILayoutRepositoryRuntimeProvider*
FindSQLUIPersistenceStatusProvider(UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject) || GEngine == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(
		WorldContextObject,
		EGetWorldErrorMode::ReturnNull);
	if (World == nullptr)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		return nullptr;
	}

	const USQLUILayoutRepositoryRuntimeSubsystem* Subsystem =
		GameInstance->GetSubsystem<USQLUILayoutRepositoryRuntimeSubsystem>();
	return IsValid(Subsystem) ? Subsystem->GetProvider() : nullptr;
}

void PopulateSQLUIPersistenceStatusDatabaseFields(
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig,
	FSQLUIPersistenceStatusSnapshot& Snapshot)
{
	FSQLUILayoutRepositoryDatabaseStatusRequest DatabaseStatusRequest;
	DatabaseStatusRequest.RuntimeConfig = RuntimeConfig;
	DatabaseStatusRequest.bIncludeSidecars = true;

	const FSQLUILayoutRepositoryDatabaseStatusResult DatabaseStatus =
		FSQLUILayoutRepositoryDatabaseManagement::GetStatus(DatabaseStatusRequest);
	Snapshot.bSucceeded = DatabaseStatus.bSucceeded;

	if (!DatabaseStatus.ErrorMessage.IsEmpty())
	{
		AppendSQLUIPersistenceStatusText(
			Snapshot.WarningText,
			DatabaseStatus.ErrorMessage);
	}

	if (!DatabaseStatus.bBackendIsSQLite)
	{
		Snapshot.StatusText =
			FString::Printf(
				TEXT("Configured backend is %s; SQLite database status is not applicable."),
				*Snapshot.ConfiguredBackendName);
		return;
	}

	Snapshot.bSQLiteDatabasePathResolved = DatabaseStatus.bDatabasePathResolved;
	Snapshot.ResolvedSQLiteDatabasePath = DatabaseStatus.DatabasePath;
	Snapshot.bSQLiteDatabaseExists = DatabaseStatus.bDatabaseExists;
	Snapshot.SQLiteDatabaseSizeBytes = DatabaseStatus.DatabaseFileSizeBytes;
	Snapshot.bSQLiteJournalExists = DatabaseStatus.bJournalExists;
	Snapshot.bSQLiteWalExists = DatabaseStatus.bWalExists;
	Snapshot.bSQLiteShmExists = DatabaseStatus.bShmExists;
	Snapshot.bSQLiteSidecarsPresent =
		DatabaseStatus.bJournalExists
		|| DatabaseStatus.bWalExists
		|| DatabaseStatus.bShmExists;

	TArray<FString> SidecarNames;
	if (DatabaseStatus.bJournalExists)
	{
		SidecarNames.Add(TEXT("journal"));
	}
	if (DatabaseStatus.bWalExists)
	{
		SidecarNames.Add(TEXT("wal"));
	}
	if (DatabaseStatus.bShmExists)
	{
		SidecarNames.Add(TEXT("shm"));
	}
	Snapshot.SQLiteSidecarSummary =
		SidecarNames.Num() > 0
			? FString::Join(SidecarNames, TEXT(", "))
			: FString();

	if (!DatabaseStatus.bDatabasePathResolved)
	{
		Snapshot.StatusText = TEXT("SQLite is configured but no database path is resolved.");
		return;
	}

	if (!DatabaseStatus.bDatabaseExists)
	{
		Snapshot.StatusText = TEXT("SQLite database path is resolved but the database file does not exist.");
		return;
	}

	const FSQLUISQLiteLayoutSchemaVersionStatus MigrationStatus =
		FSQLUISQLiteLayoutSchemaVersioning::GetLayoutSchemaVersionStatus(
			DatabaseStatus.DatabasePath);
	Snapshot.bMigrationStatusChecked = true;
	Snapshot.bMigrationStatusSucceeded = MigrationStatus.bSucceeded;
	Snapshot.bSQLiteSchemaObjectsReady = MigrationStatus.bSchemaObjectsReady;
	Snapshot.bSQLiteHasPendingMigrations = MigrationStatus.bHasPendingMigrations;
	Snapshot.LatestKnownMigrationId = MigrationStatus.LatestKnownMigrationId;
	Snapshot.LatestAppliedMigrationId = MigrationStatus.LatestAppliedMigrationId;

	if (!MigrationStatus.ErrorMessage.IsEmpty())
	{
		AppendSQLUIPersistenceStatusText(
			Snapshot.WarningText,
			MigrationStatus.ErrorMessage);
	}

	Snapshot.StatusText = MigrationStatus.bSucceeded
		? TEXT("SQLite database exists and schema status was read.")
		: TEXT("SQLite database exists but schema status reported warnings or errors.");
}
}

FSQLUIPersistenceStatusSnapshot
USQLUIPersistenceStatusLibrary::GetPersistenceStatus(UObject* WorldContextObject)
{
	const USQLUILayoutRepositoryRuntimeSettings* RuntimeSettings =
		GetDefault<USQLUILayoutRepositoryRuntimeSettings>();
	const FSQLUILayoutRepositoryRuntimeConfig RuntimeConfig =
		FSQLUILayoutRepositoryRuntimeSettingsPolicy::
			MakeRuntimeConfigFromSettingsAndCommandLine(
				RuntimeSettings,
				FCommandLine::Get());

	return GetPersistenceStatusForProvider(
		RuntimeConfig,
		FindSQLUIPersistenceStatusProvider(WorldContextObject));
}

FSQLUIPersistenceStatusSnapshot
USQLUIPersistenceStatusLibrary::GetPersistenceStatusFromRuntimeConfig(
	UObject* WorldContextObject,
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	return GetPersistenceStatusForProvider(
		RuntimeConfig,
		FindSQLUIPersistenceStatusProvider(WorldContextObject));
}

FSQLUIPersistenceStatusSnapshot
USQLUIPersistenceStatusLibrary::GetPersistenceStatusForProvider(
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig,
	USQLUILayoutRepositoryRuntimeProvider* Provider)
{
	FSQLUIPersistenceStatusSnapshot Snapshot;
	Snapshot.ConfiguredBackend = RuntimeConfig.Backend;
	Snapshot.ConfiguredBackendName =
		SQLUIPersistenceStatusBackendName(RuntimeConfig.Backend);

	if (IsValid(Provider))
	{
		Snapshot.bProviderInitialized = Provider->WasLastInitializationSuccessful();
		Snapshot.bRepositoryActive = Provider->HasRepository();
		Snapshot.ActiveBackend = Provider->GetActiveBackend();
	}
	else
	{
		Snapshot.ActiveBackend = ESQLUILayoutRepositoryBackend::Unavailable;
	}

	Snapshot.ActiveBackendName =
		SQLUIPersistenceStatusBackendName(Snapshot.ActiveBackend);

	PopulateSQLUIPersistenceStatusDatabaseFields(RuntimeConfig, Snapshot);
	return Snapshot;
}
