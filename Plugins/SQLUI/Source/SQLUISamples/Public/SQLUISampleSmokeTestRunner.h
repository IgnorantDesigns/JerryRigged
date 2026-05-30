#pragma once

#include "CoreMinimal.h"
#include "Database/SQLUIDatabaseAsyncTypes.h"
#include "Database/SQLUISQLiteLayoutReadProbe.h"
#include "Database/SQLUISQLiteLayoutSchemaMigration.h"
#include "Database/SQLUISQLiteMigrationTypes.h"
#include "Database/SQLUISQLiteProbe.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Pipeline/SQLUIRuntimeWidgetPipeline.h"
#include "UObject/Object.h"

#include "SQLUISampleSmokeTestRunner.generated.h"

class APlayerController;
class USQLUIBaseWidget;

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISampleRepositoryOperationSmokeResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListAfterSaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSavedLayoutMetadataFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ListAfterSaveLayoutCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString ListAfterSaveErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRemoveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSavedLayoutRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString RemoveErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListAfterRemoveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRemovedLayoutMetadataAbsent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ListAfterRemoveLayoutCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString ListAfterRemoveErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bClearSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ClearRemovedCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ExpectedClearRemovedCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString ClearErrorMessage;
};

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISampleSQLiteReadOnlyLayoutRepositorySmokeResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bPreparedDatabase = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListedMetadataFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadedDocumentValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSaveRejected = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRemoveRejected = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bClearRejected = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListAfterRejectedWritesSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListedMetadataFoundAfterRejectedWrites = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadAfterRejectedWritesSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadedDocumentValidAfterRejectedWrites = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bDatabaseRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ListedLayoutCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString SeedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString LoadedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISampleSQLiteSaveLayoutRepositorySmokeResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bDatabasePrepared = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bFirstSaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListAfterSaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListedSavedMetadataFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadAfterSaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadedDocumentValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSecondSaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListAfterSecondSaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListedUpdatedMetadataFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadAfterSecondSaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLatestRevisionLoaded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bDatabaseRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ListedLayoutCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString SavedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString LoadedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISampleSQLiteRemoveLayoutRepositorySmokeResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bDatabasePrepared = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListBeforeRemoveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListedMetadataFoundBeforeRemove = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadBeforeRemoveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadedDocumentValidBeforeRemove = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRemoveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bListAfterRemoveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bMetadataAbsentAfterRemove = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bLoadAfterRemoveFailedAsExpected = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRevisionsPreserved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bDatabaseRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ListedLayoutCountBeforeRemove = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 ListedLayoutCountAfterRemove = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 RevisionCountAfterRemove = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString SavedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString RemovedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString LoadedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString DatabasePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISampleSmokeTestRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Samples")
	TObjectPtr<APlayerController> OwningPlayer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bApplyProperties = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bApplyBindings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bApplyActions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bExecuteActions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bStopOnOptionalStepFailure = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseJsonLayoutFixture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseInMemoryLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseJsonFileLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseSQLiteCoreProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseDatabaseAsyncProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseSQLiteMigrationProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseSQLiteLayoutSchemaMigrationProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseSQLiteLayoutReadProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseSQLiteReadOnlyLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseSQLiteSaveLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUseSQLiteRemoveLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString SampleFilterText = TEXT("Smoke test");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString SampleFilterVariableValue = TEXT("Smoke test variable");
};

USTRUCT(BlueprintType)
struct SQLUISAMPLES_API FSQLUISampleSmokeTestResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedJsonLayoutFixture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bJsonLayoutFixtureParseSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bJsonLayoutFixtureValidationSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUILayoutValidationResult JsonLayoutFixtureValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRepositorySelectionSmokeTested = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUnavailableRepositorySelectionSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUnavailableRepositorySaveBackendUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUnavailableRepositoryLoadBackendUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString UnavailableRepositorySelectionErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedInMemoryLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRepositorySaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRepositoryLoadSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString SavedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString LoadedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString RepositorySaveErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString RepositoryLoadErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUILayoutValidationResult RepositorySaveValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUILayoutValidationResult RepositoryLoadValidation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISampleRepositoryOperationSmokeResult RepositoryOperationSmoke;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedJsonFileLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bJsonFileRepositorySaveSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bJsonFileRepositoryLoadSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString JsonFileRepositorySavedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString JsonFileRepositoryLoadedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString JsonFileRepositorySaveErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FString JsonFileRepositoryLoadErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISampleRepositoryOperationSmokeResult JsonFileRepositoryOperationSmoke;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedSQLiteCoreProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISQLiteProbeResult SQLiteCoreProbe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedDatabaseAsyncProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUIDatabaseAsyncResult DatabaseAsyncProbe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedSQLiteMigrationProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISQLiteMigrationResult SQLiteMigrationProbe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedSQLiteLayoutSchemaMigrationProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISQLiteLayoutSchemaMigrationProbeResult SQLiteLayoutSchemaMigrationProbe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedSQLiteLayoutReadProbe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISQLiteLayoutReadProbeResult SQLiteLayoutReadProbe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedSQLiteReadOnlyLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISampleSQLiteReadOnlyLayoutRepositorySmokeResult SQLiteReadOnlyLayoutRepository;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedSQLiteSaveLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISampleSQLiteSaveLayoutRepositorySmokeResult SQLiteSaveLayoutRepository;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bUsedSQLiteRemoveLayoutRepository = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	FSQLUISampleSQLiteRemoveLayoutRepositorySmokeResult SQLiteRemoveLayoutRepository;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	TArray<FString> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	TArray<FString> Warnings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	TArray<FSQLUIRuntimeWidgetPipelineStepResult> StepResults;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	int32 CreatedWidgetCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Samples")
	bool bRootWidgetValid = false;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "SQLUI|Samples")
	TObjectPtr<USQLUIBaseWidget> RootWidget = nullptr;
};

UCLASS(BlueprintType)
class SQLUISAMPLES_API USQLUISampleSmokeTestRunner : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples", meta = (WorldContext = "WorldContextObject"))
	static FSQLUISampleSmokeTestResult RunSmokeTest(
		UObject* WorldContextObject,
		const FSQLUISampleSmokeTestRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Samples", meta = (WorldContext = "WorldContextObject"))
	static FSQLUISampleSmokeTestResult RunJsonLayoutSmokeTest(
		UObject* WorldContextObject,
		const FSQLUISampleSmokeTestRequest& Request);
};
