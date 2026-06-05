#include "Layout/SQLUILayoutRepositoryRuntimeIntegration.h"

#include "Database/SQLUISQLiteSeedDatabaseCopy.h"
#include "Layout/SQLUILayoutRepository.h"
#include "Layout/SQLUILayoutRepositoryFactory.h"
#include "Layout/SQLUISQLiteLayoutRepository.h"

namespace
{
void AppendSQLUILayoutRepositoryRuntimeIntegrationError(
	FSQLUILayoutRepositoryRuntimeIntegrationResult& Result,
	const FString& ErrorMessage)
{
	if (ErrorMessage.IsEmpty())
	{
		return;
	}

	if (!Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage += TEXT(" ");
	}

	Result.ErrorMessage += ErrorMessage;
}

bool DoesSQLUILayoutRepositoryRuntimeIntegrationRequestSeedCopy(
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	return RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::SQLite
		&& (RuntimeConfig.bSQLiteCopySeedIfMissing
			|| RuntimeConfig.bSQLiteOverwriteDatabaseFromSeed);
}
}

FSQLUILayoutRepositoryRuntimeIntegrationResult
FSQLUILayoutRepositoryRuntimeIntegration::CreateRepository(
	UObject* Outer,
	const FSQLUILayoutRepositoryRuntimeIntegrationRequest& Request)
{
	FSQLUILayoutRepositoryRuntimeIntegrationResult Result;
	Result.Backend = Request.RuntimeConfig.Backend;
	Result.SQLiteDatabasePath =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(
			Request.RuntimeConfig.SQLiteDatabasePath);
	Result.SQLiteSeedDatabasePath =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteSeedDatabasePath(
			Request.RuntimeConfig.SQLiteSeedDatabasePath);

	const bool bSeedCopyRequested =
		DoesSQLUILayoutRepositoryRuntimeIntegrationRequestSeedCopy(Request.RuntimeConfig);
	Result.bSeedCopyRequested =
		Request.bRunSQLiteSeedCopyPolicy
		&& bSeedCopyRequested;
	Result.bSeedCopySkipped = !Result.bSeedCopyRequested;

	if (Result.bSeedCopyRequested)
	{
		const FSQLUISQLiteSeedDatabaseCopyRequest SeedCopyRequest =
			FSQLUILayoutRepositoryRuntimeConfigResolver::ToSeedDatabaseCopyRequest(
				Request.RuntimeConfig);
		const FSQLUISQLiteSeedDatabaseCopyResult SeedCopyResult =
			FSQLUISQLiteSeedDatabaseCopy::CopySeedDatabase(SeedCopyRequest);

		Result.SQLiteDatabasePath = SeedCopyResult.TargetDatabasePath;
		Result.SQLiteSeedDatabasePath = SeedCopyResult.SeedDatabasePath;
		Result.bSeedCopySucceeded = SeedCopyResult.bSucceeded;

		if (!SeedCopyResult.bSucceeded)
		{
			AppendSQLUILayoutRepositoryRuntimeIntegrationError(
				Result,
				SeedCopyResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime integration failed: SQLite seed database copy failed.")
					: SeedCopyResult.ErrorMessage);
			if (Request.bTreatSeedCopyFailureAsFatal)
			{
				return Result;
			}
		}
	}

	const FSQLUILayoutRepositoryFactorySettings FactorySettings =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(
			Request.RuntimeConfig);
	Result.Repository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, FactorySettings);
	Result.bRepositoryCreated = IsValid(Result.Repository.Get());
	if (!Result.bRepositoryCreated)
	{
		AppendSQLUILayoutRepositoryRuntimeIntegrationError(
			Result,
			TEXT("SQLUI layout repository runtime integration failed: repository factory returned no repository."));
		return Result;
	}

	Result.bBackendUnavailable =
		Result.Backend == ESQLUILayoutRepositoryBackend::SQLite
		&& !IsValid(Cast<USQLUISQLiteLayoutRepository>(Result.Repository.Get()));
	if (Result.bBackendUnavailable)
	{
		AppendSQLUILayoutRepositoryRuntimeIntegrationError(
			Result,
			TEXT("SQLUI layout repository runtime integration failed: SQLite repository was requested but no SQLite database path was configured."));
	}

	Result.bSucceeded =
		Result.bRepositoryCreated
		&& !Result.bBackendUnavailable
		&& (!Result.bSeedCopyRequested || Result.bSeedCopySucceeded);
	return Result;
}

FSQLUILayoutRepositoryRuntimeIntegrationResult
FSQLUILayoutRepositoryRuntimeIntegration::CreateRepositoryFromCommandLine(
	UObject* Outer,
	const TCHAR* CommandLine,
	const FSQLUILayoutRepositoryRuntimeConfig& Defaults)
{
	FSQLUILayoutRepositoryRuntimeIntegrationRequest Request;
	Request.RuntimeConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			CommandLine,
			Defaults);
	return CreateRepository(Outer, Request);
}
