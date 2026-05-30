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
	LogSQLUISampleSmokeTestSQLiteMigrationProbeResult(Result);
	LogSQLUISampleSmokeTestSQLiteLayoutSchemaMigrationProbeResult(Result);
	LogSQLUISampleSmokeTestSQLiteLayoutReadProbeResult(Result);
	LogSQLUISampleSmokeTestSQLiteReadOnlyLayoutRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteSaveLayoutRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteRemoveLayoutRepositoryResult(Result);
	LogSQLUISampleSmokeTestSQLiteClearLayoutsRepositoryResult(Result);

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
		Request.bUseSQLiteMigrationProbe = bUseSQLiteMigrationProbe;
		Request.bUseSQLiteLayoutSchemaMigrationProbe = bUseSQLiteLayoutSchemaMigrationProbe;
		Request.bUseSQLiteLayoutReadProbe = bUseSQLiteLayoutReadProbe;
		Request.bUseSQLiteReadOnlyLayoutRepository = bUseSQLiteReadOnlyLayoutRepository;
		Request.bUseSQLiteSaveLayoutRepository = bUseSQLiteSaveLayoutRepository;
		Request.bUseSQLiteRemoveLayoutRepository = bUseSQLiteRemoveLayoutRepository;
		Request.bUseSQLiteClearLayoutsRepository = bUseSQLiteClearLayoutsRepository;
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
