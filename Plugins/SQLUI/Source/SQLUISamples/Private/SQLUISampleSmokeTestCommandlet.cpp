#include "SQLUISampleSmokeTestCommandlet.h"

#include "SQLUISamplesModule.h"
#include "SQLUISampleSmokeTestRunner.h"
#include "Engine/World.h"
#include "Misc/Parse.h"

namespace
{
const TCHAR* SQLUISampleSmokeTestCommandletWorldName = TEXT("SQLUISampleSmokeTestCommandletWorld");

const TCHAR* SQLUISampleSmokeTestStepStatusToString(
	const ESQLUIRuntimeWidgetPipelineStepStatus Status)
{
	switch (Status)
	{
	case ESQLUIRuntimeWidgetPipelineStepStatus::NotRun:
		return TEXT("NotRun");
	case ESQLUIRuntimeWidgetPipelineStepStatus::Skipped:
		return TEXT("Skipped");
	case ESQLUIRuntimeWidgetPipelineStepStatus::Succeeded:
		return TEXT("Succeeded");
	case ESQLUIRuntimeWidgetPipelineStepStatus::Failed:
		return TEXT("Failed");
	default:
		return TEXT("Unknown");
	}
}

void LogSQLUISampleSmokeTestErrors(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test error: %s"),
			*Message);
	}
}

void LogSQLUISampleSmokeTestWarnings(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample smoke test warning: %s"),
			*Message);
	}
}

void LogSQLUISampleSmokeTestJsonFixtureMessages(
	const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test JSON fixture validation error: %s"),
			*Error);
	}

	for (const FString& Warning : Validation.Warnings)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample smoke test JSON fixture validation warning: %s"),
			*Warning);
	}
}

void LogSQLUISampleSmokeTestJsonFixtureResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedJsonLayoutFixture)
	{
		return;
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test JSON fixture parse %s."),
		Result.bJsonLayoutFixtureParseSucceeded ? TEXT("succeeded") : TEXT("failed"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test JSON fixture validation %s."),
		Result.bJsonLayoutFixtureValidationSucceeded ? TEXT("succeeded") : TEXT("failed"));

	LogSQLUISampleSmokeTestJsonFixtureMessages(Result.JsonLayoutFixtureValidation);
}

void LogSQLUISampleSmokeTestRepositorySelectionResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bRepositorySelectionSmokeTested)
	{
		return;
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test layout repository factory unavailable backend %s. SaveBackendUnavailable=%s LoadBackendUnavailable=%s"),
		Result.bUnavailableRepositorySelectionSucceeded ? TEXT("succeeded") : TEXT("failed"),
		Result.bUnavailableRepositorySaveBackendUnavailable ? TEXT("true") : TEXT("false"),
		Result.bUnavailableRepositoryLoadBackendUnavailable ? TEXT("true") : TEXT("false"));

	if (!Result.UnavailableRepositorySelectionErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test layout repository factory unavailable backend error: %s"),
			*Result.UnavailableRepositorySelectionErrorMessage);
	}
}

void LogSQLUISampleSmokeTestRepositoryValidationMessages(
	const TCHAR* OperationName,
	const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test in-memory layout repository %s validation error: %s"),
			OperationName,
			*Error);
	}

	for (const FString& Warning : Validation.Warnings)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample smoke test in-memory layout repository %s validation warning: %s"),
			OperationName,
			*Warning);
	}
}

void LogSQLUISampleSmokeTestRepositoryOperationResult(
	const TCHAR* RepositoryName,
	const FSQLUISampleRepositoryOperationSmokeResult& Result)
{
	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test %s list after save %s. LayoutCount=%d MetadataFound=%s"),
		RepositoryName,
		Result.bListAfterSaveSucceeded ? TEXT("succeeded") : TEXT("failed"),
		Result.ListAfterSaveLayoutCount,
		Result.bSavedLayoutMetadataFound ? TEXT("true") : TEXT("false"));

	if (!Result.ListAfterSaveErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test %s list after save error: %s"),
			RepositoryName,
			*Result.ListAfterSaveErrorMessage);
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test %s remove %s. Removed=%s"),
		RepositoryName,
		Result.bRemoveSucceeded ? TEXT("succeeded") : TEXT("failed"),
		Result.bSavedLayoutRemoved ? TEXT("true") : TEXT("false"));

	if (!Result.RemoveErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test %s remove error: %s"),
			RepositoryName,
			*Result.RemoveErrorMessage);
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test %s list after remove %s. LayoutCount=%d MetadataAbsent=%s"),
		RepositoryName,
		Result.bListAfterRemoveSucceeded ? TEXT("succeeded") : TEXT("failed"),
		Result.ListAfterRemoveLayoutCount,
		Result.bRemovedLayoutMetadataAbsent ? TEXT("true") : TEXT("false"));

	if (!Result.ListAfterRemoveErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test %s list after remove error: %s"),
			RepositoryName,
			*Result.ListAfterRemoveErrorMessage);
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test %s clear %s. RemovedCount=%d ExpectedRemovedCount=%d"),
		RepositoryName,
		Result.bClearSucceeded ? TEXT("succeeded") : TEXT("failed"),
		Result.ClearRemovedCount,
		Result.ExpectedClearRemovedCount);

	if (!Result.ClearErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test %s clear error: %s"),
			RepositoryName,
			*Result.ClearErrorMessage);
	}
}

void LogSQLUISampleSmokeTestRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedInMemoryLayoutRepository)
	{
		return;
	}

	UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample smoke test in-memory layout repository selected."));
	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test in-memory layout repository save %s. LayoutId='%s'"),
		Result.bRepositorySaveSucceeded ? TEXT("succeeded") : TEXT("failed"),
		*Result.SavedLayoutId);

	if (!Result.RepositorySaveErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test in-memory layout repository save error: %s"),
			*Result.RepositorySaveErrorMessage);
	}

	LogSQLUISampleSmokeTestRepositoryValidationMessages(
		TEXT("save"),
		Result.RepositorySaveValidation);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test in-memory layout repository load %s. LayoutId='%s'"),
		Result.bRepositoryLoadSucceeded ? TEXT("succeeded") : TEXT("failed"),
		*Result.LoadedLayoutId);

	if (!Result.RepositoryLoadErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test in-memory layout repository load error: %s"),
			*Result.RepositoryLoadErrorMessage);
	}

	LogSQLUISampleSmokeTestRepositoryValidationMessages(
		TEXT("load"),
		Result.RepositoryLoadValidation);

	if (Result.bRepositorySaveSucceeded)
	{
		LogSQLUISampleSmokeTestRepositoryOperationResult(
			TEXT("in-memory layout repository"),
			Result.RepositoryOperationSmoke);
	}
}

void LogSQLUISampleSmokeTestJsonFileRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedJsonFileLayoutRepository)
	{
		return;
	}

	UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample smoke test JSON file layout repository selected."));
	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test JSON file layout repository save %s. LayoutId='%s'"),
		Result.bJsonFileRepositorySaveSucceeded ? TEXT("succeeded") : TEXT("failed"),
		*Result.JsonFileRepositorySavedLayoutId);

	if (!Result.JsonFileRepositorySaveErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test JSON file layout repository save error: %s"),
			*Result.JsonFileRepositorySaveErrorMessage);
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test JSON file layout repository load %s. LayoutId='%s'"),
		Result.bJsonFileRepositoryLoadSucceeded ? TEXT("succeeded") : TEXT("failed"),
		*Result.JsonFileRepositoryLoadedLayoutId);

	if (!Result.JsonFileRepositoryLoadErrorMessage.IsEmpty())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test JSON file layout repository load error: %s"),
			*Result.JsonFileRepositoryLoadErrorMessage);
	}

	if (Result.bJsonFileRepositorySaveSucceeded)
	{
		LogSQLUISampleSmokeTestRepositoryOperationResult(
			TEXT("JSON file layout repository"),
			Result.JsonFileRepositoryOperationSmoke);
	}
}

void LogSQLUISampleSmokeTestSQLiteCoreProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteCoreProbe)
	{
		return;
	}

	const FSQLUISQLiteProbeResult& ProbeResult = Result.SQLiteCoreProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLiteCore probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLiteCore probe database opened: %s"),
		ProbeResult.bDatabaseOpened ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLiteCore probe database closed: %s"),
		ProbeResult.bDatabaseClosed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLiteCore probe database removed: %s"),
		ProbeResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLiteCore probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLiteCore probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestDatabaseAsyncProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedDatabaseAsyncProbe)
	{
		return;
	}

	const FSQLUIDatabaseAsyncResult& ProbeResult = Result.DatabaseAsyncProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async probe background work completed: %s"),
		ProbeResult.bBackgroundWorkCompleted ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async probe callback delivered: %s"),
		ProbeResult.bDeliveredOnGameThread ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async probe worker thread id: %d"),
		ProbeResult.WorkerThreadId);

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI database async probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI database async probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestDatabaseAsyncQueueShutdownProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedDatabaseAsyncQueueShutdownProbe)
	{
		return;
	}

	const FSQLUISampleDatabaseAsyncQueueShutdownProbeResult& ProbeResult =
		Result.DatabaseAsyncQueueShutdownProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async queue shutdown probe queue created: %s"),
		ProbeResult.bQueueCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async queue shutdown probe shutdown requested: %s"),
		ProbeResult.bShutdownRequested ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async queue shutdown probe queue reported shutdown: %s"),
		ProbeResult.bQueueReportedShutdown ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async queue shutdown probe enqueue after shutdown rejected: %s"),
		ProbeResult.bEnqueueAfterShutdownRejected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async queue shutdown probe pending work suppressed: %s"),
		ProbeResult.bPendingWorkSuppressed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async queue shutdown probe running completion suppressed: %s"),
		ProbeResult.bRunningCompletionSuppressed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async queue shutdown probe no callbacks delivered after shutdown: %s"),
		ProbeResult.bNoCallbacksDeliveredAfterShutdown ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI database async queue shutdown probe no deadlock: %s"),
		ProbeResult.bNoDeadlock ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI database async queue shutdown probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI database async queue shutdown probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestLayoutRepositoryRuntimeConfigProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedLayoutRepositoryRuntimeConfigProbe)
	{
		return;
	}

	const FSQLUISampleLayoutRepositoryRuntimeConfigProbeResult& ProbeResult =
		Result.LayoutRepositoryRuntimeConfigProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe default backend in-memory: %s"),
		ProbeResult.bDefaultBackendInMemory ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe JSON file backend parsed: %s"),
		ProbeResult.bJsonFileBackendParsed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe SQLite backend parsed: %s"),
		ProbeResult.bSQLiteBackendParsed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe relative SQLite path resolved under Saved: %s"),
		ProbeResult.bRelativeSQLitePathResolvedUnderSaved ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe absolute SQLite path preserved: %s"),
		ProbeResult.bAbsoluteSQLitePathPreserved ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe SQLite flags parsed: %s"),
		ProbeResult.bSQLiteFlagsParsed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe SQLite seed flags parsed: %s"),
		ProbeResult.bSQLiteSeedFlagsParsed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe SQLite seed copy request mapped: %s"),
		ProbeResult.bSQLiteSeedCopyRequestMapped ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe SQLite missing path unavailable: %s"),
		ProbeResult.bSQLiteMissingPathUnavailable ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe invalid backend falls back to default: %s"),
		ProbeResult.bInvalidBackendFallsBackToDefault ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe factory created SQLite repository: %s"),
		ProbeResult.bFactoryCreatedSQLiteRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe factory SQLite save succeeded: %s"),
		ProbeResult.bFactorySQLiteSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime config probe database removed: %s"),
		ProbeResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout repository runtime config probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI layout repository runtime config probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestLayoutRepositoryRuntimeIntegrationProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedLayoutRepositoryRuntimeIntegrationProbe)
	{
		return;
	}

	const FSQLUISampleLayoutRepositoryRuntimeIntegrationProbeResult& ProbeResult =
		Result.LayoutRepositoryRuntimeIntegrationProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed database path: '%s'"),
		*ProbeResult.SeedDatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed target database path: '%s'"),
		*ProbeResult.SeedTargetDatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe default created repository: %s"),
		ProbeResult.bDefaultCreatedRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe default backend in-memory: %s"),
		ProbeResult.bDefaultBackendInMemory ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe default not SQLite: %s"),
		ProbeResult.bDefaultNotSQLite ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe SQLite created repository: %s"),
		ProbeResult.bSQLiteCreatedRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe SQLite repository created: %s"),
		ProbeResult.bSQLiteRepositoryCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe SQLite save succeeded: %s"),
		ProbeResult.bSQLiteSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe SQLite database created: %s"),
		ProbeResult.bSQLiteDatabaseCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe SQLite list succeeded: %s"),
		ProbeResult.bSQLiteListSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe SQLite listed metadata found: %s"),
		ProbeResult.bSQLiteListedMetadataFound ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe SQLite missing path unavailable: %s"),
		ProbeResult.bSQLiteMissingPathUnavailable ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe SQLite missing path did not create DB: %s"),
		ProbeResult.bSQLiteMissingPathDidNotCreateDb ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed database prepared: %s"),
		ProbeResult.bSeedDatabasePrepared ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed copy requested: %s"),
		ProbeResult.bSeedCopyRequested ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed copy succeeded: %s"),
		ProbeResult.bSeedCopySucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed copied target readable: %s"),
		ProbeResult.bSeedCopiedTargetReadable ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed copied target loaded layout: %s"),
		ProbeResult.bSeedCopiedTargetLoadedLayout ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed database left intact: %s"),
		ProbeResult.bSeedDatabaseLeftIntact ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed copy failure fatal: %s"),
		ProbeResult.bSeedCopyFailureFatal ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed copy failure did not create repository: %s"),
		ProbeResult.bSeedCopyFailureDidNotCreateRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe seed copy failure did not create target: %s"),
		ProbeResult.bSeedCopyFailureDidNotCreateTarget ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime integration probe database files removed: %s"),
		ProbeResult.bDatabaseFilesRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout repository runtime integration probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI layout repository runtime integration probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestLayoutRepositoryRuntimeProviderProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedLayoutRepositoryRuntimeProviderProbe)
	{
		return;
	}

	const FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult& ProbeResult =
		Result.LayoutRepositoryRuntimeProviderProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe command-line database path: '%s'"),
		*ProbeResult.CommandLineDatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed database path: '%s'"),
		*ProbeResult.SeedDatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed target database path: '%s'"),
		*ProbeResult.SeedTargetDatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe provider created: %s"),
		ProbeResult.bProviderCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe default initialization succeeded: %s"),
		ProbeResult.bDefaultInitializationSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe default backend in-memory: %s"),
		ProbeResult.bDefaultBackendInMemory ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe default repository available: %s"),
		ProbeResult.bDefaultRepositoryAvailable ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe default repository not SQLite: %s"),
		ProbeResult.bDefaultRepositoryNotSQLite ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe reset cleared repository: %s"),
		ProbeResult.bResetClearedRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe reinitialize after reset succeeded: %s"),
		ProbeResult.bReinitializeAfterResetSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe SQLite initialization succeeded: %s"),
		ProbeResult.bSQLiteInitializationSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe SQLite backend selected: %s"),
		ProbeResult.bSQLiteBackendSelected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe SQLite repository available: %s"),
		ProbeResult.bSQLiteRepositoryAvailable ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe SQLite save succeeded: %s"),
		ProbeResult.bSQLiteSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe SQLite list succeeded: %s"),
		ProbeResult.bSQLiteListSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe SQLite load succeeded: %s"),
		ProbeResult.bSQLiteLoadSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe SQLite missing path handled: %s"),
		ProbeResult.bSQLiteMissingPathHandled ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe SQLite missing path did not create DB: %s"),
		ProbeResult.bSQLiteMissingPathDidNotCreateDb ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe command-line initialization succeeded: %s"),
		ProbeResult.bCommandLineInitializationSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe command-line SQLite save succeeded: %s"),
		ProbeResult.bCommandLineSQLiteSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed database prepared: %s"),
		ProbeResult.bSeedDatabasePrepared ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed copy initialization succeeded: %s"),
		ProbeResult.bSeedCopyInitializationSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed copy requested: %s"),
		ProbeResult.bSeedCopyRequested ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed copy succeeded: %s"),
		ProbeResult.bSeedCopySucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed copied target readable: %s"),
		ProbeResult.bSeedCopiedTargetReadable ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed database left intact: %s"),
		ProbeResult.bSeedDatabaseLeftIntact ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed copy failure fatal: %s"),
		ProbeResult.bSeedCopyFailureFatal ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed copy failure did not create repository: %s"),
		ProbeResult.bSeedCopyFailureDidNotCreateRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe seed copy failure did not create target: %s"),
		ProbeResult.bSeedCopyFailureDidNotCreateTarget ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime provider probe database files removed: %s"),
		ProbeResult.bDatabaseFilesRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout repository runtime provider probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI layout repository runtime provider probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestLayoutRepositoryRuntimeSettingsProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedLayoutRepositoryRuntimeSettingsProbe)
	{
		return;
	}

	const FSQLUISampleLayoutRepositoryRuntimeSettingsProbeResult& ProbeResult =
		Result.LayoutRepositoryRuntimeSettingsProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe command-line database path: '%s'"),
		*ProbeResult.CommandLineDatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe missing path marker database path: '%s'"),
		*ProbeResult.MissingPathMarkerDatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe default settings safe: %s"),
		ProbeResult.bDefaultSettingsSafe ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe default does not auto initialize: %s"),
		ProbeResult.bDefaultDoesNotAutoInitialize ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe settings in-memory auto init resolved: %s"),
		ProbeResult.bSettingsInMemoryAutoInitResolved ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe settings in-memory repository created: %s"),
		ProbeResult.bSettingsInMemoryRepositoryCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe settings in-memory repository not SQLite: %s"),
		ProbeResult.bSettingsInMemoryRepositoryNotSQLite ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe settings SQLite resolved: %s"),
		ProbeResult.bSettingsSQLiteResolved ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe settings SQLite repository created: %s"),
		ProbeResult.bSettingsSQLiteRepositoryCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe settings SQLite save succeeded: %s"),
		ProbeResult.bSettingsSQLiteSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe settings SQLite list succeeded: %s"),
		ProbeResult.bSettingsSQLiteListSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe settings SQLite load succeeded: %s"),
		ProbeResult.bSettingsSQLiteLoadSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe command-line override resolved SQLite: %s"),
		ProbeResult.bCommandLineOverrideResolvedSQLite ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe command-line override save succeeded: %s"),
		ProbeResult.bCommandLineOverrideSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe command-line override disabled preserved settings: %s"),
		ProbeResult.bCommandLineOverrideDisabledPreservedSettings ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe command-line override disabled did not create DB: %s"),
		ProbeResult.bCommandLineOverrideDisabledDidNotCreateDb ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe SQLite missing path unavailable: %s"),
		ProbeResult.bSQLiteMissingPathUnavailable ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe SQLite missing path did not create DB: %s"),
		ProbeResult.bSQLiteMissingPathDidNotCreateDb ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository runtime settings probe database files removed: %s"),
		ProbeResult.bDatabaseFilesRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout repository runtime settings probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI layout repository runtime settings probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestLayoutPersistenceWorkflowProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedLayoutPersistenceWorkflowProbe)
	{
		return;
	}

	const FSQLUISampleLayoutPersistenceWorkflowProbeResult& ProbeResult =
		Result.LayoutPersistenceWorkflowProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout persistence workflow probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout persistence workflow probe missing repository marker database path: '%s'"),
		*ProbeResult.MissingRepositoryMarkerDatabasePath);

	const auto LogLayoutPersistenceWorkflowBool =
		[&ProbeResult](const TCHAR* Label, bool bValue)
		{
			UE_LOG(
				LogSQLUISamples,
				Log,
				TEXT("SQLUI layout persistence workflow probe %s: %s"),
				Label,
				bValue ? TEXT("true") : TEXT("false"));
		};

	LogLayoutPersistenceWorkflowBool(TEXT("null subsystem save failed"), ProbeResult.bNullSubsystemSaveFailed);
	LogLayoutPersistenceWorkflowBool(TEXT("null subsystem list failed"), ProbeResult.bNullSubsystemListFailed);
	LogLayoutPersistenceWorkflowBool(TEXT("null subsystem load failed"), ProbeResult.bNullSubsystemLoadFailed);
	LogLayoutPersistenceWorkflowBool(TEXT("missing repository save failed"), ProbeResult.bMissingRepositorySaveFailed);
	LogLayoutPersistenceWorkflowBool(TEXT("missing repository list failed"), ProbeResult.bMissingRepositoryListFailed);
	LogLayoutPersistenceWorkflowBool(TEXT("missing repository load failed"), ProbeResult.bMissingRepositoryLoadFailed);
	LogLayoutPersistenceWorkflowBool(TEXT("missing repository did not create DB"), ProbeResult.bMissingRepositoryDidNotCreateDb);
	LogLayoutPersistenceWorkflowBool(TEXT("in-memory initialized"), ProbeResult.bInMemoryInitialized);
	LogLayoutPersistenceWorkflowBool(TEXT("in-memory save succeeded"), ProbeResult.bInMemorySaveSucceeded);
	LogLayoutPersistenceWorkflowBool(TEXT("in-memory list succeeded"), ProbeResult.bInMemoryListSucceeded);
	LogLayoutPersistenceWorkflowBool(TEXT("in-memory listed metadata found"), ProbeResult.bInMemoryListedMetadataFound);
	LogLayoutPersistenceWorkflowBool(TEXT("in-memory load succeeded"), ProbeResult.bInMemoryLoadSucceeded);
	LogLayoutPersistenceWorkflowBool(TEXT("in-memory loaded document valid"), ProbeResult.bInMemoryLoadedDocumentValid);
	LogLayoutPersistenceWorkflowBool(TEXT("in-memory loaded layout id matched"), ProbeResult.bInMemoryLoadedLayoutIdMatched);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite initialized"), ProbeResult.bSQLiteInitialized);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite save succeeded"), ProbeResult.bSQLiteSaveSucceeded);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite database created"), ProbeResult.bSQLiteDatabaseCreated);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite list succeeded"), ProbeResult.bSQLiteListSucceeded);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite listed metadata found"), ProbeResult.bSQLiteListedMetadataFound);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite load succeeded"), ProbeResult.bSQLiteLoadSucceeded);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite loaded document valid"), ProbeResult.bSQLiteLoadedDocumentValid);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite loaded layout id matched"), ProbeResult.bSQLiteLoadedLayoutIdMatched);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite unavailable handled"), ProbeResult.bSQLiteUnavailableHandled);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite unavailable workflow failed clearly"), ProbeResult.bSQLiteUnavailableWorkflowFailedClearly);
	LogLayoutPersistenceWorkflowBool(TEXT("SQLite unavailable did not create DB"), ProbeResult.bSQLiteUnavailableDidNotCreateDb);
	LogLayoutPersistenceWorkflowBool(TEXT("database files removed"), ProbeResult.bDatabaseFilesRemoved);

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout persistence workflow probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI layout persistence workflow probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestLayoutRepositoryDatabaseManagementProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedLayoutRepositoryDatabaseManagementProbe)
	{
		return;
	}

	const FSQLUISampleLayoutRepositoryDatabaseManagementProbeResult& ProbeResult =
		Result.LayoutRepositoryDatabaseManagementProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository database management probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository database management probe sidecar database path: '%s'"),
		*ProbeResult.SidecarDatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI layout repository database management probe resolved relative database path: '%s'"),
		*ProbeResult.ResolvedRelativeDatabasePath);

	const auto LogLayoutRepositoryDatabaseManagementBool =
		[&ProbeResult](const TCHAR* Label, bool bValue)
		{
			UE_LOG(
				LogSQLUISamples,
				Log,
				TEXT("SQLUI layout repository database management probe %s: %s"),
				Label,
				bValue ? TEXT("true") : TEXT("false"));
		};

	LogLayoutRepositoryDatabaseManagementBool(TEXT("non-SQLite status safe"), ProbeResult.bNonSQLiteStatusSafe);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("non-SQLite reset safe"), ProbeResult.bNonSQLiteResetSafe);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite empty path status handled"), ProbeResult.bSQLiteEmptyPathStatusHandled);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite empty path reset failed clearly"), ProbeResult.bSQLiteEmptyPathResetFailedClearly);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite status before create succeeded"), ProbeResult.bSQLiteStatusBeforeCreateSucceeded);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite status before create absent"), ProbeResult.bSQLiteStatusBeforeCreateAbsent);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite status after save detected database"), ProbeResult.bSQLiteStatusAfterSaveDetectedDatabase);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite status after save size positive"), ProbeResult.bSQLiteStatusAfterSaveSizePositive);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite reset succeeded"), ProbeResult.bSQLiteResetSucceeded);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite reset removed database"), ProbeResult.bSQLiteResetRemovedDatabase);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite reset idempotent"), ProbeResult.bSQLiteResetIdempotent);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite status after reset absent"), ProbeResult.bSQLiteStatusAfterResetAbsent);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("SQLite sidecar removal succeeded"), ProbeResult.bSQLiteSidecarRemovalSucceeded);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("relative path resolved under Saved"), ProbeResult.bRelativePathResolvedUnderSaved);
	LogLayoutRepositoryDatabaseManagementBool(TEXT("database files removed"), ProbeResult.bDatabaseFilesRemoved);

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(
			LogSQLUISamples,
			Log,
			TEXT("SQLUI layout repository database management probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI layout repository database management probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteMigrationProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteMigrationProbe)
	{
		return;
	}

	const FSQLUISQLiteMigrationResult& ProbeResult = Result.SQLiteMigrationProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration probe database opened: %s"),
		ProbeResult.bDatabaseOpened ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration probe migration table created: %s"),
		ProbeResult.bMigrationTableCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration probe migration applied: %s"),
		ProbeResult.bMigrationApplied ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration probe migration recorded: %s"),
		ProbeResult.bMigrationRecorded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration probe database closed: %s"),
		ProbeResult.bDatabaseClosed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration probe database removed: %s"),
		ProbeResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite migration probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite migration probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteLayoutSchemaMigrationProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteLayoutSchemaMigrationProbe)
	{
		return;
	}

	const FSQLUISQLiteLayoutSchemaMigrationProbeResult& ProbeResult =
		Result.SQLiteLayoutSchemaMigrationProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout schema migration probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout schema migration probe migration succeeded: %s"),
		ProbeResult.bMigrationSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout schema migration probe layouts table exists: %s"),
		ProbeResult.bLayoutsTableExists ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout schema migration probe layout revisions table exists: %s"),
		ProbeResult.bLayoutRevisionsTableExists ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout schema migration probe layout tags table exists: %s"),
		ProbeResult.bLayoutTagsTableExists ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout schema migration probe layout checkpoints table exists: %s"),
		ProbeResult.bLayoutCheckpointsTableExists ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout schema migration probe layout previews table exists: %s"),
		ProbeResult.bLayoutPreviewsTableExists ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout schema migration probe expected indexes exist: %s"),
		ProbeResult.bExpectedIndexesExist ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout schema migration probe database removed: %s"),
		ProbeResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite layout schema migration probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite layout schema migration probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteLayoutReadProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteLayoutReadProbe)
	{
		return;
	}

	const FSQLUISQLiteLayoutReadProbeResult& ProbeResult =
		Result.SQLiteLayoutReadProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe schema migration succeeded: %s"),
		ProbeResult.bSchemaMigrationSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe seed inserted: %s"),
		ProbeResult.bSeedInserted ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe list succeeded: %s"),
		ProbeResult.bListSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe listed metadata found: %s"),
		ProbeResult.bListedMetadataFound ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe listed layout count: %d"),
		ProbeResult.ListedLayoutCount);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe load succeeded: %s"),
		ProbeResult.bLoadSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe loaded document valid: %s"),
		ProbeResult.bLoadedDocumentValid ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe seed layout id: '%s' loaded layout id: '%s'"),
		*ProbeResult.SeedLayoutId,
		*ProbeResult.LoadedLayoutId);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite layout read probe database removed: %s"),
		ProbeResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite layout read probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite layout read probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteReadOnlyLayoutRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteReadOnlyLayoutRepository)
	{
		return;
	}

	const FSQLUISampleSQLiteReadOnlyLayoutRepositorySmokeResult& RepositoryResult =
		Result.SQLiteReadOnlyLayoutRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository database path: '%s'"),
		*RepositoryResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository prepared database: %s"),
		RepositoryResult.bPreparedDatabase ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository list succeeded: %s"),
		RepositoryResult.bListSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository listed metadata found: %s"),
		RepositoryResult.bListedMetadataFound ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository listed layout count: %d"),
		RepositoryResult.ListedLayoutCount);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository load succeeded: %s"),
		RepositoryResult.bLoadSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository loaded document valid: %s"),
		RepositoryResult.bLoadedDocumentValid ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository save rejected: %s"),
		RepositoryResult.bSaveRejected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository remove rejected: %s"),
		RepositoryResult.bRemoveRejected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository clear rejected: %s"),
		RepositoryResult.bClearRejected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository list after rejected writes succeeded: %s"),
		RepositoryResult.bListAfterRejectedWritesSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository load after rejected writes succeeded: %s"),
		RepositoryResult.bLoadAfterRejectedWritesSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository seed layout id: '%s' loaded layout id: '%s'"),
		*RepositoryResult.SeedLayoutId,
		*RepositoryResult.LoadedLayoutId);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite read-only layout repository database removed: %s"),
		RepositoryResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (RepositoryResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite read-only layout repository succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite read-only layout repository failed: %s"),
			*RepositoryResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteSaveLayoutRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteSaveLayoutRepository)
	{
		return;
	}

	const FSQLUISampleSQLiteSaveLayoutRepositorySmokeResult& RepositoryResult =
		Result.SQLiteSaveLayoutRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository database path: '%s'"),
		*RepositoryResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository database prepared: %s"),
		RepositoryResult.bDatabasePrepared ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository first save succeeded: %s"),
		RepositoryResult.bFirstSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository list after save succeeded: %s"),
		RepositoryResult.bListAfterSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository listed saved metadata found: %s"),
		RepositoryResult.bListedSavedMetadataFound ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository load after save succeeded: %s"),
		RepositoryResult.bLoadAfterSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository loaded document valid: %s"),
		RepositoryResult.bLoadedDocumentValid ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository second save succeeded: %s"),
		RepositoryResult.bSecondSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository list after second save succeeded: %s"),
		RepositoryResult.bListAfterSecondSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository listed updated metadata found: %s"),
		RepositoryResult.bListedUpdatedMetadataFound ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository load after second save succeeded: %s"),
		RepositoryResult.bLoadAfterSecondSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository latest revision loaded: %s"),
		RepositoryResult.bLatestRevisionLoaded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository saved layout id: '%s' loaded layout id: '%s'"),
		*RepositoryResult.SavedLayoutId,
		*RepositoryResult.LoadedLayoutId);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite SaveLayout repository database removed: %s"),
		RepositoryResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (RepositoryResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite SaveLayout repository succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite SaveLayout repository failed: %s"),
			*RepositoryResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteRemoveLayoutRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteRemoveLayoutRepository)
	{
		return;
	}

	const FSQLUISampleSQLiteRemoveLayoutRepositorySmokeResult& RepositoryResult =
		Result.SQLiteRemoveLayoutRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository database path: '%s'"),
		*RepositoryResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository database prepared: %s"),
		RepositoryResult.bDatabasePrepared ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository save succeeded: %s"),
		RepositoryResult.bSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository list before remove succeeded: %s"),
		RepositoryResult.bListBeforeRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository listed metadata found before remove: %s"),
		RepositoryResult.bListedMetadataFoundBeforeRemove ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository load before remove succeeded: %s"),
		RepositoryResult.bLoadBeforeRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository remove succeeded: %s"),
		RepositoryResult.bRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository removed: %s"),
		RepositoryResult.bRemoved ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository list after remove succeeded: %s"),
		RepositoryResult.bListAfterRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository metadata absent after remove: %s"),
		RepositoryResult.bMetadataAbsentAfterRemove ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository load after remove failed as expected: %s"),
		RepositoryResult.bLoadAfterRemoveFailedAsExpected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository revisions preserved: %s"),
		RepositoryResult.bRevisionsPreserved ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository saved layout id: '%s' removed layout id: '%s' loaded layout id: '%s' revision count after remove: %d"),
		*RepositoryResult.SavedLayoutId,
		*RepositoryResult.RemovedLayoutId,
		*RepositoryResult.LoadedLayoutId,
		RepositoryResult.RevisionCountAfterRemove);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite RemoveLayout repository database removed: %s"),
		RepositoryResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (RepositoryResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite RemoveLayout repository succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite RemoveLayout repository failed: %s"),
			*RepositoryResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteClearLayoutsRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteClearLayoutsRepository)
	{
		return;
	}

	const FSQLUISampleSQLiteClearLayoutsRepositorySmokeResult& RepositoryResult =
		Result.SQLiteClearLayoutsRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository database path: '%s'"),
		*RepositoryResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository database prepared: %s"),
		RepositoryResult.bDatabasePrepared ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository first save succeeded: %s"),
		RepositoryResult.bFirstSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository second save succeeded: %s"),
		RepositoryResult.bSecondSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository list before remove succeeded: %s"),
		RepositoryResult.bListBeforeRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository both metadata entries found before remove: %s"),
		RepositoryResult.bBothMetadataEntriesFoundBeforeRemove ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository remove succeeded: %s"),
		RepositoryResult.bRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository removed: %s"),
		RepositoryResult.bRemoved ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository list before clear succeeded: %s"),
		RepositoryResult.bListBeforeClearSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository active metadata preserved before clear: %s"),
		RepositoryResult.bActiveMetadataPreservedBeforeClear ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository removed metadata absent before clear: %s"),
		RepositoryResult.bRemovedMetadataAbsentBeforeClear ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository clear succeeded: %s"),
		RepositoryResult.bClearSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository removed count matched expected: %s"),
		RepositoryResult.bRemovedCountMatchedExpected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository list after clear succeeded: %s"),
		RepositoryResult.bListAfterClearSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository empty after clear: %s"),
		RepositoryResult.bEmptyAfterClear ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository loads after clear failed as expected: %s"),
		RepositoryResult.bLoadsAfterClearFailedAsExpected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository tables empty after clear: %s"),
		RepositoryResult.bTablesEmptyAfterClear ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository table rows after clear: layouts=%d revisions=%d tags=%d checkpoints=%d previews=%d"),
		RepositoryResult.TableRowCountsAfterClear.Layouts,
		RepositoryResult.TableRowCountsAfterClear.LayoutRevisions,
		RepositoryResult.TableRowCountsAfterClear.LayoutTags,
		RepositoryResult.TableRowCountsAfterClear.LayoutCheckpoints,
		RepositoryResult.TableRowCountsAfterClear.LayoutPreviews);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository first layout id: '%s' second layout id: '%s' removed layout id: '%s' removed count: %d"),
		*RepositoryResult.FirstLayoutId,
		*RepositoryResult.SecondLayoutId,
		*RepositoryResult.RemovedLayoutId,
		RepositoryResult.ClearRemovedCount);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite ClearLayouts repository database removed: %s"),
		RepositoryResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (RepositoryResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite ClearLayouts repository succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite ClearLayouts repository failed: %s"),
			*RepositoryResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteFullLifecycleRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteFullLifecycleRepository)
	{
		return;
	}

	const FSQLUISampleSQLiteFullLifecycleRepositorySmokeResult& RepositoryResult =
		Result.SQLiteFullLifecycleRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository database path: '%s'"),
		*RepositoryResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository database prepared: %s"),
		RepositoryResult.bDatabasePrepared ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository first save succeeded: %s"),
		RepositoryResult.bFirstSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository list after first save succeeded: %s"),
		RepositoryResult.bListAfterFirstSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository first metadata found after first save: %s"),
		RepositoryResult.bFirstMetadataFoundAfterFirstSave ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository load after first save succeeded: %s"),
		RepositoryResult.bLoadAfterFirstSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository first revision loaded: %s"),
		RepositoryResult.bFirstRevisionLoaded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository second revision save succeeded: %s"),
		RepositoryResult.bSecondRevisionSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository latest revision loaded: %s"),
		RepositoryResult.bLatestRevisionLoaded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository second layout save succeeded: %s"),
		RepositoryResult.bSecondSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository list before remove succeeded: %s"),
		RepositoryResult.bListBeforeRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository both metadata entries found before remove: %s"),
		RepositoryResult.bBothMetadataEntriesFoundBeforeRemove ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository remove succeeded: %s"),
		RepositoryResult.bRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository removed: %s"),
		RepositoryResult.bRemoved ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository list after remove succeeded: %s"),
		RepositoryResult.bListAfterRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository removed metadata absent after remove: %s"),
		RepositoryResult.bRemovedMetadataAbsentAfterRemove ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository second metadata preserved after remove: %s"),
		RepositoryResult.bSecondMetadataPreservedAfterRemove ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository removed load failed as expected: %s"),
		RepositoryResult.bRemovedLoadFailedAsExpected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository second load after remove succeeded: %s"),
		RepositoryResult.bSecondLoadAfterRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository revision history preserved after remove: %s"),
		RepositoryResult.bRevisionHistoryPreservedAfterRemove ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository clear succeeded: %s"),
		RepositoryResult.bClearSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository removed count matched expected: %s"),
		RepositoryResult.bRemovedCountMatchedExpected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository list after clear succeeded: %s"),
		RepositoryResult.bListAfterClearSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository empty after clear: %s"),
		RepositoryResult.bEmptyAfterClear ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository loads after clear failed as expected: %s"),
		RepositoryResult.bLoadsAfterClearFailedAsExpected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository tables empty after clear: %s"),
		RepositoryResult.bTablesEmptyAfterClear ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository table rows after clear: layouts=%d revisions=%d tags=%d checkpoints=%d previews=%d"),
		RepositoryResult.TableRowCountsAfterClear.Layouts,
		RepositoryResult.TableRowCountsAfterClear.LayoutRevisions,
		RepositoryResult.TableRowCountsAfterClear.LayoutTags,
		RepositoryResult.TableRowCountsAfterClear.LayoutCheckpoints,
		RepositoryResult.TableRowCountsAfterClear.LayoutPreviews);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository first layout id: '%s' second layout id: '%s' removed layout id: '%s' loaded layout id: '%s' revision count after remove: %d removed count: %d"),
		*RepositoryResult.FirstLayoutId,
		*RepositoryResult.SecondLayoutId,
		*RepositoryResult.RemovedLayoutId,
		*RepositoryResult.LoadedLayoutId,
		RepositoryResult.RevisionCountAfterRemove,
		RepositoryResult.ClearRemovedCount);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite full lifecycle repository database removed: %s"),
		RepositoryResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (RepositoryResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite full lifecycle repository succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite full lifecycle repository failed: %s"),
			*RepositoryResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteAsyncCallbackRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteAsyncCallbackRepository)
	{
		return;
	}

	const FSQLUISampleSQLiteAsyncCallbackRepositorySmokeResult& RepositoryResult =
		Result.SQLiteAsyncCallbackRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository database path: '%s'"),
		*RepositoryResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository database prepared: %s"),
		RepositoryResult.bDatabasePrepared ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository save callback delivered: %s"),
		RepositoryResult.bSaveCallbackDelivered ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository save succeeded: %s"),
		RepositoryResult.bSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository load callback delivered: %s"),
		RepositoryResult.bLoadCallbackDelivered ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository load succeeded: %s"),
		RepositoryResult.bLoadSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository loaded document valid: %s"),
		RepositoryResult.bLoadedDocumentValid ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository list after callbacks succeeded: %s"),
		RepositoryResult.bListAfterAsyncCallbacksSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository listed metadata found: %s"),
		RepositoryResult.bListedMetadataFound ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository callbacks delivered on game thread: %s"),
		RepositoryResult.bCallbacksDeliveredOnGameThread ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository saved layout id: '%s' loaded layout id: '%s' listed layout count: %d"),
		*RepositoryResult.SavedLayoutId,
		*RepositoryResult.LoadedLayoutId,
		RepositoryResult.ListedLayoutCount);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite async callback repository database removed: %s"),
		RepositoryResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (RepositoryResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite async callback repository succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite async callback repository failed: %s"),
			*RepositoryResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteSerializedAsyncCallbackRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteSerializedAsyncCallbackRepository)
	{
		return;
	}

	const FSQLUISampleSQLiteSerializedAsyncCallbackRepositorySmokeResult& RepositoryResult =
		Result.SQLiteSerializedAsyncCallbackRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository database path: '%s'"),
		*RepositoryResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository database prepared: %s"),
		RepositoryResult.bDatabasePrepared ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository first save callback delivered: %s"),
		RepositoryResult.bFirstSaveCallbackDelivered ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository first save succeeded: %s"),
		RepositoryResult.bFirstSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository second save callback delivered: %s"),
		RepositoryResult.bSecondSaveCallbackDelivered ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository second save succeeded: %s"),
		RepositoryResult.bSecondSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository load callback delivered: %s"),
		RepositoryResult.bLoadCallbackDelivered ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository load succeeded: %s"),
		RepositoryResult.bLoadSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository callbacks delivered in order: %s"),
		RepositoryResult.bCallbacksDeliveredInOrder ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository callbacks delivered on game thread: %s"),
		RepositoryResult.bCallbacksDeliveredOnGameThread ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository latest revision loaded: %s"),
		RepositoryResult.bLatestRevisionLoaded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository list after serialized callbacks succeeded: %s"),
		RepositoryResult.bListAfterSerializedCallbacksSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository listed updated metadata found: %s"),
		RepositoryResult.bListedUpdatedMetadataFound ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository saved layout id: '%s' loaded layout id: '%s' listed layout count: %d callback order: '%s'"),
		*RepositoryResult.SavedLayoutId,
		*RepositoryResult.LoadedLayoutId,
		RepositoryResult.ListedLayoutCount,
		*FString::Join(RepositoryResult.CallbackOrder, TEXT(",")));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite serialized async callback repository database removed: %s"),
		RepositoryResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (RepositoryResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite serialized async callback repository succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite serialized async callback repository failed: %s"),
			*RepositoryResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteFactoryLayoutRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteFactoryLayoutRepository)
	{
		return;
	}

	const FSQLUISampleSQLiteFactoryLayoutRepositorySmokeResult& RepositoryResult =
		Result.SQLiteFactoryLayoutRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository database path: '%s'"),
		*RepositoryResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository database prepared: %s"),
		RepositoryResult.bDatabasePrepared ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository created repository: %s"),
		RepositoryResult.bCreatedRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository created SQLite repository: %s"),
		RepositoryResult.bCreatedSQLiteRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository save succeeded: %s"),
		RepositoryResult.bSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository list succeeded: %s"),
		RepositoryResult.bListSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository listed metadata found: %s"),
		RepositoryResult.bListedMetadataFound ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository load succeeded: %s"),
		RepositoryResult.bLoadSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository loaded document valid: %s"),
		RepositoryResult.bLoadedDocumentValid ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository remove succeeded: %s"),
		RepositoryResult.bRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository removed: %s"),
		RepositoryResult.bRemoved ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository metadata absent after remove: %s"),
		RepositoryResult.bMetadataAbsentAfterRemove ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository clear succeeded: %s"),
		RepositoryResult.bClearSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository missing path unavailable: %s"),
		RepositoryResult.bMissingPathUnavailable ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository saved layout id: '%s' loaded layout id: '%s' removed layout id: '%s' listed layout count: %d listed after remove count: %d clear removed count: %d"),
		*RepositoryResult.SavedLayoutId,
		*RepositoryResult.LoadedLayoutId,
		*RepositoryResult.RemovedLayoutId,
		RepositoryResult.ListedLayoutCount,
		RepositoryResult.ListedLayoutCountAfterRemove,
		RepositoryResult.ClearRemovedCount);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory layout repository database removed: %s"),
		RepositoryResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (RepositoryResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite factory layout repository succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite factory layout repository failed: %s"),
			*RepositoryResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteFactorySchemaInitRepositoryResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteFactorySchemaInitRepository)
	{
		return;
	}

	const FSQLUISampleSQLiteFactorySchemaInitRepositorySmokeResult& RepositoryResult =
		Result.SQLiteFactorySchemaInitRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository database path: '%s'"),
		*RepositoryResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository database absent before start: %s"),
		RepositoryResult.bDatabaseAbsentBeforeStart ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository created repository: %s"),
		RepositoryResult.bCreatedRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository created SQLite repository: %s"),
		RepositoryResult.bCreatedSQLiteRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository save initialized schema: %s"),
		RepositoryResult.bSaveInitializedSchema ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository database created: %s"),
		RepositoryResult.bDatabaseCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository save succeeded: %s"),
		RepositoryResult.bSaveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository list succeeded: %s"),
		RepositoryResult.bListSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository listed metadata found: %s"),
		RepositoryResult.bListedMetadataFound ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository load succeeded: %s"),
		RepositoryResult.bLoadSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository loaded document valid: %s"),
		RepositoryResult.bLoadedDocumentValid ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository remove succeeded: %s"),
		RepositoryResult.bRemoveSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository clear succeeded: %s"),
		RepositoryResult.bClearSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository missing db without init failed: %s"),
		RepositoryResult.bMissingDbWithoutInitFailed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository missing db without init not created: %s"),
		RepositoryResult.bMissingDbWithoutInitNotCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository saved layout id: '%s' loaded layout id: '%s' listed layout count: %d clear removed count: %d missing database path: '%s'"),
		*RepositoryResult.SavedLayoutId,
		*RepositoryResult.LoadedLayoutId,
		RepositoryResult.ListedLayoutCount,
		RepositoryResult.ClearRemovedCount,
		*RepositoryResult.MissingDatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite factory schema init repository database removed: %s"),
		RepositoryResult.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (RepositoryResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite factory schema init repository succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite factory schema init repository failed: %s"),
			*RepositoryResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteSchemaInitHardeningResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteSchemaInitHardening)
	{
		return;
	}

	const FSQLUISampleSQLiteSchemaInitHardeningSmokeResult& HardeningResult =
		Result.SQLiteSchemaInitHardening;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening missing DB create disabled failed: %s"),
		HardeningResult.bMissingDbCreateDisabledFailed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening missing DB create disabled not created: %s"),
		HardeningResult.bMissingDbCreateDisabledNotCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening empty DB create enabled succeeded: %s"),
		HardeningResult.bEmptyDbCreateEnabledSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening empty DB schema ready: %s"),
		HardeningResult.bEmptyDbSchemaReady ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening already initialized succeeded: %s"),
		HardeningResult.bAlreadyInitializedSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening already initialized detected: %s"),
		HardeningResult.bAlreadyInitializedDetected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening migration row not duplicated: %s"),
		HardeningResult.bMigrationRowNotDuplicated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening complete schema missing migration succeeded: %s"),
		HardeningResult.bCompleteSchemaMissingMigrationSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening complete schema missing migration recorded: %s"),
		HardeningResult.bCompleteSchemaMissingMigrationRecorded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening partial schema failed clearly: %s"),
		HardeningResult.bPartialSchemaFailedClearly ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening partial schema reported missing objects: %s"),
		HardeningResult.bPartialSchemaReportedMissingObjects ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening read-only init blocked: %s"),
		HardeningResult.bReadOnlyInitBlocked ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening read-only init did not create DB: %s"),
		HardeningResult.bReadOnlyInitDidNotCreateDb ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite schema init hardening database files removed: %s"),
		HardeningResult.bDatabaseFilesRemoved ? TEXT("true") : TEXT("false"));

	if (HardeningResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite schema init hardening succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite schema init hardening failed: %s"),
			*HardeningResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestPersistenceStatusSurfaceProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedPersistenceStatusSurfaceProbe)
	{
		return;
	}

	const FSQLUISamplePersistenceStatusSurfaceProbeResult& ProbeResult =
		Result.PersistenceStatusSurfaceProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI persistence status surface probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI persistence status surface probe sidecar database path: '%s'"),
		*ProbeResult.SidecarDatabasePath);

	const auto LogPersistenceStatusSurfaceBool =
		[](const TCHAR* Label, bool bValue)
		{
			UE_LOG(
				LogSQLUISamples,
				Log,
				TEXT("SQLUI persistence status surface probe %s: %s"),
				Label,
				bValue ? TEXT("true") : TEXT("false"));
		};

	LogPersistenceStatusSurfaceBool(TEXT("default status succeeded"), ProbeResult.bDefaultStatusSucceeded);
	LogPersistenceStatusSurfaceBool(TEXT("default backend InMemory"), ProbeResult.bDefaultBackendInMemory);
	LogPersistenceStatusSurfaceBool(TEXT("default provider not initialized"), ProbeResult.bDefaultProviderNotInitialized);
	LogPersistenceStatusSurfaceBool(TEXT("default repository not active"), ProbeResult.bDefaultRepositoryNotActive);
	LogPersistenceStatusSurfaceBool(TEXT("default status did not create DB"), ProbeResult.bDefaultStatusDidNotCreateDb);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite database prepared"), ProbeResult.bSQLiteDatabasePrepared);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite status succeeded"), ProbeResult.bSQLiteStatusSucceeded);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite path resolved"), ProbeResult.bSQLitePathResolved);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite database detected"), ProbeResult.bSQLiteDatabaseDetected);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite database size positive"), ProbeResult.bSQLiteDatabaseSizePositive);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite sidecars detected"), ProbeResult.bSQLiteSidecarsDetected);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite migration status checked"), ProbeResult.bSQLiteMigrationStatusChecked);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite migration status succeeded"), ProbeResult.bSQLiteMigrationStatusSucceeded);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite schema ready"), ProbeResult.bSQLiteSchemaReady);
	LogPersistenceStatusSurfaceBool(TEXT("SQLite status read-only"), ProbeResult.bSQLiteStatusReadOnly);
	LogPersistenceStatusSurfaceBool(TEXT("database files removed"), ProbeResult.bDatabaseFilesRemoved);

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(
			LogSQLUISamples,
			Log,
			TEXT("SQLUI persistence status surface probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI persistence status surface probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestPersistenceStatusDisplayRowsProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedPersistenceStatusDisplayRowsProbe)
	{
		return;
	}

	const FSQLUISamplePersistenceStatusDisplayRowsProbeResult& ProbeResult =
		Result.PersistenceStatusDisplayRowsProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI persistence status display rows probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI persistence status display rows probe sidecar database path: '%s'"),
		*ProbeResult.SidecarDatabasePath);

	const auto LogPersistenceStatusDisplayRowsBool =
		[](const TCHAR* Label, bool bValue)
		{
			UE_LOG(
				LogSQLUISamples,
				Log,
				TEXT("SQLUI persistence status display rows probe %s: %s"),
				Label,
				bValue ? TEXT("true") : TEXT("false"));
		};

	LogPersistenceStatusDisplayRowsBool(TEXT("default rows generated"), ProbeResult.bDefaultRowsGenerated);
	LogPersistenceStatusDisplayRowsBool(TEXT("default backend row found"), ProbeResult.bDefaultBackendRowFound);
	LogPersistenceStatusDisplayRowsBool(TEXT("default provider row found"), ProbeResult.bDefaultProviderRowFound);
	LogPersistenceStatusDisplayRowsBool(TEXT("default repository row found"), ProbeResult.bDefaultRepositoryRowFound);
	LogPersistenceStatusDisplayRowsBool(TEXT("default SQLite rows graceful"), ProbeResult.bDefaultSQLiteRowsGraceful);
	LogPersistenceStatusDisplayRowsBool(TEXT("default rows did not create DB"), ProbeResult.bDefaultRowsDidNotCreateDb);
	LogPersistenceStatusDisplayRowsBool(TEXT("SQLite database prepared"), ProbeResult.bSQLiteDatabasePrepared);
	LogPersistenceStatusDisplayRowsBool(TEXT("SQLite rows generated"), ProbeResult.bSQLiteRowsGenerated);
	LogPersistenceStatusDisplayRowsBool(TEXT("SQLite path row found"), ProbeResult.bSQLitePathRowFound);
	LogPersistenceStatusDisplayRowsBool(TEXT("SQLite database exists row found"), ProbeResult.bSQLiteDatabaseExistsRowFound);
	LogPersistenceStatusDisplayRowsBool(TEXT("SQLite database size row found"), ProbeResult.bSQLiteDatabaseSizeRowFound);
	LogPersistenceStatusDisplayRowsBool(TEXT("SQLite sidecars row found"), ProbeResult.bSQLiteSidecarsRowFound);
	LogPersistenceStatusDisplayRowsBool(TEXT("SQLite schema row found"), ProbeResult.bSQLiteSchemaRowFound);
	LogPersistenceStatusDisplayRowsBool(TEXT("SQLite rows read-only"), ProbeResult.bSQLiteRowsReadOnly);
	LogPersistenceStatusDisplayRowsBool(TEXT("database files removed"), ProbeResult.bDatabaseFilesRemoved);

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(
			LogSQLUISamples,
			Log,
			TEXT("SQLUI persistence status display rows probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI persistence status display rows probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestPersistenceStatusSampleSurfaceProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedPersistenceStatusSampleSurfaceProbe)
	{
		return;
	}

	const FSQLUISamplePersistenceStatusSampleSurfaceProbeResult& ProbeResult =
		Result.PersistenceStatusSampleSurfaceProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI persistence status sample surface probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI persistence status sample surface probe sidecar database path: '%s'"),
		*ProbeResult.SidecarDatabasePath);

	const auto LogPersistenceStatusSampleSurfaceBool =
		[](const TCHAR* Label, bool bValue)
		{
			UE_LOG(
				LogSQLUISamples,
				Log,
				TEXT("SQLUI persistence status sample surface probe %s: %s"),
				Label,
				bValue ? TEXT("true") : TEXT("false"));
		};

	LogPersistenceStatusSampleSurfaceBool(TEXT("presenter created"), ProbeResult.bPresenterCreated);
	LogPersistenceStatusSampleSurfaceBool(TEXT("panel adapter created"), ProbeResult.bPanelAdapterCreated);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget class derived from UUserWidget"),
		ProbeResult.bPanelWidgetClassDerivedFromUserWidget);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("Blueprint refresh function callable"),
		ProbeResult.bBlueprintRefreshFunctionCallable);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("Blueprint runtime config refresh function callable"),
		ProbeResult.bBlueprintRuntimeConfigRefreshFunctionCallable);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel adapter Blueprint refresh function callable"),
		ProbeResult.bPanelAdapterBlueprintRefreshFunctionCallable);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel adapter Blueprint runtime config refresh function callable"),
		ProbeResult.bPanelAdapterBlueprintRuntimeConfigRefreshFunctionCallable);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget Blueprint refresh function callable"),
		ProbeResult.bPanelWidgetBlueprintRefreshFunctionCallable);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget Blueprint runtime config refresh function callable"),
		ProbeResult.bPanelWidgetBlueprintRuntimeConfigRefreshFunctionCallable);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("presenter refresh functions not BlueprintPure"),
		ProbeResult.bPresenterRefreshFunctionsNotBlueprintPure);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel adapter refresh functions not BlueprintPure"),
		ProbeResult.bPanelAdapterRefreshFunctionsNotBlueprintPure);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget refresh functions not BlueprintPure"),
		ProbeResult.bPanelWidgetRefreshFunctionsNotBlueprintPure);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget cached getter functions BlueprintPure"),
		ProbeResult.bPanelWidgetCachedGetterFunctionsBlueprintPure);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget rows property Blueprint visible"),
		ProbeResult.bPanelWidgetRowsPropertyBlueprintVisible);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget formatted lines property Blueprint visible"),
		ProbeResult.bPanelWidgetFormattedLinesPropertyBlueprintVisible);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget refresh result property Blueprint visible"),
		ProbeResult.bPanelWidgetRefreshResultPropertyBlueprintVisible);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget summary text property Blueprint visible"),
		ProbeResult.bPanelWidgetSummaryTextPropertyBlueprintVisible);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("panel widget contract validated without asset or viewport"),
		ProbeResult.bPanelWidgetContractValidatedWithoutAssetOrViewport);
	LogPersistenceStatusSampleSurfaceBool(
		TEXT("Blueprint refresh result reflected"),
		ProbeResult.bBlueprintRefreshResultReflected);
	LogPersistenceStatusSampleSurfaceBool(TEXT("default rows presented"), ProbeResult.bDefaultRowsPresented);
	LogPersistenceStatusSampleSurfaceBool(TEXT("explicit refresh result succeeded"), ProbeResult.bExplicitRefreshResultSucceeded);
	LogPersistenceStatusSampleSurfaceBool(TEXT("panel adapter refresh succeeded"), ProbeResult.bPanelAdapterRefreshSucceeded);
	LogPersistenceStatusSampleSurfaceBool(TEXT("panel adapter rows matched presenter"), ProbeResult.bPanelAdapterRowsMatchedPresenter);
	LogPersistenceStatusSampleSurfaceBool(TEXT("panel adapter did not create DB"), ProbeResult.bPanelAdapterDidNotCreateDb);
	LogPersistenceStatusSampleSurfaceBool(TEXT("default formatted lines generated"), ProbeResult.bDefaultFormattedLinesGenerated);
	LogPersistenceStatusSampleSurfaceBool(TEXT("default backend line found"), ProbeResult.bDefaultBackendLineFound);
	LogPersistenceStatusSampleSurfaceBool(TEXT("default provider line found"), ProbeResult.bDefaultProviderLineFound);
	LogPersistenceStatusSampleSurfaceBool(TEXT("default repository line found"), ProbeResult.bDefaultRepositoryLineFound);
	LogPersistenceStatusSampleSurfaceBool(TEXT("default SQLite rows graceful"), ProbeResult.bDefaultSQLiteRowsGraceful);
	LogPersistenceStatusSampleSurfaceBool(TEXT("default surface did not create DB"), ProbeResult.bDefaultSurfaceDidNotCreateDb);
	LogPersistenceStatusSampleSurfaceBool(TEXT("repeated refresh succeeded"), ProbeResult.bRepeatedRefreshSucceeded);
	LogPersistenceStatusSampleSurfaceBool(TEXT("repeated refresh deterministic"), ProbeResult.bRepeatedRefreshDeterministic);
	LogPersistenceStatusSampleSurfaceBool(TEXT("repeated refresh did not create DB"), ProbeResult.bRepeatedRefreshDidNotCreateDb);
	LogPersistenceStatusSampleSurfaceBool(TEXT("panel adapter repeated refresh succeeded"), ProbeResult.bPanelAdapterRepeatedRefreshSucceeded);
	LogPersistenceStatusSampleSurfaceBool(TEXT("panel adapter repeated refresh deterministic"), ProbeResult.bPanelAdapterRepeatedRefreshDeterministic);
	LogPersistenceStatusSampleSurfaceBool(TEXT("missing SQLite rows presented"), ProbeResult.bMissingSQLiteRowsPresented);
	LogPersistenceStatusSampleSurfaceBool(TEXT("missing SQLite path line found"), ProbeResult.bMissingSQLitePathLineFound);
	LogPersistenceStatusSampleSurfaceBool(TEXT("missing SQLite database absent line found"), ProbeResult.bMissingSQLiteDatabaseAbsentLineFound);
	LogPersistenceStatusSampleSurfaceBool(TEXT("missing SQLite surface did not create DB"), ProbeResult.bMissingSQLiteSurfaceDidNotCreateDb);
	LogPersistenceStatusSampleSurfaceBool(TEXT("sidecar still present after refresh"), ProbeResult.bSidecarStillPresentAfterRefresh);
	LogPersistenceStatusSampleSurfaceBool(TEXT("database files removed"), ProbeResult.bDatabaseFilesRemoved);

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(
			LogSQLUISamples,
			Log,
			TEXT("SQLUI persistence status sample surface probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI persistence status sample surface probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestPersistenceSettingsDraftProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedPersistenceSettingsDraftProbe)
	{
		return;
	}

	const FSQLUISamplePersistenceSettingsDraftProbeResult& ProbeResult =
		Result.PersistenceSettingsDraftProbe;

	const auto LogPersistenceSettingsDraftBool =
		[](const TCHAR* Label, const bool bValue)
		{
			UE_LOG(
				LogSQLUISamples,
				Log,
				TEXT("SQLUI persistence settings draft probe %s: %s"),
				Label,
				bValue ? TEXT("true") : TEXT("false"));
		};

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI persistence settings draft probe database path: '%s'"),
		*ProbeResult.DatabasePath);

	LogPersistenceSettingsDraftBool(TEXT("default draft created"), ProbeResult.bDefaultDraftCreated);
	LogPersistenceSettingsDraftBool(TEXT("default draft validated"), ProbeResult.bDefaultDraftValidated);
	LogPersistenceSettingsDraftBool(TEXT("default InMemory safe"), ProbeResult.bDefaultInMemorySafe);
	LogPersistenceSettingsDraftBool(TEXT("current draft validated"), ProbeResult.bCurrentDraftValidated);
	LogPersistenceSettingsDraftBool(TEXT("unknown backend rejected"), ProbeResult.bUnknownBackendRejected);
	LogPersistenceSettingsDraftBool(TEXT("SQLite draft represented"), ProbeResult.bSQLiteDraftRepresented);
	LogPersistenceSettingsDraftBool(TEXT("SQLite draft did not create DB"), ProbeResult.bSQLiteDraftDidNotCreateDb);
	LogPersistenceSettingsDraftBool(TEXT("SQLite empty path rejected"), ProbeResult.bSQLiteEmptyPathRejected);
	LogPersistenceSettingsDraftBool(TEXT("provider auto-init pending validated"), ProbeResult.bProviderAutoInitPendingValidated);
	LogPersistenceSettingsDraftBool(TEXT("provider auto-init policy unchanged"), ProbeResult.bProviderAutoInitPolicyUnchanged);
	LogPersistenceSettingsDraftBool(TEXT("default display generated"), ProbeResult.bDefaultDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("default display safe"), ProbeResult.bDefaultDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("unknown backend display shows error"), ProbeResult.bUnknownBackendDisplayShowsError);
	LogPersistenceSettingsDraftBool(TEXT("SQLite draft display generated"), ProbeResult.bSQLiteDraftDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("SQLite display did not create DB"), ProbeResult.bSQLiteDisplayDidNotCreateDb);
	LogPersistenceSettingsDraftBool(TEXT("SQLite empty path display shows error"), ProbeResult.bSQLiteEmptyPathDisplayShowsError);
	LogPersistenceSettingsDraftBool(TEXT("provider auto-init display pending"), ProbeResult.bProviderAutoInitDisplayPending);
	LogPersistenceSettingsDraftBool(TEXT("default apply preview safe"), ProbeResult.bDefaultApplyPreviewSafe);
	LogPersistenceSettingsDraftBool(TEXT("current apply preview no changes"), ProbeResult.bCurrentApplyPreviewNoChanges);
	LogPersistenceSettingsDraftBool(TEXT("backend change apply preview detected"), ProbeResult.bBackendChangeApplyPreviewDetected);
	LogPersistenceSettingsDraftBool(TEXT("SQLite apply preview safe"), ProbeResult.bSQLiteApplyPreviewSafe);
	LogPersistenceSettingsDraftBool(TEXT("unknown backend apply preview rejected"), ProbeResult.bUnknownBackendApplyPreviewRejected);
	LogPersistenceSettingsDraftBool(TEXT("SQLite empty path apply preview rejected"), ProbeResult.bSQLiteEmptyPathApplyPreviewRejected);
	LogPersistenceSettingsDraftBool(TEXT("provider auto-init apply preview detected"), ProbeResult.bProviderAutoInitApplyPreviewDetected);
	LogPersistenceSettingsDraftBool(TEXT("default apply contract safe"), ProbeResult.bDefaultApplyContractSafe);
	LogPersistenceSettingsDraftBool(TEXT("current apply contract no changes"), ProbeResult.bCurrentApplyContractNoChanges);
	LogPersistenceSettingsDraftBool(TEXT("apply execution unavailable"), ProbeResult.bApplyExecutionUnavailable);
	LogPersistenceSettingsDraftBool(TEXT("default apply request unavailable"), ProbeResult.bDefaultApplyRequestUnavailable);
	LogPersistenceSettingsDraftBool(TEXT("default apply request did not mutate"), ProbeResult.bDefaultApplyRequestDidNotMutate);
	LogPersistenceSettingsDraftBool(TEXT("unknown backend apply request blocked"), ProbeResult.bUnknownBackendApplyRequestBlocked);
	LogPersistenceSettingsDraftBool(TEXT("SQLite apply request preview only"), ProbeResult.bSQLiteApplyRequestPreviewOnly);
	LogPersistenceSettingsDraftBool(TEXT("SQLite apply request did not create DB"), ProbeResult.bSQLiteApplyRequestDidNotCreateDb);
	LogPersistenceSettingsDraftBool(TEXT("provider auto-init apply request did not change policy"), ProbeResult.bProviderAutoInitApplyRequestDidNotChangePolicy);
	LogPersistenceSettingsDraftBool(TEXT("repeated apply request deterministic"), ProbeResult.bRepeatedApplyRequestDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("apply request preserved config files"), ProbeResult.bApplyRequestPreservedConfigFiles);
	LogPersistenceSettingsDraftBool(TEXT("apply request did not create directory"), ProbeResult.bApplyRequestDidNotCreateDirectory);
	LogPersistenceSettingsDraftBool(TEXT("default apply result display safe"), ProbeResult.bDefaultApplyResultDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("unknown backend apply result display blocked"), ProbeResult.bUnknownBackendApplyResultDisplayBlocked);
	LogPersistenceSettingsDraftBool(TEXT("SQLite apply result display preview only"), ProbeResult.bSQLiteApplyResultDisplayPreviewOnly);
	LogPersistenceSettingsDraftBool(TEXT("provider auto-init apply result display pending"), ProbeResult.bProviderAutoInitApplyResultDisplayPending);
	LogPersistenceSettingsDraftBool(TEXT("repeated apply result display deterministic"), ProbeResult.bRepeatedApplyResultDisplayDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("apply result display preserved config files"), ProbeResult.bApplyResultDisplayPreservedConfigFiles);
	LogPersistenceSettingsDraftBool(TEXT("apply result display did not create directory"), ProbeResult.bApplyResultDisplayDidNotCreateDirectory);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter default display generated"), ProbeResult.bApplyResultAdapterDefaultDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter default display safe"), ProbeResult.bApplyResultAdapterDefaultDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter current display no changes"), ProbeResult.bApplyResultAdapterCurrentDisplayNoChanges);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter execution unavailable"), ProbeResult.bApplyResultAdapterExecutionUnavailable);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter unknown backend shows error"), ProbeResult.bApplyResultAdapterUnknownBackendShowsError);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter SQLite display generated"), ProbeResult.bApplyResultAdapterSQLiteDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter SQLite display did not create DB"), ProbeResult.bApplyResultAdapterSQLiteDisplayDidNotCreateDb);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter provider auto-init pending"), ProbeResult.bApplyResultAdapterProviderAutoInitPending);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter repeated display deterministic"), ProbeResult.bApplyResultAdapterRepeatedDisplayDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter preserved config files"), ProbeResult.bApplyResultAdapterPreservedConfigFiles);
	LogPersistenceSettingsDraftBool(TEXT("apply result adapter did not create directory"), ProbeResult.bApplyResultAdapterDidNotCreateDirectory);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget class derived from UUserWidget"), ProbeResult.bApplyResultPanelWidgetClassDerivedFromUserWidget);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget Blueprint default refresh function callable"), ProbeResult.bApplyResultPanelWidgetBlueprintDefaultRefreshFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget Blueprint current refresh function callable"), ProbeResult.bApplyResultPanelWidgetBlueprintCurrentRefreshFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget Blueprint build function callable"), ProbeResult.bApplyResultPanelWidgetBlueprintBuildFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget refresh functions not BlueprintPure"), ProbeResult.bApplyResultPanelWidgetRefreshFunctionsNotBlueprintPure);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget cached getter functions BlueprintPure"), ProbeResult.bApplyResultPanelWidgetCachedGetterFunctionsBlueprintPure);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget rows property Blueprint visible"), ProbeResult.bApplyResultPanelWidgetRowsPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget formatted lines property Blueprint visible"), ProbeResult.bApplyResultPanelWidgetFormattedLinesPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget refresh result property Blueprint visible"), ProbeResult.bApplyResultPanelWidgetRefreshResultPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget summary text property Blueprint visible"), ProbeResult.bApplyResultPanelWidgetSummaryTextPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget apply result flags Blueprint visible"), ProbeResult.bApplyResultPanelWidgetApplyResultFlagsBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget contract validated without asset or viewport"), ProbeResult.bApplyResultPanelWidgetContractValidatedWithoutAssetOrViewport);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget default display generated"), ProbeResult.bApplyResultPanelWidgetDefaultDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget default display safe"), ProbeResult.bApplyResultPanelWidgetDefaultDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget unknown backend shows error"), ProbeResult.bApplyResultPanelWidgetUnknownBackendShowsError);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget SQLite display did not create DB"), ProbeResult.bApplyResultPanelWidgetSQLiteDisplayDidNotCreateDb);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget provider auto-init pending"), ProbeResult.bApplyResultPanelWidgetProviderAutoInitPending);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget repeated display deterministic"), ProbeResult.bApplyResultPanelWidgetRepeatedDisplayDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget preserved config files"), ProbeResult.bApplyResultPanelWidgetPreservedConfigFiles);
	LogPersistenceSettingsDraftBool(TEXT("apply result panel widget did not create directory"), ProbeResult.bApplyResultPanelWidgetDidNotCreateDirectory);
	LogPersistenceSettingsDraftBool(TEXT("apply config target default runtime unavailable"), ProbeResult.bApplyConfigTargetDefaultRuntimeUnavailable);
	LogPersistenceSettingsDraftBool(TEXT("apply config target smoke target validated"), ProbeResult.bApplyConfigTargetSmokeTargetValidated);
	LogPersistenceSettingsDraftBool(TEXT("apply config target valid draft wrote smoke config"), ProbeResult.bApplyConfigTargetValidDraftWroteSmokeConfig);
	LogPersistenceSettingsDraftBool(TEXT("apply config target recorded expected values"), ProbeResult.bApplyConfigTargetRecordedExpectedValues);
	LogPersistenceSettingsDraftBool(TEXT("apply config target invalid draft refused"), ProbeResult.bApplyConfigTargetInvalidDraftRefused);
	LogPersistenceSettingsDraftBool(TEXT("apply config target invalid draft did not mutate"), ProbeResult.bApplyConfigTargetInvalidDraftDidNotMutate);
	LogPersistenceSettingsDraftBool(TEXT("apply config target unsafe path rejected"), ProbeResult.bApplyConfigTargetUnsafePathRejected);
	LogPersistenceSettingsDraftBool(TEXT("apply config target runtime policy unchanged"), ProbeResult.bApplyConfigTargetRuntimePolicyUnchanged);
	LogPersistenceSettingsDraftBool(TEXT("apply config target SQLite did not create DB"), ProbeResult.bApplyConfigTargetSQLiteDidNotCreateDb);
	LogPersistenceSettingsDraftBool(TEXT("apply config target no lifecycle side effects"), ProbeResult.bApplyConfigTargetNoLifecycleSideEffects);
	LogPersistenceSettingsDraftBool(TEXT("apply config target artifact cleaned"), ProbeResult.bApplyConfigTargetArtifactCleaned);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy default runtime unavailable"), ProbeResult.bApplyConfigTargetPolicyDefaultRuntimeUnavailable);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy default cannot write"), ProbeResult.bApplyConfigTargetPolicyDefaultCannotWrite);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy smoke-owned resolved"), ProbeResult.bApplyConfigTargetPolicySmokeOwnedResolved);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy smoke-owned can write"), ProbeResult.bApplyConfigTargetPolicySmokeOwnedCanWrite);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy future real target unavailable"), ProbeResult.bApplyConfigTargetPolicyFutureRealTargetUnavailable);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy documented strategy resolved"), ProbeResult.bApplyConfigTargetPolicyDocumentedStrategyResolved);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy documented strategy cannot write"), ProbeResult.bApplyConfigTargetPolicyDocumentedStrategyCannotWrite);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy documented strategy no writable path"), ProbeResult.bApplyConfigTargetPolicyDocumentedStrategyNoWritablePath);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy documented strategy deterministic"), ProbeResult.bApplyConfigTargetPolicyDocumentedStrategyDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy production apply disabled"), ProbeResult.bApplyConfigTargetPolicyProductionApplyDisabled);
	LogPersistenceSettingsDraftBool(TEXT("apply config target policy repeated deterministic"), ProbeResult.bApplyConfigTargetPolicyRepeatedDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("backend change apply contract detected"), ProbeResult.bBackendChangeApplyContractDetected);
	LogPersistenceSettingsDraftBool(TEXT("SQLite apply contract safe"), ProbeResult.bSQLiteApplyContractSafe);
	LogPersistenceSettingsDraftBool(TEXT("unknown backend apply contract blocked"), ProbeResult.bUnknownBackendApplyContractBlocked);
	LogPersistenceSettingsDraftBool(TEXT("SQLite empty path apply contract blocked"), ProbeResult.bSQLiteEmptyPathApplyContractBlocked);
	LogPersistenceSettingsDraftBool(TEXT("provider auto-init apply contract detected"), ProbeResult.bProviderAutoInitApplyContractDetected);
	LogPersistenceSettingsDraftBool(TEXT("cancel preview safe"), ProbeResult.bCancelPreviewSafe);
	LogPersistenceSettingsDraftBool(TEXT("cancel preview would discard changes"), ProbeResult.bCancelPreviewWouldDiscardChanges);
	LogPersistenceSettingsDraftBool(TEXT("default apply preview display safe"), ProbeResult.bDefaultApplyPreviewDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("current apply preview display no changes"), ProbeResult.bCurrentApplyPreviewDisplayNoChanges);
	LogPersistenceSettingsDraftBool(TEXT("backend change apply preview display detected"), ProbeResult.bBackendChangeApplyPreviewDisplayDetected);
	LogPersistenceSettingsDraftBool(TEXT("SQLite apply preview display safe"), ProbeResult.bSQLiteApplyPreviewDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("unknown backend apply preview display shows error"), ProbeResult.bUnknownBackendApplyPreviewDisplayShowsError);
	LogPersistenceSettingsDraftBool(TEXT("SQLite empty path apply preview display shows error"), ProbeResult.bSQLiteEmptyPathApplyPreviewDisplayShowsError);
	LogPersistenceSettingsDraftBool(TEXT("provider auto-init apply preview display pending"), ProbeResult.bProviderAutoInitApplyPreviewDisplayPending);
	LogPersistenceSettingsDraftBool(TEXT("default apply contract display safe"), ProbeResult.bDefaultApplyContractDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("current apply contract display no changes"), ProbeResult.bCurrentApplyContractDisplayNoChanges);
	LogPersistenceSettingsDraftBool(TEXT("apply contract display execution unavailable"), ProbeResult.bApplyContractDisplayExecutionUnavailable);
	LogPersistenceSettingsDraftBool(TEXT("backend change apply contract display detected"), ProbeResult.bBackendChangeApplyContractDisplayDetected);
	LogPersistenceSettingsDraftBool(TEXT("SQLite apply contract display safe"), ProbeResult.bSQLiteApplyContractDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("unknown backend apply contract display shows error"), ProbeResult.bUnknownBackendApplyContractDisplayShowsError);
	LogPersistenceSettingsDraftBool(TEXT("SQLite empty path apply contract display shows error"), ProbeResult.bSQLiteEmptyPathApplyContractDisplayShowsError);
	LogPersistenceSettingsDraftBool(TEXT("provider auto-init apply contract display pending"), ProbeResult.bProviderAutoInitApplyContractDisplayPending);
	LogPersistenceSettingsDraftBool(TEXT("cancel preview display would discard changes"), ProbeResult.bCancelPreviewDisplayWouldDiscardChanges);
	LogPersistenceSettingsDraftBool(TEXT("sample adapter default display generated"), ProbeResult.bSampleAdapterDefaultDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("sample adapter default display safe"), ProbeResult.bSampleAdapterDefaultDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("sample adapter unknown backend display shows error"), ProbeResult.bSampleAdapterUnknownBackendDisplayShowsError);
	LogPersistenceSettingsDraftBool(TEXT("sample adapter SQLite draft display generated"), ProbeResult.bSampleAdapterSQLiteDraftDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("sample adapter SQLite display did not create DB"), ProbeResult.bSampleAdapterSQLiteDisplayDidNotCreateDb);
	LogPersistenceSettingsDraftBool(TEXT("sample adapter SQLite empty path display shows error"), ProbeResult.bSampleAdapterSQLiteEmptyPathDisplayShowsError);
	LogPersistenceSettingsDraftBool(TEXT("sample adapter provider auto-init display pending"), ProbeResult.bSampleAdapterProviderAutoInitDisplayPending);
	LogPersistenceSettingsDraftBool(TEXT("sample adapter repeated display deterministic"), ProbeResult.bSampleAdapterRepeatedDisplayDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter default display generated"), ProbeResult.bApplyPreviewAdapterDefaultDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter default display safe"), ProbeResult.bApplyPreviewAdapterDefaultDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter current display no changes"), ProbeResult.bApplyPreviewAdapterCurrentDisplayNoChanges);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter backend change detected"), ProbeResult.bApplyPreviewAdapterBackendChangeDetected);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter SQLite display generated"), ProbeResult.bApplyPreviewAdapterSQLiteDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter SQLite display did not create DB"), ProbeResult.bApplyPreviewAdapterSQLiteDisplayDidNotCreateDb);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter unknown backend shows error"), ProbeResult.bApplyPreviewAdapterUnknownBackendShowsError);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter SQLite empty path shows error"), ProbeResult.bApplyPreviewAdapterSQLiteEmptyPathShowsError);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter provider auto-init pending"), ProbeResult.bApplyPreviewAdapterProviderAutoInitPending);
	LogPersistenceSettingsDraftBool(TEXT("apply preview adapter repeated display deterministic"), ProbeResult.bApplyPreviewAdapterRepeatedDisplayDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter default display generated"), ProbeResult.bApplyContractAdapterDefaultDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter default display safe"), ProbeResult.bApplyContractAdapterDefaultDisplaySafe);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter current display no changes"), ProbeResult.bApplyContractAdapterCurrentDisplayNoChanges);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter execution unavailable"), ProbeResult.bApplyContractAdapterExecutionUnavailable);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter backend change detected"), ProbeResult.bApplyContractAdapterBackendChangeDetected);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter SQLite display generated"), ProbeResult.bApplyContractAdapterSQLiteDisplayGenerated);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter SQLite display did not create DB"), ProbeResult.bApplyContractAdapterSQLiteDisplayDidNotCreateDb);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter unknown backend shows error"), ProbeResult.bApplyContractAdapterUnknownBackendShowsError);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter SQLite empty path shows error"), ProbeResult.bApplyContractAdapterSQLiteEmptyPathShowsError);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter provider auto-init pending"), ProbeResult.bApplyContractAdapterProviderAutoInitPending);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter cancel preview would discard changes"), ProbeResult.bApplyContractAdapterCancelPreviewWouldDiscardChanges);
	LogPersistenceSettingsDraftBool(TEXT("apply contract adapter repeated display deterministic"), ProbeResult.bApplyContractAdapterRepeatedDisplayDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget class derived from UUserWidget"), ProbeResult.bApplyContractPanelWidgetClassDerivedFromUserWidget);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget Blueprint default refresh function callable"), ProbeResult.bApplyContractPanelWidgetBlueprintDefaultRefreshFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget Blueprint current refresh function callable"), ProbeResult.bApplyContractPanelWidgetBlueprintCurrentRefreshFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget Blueprint build function callable"), ProbeResult.bApplyContractPanelWidgetBlueprintBuildFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget refresh functions not BlueprintPure"), ProbeResult.bApplyContractPanelWidgetRefreshFunctionsNotBlueprintPure);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget cached getter functions BlueprintPure"), ProbeResult.bApplyContractPanelWidgetCachedGetterFunctionsBlueprintPure);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget rows property Blueprint visible"), ProbeResult.bApplyContractPanelWidgetRowsPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget formatted lines property Blueprint visible"), ProbeResult.bApplyContractPanelWidgetFormattedLinesPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget refresh result property Blueprint visible"), ProbeResult.bApplyContractPanelWidgetRefreshResultPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget summary text property Blueprint visible"), ProbeResult.bApplyContractPanelWidgetSummaryTextPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget contract flags Blueprint visible"), ProbeResult.bApplyContractPanelWidgetContractFlagsBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply contract panel widget contract validated without asset or viewport"), ProbeResult.bApplyContractPanelWidgetContractValidatedWithoutAssetOrViewport);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget class derived from UUserWidget"), ProbeResult.bApplyPreviewPanelWidgetClassDerivedFromUserWidget);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget Blueprint default refresh function callable"), ProbeResult.bApplyPreviewPanelWidgetBlueprintDefaultRefreshFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget Blueprint current refresh function callable"), ProbeResult.bApplyPreviewPanelWidgetBlueprintCurrentRefreshFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget Blueprint build function callable"), ProbeResult.bApplyPreviewPanelWidgetBlueprintBuildFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget refresh functions not BlueprintPure"), ProbeResult.bApplyPreviewPanelWidgetRefreshFunctionsNotBlueprintPure);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget cached getter functions BlueprintPure"), ProbeResult.bApplyPreviewPanelWidgetCachedGetterFunctionsBlueprintPure);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget rows property Blueprint visible"), ProbeResult.bApplyPreviewPanelWidgetRowsPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget formatted lines property Blueprint visible"), ProbeResult.bApplyPreviewPanelWidgetFormattedLinesPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget refresh result property Blueprint visible"), ProbeResult.bApplyPreviewPanelWidgetRefreshResultPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget summary text property Blueprint visible"), ProbeResult.bApplyPreviewPanelWidgetSummaryTextPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget preview flags Blueprint visible"), ProbeResult.bApplyPreviewPanelWidgetPreviewFlagsBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("apply preview panel widget contract validated without asset or viewport"), ProbeResult.bApplyPreviewPanelWidgetContractValidatedWithoutAssetOrViewport);
	LogPersistenceSettingsDraftBool(TEXT("panel widget class derived from UUserWidget"), ProbeResult.bPanelWidgetClassDerivedFromUserWidget);
	LogPersistenceSettingsDraftBool(TEXT("panel widget Blueprint default refresh function callable"), ProbeResult.bPanelWidgetBlueprintDefaultRefreshFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("panel widget Blueprint current refresh function callable"), ProbeResult.bPanelWidgetBlueprintCurrentRefreshFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("panel widget Blueprint build function callable"), ProbeResult.bPanelWidgetBlueprintBuildFunctionCallable);
	LogPersistenceSettingsDraftBool(TEXT("panel widget refresh functions not BlueprintPure"), ProbeResult.bPanelWidgetRefreshFunctionsNotBlueprintPure);
	LogPersistenceSettingsDraftBool(TEXT("panel widget cached getter functions BlueprintPure"), ProbeResult.bPanelWidgetCachedGetterFunctionsBlueprintPure);
	LogPersistenceSettingsDraftBool(TEXT("panel widget rows property Blueprint visible"), ProbeResult.bPanelWidgetRowsPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("panel widget formatted lines property Blueprint visible"), ProbeResult.bPanelWidgetFormattedLinesPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("panel widget refresh result property Blueprint visible"), ProbeResult.bPanelWidgetRefreshResultPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("panel widget summary text property Blueprint visible"), ProbeResult.bPanelWidgetSummaryTextPropertyBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("panel widget validation flags Blueprint visible"), ProbeResult.bPanelWidgetValidationFlagsBlueprintVisible);
	LogPersistenceSettingsDraftBool(TEXT("panel widget contract validated without asset or viewport"), ProbeResult.bPanelWidgetContractValidatedWithoutAssetOrViewport);
	LogPersistenceSettingsDraftBool(TEXT("repeated validation deterministic"), ProbeResult.bRepeatedValidationDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("repeated display deterministic"), ProbeResult.bRepeatedDisplayDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("repeated apply preview deterministic"), ProbeResult.bRepeatedApplyPreviewDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("repeated apply preview display deterministic"), ProbeResult.bRepeatedApplyPreviewDisplayDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("repeated apply contract display deterministic"), ProbeResult.bRepeatedApplyContractDisplayDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("repeated apply contract deterministic"), ProbeResult.bRepeatedApplyContractDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("repeated cancel preview deterministic"), ProbeResult.bRepeatedCancelPreviewDeterministic);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during validation"), ProbeResult.bSidecarPreservedDuringValidation);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during apply preview"), ProbeResult.bSidecarPreservedDuringApplyPreview);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during apply preview display"), ProbeResult.bSidecarPreservedDuringApplyPreviewDisplay);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during apply preview adapter"), ProbeResult.bSidecarPreservedDuringApplyPreviewAdapter);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during apply contract adapter"), ProbeResult.bSidecarPreservedDuringApplyContractAdapter);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during apply contract display"), ProbeResult.bSidecarPreservedDuringApplyContractDisplay);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during apply contract"), ProbeResult.bSidecarPreservedDuringApplyContract);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during apply request"), ProbeResult.bSidecarPreservedDuringApplyRequest);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during apply result display"), ProbeResult.bSidecarPreservedDuringApplyResultDisplay);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during apply result adapter"), ProbeResult.bSidecarPreservedDuringApplyResultAdapter);
	LogPersistenceSettingsDraftBool(TEXT("sidecar preserved during cancel preview"), ProbeResult.bSidecarPreservedDuringCancelPreview);
	LogPersistenceSettingsDraftBool(TEXT("database files removed"), ProbeResult.bDatabaseFilesRemoved);

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI persistence settings draft probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI persistence settings draft probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteSeedDatabaseCopyPolicyProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteSeedDatabaseCopyPolicyProbe)
	{
		return;
	}

	const FSQLUISampleSQLiteSeedDatabaseCopyPolicyProbeResult& ProbeResult =
		Result.SQLiteSeedDatabaseCopyPolicyProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe seed database created: %s"),
		ProbeResult.bSeedDatabaseCreated ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe missing target copied: %s"),
		ProbeResult.bMissingTargetCopied ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe copied target readable: %s"),
		ProbeResult.bCopiedTargetReadable ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe copied target loaded seed layout: %s"),
		ProbeResult.bCopiedTargetLoadedSeedLayout ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe existing target preserved without overwrite: %s"),
		ProbeResult.bExistingTargetPreservedWithoutOverwrite ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe overwrite target copied seed: %s"),
		ProbeResult.bOverwriteTargetCopiedSeed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe overwrite removed existing layout: %s"),
		ProbeResult.bOverwriteRemovedExistingLayout ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe missing seed failed: %s"),
		ProbeResult.bMissingSeedFailed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe missing seed did not create target: %s"),
		ProbeResult.bMissingSeedDidNotCreateTarget ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe same path failed: %s"),
		ProbeResult.bSamePathFailed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe same path left seed intact: %s"),
		ProbeResult.bSamePathLeftSeedIntact ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe runtime config seed flags parsed: %s"),
		ProbeResult.bRuntimeConfigSeedFlagsParsed ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite seed database copy policy probe database files removed: %s"),
		ProbeResult.bDatabaseFilesRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite seed database copy policy probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite seed database copy policy probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestSQLiteMigrationVersioningPolicyProbeResult(
	const FSQLUISampleSmokeTestResult& Result)
{
	if (!Result.bUsedSQLiteMigrationVersioningPolicyProbe)
	{
		return;
	}

	const FSQLUISampleSQLiteMigrationVersioningPolicyProbeResult& ProbeResult =
		Result.SQLiteMigrationVersioningPolicyProbe;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe current initial schema status succeeded: %s"),
		ProbeResult.bCurrentInitialSchemaStatusSucceeded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe latest known migration matched: %s"),
		ProbeResult.bLatestKnownMigrationMatched ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe no pending known migrations: %s"),
		ProbeResult.bNoPendingKnownMigrations ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe complete schema missing record detected: %s"),
		ProbeResult.bCompleteSchemaMissingRecordDetected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe missing record repaired non-destructively: %s"),
		ProbeResult.bMissingRecordRepairedNonDestructively ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe partial schema failed clearly: %s"),
		ProbeResult.bPartialSchemaFailedClearly ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe smoke migrations applied in order: %s"),
		ProbeResult.bSmokeMigrationsAppliedInOrder ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe smoke migrations idempotent: %s"),
		ProbeResult.bSmokeMigrationsIdempotent ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe smoke pending migration detected: %s"),
		ProbeResult.bSmokePendingMigrationDetected ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe smoke pending migration applied: %s"),
		ProbeResult.bSmokePendingMigrationApplied ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe failing migration failed clearly: %s"),
		ProbeResult.bFailingMigrationFailedClearly ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe failing migration not recorded: %s"),
		ProbeResult.bFailingMigrationNotRecorded ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI SQLite migration versioning policy probe database files removed: %s"),
		ProbeResult.bDatabaseFilesRemoved ? TEXT("true") : TEXT("false"));

	if (ProbeResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite migration versioning policy probe succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI SQLite migration versioning policy probe failed: %s"),
			*ProbeResult.ErrorMessage);
	}
}

void LogSQLUISampleSmokeTestStepErrors(
	const TCHAR* StepName,
	const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test step '%s' error: %s"),
			StepName,
			*Message);
	}
}

void LogSQLUISampleSmokeTestStepWarnings(
	const TCHAR* StepName,
	const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample smoke test step '%s' warning: %s"),
			StepName,
			*Message);
	}
}

void LogSQLUISampleSmokeTestResult(const FSQLUISampleSmokeTestResult& Result)
{
	LogSQLUISampleSmokeTestJsonFixtureResult(Result);
	LogSQLUISampleSmokeTestRepositorySelectionResult(Result);
	LogSQLUISampleSmokeTestRepositoryResult(Result);
	LogSQLUISampleSmokeTestJsonFileRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteCoreProbeResult(Result);
	LogSQLUISampleSmokeTestDatabaseAsyncProbeResult(Result);
	LogSQLUISampleSmokeTestDatabaseAsyncQueueShutdownProbeResult(Result);
	LogSQLUISampleSmokeTestLayoutRepositoryRuntimeConfigProbeResult(Result);
	LogSQLUISampleSmokeTestLayoutRepositoryRuntimeIntegrationProbeResult(Result);
	LogSQLUISampleSmokeTestLayoutRepositoryRuntimeProviderProbeResult(Result);
	LogSQLUISampleSmokeTestLayoutRepositoryRuntimeSettingsProbeResult(Result);
	LogSQLUISampleSmokeTestLayoutPersistenceWorkflowProbeResult(Result);
	LogSQLUISampleSmokeTestLayoutRepositoryDatabaseManagementProbeResult(Result);
	LogSQLUISampleSmokeTestPersistenceStatusSurfaceProbeResult(Result);
	LogSQLUISampleSmokeTestPersistenceStatusDisplayRowsProbeResult(Result);
	LogSQLUISampleSmokeTestPersistenceStatusSampleSurfaceProbeResult(Result);
	LogSQLUISampleSmokeTestPersistenceSettingsDraftProbeResult(Result);
	LogSQLUISampleSmokeTestSQLiteMigrationProbeResult(Result);
	LogSQLUISampleSmokeTestSQLiteLayoutSchemaMigrationProbeResult(Result);
	LogSQLUISampleSmokeTestSQLiteLayoutReadProbeResult(Result);
	LogSQLUISampleSmokeTestSQLiteReadOnlyLayoutRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteSaveLayoutRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteRemoveLayoutRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteClearLayoutsRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteFullLifecycleRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteAsyncCallbackRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteSerializedAsyncCallbackRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteFactoryLayoutRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteFactorySchemaInitRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteSchemaInitHardeningResult(Result);
	LogSQLUISampleSmokeTestSQLiteSeedDatabaseCopyPolicyProbeResult(Result);
	LogSQLUISampleSmokeTestSQLiteMigrationVersioningPolicyProbeResult(Result);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test root widget valid: %s"),
		Result.bRootWidgetValid ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test created widget count: %d"),
		Result.CreatedWidgetCount);

	LogSQLUISampleSmokeTestErrors(Result.Errors);
	LogSQLUISampleSmokeTestWarnings(Result.Warnings);

	for (const FSQLUIRuntimeWidgetPipelineStepResult& StepResult : Result.StepResults)
	{
		UE_LOG(
			LogSQLUISamples,
			Log,
			TEXT("SQLUI sample smoke test step '%s': %s"),
			*StepResult.StepName,
			SQLUISampleSmokeTestStepStatusToString(StepResult.Status));

		LogSQLUISampleSmokeTestStepErrors(
			*StepResult.StepName,
			StepResult.Errors);

		LogSQLUISampleSmokeTestStepWarnings(
			*StepResult.StepName,
			StepResult.Warnings);
	}
}

UWorld* CreateSQLUISampleSmokeTestCommandletWorld()
{
	return UWorld::CreateWorld(
		EWorldType::Game,
		false,
		SQLUISampleSmokeTestCommandletWorldName);
}

void DestroySQLUISampleSmokeTestCommandletWorld(UWorld* World)
{
	if (World)
	{
		World->DestroyWorld(false);
	}
}
}

USQLUISampleSmokeTestCommandlet::USQLUISampleSmokeTestCommandlet()
{
	IsClient = false;
	IsEditor = false;
	IsServer = false;
	LogToConsole = true;
}

int32 USQLUISampleSmokeTestCommandlet::Main(const FString& Params)
{
	UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample smoke test commandlet started."));
	const bool bUseInMemoryLayoutRepository =
		FParse::Param(*Params, TEXT("UseInMemoryLayoutRepository"))
		|| FParse::Param(*Params, TEXT("InMemoryLayoutRepository"));
	const bool bUseJsonFileLayoutRepository =
		FParse::Param(*Params, TEXT("UseJsonFileLayoutRepository"))
		|| FParse::Param(*Params, TEXT("JsonFileLayoutRepository"));
	const bool bUseSQLiteCoreProbe =
		FParse::Param(*Params, TEXT("UseSQLiteCoreProbe"))
		|| FParse::Param(*Params, TEXT("SQLiteCoreProbe"));
	const bool bUseDatabaseAsyncProbe =
		FParse::Param(*Params, TEXT("UseDatabaseAsyncProbe"))
		|| FParse::Param(*Params, TEXT("DatabaseAsyncProbe"));
	const bool bUseDatabaseAsyncQueueShutdownProbe =
		FParse::Param(*Params, TEXT("UseDatabaseAsyncQueueShutdownProbe"))
		|| FParse::Param(*Params, TEXT("DatabaseAsyncQueueShutdownProbe"));
	const bool bUseLayoutRepositoryRuntimeConfigProbe =
		FParse::Param(*Params, TEXT("UseLayoutRepositoryRuntimeConfigProbe"))
		|| FParse::Param(*Params, TEXT("LayoutRepositoryRuntimeConfigProbe"));
	const bool bUseLayoutRepositoryRuntimeIntegrationProbe =
		FParse::Param(*Params, TEXT("UseLayoutRepositoryRuntimeIntegrationProbe"))
		|| FParse::Param(*Params, TEXT("LayoutRepositoryRuntimeIntegrationProbe"));
	const bool bUseLayoutRepositoryRuntimeProviderProbe =
		FParse::Param(*Params, TEXT("UseLayoutRepositoryRuntimeProviderProbe"))
		|| FParse::Param(*Params, TEXT("LayoutRepositoryRuntimeProviderProbe"));
	const bool bUseLayoutRepositoryRuntimeSettingsProbe =
		FParse::Param(*Params, TEXT("UseLayoutRepositoryRuntimeSettingsProbe"))
		|| FParse::Param(*Params, TEXT("LayoutRepositoryRuntimeSettingsProbe"));
	const bool bUseLayoutPersistenceWorkflowProbe =
		FParse::Param(*Params, TEXT("UseLayoutPersistenceWorkflowProbe"))
		|| FParse::Param(*Params, TEXT("LayoutPersistenceWorkflowProbe"));
	const bool bUseLayoutRepositoryDatabaseManagementProbe =
		FParse::Param(*Params, TEXT("UseLayoutRepositoryDatabaseManagementProbe"))
		|| FParse::Param(*Params, TEXT("LayoutRepositoryDatabaseManagementProbe"));
	const bool bUsePersistenceStatusSurfaceProbe =
		FParse::Param(*Params, TEXT("UsePersistenceStatusSurfaceProbe"))
		|| FParse::Param(*Params, TEXT("PersistenceStatusSurfaceProbe"));
	const bool bUsePersistenceStatusDisplayRowsProbe =
		FParse::Param(*Params, TEXT("UsePersistenceStatusDisplayRowsProbe"))
		|| FParse::Param(*Params, TEXT("PersistenceStatusDisplayRowsProbe"));
	const bool bUsePersistenceStatusSampleSurfaceProbe =
		FParse::Param(*Params, TEXT("UsePersistenceStatusSampleSurfaceProbe"))
		|| FParse::Param(*Params, TEXT("PersistenceStatusSampleSurfaceProbe"));
	const bool bUsePersistenceSettingsDraftProbe =
		FParse::Param(*Params, TEXT("UsePersistenceSettingsDraftProbe"))
		|| FParse::Param(*Params, TEXT("PersistenceSettingsDraftProbe"));
	const bool bUseSQLiteMigrationProbe =
		FParse::Param(*Params, TEXT("UseSQLiteMigrationProbe"))
		|| FParse::Param(*Params, TEXT("SQLiteMigrationProbe"));
	const bool bUseSQLiteLayoutSchemaMigrationProbe =
		FParse::Param(*Params, TEXT("UseSQLiteLayoutSchemaMigrationProbe"))
		|| FParse::Param(*Params, TEXT("SQLiteLayoutSchemaMigrationProbe"));
	const bool bUseSQLiteLayoutReadProbe =
		FParse::Param(*Params, TEXT("UseSQLiteLayoutReadProbe"))
		|| FParse::Param(*Params, TEXT("SQLiteLayoutReadProbe"));
	const bool bUseSQLiteReadOnlyLayoutRepository =
		FParse::Param(*Params, TEXT("UseSQLiteReadOnlyLayoutRepository"))
		|| FParse::Param(*Params, TEXT("SQLiteReadOnlyLayoutRepository"));
	const bool bUseSQLiteSaveLayoutRepository =
		FParse::Param(*Params, TEXT("UseSQLiteSaveLayoutRepository"))
		|| FParse::Param(*Params, TEXT("SQLiteSaveLayoutRepository"));
	const bool bUseSQLiteRemoveLayoutRepository =
		FParse::Param(*Params, TEXT("UseSQLiteRemoveLayoutRepository"))
		|| FParse::Param(*Params, TEXT("SQLiteRemoveLayoutRepository"));
	const bool bUseSQLiteClearLayoutsRepository =
		FParse::Param(*Params, TEXT("UseSQLiteClearLayoutsRepository"))
		|| FParse::Param(*Params, TEXT("SQLiteClearLayoutsRepository"));
	const bool bUseSQLiteFullLifecycleRepository =
		FParse::Param(*Params, TEXT("UseSQLiteFullLifecycleRepository"))
		|| FParse::Param(*Params, TEXT("SQLiteFullLifecycleRepository"));
	const bool bUseSQLiteAsyncCallbackRepository =
		FParse::Param(*Params, TEXT("UseSQLiteAsyncCallbackRepository"))
		|| FParse::Param(*Params, TEXT("SQLiteAsyncCallbackRepository"));
	const bool bUseSQLiteSerializedAsyncCallbackRepository =
		FParse::Param(*Params, TEXT("UseSQLiteSerializedAsyncCallbackRepository"))
		|| FParse::Param(*Params, TEXT("SQLiteSerializedAsyncCallbackRepository"));
	const bool bUseSQLiteFactoryLayoutRepository =
		FParse::Param(*Params, TEXT("UseSQLiteFactoryLayoutRepository"))
		|| FParse::Param(*Params, TEXT("SQLiteFactoryLayoutRepository"));
	const bool bUseSQLiteFactorySchemaInitRepository =
		FParse::Param(*Params, TEXT("UseSQLiteFactorySchemaInitRepository"))
		|| FParse::Param(*Params, TEXT("SQLiteFactorySchemaInitRepository"));
	const bool bUseSQLiteSchemaInitHardening =
		FParse::Param(*Params, TEXT("UseSQLiteSchemaInitHardening"))
		|| FParse::Param(*Params, TEXT("SQLiteSchemaInitHardening"));
	const bool bUseSQLiteSeedDatabaseCopyPolicyProbe =
		FParse::Param(*Params, TEXT("UseSQLiteSeedDatabaseCopyPolicyProbe"))
		|| FParse::Param(*Params, TEXT("SQLiteSeedDatabaseCopyPolicyProbe"));
	const bool bUseSQLiteMigrationVersioningPolicyProbe =
		FParse::Param(*Params, TEXT("UseSQLiteMigrationVersioningPolicyProbe"))
		|| FParse::Param(*Params, TEXT("SQLiteMigrationVersioningPolicyProbe"));
	const bool bUseJsonLayoutFixture =
		FParse::Param(*Params, TEXT("UseJsonLayoutFixture"))
		|| FParse::Param(*Params, TEXT("JsonLayoutFixture"))
		|| bUseInMemoryLayoutRepository
		|| bUseJsonFileLayoutRepository;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test JSON fixture selected: %s"),
		bUseJsonLayoutFixture ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test in-memory layout repository selected: %s"),
		bUseInMemoryLayoutRepository ? TEXT("true") : TEXT("false"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample smoke test JSON file layout repository selected: %s"),
		bUseJsonFileLayoutRepository ? TEXT("true") : TEXT("false"));

	if (bUseSQLiteCoreProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLiteCore probe selected: true"));
	}

	if (bUseDatabaseAsyncProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI database async probe selected: true"));
	}

	if (bUseDatabaseAsyncQueueShutdownProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI database async queue shutdown probe selected: true"));
	}

	if (bUseLayoutRepositoryRuntimeConfigProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout repository runtime config probe selected: true"));
	}

	if (bUseLayoutRepositoryRuntimeIntegrationProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout repository runtime integration probe selected: true"));
	}

	if (bUseLayoutRepositoryRuntimeProviderProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout repository runtime provider probe selected: true"));
	}

	if (bUseLayoutRepositoryRuntimeSettingsProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout repository runtime settings probe selected: true"));
	}

	if (bUseLayoutPersistenceWorkflowProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout persistence workflow probe selected: true"));
	}

	if (bUseLayoutRepositoryDatabaseManagementProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI layout repository database management probe selected: true"));
	}

	if (bUsePersistenceStatusSurfaceProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI persistence status surface probe selected: true"));
	}

	if (bUsePersistenceStatusDisplayRowsProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI persistence status display rows probe selected: true"));
	}

	if (bUsePersistenceStatusSampleSurfaceProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI persistence status sample surface probe selected: true"));
	}

	if (bUsePersistenceSettingsDraftProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI persistence settings draft probe selected: true"));
	}

	if (bUseSQLiteMigrationProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite migration probe selected: true"));
	}

	if (bUseSQLiteLayoutSchemaMigrationProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite layout schema migration probe selected: true"));
	}

	if (bUseSQLiteLayoutReadProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite layout read probe selected: true"));
	}

	if (bUseSQLiteReadOnlyLayoutRepository)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite read-only layout repository selected: true"));
	}

	if (bUseSQLiteSaveLayoutRepository)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite SaveLayout repository selected: true"));
	}

	if (bUseSQLiteRemoveLayoutRepository)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite RemoveLayout repository selected: true"));
	}

	if (bUseSQLiteClearLayoutsRepository)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite ClearLayouts repository selected: true"));
	}

	if (bUseSQLiteFullLifecycleRepository)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite full lifecycle repository selected: true"));
	}

	if (bUseSQLiteAsyncCallbackRepository)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite async callback repository selected: true"));
	}

	if (bUseSQLiteSerializedAsyncCallbackRepository)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite serialized async callback repository selected: true"));
	}

	if (bUseSQLiteFactoryLayoutRepository)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite factory layout repository selected: true"));
	}

	if (bUseSQLiteFactorySchemaInitRepository)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite factory schema init repository selected: true"));
	}

	if (bUseSQLiteSchemaInitHardening)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite schema init hardening selected: true"));
	}

	if (bUseSQLiteSeedDatabaseCopyPolicyProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite seed database copy policy probe selected: true"));
	}

	if (bUseSQLiteMigrationVersioningPolicyProbe)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI SQLite migration versioning policy probe selected: true"));
	}

	UWorld* CommandletWorld = CreateSQLUISampleSmokeTestCommandletWorld();
	if (!CommandletWorld)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample smoke test commandlet could not create a transient world context."));
		return 1;
	}

	FSQLUISampleSmokeTestResult Result;
	{
		FSQLUISampleSmokeTestRequest Request;
		Request.bUseJsonLayoutFixture = bUseJsonLayoutFixture;
		Request.bUseInMemoryLayoutRepository = bUseInMemoryLayoutRepository;
		Request.bUseJsonFileLayoutRepository = bUseJsonFileLayoutRepository;
		Request.bUseSQLiteCoreProbe = bUseSQLiteCoreProbe;
		Request.bUseDatabaseAsyncProbe = bUseDatabaseAsyncProbe;
		Request.bUseDatabaseAsyncQueueShutdownProbe = bUseDatabaseAsyncQueueShutdownProbe;
		Request.bUseLayoutRepositoryRuntimeConfigProbe = bUseLayoutRepositoryRuntimeConfigProbe;
		Request.bUseLayoutRepositoryRuntimeIntegrationProbe = bUseLayoutRepositoryRuntimeIntegrationProbe;
		Request.bUseLayoutRepositoryRuntimeProviderProbe = bUseLayoutRepositoryRuntimeProviderProbe;
		Request.bUseLayoutRepositoryRuntimeSettingsProbe = bUseLayoutRepositoryRuntimeSettingsProbe;
		Request.bUseLayoutPersistenceWorkflowProbe = bUseLayoutPersistenceWorkflowProbe;
		Request.bUseLayoutRepositoryDatabaseManagementProbe = bUseLayoutRepositoryDatabaseManagementProbe;
		Request.bUsePersistenceStatusSurfaceProbe = bUsePersistenceStatusSurfaceProbe;
		Request.bUsePersistenceStatusDisplayRowsProbe = bUsePersistenceStatusDisplayRowsProbe;
		Request.bUsePersistenceStatusSampleSurfaceProbe = bUsePersistenceStatusSampleSurfaceProbe;
		Request.bUsePersistenceSettingsDraftProbe = bUsePersistenceSettingsDraftProbe;
		Request.bUseSQLiteMigrationProbe = bUseSQLiteMigrationProbe;
		Request.bUseSQLiteLayoutSchemaMigrationProbe = bUseSQLiteLayoutSchemaMigrationProbe;
		Request.bUseSQLiteLayoutReadProbe = bUseSQLiteLayoutReadProbe;
		Request.bUseSQLiteReadOnlyLayoutRepository = bUseSQLiteReadOnlyLayoutRepository;
		Request.bUseSQLiteSaveLayoutRepository = bUseSQLiteSaveLayoutRepository;
		Request.bUseSQLiteRemoveLayoutRepository = bUseSQLiteRemoveLayoutRepository;
		Request.bUseSQLiteClearLayoutsRepository = bUseSQLiteClearLayoutsRepository;
		Request.bUseSQLiteFullLifecycleRepository = bUseSQLiteFullLifecycleRepository;
		Request.bUseSQLiteAsyncCallbackRepository = bUseSQLiteAsyncCallbackRepository;
		Request.bUseSQLiteSerializedAsyncCallbackRepository = bUseSQLiteSerializedAsyncCallbackRepository;
		Request.bUseSQLiteFactoryLayoutRepository = bUseSQLiteFactoryLayoutRepository;
		Request.bUseSQLiteFactorySchemaInitRepository = bUseSQLiteFactorySchemaInitRepository;
		Request.bUseSQLiteSchemaInitHardening = bUseSQLiteSchemaInitHardening;
		Request.bUseSQLiteSeedDatabaseCopyPolicyProbe = bUseSQLiteSeedDatabaseCopyPolicyProbe;
		Request.bUseSQLiteMigrationVersioningPolicyProbe = bUseSQLiteMigrationVersioningPolicyProbe;
		Result = USQLUISampleSmokeTestRunner::RunSmokeTest(CommandletWorld, Request);
	}

	if (Result.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample smoke test commandlet succeeded."));
	}
	else
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample smoke test commandlet failed."));
	}

	LogSQLUISampleSmokeTestResult(Result);
	DestroySQLUISampleSmokeTestCommandletWorld(CommandletWorld);

	return Result.bSucceeded ? 0 : 1;
}
