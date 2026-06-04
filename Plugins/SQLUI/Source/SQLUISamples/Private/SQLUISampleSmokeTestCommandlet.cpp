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
