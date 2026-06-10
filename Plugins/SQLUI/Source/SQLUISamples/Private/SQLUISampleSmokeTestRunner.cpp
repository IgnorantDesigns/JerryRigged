#include "SQLUISampleSmokeTestRunner.h"

#include "SQLUISamplePersistenceStatusPanelAdapter.h"
#include "SQLUISamplePersistenceStatusPanelWidget.h"
#include "SQLUISamplePersistenceStatusPresenter.h"
#include "SQLUISamplePersistenceSettingsDraftPanelWidget.h"
#include "SQLUISamplePersistenceSettingsDraftPresenter.h"
#include "Database/SQLUIDatabaseAsyncQueue.h"
#include "Database/SQLUIDatabaseAsyncRunner.h"
#include "Database/SQLUISQLiteLayoutReadProbe.h"
#include "Database/SQLUISQLiteLayoutSchemaMigration.h"
#include "Database/SQLUISQLiteLayoutSchemaVersioning.h"
#include "Database/SQLUISQLiteMigrationRunner.h"
#include "Database/SQLUISQLiteProbe.h"
#include "Database/SQLUISQLiteSeedDatabaseCopy.h"
#include "Async/TaskGraphInterfaces.h"
#include "Engine/GameInstance.h"
#include "HAL/Event.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Layout/ISQLUILayoutRepository.h"
#include "Layout/SQLUIInMemoryLayoutRepository.h"
#include "Layout/SQLUIJsonFileLayoutRepository.h"
#include "Layout/SQLUIPersistenceSettingsDraft.h"
#include "Layout/SQLUIPersistenceSettingsDraftDisplay.h"
#include "Layout/SQLUIPersistenceStatus.h"
#include "Layout/SQLUIPersistenceStatusDisplay.h"
#include "Layout/SQLUILayoutJson.h"
#include "Layout/SQLUILayoutRepositoryDatabaseManagement.h"
#include "Layout/SQLUILayoutRepositoryFactory.h"
#include "Layout/SQLUILayoutPersistenceWorkflow.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"
#include "Layout/SQLUILayoutRepositoryRuntimeIntegration.h"
#include "Layout/SQLUILayoutRepositoryRuntimeProvider.h"
#include "Layout/SQLUILayoutRepositoryRuntimeSettings.h"
#include "Layout/SQLUISQLiteLayoutRepository.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "Variables/SQLUIVariableStore.h"
#include "Variables/SQLUIVariableTypes.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"
#include "WidgetCatalog/SQLUIWidgetCatalogRegistrar.h"
#include "UObject/Class.h"
#include "UObject/Package.h"

namespace
{
const TCHAR* SQLUISampleSmokeTestFilterVariableName = TEXT("Sample.FilterText");

struct FSQLUISampleLayoutJsonFixture
{
	static FString GetJsonString()
	{
		return TEXT(R"SQLUIJSON(
{
	"Version": {
		"SchemaVersion": 1,
		"Revision": 1,
		"Label": "SQLUI Sample JSON Layout Fixture"
	},
	"Metadata": {
		"LayoutId": "sqlui.sample.json-layout-fixture",
		"DisplayName": "SQLUI Sample JSON Layout Fixture",
		"Description": "Minimal SQLUISamples JSON layout fixture for the runtime widget pipeline smoke test.",
		"CreatedBy": "SQLUISamples",
		"CreatedAtUtc": "",
		"UpdatedAtUtc": "",
		"Tags": [],
		"SearchMetadata": {}
	},
	"RootWidgetId": "SQLUI.Sample.Json.FilterRoot",
	"Nodes": [
		{
			"WidgetId": "SQLUI.Sample.Json.FilterRoot",
			"ParentWidgetId": "",
			"WidgetTypeKey": "SQLUI.FilterBox",
			"ChildWidgetIds": [],
			"Properties": {
				"FilterText": "Smoke test JSON fixture",
				"IsEnabled": "true"
			},
			"Bindings": [
				{
					"BindingId": "SQLUI.Sample.Json.FilterTextBinding",
					"TargetProperty": "FilterText",
					"SourceKey": "Variable",
					"SourcePath": "Sample.FilterText",
					"TransformKey": "",
					"Options": {}
				}
			],
			"Actions": [],
			"Tags": [],
			"SearchMetadata": {}
		}
	],
	"Properties": {}
}
)SQLUIJSON");
	}
};

struct FSQLUISampleSmokeTestLayoutResult
{
	bool bSucceeded = true;
	bool bUsedJsonLayoutFixture = false;
	bool bJsonLayoutFixtureParseSucceeded = false;
	bool bJsonLayoutFixtureValidationSucceeded = false;
	FSQLUILayoutValidationResult JsonLayoutFixtureValidation;
	bool bRepositorySelectionSmokeTested = false;
	bool bUnavailableRepositorySelectionSucceeded = false;
	bool bUnavailableRepositorySaveBackendUnavailable = false;
	bool bUnavailableRepositoryLoadBackendUnavailable = false;
	FString UnavailableRepositorySelectionErrorMessage;
	bool bUsedInMemoryLayoutRepository = false;
	bool bRepositorySaveSucceeded = false;
	bool bRepositoryLoadSucceeded = false;
	FString SavedLayoutId;
	FString LoadedLayoutId;
	FString RepositorySaveErrorMessage;
	FString RepositoryLoadErrorMessage;
	FSQLUILayoutValidationResult RepositorySaveValidation;
	FSQLUILayoutValidationResult RepositoryLoadValidation;
	FSQLUISampleRepositoryOperationSmokeResult RepositoryOperationSmoke;
	bool bUsedJsonFileLayoutRepository = false;
	bool bJsonFileRepositorySaveSucceeded = false;
	bool bJsonFileRepositoryLoadSucceeded = false;
	FString JsonFileRepositorySavedLayoutId;
	FString JsonFileRepositoryLoadedLayoutId;
	FString JsonFileRepositorySaveErrorMessage;
	FString JsonFileRepositoryLoadErrorMessage;
	FSQLUILayoutValidationResult JsonFileRepositorySaveValidation;
	FSQLUILayoutValidationResult JsonFileRepositoryLoadValidation;
	FSQLUISampleRepositoryOperationSmokeResult JsonFileRepositoryOperationSmoke;
	TArray<FString> Warnings;
	FSQLUILayoutDocument Document;
};

void AddSQLUISampleSmokeTestError(
	FSQLUISampleSmokeTestResult& Result,
	const FString& ErrorMessage)
{
	if (!ErrorMessage.IsEmpty())
	{
		Result.Errors.Add(ErrorMessage);
	}

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = ErrorMessage;
	}
}

UObject* ResolveSQLUISampleSmokeTestOuter(
	UObject* WorldContextObject,
	const FSQLUISampleSmokeTestRequest& Request)
{
	if (IsValid(WorldContextObject))
	{
		return WorldContextObject;
	}

	if (IsValid(Request.OwningPlayer.Get()))
	{
		return Request.OwningPlayer.Get();
	}

	return GetTransientPackage();
}

FSQLUILayoutDocument MakeSQLUISampleSmokeTestDocument(
	const FSQLUISampleSmokeTestRequest& Request)
{
	FSQLUILayoutDocument Document;
	Document.Version.SchemaVersion = 1;
	Document.Version.Revision = 1;
	Document.Version.Label = TEXT("SQLUI Sample Pipeline Smoke Test");
	Document.Metadata.LayoutId = TEXT("sqlui.sample.pipeline.smoke-test");
	Document.Metadata.DisplayName = TEXT("SQLUI Sample Pipeline Smoke Test");
	Document.Metadata.Description = TEXT("Minimal in-memory SQLUI runtime widget pipeline smoke test layout.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.RootWidgetId = TEXT("SQLUI.Sample.FilterRoot");

	FSQLUILayoutBinding FilterTextBinding;
	FilterTextBinding.BindingId = TEXT("SQLUI.Sample.FilterTextBinding");
	FilterTextBinding.TargetProperty = TEXT("FilterText");
	FilterTextBinding.SourceKey = TEXT("Variable");
	FilterTextBinding.SourcePath = SQLUISampleSmokeTestFilterVariableName;

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = Document.RootWidgetId;
	RootNode.WidgetTypeKey = FSQLUIWidgetTypeKeys::FilterBox().Value;
	RootNode.Properties.Add(TEXT("FilterText"), Request.SampleFilterText);
	RootNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));
	RootNode.Bindings.Add(FilterTextBinding);

	Document.Nodes.Add(RootNode);
	return Document;
}

bool IsSQLUISampleLayoutJsonParseFailureMessage(const FString& ErrorMessage)
{
	return ErrorMessage.Contains(TEXT("deserialize"), ESearchCase::IgnoreCase);
}

bool DidSQLUISampleLayoutJsonParseFail(const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		if (IsSQLUISampleLayoutJsonParseFailureMessage(Error))
		{
			return true;
		}
	}

	return false;
}

FSQLUILayoutSaveResult SaveSQLUISampleLayoutToRepository(
	ISQLUILayoutRepository* Repository,
	const TCHAR* RepositoryName,
	const FSQLUILayoutDocument& Document)
{
	FSQLUILayoutSaveResult SaveResult;
	SaveResult.SavedLayoutId = Document.Metadata.LayoutId;

	if (!Repository)
	{
		SaveResult.ErrorMessage = FString::Printf(
			TEXT("SQLUI sample smoke test failed: could not create %s."),
			RepositoryName);
		return SaveResult;
	}

	bool bSaveCompleted = false;
	Repository->SaveLayout(
		Document,
		FSQLUILayoutSaveCompleteDelegate::CreateLambda(
			[&SaveResult, &bSaveCompleted](const FSQLUILayoutSaveResult& InSaveResult)
			{
				SaveResult = InSaveResult;
				bSaveCompleted = true;
			}));

	if (!bSaveCompleted)
	{
		SaveResult.bSucceeded = false;
		SaveResult.ErrorMessage = FString::Printf(
			TEXT("SQLUI sample smoke test failed: %s save did not complete."),
			RepositoryName);
	}

	return SaveResult;
}

FSQLUILayoutLoadResult LoadSQLUISampleLayoutFromRepository(
	ISQLUILayoutRepository* Repository,
	const TCHAR* RepositoryName,
	const FString& LayoutId)
{
	FSQLUILayoutLoadResult LoadResult;
	LoadResult.Document.Metadata.LayoutId = LayoutId;

	if (!Repository)
	{
		LoadResult.ErrorMessage = FString::Printf(
			TEXT("SQLUI sample smoke test failed: could not create %s."),
			RepositoryName);
		return LoadResult;
	}

	bool bLoadCompleted = false;
	Repository->LoadLayout(
		LayoutId,
		FSQLUILayoutLoadCompleteDelegate::CreateLambda(
			[&LoadResult, &bLoadCompleted](const FSQLUILayoutLoadResult& InLoadResult)
			{
				LoadResult = InLoadResult;
				bLoadCompleted = true;
			}));

	if (!bLoadCompleted)
	{
		LoadResult.bSucceeded = false;
		LoadResult.ErrorMessage = FString::Printf(
			TEXT("SQLUI sample smoke test failed: %s load did not complete."),
			RepositoryName);
	}

	return LoadResult;
}

bool DoesSQLUISampleLayoutMetadataMatch(
	const FSQLUILayoutMetadata& Candidate,
	const FSQLUILayoutMetadata& Expected)
{
	return Candidate.LayoutId == Expected.LayoutId
		&& Candidate.DisplayName == Expected.DisplayName
		&& Candidate.Description == Expected.Description
		&& Candidate.CreatedBy == Expected.CreatedBy;
}

bool DoSQLUISampleTagSetsMatch(
	const TArray<FString>& CandidateTags,
	const TArray<FString>& ExpectedTags)
{
	TSet<FString> CandidateTagSet;
	for (const FString& Tag : CandidateTags)
	{
		CandidateTagSet.Add(Tag);
	}

	TSet<FString> ExpectedTagSet;
	for (const FString& Tag : ExpectedTags)
	{
		ExpectedTagSet.Add(Tag);
	}

	if (CandidateTagSet.Num() != ExpectedTagSet.Num())
	{
		return false;
	}

	for (const FString& ExpectedTag : ExpectedTagSet)
	{
		if (!CandidateTagSet.Contains(ExpectedTag))
		{
			return false;
		}
	}

	return true;
}

bool DoesSQLUISampleLayoutMetadataAndTagsMatch(
	const FSQLUILayoutMetadata& Candidate,
	const FSQLUILayoutMetadata& Expected)
{
	return DoesSQLUISampleLayoutMetadataMatch(Candidate, Expected)
		&& DoSQLUISampleTagSetsMatch(Candidate.Tags, Expected.Tags);
}

bool DoesSQLUISampleLayoutMetadataListContain(
	const TArray<FSQLUILayoutMetadata>& Layouts,
	const FSQLUILayoutMetadata& Expected)
{
	for (const FSQLUILayoutMetadata& Layout : Layouts)
	{
		if (DoesSQLUISampleLayoutMetadataMatch(Layout, Expected))
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUISampleLayoutMetadataAndTagsListContain(
	const TArray<FSQLUILayoutMetadata>& Layouts,
	const FSQLUILayoutMetadata& Expected)
{
	for (const FSQLUILayoutMetadata& Layout : Layouts)
	{
		if (DoesSQLUISampleLayoutMetadataAndTagsMatch(Layout, Expected))
		{
			return true;
		}
	}

	return false;
}

FString MakeSQLUISampleRepositoryOperationError(
	const TCHAR* RepositoryName,
	const TCHAR* OperationName,
	const FString& ErrorMessage)
{
	if (!ErrorMessage.IsEmpty())
	{
		return ErrorMessage;
	}

	return FString::Printf(
		TEXT("SQLUI sample smoke test failed: %s %s failed."),
		RepositoryName,
		OperationName);
}

FString MakeSQLUISampleRepositoryMissingMetadataError(
	const TCHAR* RepositoryName,
	const FString& LayoutId)
{
	return FString::Printf(
		TEXT("SQLUI sample smoke test failed: %s list after save did not include saved layout metadata for layout id '%s'."),
		RepositoryName,
		*LayoutId);
}

FString MakeSQLUISampleRepositoryUnexpectedMetadataError(
	const TCHAR* RepositoryName,
	const FString& LayoutId)
{
	return FString::Printf(
		TEXT("SQLUI sample smoke test failed: %s list after remove still included layout metadata for layout id '%s'."),
		RepositoryName,
		*LayoutId);
}

FString MakeSQLUISampleRepositoryRemoveDidNotRemoveError(
	const TCHAR* RepositoryName,
	const FString& LayoutId,
	const FString& ErrorMessage)
{
	if (!ErrorMessage.IsEmpty())
	{
		return ErrorMessage;
	}

	return FString::Printf(
		TEXT("SQLUI sample smoke test failed: %s remove succeeded but did not remove layout id '%s'."),
		RepositoryName,
		*LayoutId);
}

FString MakeSQLUISampleRepositoryClearCountError(
	const TCHAR* RepositoryName,
	const int32 RemovedCount,
	const int32 ExpectedRemovedCount)
{
	return FString::Printf(
		TEXT("SQLUI sample smoke test failed: %s clear removed %d layout(s), expected %d."),
		RepositoryName,
		RemovedCount,
		ExpectedRemovedCount);
}

bool DidSQLUISampleRepositoryOperationSmokeSucceed(
	const FSQLUISampleRepositoryOperationSmokeResult& Result)
{
	return Result.bListAfterSaveSucceeded
		&& Result.bSavedLayoutMetadataFound
		&& Result.bRemoveSucceeded
		&& Result.bSavedLayoutRemoved
		&& Result.bListAfterRemoveSucceeded
		&& Result.bRemovedLayoutMetadataAbsent
		&& Result.bClearSucceeded;
}

void RunSQLUISampleUnavailableRepositorySelectionSmoke(
	UObject* Outer,
	const FSQLUILayoutDocument& Document,
	FSQLUISampleSmokeTestLayoutResult& Result)
{
	Result.bRepositorySelectionSmokeTested = true;

	FSQLUILayoutRepositoryFactorySettings Settings;
	Settings.Backend = ESQLUILayoutRepositoryBackend::Unavailable;

	USQLUILayoutRepository* Repository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, Settings);
	if (!IsValid(Repository))
	{
		Result.UnavailableRepositorySelectionErrorMessage =
			TEXT("SQLUI sample smoke test failed: layout repository factory did not create an unavailable repository.");
		return;
	}

	const FSQLUILayoutSaveResult SaveResult = SaveSQLUISampleLayoutToRepository(
		Repository,
		TEXT("unavailable layout repository"),
		Document);
	const FSQLUILayoutLoadResult LoadResult = LoadSQLUISampleLayoutFromRepository(
		Repository,
		TEXT("unavailable layout repository"),
		Document.Metadata.LayoutId);

	Result.bUnavailableRepositorySaveBackendUnavailable =
		!SaveResult.bSucceeded && SaveResult.bBackendUnavailable;
	Result.bUnavailableRepositoryLoadBackendUnavailable =
		!LoadResult.bSucceeded && LoadResult.bBackendUnavailable;
	Result.bUnavailableRepositorySelectionSucceeded =
		Result.bUnavailableRepositorySaveBackendUnavailable
		&& Result.bUnavailableRepositoryLoadBackendUnavailable;

	if (!Result.bUnavailableRepositorySelectionSucceeded)
	{
		Result.UnavailableRepositorySelectionErrorMessage = FString::Printf(
			TEXT("SQLUI sample smoke test failed: unavailable layout repository did not report backend unavailable cleanly. SaveError='%s' LoadError='%s'"),
			*SaveResult.ErrorMessage,
			*LoadResult.ErrorMessage);
	}
}

template<typename LayoutRepositoryType>
void RunSQLUISampleLayoutRepositorySmoke(
	LayoutRepositoryType* Repository,
	const TCHAR* RepositoryName,
	const FSQLUILayoutDocument& Document,
	FSQLUILayoutSaveResult& OutSaveResult,
	FSQLUILayoutLoadResult& OutLoadResult,
	FSQLUISampleRepositoryOperationSmokeResult& OutOperationResult)
{
	OutSaveResult = SaveSQLUISampleLayoutToRepository(
		Repository,
		RepositoryName,
		Document);

	const FString LayoutId = Document.Metadata.LayoutId;
	if (!OutSaveResult.bSucceeded)
	{
		return;
	}

	const FSQLUILayoutRepositoryListResult ListAfterSaveResult = Repository->ListLayouts();
	OutOperationResult.bListAfterSaveSucceeded = ListAfterSaveResult.bSucceeded;
	OutOperationResult.ListAfterSaveLayoutCount = ListAfterSaveResult.Layouts.Num();
	if (!ListAfterSaveResult.bSucceeded)
	{
		OutOperationResult.ListAfterSaveErrorMessage =
			MakeSQLUISampleRepositoryOperationError(
				RepositoryName,
				TEXT("list after save"),
				ListAfterSaveResult.ErrorMessage);
	}
	else
	{
		OutOperationResult.bSavedLayoutMetadataFound =
			DoesSQLUISampleLayoutMetadataListContain(
				ListAfterSaveResult.Layouts,
				Document.Metadata);
		if (!OutOperationResult.bSavedLayoutMetadataFound)
		{
			OutOperationResult.ListAfterSaveErrorMessage =
				MakeSQLUISampleRepositoryMissingMetadataError(
					RepositoryName,
					LayoutId);
		}
	}

	OutLoadResult = LoadSQLUISampleLayoutFromRepository(
		Repository,
		RepositoryName,
		LayoutId);

	const FSQLUILayoutRepositoryRemoveResult RemoveResult = Repository->RemoveLayout(LayoutId);
	OutOperationResult.bRemoveSucceeded = RemoveResult.bSucceeded;
	OutOperationResult.bSavedLayoutRemoved = RemoveResult.bRemoved;
	if (!RemoveResult.bSucceeded)
	{
		OutOperationResult.RemoveErrorMessage =
			MakeSQLUISampleRepositoryOperationError(
				RepositoryName,
				TEXT("remove"),
				RemoveResult.ErrorMessage);
	}
	else if (!RemoveResult.bRemoved)
	{
		OutOperationResult.RemoveErrorMessage =
			MakeSQLUISampleRepositoryRemoveDidNotRemoveError(
				RepositoryName,
				LayoutId,
				RemoveResult.ErrorMessage);
	}

	const FSQLUILayoutRepositoryListResult ListAfterRemoveResult = Repository->ListLayouts();
	OutOperationResult.bListAfterRemoveSucceeded = ListAfterRemoveResult.bSucceeded;
	OutOperationResult.ListAfterRemoveLayoutCount = ListAfterRemoveResult.Layouts.Num();
	if (!ListAfterRemoveResult.bSucceeded)
	{
		OutOperationResult.ListAfterRemoveErrorMessage =
			MakeSQLUISampleRepositoryOperationError(
				RepositoryName,
				TEXT("list after remove"),
				ListAfterRemoveResult.ErrorMessage);
	}
	else
	{
		OutOperationResult.bRemovedLayoutMetadataAbsent =
			!DoesSQLUISampleLayoutMetadataListContain(
				ListAfterRemoveResult.Layouts,
				Document.Metadata);
		if (!OutOperationResult.bRemovedLayoutMetadataAbsent)
		{
			OutOperationResult.ListAfterRemoveErrorMessage =
				MakeSQLUISampleRepositoryUnexpectedMetadataError(
					RepositoryName,
					LayoutId);
		}
	}

	OutOperationResult.ExpectedClearRemovedCount =
		ListAfterRemoveResult.bSucceeded ? ListAfterRemoveResult.Layouts.Num() : 0;

	const FSQLUILayoutRepositoryClearResult ClearResult = Repository->ClearLayouts();
	OutOperationResult.ClearRemovedCount = ClearResult.RemovedCount;
	if (!ClearResult.bSucceeded)
	{
		OutOperationResult.ClearErrorMessage =
			MakeSQLUISampleRepositoryOperationError(
				RepositoryName,
				TEXT("clear"),
				ClearResult.ErrorMessage);
	}
	else if (!ListAfterRemoveResult.bSucceeded)
	{
		OutOperationResult.ClearErrorMessage = FString::Printf(
			TEXT("SQLUI sample smoke test failed: %s clear succeeded but RemovedCount could not be verified because list after remove failed."),
			RepositoryName);
	}
	else if (ClearResult.RemovedCount != OutOperationResult.ExpectedClearRemovedCount)
	{
		OutOperationResult.ClearErrorMessage =
			MakeSQLUISampleRepositoryClearCountError(
				RepositoryName,
				ClearResult.RemovedCount,
				OutOperationResult.ExpectedClearRemovedCount);
	}
	else
	{
		OutOperationResult.bClearSucceeded = true;
	}
}

FString MakeSQLUISampleJsonFileLayoutRepositoryBaseDirectory()
{
	FString BaseDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("Layouts"));
	FPaths::NormalizeDirectoryName(BaseDirectory);
	return BaseDirectory;
}

void AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
	FSQLUISampleSQLiteReadOnlyLayoutRepositorySmokeResult& Result,
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

FString MakeSQLUISampleSQLiteReadOnlyLayoutRepositoryDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteReadOnlyRepository"),
		TEXT("SQLiteReadOnlyRepository.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUISampleSQLiteReadOnlyLayoutRepositoryFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteReadOnlyLayoutRepositorySmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite read-only layout repository smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
	const TArray<FSQLUILayoutMetadata>& Layouts,
	const FSQLUILayoutMetadata& Expected)
{
	for (const FSQLUILayoutMetadata& Layout : Layouts)
	{
		if (DoesSQLUISampleLayoutMetadataAndTagsMatch(Layout, Expected))
		{
			return true;
		}
	}

	return false;
}

bool IsSQLUISampleSQLiteReadOnlyWriteRejectionMessage(const FString& ErrorMessage)
{
	return !ErrorMessage.IsEmpty()
		&& (ErrorMessage.Contains(TEXT("read-only"), ESearchCase::IgnoreCase)
			|| ErrorMessage.Contains(TEXT("unsupported"), ESearchCase::IgnoreCase));
}

FSQLUISampleSQLiteReadOnlyLayoutRepositorySmokeResult RunSQLUISampleSQLiteReadOnlyLayoutRepositorySmoke(
	UObject* Outer)
{
	FSQLUISampleSQLiteReadOnlyLayoutRepositorySmokeResult Result;
	Result.DatabasePath = MakeSQLUISampleSQLiteReadOnlyLayoutRepositoryDatabasePath();

	const FSQLUISQLiteLayoutReadProbeResult PrepareResult =
		FSQLUISQLiteLayoutReadProbe::RunProbe(Result.DatabasePath, false);
	Result.SeedLayoutId = PrepareResult.SeedLayoutId;
	Result.bPreparedDatabase =
		PrepareResult.bSucceeded
		&& PrepareResult.bSchemaMigrationSucceeded
		&& PrepareResult.bSeedInserted
		&& PrepareResult.bListSucceeded
		&& PrepareResult.bLoadSucceeded
		&& PrepareResult.bLoadedDocumentValid;

	if (!Result.bPreparedDatabase)
	{
		AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
			Result,
			PrepareResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite read-only layout repository smoke failed: could not prepare probe database.")
				: PrepareResult.ErrorMessage);
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteReadOnlyLayoutRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
			Result,
			TEXT("SQLUI SQLite read-only layout repository smoke failed: could not create repository object."));
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteReadOnlyLayoutRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = Result.DatabasePath;
	RepositorySettings.bReadOnly = true;
	Repository->Configure(RepositorySettings);

	const FSQLUILayoutRepositoryListResult ListResult = Repository->ListLayouts();
	Result.bListSucceeded = ListResult.bSucceeded;
	Result.ListedLayoutCount = ListResult.Layouts.Num();
	if (!Result.bListSucceeded)
	{
		AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
			Result,
			ListResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite read-only layout repository smoke failed: ListLayouts failed.")
				: ListResult.ErrorMessage);
	}

	const FSQLUILayoutLoadResult LoadResult = LoadSQLUISampleLayoutFromRepository(
		Repository,
		TEXT("SQLite read-only layout repository"),
		Result.SeedLayoutId);
	Result.bLoadSucceeded = LoadResult.bSucceeded;
	Result.LoadedLayoutId = LoadResult.Document.Metadata.LayoutId;
	Result.bLoadedDocumentValid =
		LoadResult.bSucceeded
		&& LoadResult.Validation.bIsValid
		&& Result.LoadedLayoutId == Result.SeedLayoutId;

	if (!Result.bLoadSucceeded)
	{
		AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
			Result,
			LoadResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite read-only layout repository smoke failed: LoadLayout failed.")
				: LoadResult.ErrorMessage);
	}
	else if (!Result.bLoadedDocumentValid)
	{
		AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite read-only layout repository smoke failed: loaded layout id '%s' did not match seed layout id '%s' or validation failed."),
				*Result.LoadedLayoutId,
				*Result.SeedLayoutId));
	}

	if (Result.bListSucceeded && Result.bLoadedDocumentValid)
	{
		Result.bListedMetadataFound =
			DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
				ListResult.Layouts,
				LoadResult.Document.Metadata);
		if (!Result.bListedMetadataFound)
		{
			AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite read-only layout repository smoke failed: ListLayouts did not include loaded metadata and tags for layout id '%s'."),
					*Result.SeedLayoutId));
		}
	}

	if (Result.bLoadedDocumentValid)
	{
		const FSQLUILayoutSaveResult SaveResult = SaveSQLUISampleLayoutToRepository(
			Repository,
			TEXT("SQLite read-only layout repository"),
			LoadResult.Document);
		Result.bSaveRejected =
			!SaveResult.bSucceeded
			&& IsSQLUISampleSQLiteReadOnlyWriteRejectionMessage(SaveResult.ErrorMessage);
		if (!Result.bSaveRejected)
		{
			AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite read-only layout repository smoke failed: SaveLayout was not clearly rejected. bSucceeded=%s Error='%s'."),
					SaveResult.bSucceeded ? TEXT("true") : TEXT("false"),
					*SaveResult.ErrorMessage));
		}

		const FSQLUILayoutRepositoryRemoveResult RemoveResult =
			Repository->RemoveLayout(Result.SeedLayoutId);
		Result.bRemoveRejected =
			!RemoveResult.bSucceeded
			&& !RemoveResult.bRemoved
			&& IsSQLUISampleSQLiteReadOnlyWriteRejectionMessage(RemoveResult.ErrorMessage);
		if (!Result.bRemoveRejected)
		{
			AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite read-only layout repository smoke failed: RemoveLayout was not clearly rejected. bSucceeded=%s bRemoved=%s Error='%s'."),
					RemoveResult.bSucceeded ? TEXT("true") : TEXT("false"),
					RemoveResult.bRemoved ? TEXT("true") : TEXT("false"),
					*RemoveResult.ErrorMessage));
		}

		const FSQLUILayoutRepositoryClearResult ClearResult = Repository->ClearLayouts();
		Result.bClearRejected =
			!ClearResult.bSucceeded
			&& ClearResult.RemovedCount == 0
			&& IsSQLUISampleSQLiteReadOnlyWriteRejectionMessage(ClearResult.ErrorMessage);
		if (!Result.bClearRejected)
		{
			AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite read-only layout repository smoke failed: ClearLayouts was not clearly rejected. bSucceeded=%s RemovedCount=%d Error='%s'."),
					ClearResult.bSucceeded ? TEXT("true") : TEXT("false"),
					ClearResult.RemovedCount,
					*ClearResult.ErrorMessage));
		}

		const FSQLUILayoutRepositoryListResult ListAfterRejectedWritesResult =
			Repository->ListLayouts();
		Result.bListAfterRejectedWritesSucceeded = ListAfterRejectedWritesResult.bSucceeded;
		if (!Result.bListAfterRejectedWritesSucceeded)
		{
			AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
				Result,
				ListAfterRejectedWritesResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite read-only layout repository smoke failed: ListLayouts failed after rejected write attempts.")
					: ListAfterRejectedWritesResult.ErrorMessage);
		}
		else
		{
			Result.bListedMetadataFoundAfterRejectedWrites =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterRejectedWritesResult.Layouts,
					LoadResult.Document.Metadata);
			if (!Result.bListedMetadataFoundAfterRejectedWrites)
			{
				AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
					Result,
					FString::Printf(
						TEXT("SQLUI SQLite read-only layout repository smoke failed: ListLayouts after rejected write attempts did not include loaded metadata and tags for layout id '%s'."),
						*Result.SeedLayoutId));
			}
		}

		const FSQLUILayoutLoadResult LoadAfterRejectedWritesResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite read-only layout repository after rejected write attempts"),
				Result.SeedLayoutId);
		Result.bLoadAfterRejectedWritesSucceeded = LoadAfterRejectedWritesResult.bSucceeded;
		Result.bLoadedDocumentValidAfterRejectedWrites =
			LoadAfterRejectedWritesResult.bSucceeded
			&& LoadAfterRejectedWritesResult.Validation.bIsValid
			&& LoadAfterRejectedWritesResult.Document.Metadata.LayoutId == Result.SeedLayoutId;
		if (!Result.bLoadAfterRejectedWritesSucceeded)
		{
			AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
				Result,
				LoadAfterRejectedWritesResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite read-only layout repository smoke failed: LoadLayout failed after rejected write attempts.")
					: LoadAfterRejectedWritesResult.ErrorMessage);
		}
		else if (!Result.bLoadedDocumentValidAfterRejectedWrites)
		{
			AppendSQLUISampleSQLiteReadOnlyLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite read-only layout repository smoke failed: loaded layout id '%s' after rejected write attempts did not match seed layout id '%s' or validation failed."),
					*LoadAfterRejectedWritesResult.Document.Metadata.LayoutId,
					*Result.SeedLayoutId));
		}
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleSQLiteReadOnlyLayoutRepositoryFiles(Result.DatabasePath, Result);

	Result.bSucceeded =
		Result.bPreparedDatabase
		&& Result.bListSucceeded
		&& Result.bListedMetadataFound
		&& Result.bLoadSucceeded
		&& Result.bLoadedDocumentValid
		&& Result.bSaveRejected
		&& Result.bRemoveRejected
		&& Result.bClearRejected
		&& Result.bListAfterRejectedWritesSucceeded
		&& Result.bListedMetadataFoundAfterRejectedWrites
		&& Result.bLoadAfterRejectedWritesSucceeded
		&& Result.bLoadedDocumentValidAfterRejectedWrites
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
	FSQLUISampleSQLiteSaveLayoutRepositorySmokeResult& Result,
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

FString MakeSQLUISampleSQLiteSaveLayoutRepositoryDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteSaveLayoutRepository"),
		TEXT("SQLiteSaveLayoutRepository.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUISampleSQLiteSaveLayoutRepositoryFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteSaveLayoutRepositorySmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite SaveLayout repository smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteSaveLayoutRepositoryDocument()
{
	FSQLUILayoutDocument Document;
	Document.Version.SchemaVersion = 1;
	Document.Version.Revision = 1;
	Document.Version.Label = TEXT("SQLite SaveLayout Repository Probe");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.sqlite-save-layout-repository");
	Document.Metadata.DisplayName = TEXT("SQLUI SQLite SaveLayout Repository Probe");
	Document.Metadata.Description = TEXT("Smoke/probe layout for SQLite SaveLayout repository mapping.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-05-30T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = Document.Metadata.CreatedAtUtc;
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("save-layout"));
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteSaveLayoutRepository"));
	Document.RootWidgetId = TEXT("SQLUI.SQLite.SaveLayoutRepository.Root");

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = Document.RootWidgetId;
	RootNode.WidgetTypeKey = TEXT("SQLUI.ProbeRoot");
	RootNode.Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
	RootNode.Tags.Add(TEXT("sqlite"));
	RootNode.Tags.Add(TEXT("save-layout"));
	RootNode.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteSaveLayoutRepository"));

	Document.Nodes.Add(RootNode);
	return Document;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteSaveLayoutRepositoryUpdatedDocument(
	const FSQLUILayoutDocument& OriginalDocument)
{
	FSQLUILayoutDocument UpdatedDocument = OriginalDocument;
	UpdatedDocument.Version.Label = TEXT("SQLite SaveLayout Repository Probe Updated");
	UpdatedDocument.Metadata.DisplayName = TEXT("SQLUI SQLite SaveLayout Repository Probe Updated");
	UpdatedDocument.Metadata.Description = TEXT("Updated smoke/probe layout for SQLite SaveLayout repository mapping.");
	UpdatedDocument.Metadata.UpdatedAtUtc = TEXT("2026-05-30T00:01:00Z");
	UpdatedDocument.Metadata.Tags.Add(TEXT("updated"));
	if (UpdatedDocument.Nodes.Num() > 0)
	{
		UpdatedDocument.Nodes[0].Properties.Add(TEXT("Text"), UpdatedDocument.Metadata.DisplayName);
	}
	return UpdatedDocument;
}

FSQLUISampleSQLiteSaveLayoutRepositorySmokeResult RunSQLUISampleSQLiteSaveLayoutRepositorySmoke(
	UObject* Outer)
{
	FSQLUISampleSQLiteSaveLayoutRepositorySmokeResult Result;
	Result.DatabasePath = MakeSQLUISampleSQLiteSaveLayoutRepositoryDatabasePath();

	const FSQLUISQLiteLayoutSchemaMigrationProbeResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::RunProbe(Result.DatabasePath, false);
	Result.bDatabasePrepared = SchemaResult.bSucceeded && SchemaResult.bMigrationSucceeded;
	if (!Result.bDatabasePrepared)
	{
		AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
			Result,
			SchemaResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite SaveLayout repository smoke failed: could not prepare probe database.")
				: SchemaResult.ErrorMessage);
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteSaveLayoutRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
			Result,
			TEXT("SQLUI SQLite SaveLayout repository smoke failed: could not create repository object."));
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteSaveLayoutRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = Result.DatabasePath;
	RepositorySettings.bReadOnly = false;
	Repository->Configure(RepositorySettings);

	const FSQLUILayoutDocument FirstDocument =
		MakeSQLUISampleSQLiteSaveLayoutRepositoryDocument();
	const FSQLUILayoutSaveResult FirstSaveResult = SaveSQLUISampleLayoutToRepository(
		Repository,
		TEXT("SQLite SaveLayout repository"),
		FirstDocument);
	Result.SavedLayoutId = FirstSaveResult.SavedLayoutId;
	Result.bFirstSaveSucceeded =
		FirstSaveResult.bSucceeded
		&& FirstSaveResult.SavedLayoutId == FirstDocument.Metadata.LayoutId
		&& FirstSaveResult.Validation.bIsValid;
	if (!Result.bFirstSaveSucceeded)
	{
		AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
			Result,
			FirstSaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite SaveLayout repository smoke failed: first SaveLayout failed.")
				: FirstSaveResult.ErrorMessage);
	}

	if (Result.bFirstSaveSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListAfterSaveResult =
			Repository->ListLayouts();
		Result.bListAfterSaveSucceeded = ListAfterSaveResult.bSucceeded;
		Result.ListedLayoutCount = ListAfterSaveResult.Layouts.Num();
		if (!Result.bListAfterSaveSucceeded)
		{
			AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
				Result,
				ListAfterSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite SaveLayout repository smoke failed: ListLayouts failed after first save.")
					: ListAfterSaveResult.ErrorMessage);
		}
		else
		{
			Result.bListedSavedMetadataFound =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterSaveResult.Layouts,
					FirstDocument.Metadata);
			if (!Result.bListedSavedMetadataFound)
			{
				AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
					Result,
					TEXT("SQLUI SQLite SaveLayout repository smoke failed: ListLayouts did not include saved metadata and tags after first save."));
			}
		}

		const FSQLUILayoutLoadResult LoadAfterSaveResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite SaveLayout repository after first save"),
				FirstDocument.Metadata.LayoutId);
		Result.bLoadAfterSaveSucceeded = LoadAfterSaveResult.bSucceeded;
		Result.LoadedLayoutId = LoadAfterSaveResult.Document.Metadata.LayoutId;
		Result.bLoadedDocumentValid =
			LoadAfterSaveResult.bSucceeded
			&& LoadAfterSaveResult.Validation.bIsValid
			&& LoadAfterSaveResult.Document.Metadata.LayoutId == FirstDocument.Metadata.LayoutId
			&& LoadAfterSaveResult.Document.Version.Revision == 1;
		if (!Result.bLoadAfterSaveSucceeded)
		{
			AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
				Result,
				LoadAfterSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite SaveLayout repository smoke failed: LoadLayout failed after first save.")
					: LoadAfterSaveResult.ErrorMessage);
		}
		else if (!Result.bLoadedDocumentValid)
		{
			AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
				Result,
				TEXT("SQLUI SQLite SaveLayout repository smoke failed: loaded document after first save was invalid or not revision 1."));
		}
	}

	const FSQLUILayoutDocument UpdatedDocument =
		MakeSQLUISampleSQLiteSaveLayoutRepositoryUpdatedDocument(FirstDocument);
	if (Result.bLoadedDocumentValid)
	{
		const FSQLUILayoutSaveResult SecondSaveResult = SaveSQLUISampleLayoutToRepository(
			Repository,
			TEXT("SQLite SaveLayout repository second save"),
			UpdatedDocument);
		Result.bSecondSaveSucceeded =
			SecondSaveResult.bSucceeded
			&& SecondSaveResult.SavedLayoutId == UpdatedDocument.Metadata.LayoutId
			&& SecondSaveResult.Validation.bIsValid;
		if (!Result.bSecondSaveSucceeded)
		{
			AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
				Result,
				SecondSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite SaveLayout repository smoke failed: second SaveLayout failed.")
					: SecondSaveResult.ErrorMessage);
		}
	}

	if (Result.bSecondSaveSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListAfterSecondSaveResult =
			Repository->ListLayouts();
		Result.bListAfterSecondSaveSucceeded = ListAfterSecondSaveResult.bSucceeded;
		if (!Result.bListAfterSecondSaveSucceeded)
		{
			AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
				Result,
				ListAfterSecondSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite SaveLayout repository smoke failed: ListLayouts failed after second save.")
					: ListAfterSecondSaveResult.ErrorMessage);
		}
		else
		{
			Result.bListedUpdatedMetadataFound =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterSecondSaveResult.Layouts,
					UpdatedDocument.Metadata);
			if (!Result.bListedUpdatedMetadataFound)
			{
				AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
					Result,
					TEXT("SQLUI SQLite SaveLayout repository smoke failed: ListLayouts did not include updated metadata and tags after second save."));
			}
		}

		const FSQLUILayoutLoadResult LoadAfterSecondSaveResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite SaveLayout repository after second save"),
				UpdatedDocument.Metadata.LayoutId);
		Result.bLoadAfterSecondSaveSucceeded = LoadAfterSecondSaveResult.bSucceeded;
		Result.LoadedLayoutId = LoadAfterSecondSaveResult.Document.Metadata.LayoutId;
		Result.bLatestRevisionLoaded =
			LoadAfterSecondSaveResult.bSucceeded
			&& LoadAfterSecondSaveResult.Validation.bIsValid
			&& LoadAfterSecondSaveResult.Document.Metadata.LayoutId == UpdatedDocument.Metadata.LayoutId
			&& LoadAfterSecondSaveResult.Document.Metadata.DisplayName == UpdatedDocument.Metadata.DisplayName
			&& LoadAfterSecondSaveResult.Document.Version.Revision == 2;
		if (!Result.bLoadAfterSecondSaveSucceeded)
		{
			AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
				Result,
				LoadAfterSecondSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite SaveLayout repository smoke failed: LoadLayout failed after second save.")
					: LoadAfterSecondSaveResult.ErrorMessage);
		}
		else if (!Result.bLatestRevisionLoaded)
		{
			AppendSQLUISampleSQLiteSaveLayoutRepositoryError(
				Result,
				TEXT("SQLUI SQLite SaveLayout repository smoke failed: LoadLayout did not return the latest saved revision."));
		}
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleSQLiteSaveLayoutRepositoryFiles(Result.DatabasePath, Result);

	Result.bSucceeded =
		Result.bDatabasePrepared
		&& Result.bFirstSaveSucceeded
		&& Result.bListAfterSaveSucceeded
		&& Result.bListedSavedMetadataFound
		&& Result.bLoadAfterSaveSucceeded
		&& Result.bLoadedDocumentValid
		&& Result.bSecondSaveSucceeded
		&& Result.bListAfterSecondSaveSucceeded
		&& Result.bListedUpdatedMetadataFound
		&& Result.bLoadAfterSecondSaveSucceeded
		&& Result.bLatestRevisionLoaded
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
	FSQLUISampleSQLiteRemoveLayoutRepositorySmokeResult& Result,
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

FString MakeSQLUISampleSQLiteRemoveLayoutRepositoryDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteRemoveLayoutRepository"),
		TEXT("SQLiteRemoveLayoutRepository.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUISampleSQLiteRemoveLayoutRepositoryFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteRemoveLayoutRepositorySmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite RemoveLayout repository smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteRemoveLayoutRepositoryDocument()
{
	FSQLUILayoutDocument Document = MakeSQLUISampleSQLiteSaveLayoutRepositoryDocument();
	Document.Version.Label = TEXT("SQLite RemoveLayout Repository Probe");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.sqlite-remove-layout-repository");
	Document.Metadata.DisplayName = TEXT("SQLUI SQLite RemoveLayout Repository Probe");
	Document.Metadata.Description = TEXT("Smoke/probe layout for SQLite RemoveLayout repository mapping.");
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("remove-layout"));
	Document.Metadata.SearchMetadata.Reset();
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteRemoveLayoutRepository"));
	Document.RootWidgetId = TEXT("SQLUI.SQLite.RemoveLayoutRepository.Root");

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].WidgetTypeKey = TEXT("SQLUI.ProbeRoot");
		Document.Nodes[0].Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(TEXT("remove-layout"));
		Document.Nodes[0].SearchMetadata.Reset();
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteRemoveLayoutRepository"));
	}

	return Document;
}

FSQLUISampleSQLiteRemoveLayoutRepositorySmokeResult RunSQLUISampleSQLiteRemoveLayoutRepositorySmoke(
	UObject* Outer)
{
	FSQLUISampleSQLiteRemoveLayoutRepositorySmokeResult Result;
	Result.DatabasePath = MakeSQLUISampleSQLiteRemoveLayoutRepositoryDatabasePath();

	const FSQLUISQLiteLayoutSchemaMigrationProbeResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::RunProbe(Result.DatabasePath, false);
	Result.bDatabasePrepared = SchemaResult.bSucceeded && SchemaResult.bMigrationSucceeded;
	if (!Result.bDatabasePrepared)
	{
		AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
			Result,
			SchemaResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite RemoveLayout repository smoke failed: could not prepare probe database.")
				: SchemaResult.ErrorMessage);
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteRemoveLayoutRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
			Result,
			TEXT("SQLUI SQLite RemoveLayout repository smoke failed: could not create repository object."));
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteRemoveLayoutRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = Result.DatabasePath;
	RepositorySettings.bReadOnly = false;
	Repository->Configure(RepositorySettings);

	const FSQLUILayoutDocument Document =
		MakeSQLUISampleSQLiteRemoveLayoutRepositoryDocument();
	const FSQLUILayoutSaveResult SaveResult = SaveSQLUISampleLayoutToRepository(
		Repository,
		TEXT("SQLite RemoveLayout repository"),
		Document);
	Result.SavedLayoutId = SaveResult.SavedLayoutId;
	Result.bSaveSucceeded =
		SaveResult.bSucceeded
		&& SaveResult.SavedLayoutId == Document.Metadata.LayoutId
		&& SaveResult.Validation.bIsValid;
	if (!Result.bSaveSucceeded)
	{
		AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
			Result,
			SaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite RemoveLayout repository smoke failed: SaveLayout failed.")
				: SaveResult.ErrorMessage);
	}

	if (Result.bSaveSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListBeforeRemoveResult =
			Repository->ListLayouts();
		Result.bListBeforeRemoveSucceeded = ListBeforeRemoveResult.bSucceeded;
		Result.ListedLayoutCountBeforeRemove = ListBeforeRemoveResult.Layouts.Num();
		if (!Result.bListBeforeRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
				Result,
				ListBeforeRemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite RemoveLayout repository smoke failed: ListLayouts failed before remove.")
					: ListBeforeRemoveResult.ErrorMessage);
		}
		else
		{
			Result.bListedMetadataFoundBeforeRemove =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListBeforeRemoveResult.Layouts,
					Document.Metadata);
			if (!Result.bListedMetadataFoundBeforeRemove)
			{
				AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
					Result,
					TEXT("SQLUI SQLite RemoveLayout repository smoke failed: ListLayouts did not include saved metadata and tags before remove."));
			}
		}

		const FSQLUILayoutLoadResult LoadBeforeRemoveResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite RemoveLayout repository before remove"),
				Document.Metadata.LayoutId);
		Result.bLoadBeforeRemoveSucceeded = LoadBeforeRemoveResult.bSucceeded;
		Result.LoadedLayoutId = LoadBeforeRemoveResult.Document.Metadata.LayoutId;
		Result.bLoadedDocumentValidBeforeRemove =
			LoadBeforeRemoveResult.bSucceeded
			&& LoadBeforeRemoveResult.Validation.bIsValid
			&& LoadBeforeRemoveResult.Document.Metadata.LayoutId == Document.Metadata.LayoutId;
		if (!Result.bLoadBeforeRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
				Result,
				LoadBeforeRemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite RemoveLayout repository smoke failed: LoadLayout failed before remove.")
					: LoadBeforeRemoveResult.ErrorMessage);
		}
		else if (!Result.bLoadedDocumentValidBeforeRemove)
		{
			AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
				Result,
				TEXT("SQLUI SQLite RemoveLayout repository smoke failed: loaded document before remove was invalid or did not match the saved layout id."));
		}
	}

	if (Result.bLoadedDocumentValidBeforeRemove)
	{
		const FSQLUILayoutRepositoryRemoveResult RemoveResult =
			Repository->RemoveLayout(Document.Metadata.LayoutId);
		Result.RemovedLayoutId = RemoveResult.RemovedLayoutId;
		Result.bRemoveSucceeded = RemoveResult.bSucceeded;
		Result.bRemoved =
			RemoveResult.bSucceeded
			&& RemoveResult.bRemoved
			&& RemoveResult.RemovedLayoutId == Document.Metadata.LayoutId;
		if (!Result.bRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
				Result,
				RemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite RemoveLayout repository smoke failed: RemoveLayout failed.")
					: RemoveResult.ErrorMessage);
		}
		else if (!Result.bRemoved)
		{
			AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
				Result,
				TEXT("SQLUI SQLite RemoveLayout repository smoke failed: RemoveLayout succeeded but did not report bRemoved=true for the saved layout id."));
		}
	}

	if (Result.bRemoved)
	{
		const FSQLUILayoutRepositoryListResult ListAfterRemoveResult =
			Repository->ListLayouts();
		Result.bListAfterRemoveSucceeded = ListAfterRemoveResult.bSucceeded;
		Result.ListedLayoutCountAfterRemove = ListAfterRemoveResult.Layouts.Num();
		if (!Result.bListAfterRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
				Result,
				ListAfterRemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite RemoveLayout repository smoke failed: ListLayouts failed after remove.")
					: ListAfterRemoveResult.ErrorMessage);
		}
		else
		{
			Result.bMetadataAbsentAfterRemove =
				!DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterRemoveResult.Layouts,
					Document.Metadata);
			if (!Result.bMetadataAbsentAfterRemove)
			{
				AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
					Result,
					TEXT("SQLUI SQLite RemoveLayout repository smoke failed: ListLayouts still included removed metadata after remove."));
			}
		}

		const FSQLUILayoutLoadResult LoadAfterRemoveResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite RemoveLayout repository after remove"),
				Document.Metadata.LayoutId);
		Result.bLoadAfterRemoveFailedAsExpected =
			!LoadAfterRemoveResult.bSucceeded
			&& !LoadAfterRemoveResult.bBackendUnavailable
			&& !LoadAfterRemoveResult.ErrorMessage.IsEmpty();
		if (!Result.bLoadAfterRemoveFailedAsExpected)
		{
			AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite RemoveLayout repository smoke failed: LoadLayout after remove did not fail as an ordinary not-found/deleted-layout result. bSucceeded=%s bBackendUnavailable=%s Error='%s'."),
					LoadAfterRemoveResult.bSucceeded ? TEXT("true") : TEXT("false"),
					LoadAfterRemoveResult.bBackendUnavailable ? TEXT("true") : TEXT("false"),
					*LoadAfterRemoveResult.ErrorMessage));
		}

		FString RevisionCountErrorMessage;
		const bool bRevisionCountQueried = FSQLUISQLiteLayoutReadProbe::CountLayoutRevisions(
			Result.DatabasePath,
			Document.Metadata.LayoutId,
			Result.RevisionCountAfterRemove,
			RevisionCountErrorMessage);
		Result.bRevisionsPreserved =
			bRevisionCountQueried
			&& Result.RevisionCountAfterRemove >= 1;
		if (!Result.bRevisionsPreserved)
		{
			AppendSQLUISampleSQLiteRemoveLayoutRepositoryError(
				Result,
				RevisionCountErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite RemoveLayout repository smoke failed: revision history was not preserved after soft delete.")
					: RevisionCountErrorMessage);
		}
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleSQLiteRemoveLayoutRepositoryFiles(Result.DatabasePath, Result);

	Result.bSucceeded =
		Result.bDatabasePrepared
		&& Result.bSaveSucceeded
		&& Result.bListBeforeRemoveSucceeded
		&& Result.bListedMetadataFoundBeforeRemove
		&& Result.bLoadBeforeRemoveSucceeded
		&& Result.bLoadedDocumentValidBeforeRemove
		&& Result.bRemoveSucceeded
		&& Result.bRemoved
		&& Result.bListAfterRemoveSucceeded
		&& Result.bMetadataAbsentAfterRemove
		&& Result.bLoadAfterRemoveFailedAsExpected
		&& Result.bRevisionsPreserved
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
	FSQLUISampleSQLiteClearLayoutsRepositorySmokeResult& Result,
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

FString MakeSQLUISampleSQLiteClearLayoutsRepositoryDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteClearLayoutsRepository"),
		TEXT("SQLiteClearLayoutsRepository.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUISampleSQLiteClearLayoutsRepositoryFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteClearLayoutsRepositorySmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite ClearLayouts repository smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteClearLayoutsRepositoryDocument(
	const TCHAR* LayoutIdSuffix,
	const TCHAR* DisplayNameSuffix,
	const TCHAR* TagSuffix)
{
	FSQLUILayoutDocument Document = MakeSQLUISampleSQLiteSaveLayoutRepositoryDocument();
	Document.Version.Label = FString::Printf(
		TEXT("SQLite ClearLayouts Repository Probe %s"),
		DisplayNameSuffix);
	Document.Metadata.LayoutId = FString::Printf(
		TEXT("sqlui.smoke.sqlite-clear-layouts-repository.%s"),
		LayoutIdSuffix);
	Document.Metadata.DisplayName = FString::Printf(
		TEXT("SQLUI SQLite ClearLayouts Repository Probe %s"),
		DisplayNameSuffix);
	Document.Metadata.Description = TEXT("Smoke/probe layout for SQLite ClearLayouts repository mapping.");
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("clear-layouts"));
	Document.Metadata.Tags.Add(TagSuffix);
	Document.Metadata.SearchMetadata.Reset();
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteClearLayoutsRepository"));
	Document.RootWidgetId = FString::Printf(
		TEXT("SQLUI.SQLite.ClearLayoutsRepository.%s.Root"),
		LayoutIdSuffix);

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].WidgetTypeKey = TEXT("SQLUI.ProbeRoot");
		Document.Nodes[0].Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(TEXT("clear-layouts"));
		Document.Nodes[0].Tags.Add(TagSuffix);
		Document.Nodes[0].SearchMetadata.Reset();
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteClearLayoutsRepository"));
	}

	return Document;
}

FSQLUISampleSQLiteClearLayoutsRepositorySmokeResult RunSQLUISampleSQLiteClearLayoutsRepositorySmoke(
	UObject* Outer)
{
	FSQLUISampleSQLiteClearLayoutsRepositorySmokeResult Result;
	Result.DatabasePath = MakeSQLUISampleSQLiteClearLayoutsRepositoryDatabasePath();

	const FSQLUISQLiteLayoutSchemaMigrationProbeResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::RunProbe(Result.DatabasePath, false);
	Result.bDatabasePrepared = SchemaResult.bSucceeded && SchemaResult.bMigrationSucceeded;
	if (!Result.bDatabasePrepared)
	{
		AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
			Result,
			SchemaResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite ClearLayouts repository smoke failed: could not prepare probe database.")
				: SchemaResult.ErrorMessage);
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteClearLayoutsRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
			Result,
			TEXT("SQLUI SQLite ClearLayouts repository smoke failed: could not create repository object."));
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteClearLayoutsRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = Result.DatabasePath;
	RepositorySettings.bReadOnly = false;
	Repository->Configure(RepositorySettings);

	const FSQLUILayoutDocument FirstDocument =
		MakeSQLUISampleSQLiteClearLayoutsRepositoryDocument(
			TEXT("first"),
			TEXT("First"),
			TEXT("first"));
	const FSQLUILayoutDocument SecondDocument =
		MakeSQLUISampleSQLiteClearLayoutsRepositoryDocument(
			TEXT("second"),
			TEXT("Second"),
			TEXT("second"));

	const FSQLUILayoutSaveResult FirstSaveResult = SaveSQLUISampleLayoutToRepository(
		Repository,
		TEXT("SQLite ClearLayouts repository first layout"),
		FirstDocument);
	Result.FirstLayoutId = FirstSaveResult.SavedLayoutId;
	Result.bFirstSaveSucceeded =
		FirstSaveResult.bSucceeded
		&& FirstSaveResult.SavedLayoutId == FirstDocument.Metadata.LayoutId
		&& FirstSaveResult.Validation.bIsValid;
	if (!Result.bFirstSaveSucceeded)
	{
		AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
			Result,
			FirstSaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite ClearLayouts repository smoke failed: first SaveLayout failed.")
				: FirstSaveResult.ErrorMessage);
	}

	const FSQLUILayoutSaveResult SecondSaveResult = SaveSQLUISampleLayoutToRepository(
		Repository,
		TEXT("SQLite ClearLayouts repository second layout"),
		SecondDocument);
	Result.SecondLayoutId = SecondSaveResult.SavedLayoutId;
	Result.bSecondSaveSucceeded =
		SecondSaveResult.bSucceeded
		&& SecondSaveResult.SavedLayoutId == SecondDocument.Metadata.LayoutId
		&& SecondSaveResult.Validation.bIsValid;
	if (!Result.bSecondSaveSucceeded)
	{
		AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
			Result,
			SecondSaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite ClearLayouts repository smoke failed: second SaveLayout failed.")
				: SecondSaveResult.ErrorMessage);
	}

	if (Result.bFirstSaveSucceeded && Result.bSecondSaveSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListBeforeRemoveResult =
			Repository->ListLayouts();
		Result.bListBeforeRemoveSucceeded = ListBeforeRemoveResult.bSucceeded;
		Result.ListedLayoutCountBeforeRemove = ListBeforeRemoveResult.Layouts.Num();
		if (!Result.bListBeforeRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				ListBeforeRemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite ClearLayouts repository smoke failed: ListLayouts failed before remove.")
					: ListBeforeRemoveResult.ErrorMessage);
		}
		else
		{
			Result.bBothMetadataEntriesFoundBeforeRemove =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListBeforeRemoveResult.Layouts,
					FirstDocument.Metadata)
				&& DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListBeforeRemoveResult.Layouts,
					SecondDocument.Metadata);
			if (!Result.bBothMetadataEntriesFoundBeforeRemove)
			{
				AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
					Result,
					TEXT("SQLUI SQLite ClearLayouts repository smoke failed: ListLayouts did not include both saved metadata/tag entries before remove."));
			}
		}
	}

	if (Result.bBothMetadataEntriesFoundBeforeRemove)
	{
		const FSQLUILayoutRepositoryRemoveResult RemoveResult =
			Repository->RemoveLayout(FirstDocument.Metadata.LayoutId);
		Result.RemovedLayoutId = RemoveResult.RemovedLayoutId;
		Result.bRemoveSucceeded = RemoveResult.bSucceeded;
		Result.bRemoved =
			RemoveResult.bSucceeded
			&& RemoveResult.bRemoved
			&& RemoveResult.RemovedLayoutId == FirstDocument.Metadata.LayoutId;
		if (!Result.bRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				RemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite ClearLayouts repository smoke failed: RemoveLayout failed.")
					: RemoveResult.ErrorMessage);
		}
		else if (!Result.bRemoved)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				TEXT("SQLUI SQLite ClearLayouts repository smoke failed: RemoveLayout succeeded but did not report bRemoved=true for the first layout id."));
		}
	}

	if (Result.bRemoved)
	{
		const FSQLUILayoutRepositoryListResult ListBeforeClearResult =
			Repository->ListLayouts();
		Result.bListBeforeClearSucceeded = ListBeforeClearResult.bSucceeded;
		Result.ListedLayoutCountBeforeClear = ListBeforeClearResult.Layouts.Num();
		if (!Result.bListBeforeClearSucceeded)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				ListBeforeClearResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite ClearLayouts repository smoke failed: ListLayouts failed before clear.")
					: ListBeforeClearResult.ErrorMessage);
		}
		else
		{
			Result.bActiveMetadataPreservedBeforeClear =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListBeforeClearResult.Layouts,
					SecondDocument.Metadata);
			Result.bRemovedMetadataAbsentBeforeClear =
				!DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListBeforeClearResult.Layouts,
					FirstDocument.Metadata);
			if (!Result.bActiveMetadataPreservedBeforeClear)
			{
				AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
					Result,
					TEXT("SQLUI SQLite ClearLayouts repository smoke failed: active metadata was not preserved before clear."));
			}
			if (!Result.bRemovedMetadataAbsentBeforeClear)
			{
				AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
					Result,
					TEXT("SQLUI SQLite ClearLayouts repository smoke failed: removed metadata was still listed before clear."));
			}
		}
	}

	if (Result.bListBeforeClearSucceeded)
	{
		const FSQLUILayoutRepositoryClearResult ClearResult = Repository->ClearLayouts();
		Result.bClearSucceeded = ClearResult.bSucceeded;
		Result.ClearRemovedCount = ClearResult.RemovedCount;
		Result.bRemovedCountMatchedExpected =
			ClearResult.bSucceeded
			&& ClearResult.RemovedCount == 2;
		if (!Result.bClearSucceeded)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				ClearResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite ClearLayouts repository smoke failed: ClearLayouts failed.")
					: ClearResult.ErrorMessage);
		}
		else if (!Result.bRemovedCountMatchedExpected)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite ClearLayouts repository smoke failed: ClearLayouts removed %d layout rows, expected 2."),
					ClearResult.RemovedCount));
		}
	}

	if (Result.bClearSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListAfterClearResult =
			Repository->ListLayouts();
		Result.bListAfterClearSucceeded = ListAfterClearResult.bSucceeded;
		Result.ListedLayoutCountAfterClear = ListAfterClearResult.Layouts.Num();
		Result.bEmptyAfterClear =
			ListAfterClearResult.bSucceeded
			&& ListAfterClearResult.Layouts.Num() == 0;
		if (!Result.bListAfterClearSucceeded)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				ListAfterClearResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite ClearLayouts repository smoke failed: ListLayouts failed after clear.")
					: ListAfterClearResult.ErrorMessage);
		}
		else if (!Result.bEmptyAfterClear)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite ClearLayouts repository smoke failed: ListLayouts returned %d layout(s) after clear."),
					ListAfterClearResult.Layouts.Num()));
		}

		const FSQLUILayoutLoadResult FirstLoadAfterClearResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite ClearLayouts repository after clear first layout"),
				FirstDocument.Metadata.LayoutId);
		const FSQLUILayoutLoadResult SecondLoadAfterClearResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite ClearLayouts repository after clear second layout"),
				SecondDocument.Metadata.LayoutId);
		Result.bLoadsAfterClearFailedAsExpected =
			!FirstLoadAfterClearResult.bSucceeded
			&& !FirstLoadAfterClearResult.bBackendUnavailable
			&& !FirstLoadAfterClearResult.ErrorMessage.IsEmpty()
			&& !SecondLoadAfterClearResult.bSucceeded
			&& !SecondLoadAfterClearResult.bBackendUnavailable
			&& !SecondLoadAfterClearResult.ErrorMessage.IsEmpty();
		if (!Result.bLoadsAfterClearFailedAsExpected)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite ClearLayouts repository smoke failed: LoadLayout after clear did not fail as ordinary not-found results. FirstSucceeded=%s FirstBackendUnavailable=%s FirstError='%s' SecondSucceeded=%s SecondBackendUnavailable=%s SecondError='%s'."),
					FirstLoadAfterClearResult.bSucceeded ? TEXT("true") : TEXT("false"),
					FirstLoadAfterClearResult.bBackendUnavailable ? TEXT("true") : TEXT("false"),
					*FirstLoadAfterClearResult.ErrorMessage,
					SecondLoadAfterClearResult.bSucceeded ? TEXT("true") : TEXT("false"),
					SecondLoadAfterClearResult.bBackendUnavailable ? TEXT("true") : TEXT("false"),
					*SecondLoadAfterClearResult.ErrorMessage));
		}

		FString RowCountErrorMessage;
		const bool bRowCountsQueried = FSQLUISQLiteLayoutReadProbe::CountLayoutSchemaRows(
			Result.DatabasePath,
			Result.TableRowCountsAfterClear,
			RowCountErrorMessage);
		Result.bTablesEmptyAfterClear =
			bRowCountsQueried
			&& Result.TableRowCountsAfterClear.Layouts == 0
			&& Result.TableRowCountsAfterClear.LayoutRevisions == 0
			&& Result.TableRowCountsAfterClear.LayoutTags == 0
			&& Result.TableRowCountsAfterClear.LayoutCheckpoints == 0
			&& Result.TableRowCountsAfterClear.LayoutPreviews == 0;
		if (!Result.bTablesEmptyAfterClear)
		{
			AppendSQLUISampleSQLiteClearLayoutsRepositoryError(
				Result,
				RowCountErrorMessage.IsEmpty()
					? FString::Printf(
						TEXT("SQLUI SQLite ClearLayouts repository smoke failed: schema rows remained after clear. Layouts=%d Revisions=%d Tags=%d Checkpoints=%d Previews=%d."),
						Result.TableRowCountsAfterClear.Layouts,
						Result.TableRowCountsAfterClear.LayoutRevisions,
						Result.TableRowCountsAfterClear.LayoutTags,
						Result.TableRowCountsAfterClear.LayoutCheckpoints,
						Result.TableRowCountsAfterClear.LayoutPreviews)
					: RowCountErrorMessage);
		}
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleSQLiteClearLayoutsRepositoryFiles(Result.DatabasePath, Result);

	Result.bSucceeded =
		Result.bDatabasePrepared
		&& Result.bFirstSaveSucceeded
		&& Result.bSecondSaveSucceeded
		&& Result.bListBeforeRemoveSucceeded
		&& Result.bBothMetadataEntriesFoundBeforeRemove
		&& Result.bRemoveSucceeded
		&& Result.bRemoved
		&& Result.bListBeforeClearSucceeded
		&& Result.bActiveMetadataPreservedBeforeClear
		&& Result.bRemovedMetadataAbsentBeforeClear
		&& Result.bClearSucceeded
		&& Result.bRemovedCountMatchedExpected
		&& Result.bListAfterClearSucceeded
		&& Result.bEmptyAfterClear
		&& Result.bLoadsAfterClearFailedAsExpected
		&& Result.bTablesEmptyAfterClear
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
	FSQLUISampleSQLiteFullLifecycleRepositorySmokeResult& Result,
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

FString MakeSQLUISampleSQLiteFullLifecycleRepositoryDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteFullLifecycleRepository"),
		TEXT("SQLiteFullLifecycleRepository.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUISampleSQLiteFullLifecycleRepositoryFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteFullLifecycleRepositorySmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite full lifecycle repository smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteFullLifecycleRepositoryDocument(
	const TCHAR* LayoutIdSuffix,
	const TCHAR* DisplayNameSuffix,
	const TCHAR* TagSuffix)
{
	FSQLUILayoutDocument Document = MakeSQLUISampleSQLiteSaveLayoutRepositoryDocument();
	Document.Version.Label = FString::Printf(
		TEXT("SQLite Full Lifecycle Repository Probe %s"),
		DisplayNameSuffix);
	Document.Metadata.LayoutId = FString::Printf(
		TEXT("sqlui.smoke.sqlite-full-lifecycle.%s"),
		LayoutIdSuffix);
	Document.Metadata.DisplayName = FString::Printf(
		TEXT("SQLUI SQLite Full Lifecycle Repository Probe %s"),
		DisplayNameSuffix);
	Document.Metadata.Description = TEXT("Smoke/probe layout for SQLite full lifecycle repository mapping.");
	Document.Metadata.CreatedAtUtc = TEXT("2026-05-30T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = Document.Metadata.CreatedAtUtc;
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("full-lifecycle"));
	Document.Metadata.Tags.Add(TagSuffix);
	Document.Metadata.SearchMetadata.Reset();
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteFullLifecycleRepository"));
	Document.RootWidgetId = FString::Printf(
		TEXT("SQLUI.SQLite.FullLifecycleRepository.%s.Root"),
		DisplayNameSuffix);

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].WidgetTypeKey = TEXT("SQLUI.ProbeRoot");
		Document.Nodes[0].Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(TEXT("full-lifecycle"));
		Document.Nodes[0].Tags.Add(TagSuffix);
		Document.Nodes[0].SearchMetadata.Reset();
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteFullLifecycleRepository"));
	}

	return Document;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteFullLifecycleRepositoryUpdatedDocument(
	const FSQLUILayoutDocument& OriginalDocument)
{
	FSQLUILayoutDocument UpdatedDocument = OriginalDocument;
	UpdatedDocument.Version.Label = TEXT("SQLite Full Lifecycle Repository Probe First Updated");
	UpdatedDocument.Metadata.DisplayName = TEXT("SQLUI SQLite Full Lifecycle Repository Probe First Updated");
	UpdatedDocument.Metadata.Description = TEXT("Updated smoke/probe layout for SQLite full lifecycle repository mapping.");
	UpdatedDocument.Metadata.UpdatedAtUtc = TEXT("2026-05-30T00:02:00Z");
	UpdatedDocument.Metadata.Tags.Add(TEXT("updated"));

	if (UpdatedDocument.Nodes.Num() > 0)
	{
		UpdatedDocument.Nodes[0].Properties.Add(TEXT("Text"), UpdatedDocument.Metadata.DisplayName);
		UpdatedDocument.Nodes[0].Tags.Add(TEXT("updated"));
	}

	return UpdatedDocument;
}

bool IsSQLUISampleLoadFailureOrdinaryNotFound(
	const FSQLUILayoutLoadResult& LoadResult)
{
	return !LoadResult.bSucceeded
		&& !LoadResult.bBackendUnavailable
		&& !LoadResult.ErrorMessage.IsEmpty();
}

bool DoesSQLUISampleLoadedDocumentMatchUpdatedFullLifecycleDocument(
	const FSQLUILayoutDocument& LoadedDocument,
	const FSQLUILayoutDocument& ExpectedDocument)
{
	return LoadedDocument.Metadata.LayoutId == ExpectedDocument.Metadata.LayoutId
		&& LoadedDocument.Metadata.DisplayName == ExpectedDocument.Metadata.DisplayName
		&& LoadedDocument.Metadata.Description == ExpectedDocument.Metadata.Description
		&& LoadedDocument.Metadata.UpdatedAtUtc == ExpectedDocument.Metadata.UpdatedAtUtc
		&& LoadedDocument.Version.Label == ExpectedDocument.Version.Label
		&& DoSQLUISampleTagSetsMatch(LoadedDocument.Metadata.Tags, ExpectedDocument.Metadata.Tags);
}

FSQLUISampleSQLiteFullLifecycleRepositorySmokeResult RunSQLUISampleSQLiteFullLifecycleRepositorySmoke(
	UObject* Outer)
{
	FSQLUISampleSQLiteFullLifecycleRepositorySmokeResult Result;
	Result.DatabasePath = MakeSQLUISampleSQLiteFullLifecycleRepositoryDatabasePath();

	const FSQLUISQLiteLayoutSchemaMigrationProbeResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::RunProbe(Result.DatabasePath, false);
	Result.bDatabasePrepared = SchemaResult.bSucceeded && SchemaResult.bMigrationSucceeded;
	if (!Result.bDatabasePrepared)
	{
		AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
			Result,
			SchemaResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite full lifecycle repository smoke failed: could not prepare probe database.")
				: SchemaResult.ErrorMessage);
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteFullLifecycleRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
			Result,
			TEXT("SQLUI SQLite full lifecycle repository smoke failed: could not create repository object."));
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteFullLifecycleRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = Result.DatabasePath;
	RepositorySettings.bReadOnly = false;
	Repository->Configure(RepositorySettings);

	const FSQLUILayoutDocument FirstDocument =
		MakeSQLUISampleSQLiteFullLifecycleRepositoryDocument(
			TEXT("first"),
			TEXT("First"),
			TEXT("first"));
	const FSQLUILayoutDocument UpdatedFirstDocument =
		MakeSQLUISampleSQLiteFullLifecycleRepositoryUpdatedDocument(FirstDocument);
	const FSQLUILayoutDocument SecondDocument =
		MakeSQLUISampleSQLiteFullLifecycleRepositoryDocument(
			TEXT("second"),
			TEXT("Second"),
			TEXT("second"));
	Result.FirstLayoutId = FirstDocument.Metadata.LayoutId;
	Result.SecondLayoutId = SecondDocument.Metadata.LayoutId;

	const FSQLUILayoutSaveResult FirstSaveResult = SaveSQLUISampleLayoutToRepository(
		Repository,
		TEXT("SQLite full lifecycle repository first layout"),
		FirstDocument);
	Result.FirstLayoutId = FirstSaveResult.SavedLayoutId;
	Result.bFirstSaveSucceeded =
		FirstSaveResult.bSucceeded
		&& FirstSaveResult.SavedLayoutId == FirstDocument.Metadata.LayoutId
		&& FirstSaveResult.Validation.bIsValid;
	if (!Result.bFirstSaveSucceeded)
	{
		AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
			Result,
			FirstSaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite full lifecycle repository smoke failed: first SaveLayout failed.")
				: FirstSaveResult.ErrorMessage);
	}

	if (Result.bFirstSaveSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListAfterFirstSaveResult =
			Repository->ListLayouts();
		Result.bListAfterFirstSaveSucceeded = ListAfterFirstSaveResult.bSucceeded;
		Result.ListedLayoutCountAfterFirstSave = ListAfterFirstSaveResult.Layouts.Num();
		if (!Result.bListAfterFirstSaveSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				ListAfterFirstSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: ListLayouts failed after first save.")
					: ListAfterFirstSaveResult.ErrorMessage);
		}
		else
		{
			Result.bFirstMetadataFoundAfterFirstSave =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterFirstSaveResult.Layouts,
					FirstDocument.Metadata);
			if (!Result.bFirstMetadataFoundAfterFirstSave)
			{
				AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
					Result,
					TEXT("SQLUI SQLite full lifecycle repository smoke failed: ListLayouts did not include first saved metadata and tags after first save."));
			}
		}

		const FSQLUILayoutLoadResult LoadAfterFirstSaveResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite full lifecycle repository after first save"),
				FirstDocument.Metadata.LayoutId);
		Result.bLoadAfterFirstSaveSucceeded = LoadAfterFirstSaveResult.bSucceeded;
		Result.LoadedLayoutId = LoadAfterFirstSaveResult.Document.Metadata.LayoutId;
		Result.bFirstRevisionLoaded =
			LoadAfterFirstSaveResult.bSucceeded
			&& LoadAfterFirstSaveResult.Validation.bIsValid
			&& LoadAfterFirstSaveResult.Document.Metadata.LayoutId == FirstDocument.Metadata.LayoutId
			&& LoadAfterFirstSaveResult.Document.Version.Revision == 1;
		if (!Result.bLoadAfterFirstSaveSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				LoadAfterFirstSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: LoadLayout failed after first save.")
					: LoadAfterFirstSaveResult.ErrorMessage);
		}
		else if (!Result.bFirstRevisionLoaded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				TEXT("SQLUI SQLite full lifecycle repository smoke failed: loaded first layout was invalid, mismatched, or not revision 1."));
		}
	}

	if (Result.bFirstRevisionLoaded)
	{
		const FSQLUILayoutSaveResult SecondRevisionSaveResult = SaveSQLUISampleLayoutToRepository(
			Repository,
			TEXT("SQLite full lifecycle repository first layout update"),
			UpdatedFirstDocument);
		Result.bSecondRevisionSaveSucceeded =
			SecondRevisionSaveResult.bSucceeded
			&& SecondRevisionSaveResult.SavedLayoutId == UpdatedFirstDocument.Metadata.LayoutId
			&& SecondRevisionSaveResult.Validation.bIsValid;
		if (!Result.bSecondRevisionSaveSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				SecondRevisionSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: second revision SaveLayout failed.")
					: SecondRevisionSaveResult.ErrorMessage);
		}
	}

	if (Result.bSecondRevisionSaveSucceeded)
	{
		const FSQLUILayoutLoadResult LoadAfterUpdateResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite full lifecycle repository after first layout update"),
				UpdatedFirstDocument.Metadata.LayoutId);
		Result.bLatestRevisionLoaded =
			LoadAfterUpdateResult.bSucceeded
			&& LoadAfterUpdateResult.Validation.bIsValid
			&& LoadAfterUpdateResult.Document.Version.Revision == 2
			&& DoesSQLUISampleLoadedDocumentMatchUpdatedFullLifecycleDocument(
				LoadAfterUpdateResult.Document,
				UpdatedFirstDocument);
		Result.LoadedLayoutId = LoadAfterUpdateResult.Document.Metadata.LayoutId;
		if (!LoadAfterUpdateResult.bSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				LoadAfterUpdateResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: LoadLayout failed after second revision save.")
					: LoadAfterUpdateResult.ErrorMessage);
		}
		else if (!Result.bLatestRevisionLoaded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				TEXT("SQLUI SQLite full lifecycle repository smoke failed: LoadLayout did not return the updated revision 2 document."));
		}
	}

	if (Result.bLatestRevisionLoaded)
	{
		const FSQLUILayoutSaveResult SecondSaveResult = SaveSQLUISampleLayoutToRepository(
			Repository,
			TEXT("SQLite full lifecycle repository second layout"),
			SecondDocument);
		Result.SecondLayoutId = SecondSaveResult.SavedLayoutId;
		Result.bSecondSaveSucceeded =
			SecondSaveResult.bSucceeded
			&& SecondSaveResult.SavedLayoutId == SecondDocument.Metadata.LayoutId
			&& SecondSaveResult.Validation.bIsValid;
		if (!Result.bSecondSaveSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				SecondSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: second layout SaveLayout failed.")
					: SecondSaveResult.ErrorMessage);
		}
	}

	if (Result.bSecondSaveSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListBeforeRemoveResult =
			Repository->ListLayouts();
		Result.bListBeforeRemoveSucceeded = ListBeforeRemoveResult.bSucceeded;
		Result.ListedLayoutCountBeforeRemove = ListBeforeRemoveResult.Layouts.Num();
		if (!Result.bListBeforeRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				ListBeforeRemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: ListLayouts failed before remove.")
					: ListBeforeRemoveResult.ErrorMessage);
		}
		else
		{
			Result.bBothMetadataEntriesFoundBeforeRemove =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListBeforeRemoveResult.Layouts,
					UpdatedFirstDocument.Metadata)
				&& DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListBeforeRemoveResult.Layouts,
					SecondDocument.Metadata);
			if (!Result.bBothMetadataEntriesFoundBeforeRemove)
			{
				AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
					Result,
					TEXT("SQLUI SQLite full lifecycle repository smoke failed: ListLayouts did not include updated first and second metadata/tag entries before remove."));
			}
		}
	}

	if (Result.bBothMetadataEntriesFoundBeforeRemove)
	{
		const FSQLUILayoutRepositoryRemoveResult RemoveResult =
			Repository->RemoveLayout(UpdatedFirstDocument.Metadata.LayoutId);
		Result.RemovedLayoutId = RemoveResult.RemovedLayoutId;
		Result.bRemoveSucceeded = RemoveResult.bSucceeded;
		Result.bRemoved =
			RemoveResult.bSucceeded
			&& RemoveResult.bRemoved
			&& RemoveResult.RemovedLayoutId == UpdatedFirstDocument.Metadata.LayoutId;
		if (!Result.bRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				RemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: RemoveLayout failed.")
					: RemoveResult.ErrorMessage);
		}
		else if (!Result.bRemoved)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				TEXT("SQLUI SQLite full lifecycle repository smoke failed: RemoveLayout succeeded but did not report bRemoved=true for the first layout id."));
		}
	}

	if (Result.bRemoved)
	{
		const FSQLUILayoutRepositoryListResult ListAfterRemoveResult =
			Repository->ListLayouts();
		Result.bListAfterRemoveSucceeded = ListAfterRemoveResult.bSucceeded;
		Result.ListedLayoutCountAfterRemove = ListAfterRemoveResult.Layouts.Num();
		if (!Result.bListAfterRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				ListAfterRemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: ListLayouts failed after remove.")
					: ListAfterRemoveResult.ErrorMessage);
		}
		else
		{
			Result.bRemovedMetadataAbsentAfterRemove =
				!DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterRemoveResult.Layouts,
					UpdatedFirstDocument.Metadata);
			Result.bSecondMetadataPreservedAfterRemove =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterRemoveResult.Layouts,
					SecondDocument.Metadata);
			if (!Result.bRemovedMetadataAbsentAfterRemove)
			{
				AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
					Result,
					TEXT("SQLUI SQLite full lifecycle repository smoke failed: removed first metadata was still listed after remove."));
			}
			if (!Result.bSecondMetadataPreservedAfterRemove)
			{
				AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
					Result,
					TEXT("SQLUI SQLite full lifecycle repository smoke failed: second metadata was not preserved after removing the first layout."));
			}
		}

		const FSQLUILayoutLoadResult RemovedLoadResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite full lifecycle repository removed first layout"),
				UpdatedFirstDocument.Metadata.LayoutId);
		Result.bRemovedLoadFailedAsExpected =
			IsSQLUISampleLoadFailureOrdinaryNotFound(RemovedLoadResult);
		if (!Result.bRemovedLoadFailedAsExpected)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite full lifecycle repository smoke failed: LoadLayout for removed first layout did not fail as an ordinary not-found/deleted-layout result. bSucceeded=%s bBackendUnavailable=%s Error='%s'."),
					RemovedLoadResult.bSucceeded ? TEXT("true") : TEXT("false"),
					RemovedLoadResult.bBackendUnavailable ? TEXT("true") : TEXT("false"),
					*RemovedLoadResult.ErrorMessage));
		}

		const FSQLUILayoutLoadResult SecondLoadAfterRemoveResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite full lifecycle repository second layout after remove"),
				SecondDocument.Metadata.LayoutId);
		Result.bSecondLoadAfterRemoveSucceeded =
			SecondLoadAfterRemoveResult.bSucceeded
			&& SecondLoadAfterRemoveResult.Validation.bIsValid
			&& SecondLoadAfterRemoveResult.Document.Metadata.LayoutId == SecondDocument.Metadata.LayoutId;
		if (!Result.bSecondLoadAfterRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				SecondLoadAfterRemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: second layout LoadLayout failed after removing the first layout.")
					: SecondLoadAfterRemoveResult.ErrorMessage);
		}

		FString RevisionCountErrorMessage;
		const bool bRevisionCountQueried = FSQLUISQLiteLayoutReadProbe::CountLayoutRevisions(
			Result.DatabasePath,
			UpdatedFirstDocument.Metadata.LayoutId,
			Result.RevisionCountAfterRemove,
			RevisionCountErrorMessage);
		Result.bRevisionHistoryPreservedAfterRemove =
			bRevisionCountQueried
			&& Result.RevisionCountAfterRemove == 2;
		if (!Result.bRevisionHistoryPreservedAfterRemove)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				RevisionCountErrorMessage.IsEmpty()
					? FString::Printf(
						TEXT("SQLUI SQLite full lifecycle repository smoke failed: first layout revision count after remove was %d, expected 2."),
						Result.RevisionCountAfterRemove)
					: RevisionCountErrorMessage);
		}
	}

	if (Result.bRevisionHistoryPreservedAfterRemove)
	{
		const FSQLUILayoutRepositoryClearResult ClearResult = Repository->ClearLayouts();
		Result.bClearSucceeded = ClearResult.bSucceeded;
		Result.ClearRemovedCount = ClearResult.RemovedCount;
		Result.bRemovedCountMatchedExpected =
			ClearResult.bSucceeded
			&& ClearResult.RemovedCount == 2;
		if (!Result.bClearSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				ClearResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: ClearLayouts failed.")
					: ClearResult.ErrorMessage);
		}
		else if (!Result.bRemovedCountMatchedExpected)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite full lifecycle repository smoke failed: ClearLayouts removed %d layout rows, expected 2."),
					ClearResult.RemovedCount));
		}
	}

	if (Result.bClearSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListAfterClearResult =
			Repository->ListLayouts();
		Result.bListAfterClearSucceeded = ListAfterClearResult.bSucceeded;
		Result.ListedLayoutCountAfterClear = ListAfterClearResult.Layouts.Num();
		Result.bEmptyAfterClear =
			ListAfterClearResult.bSucceeded
			&& ListAfterClearResult.Layouts.Num() == 0;
		if (!Result.bListAfterClearSucceeded)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				ListAfterClearResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed: ListLayouts failed after clear.")
					: ListAfterClearResult.ErrorMessage);
		}
		else if (!Result.bEmptyAfterClear)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite full lifecycle repository smoke failed: ListLayouts returned %d layout(s) after clear."),
					ListAfterClearResult.Layouts.Num()));
		}

		const FSQLUILayoutLoadResult FirstLoadAfterClearResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite full lifecycle repository after clear first layout"),
				UpdatedFirstDocument.Metadata.LayoutId);
		const FSQLUILayoutLoadResult SecondLoadAfterClearResult =
			LoadSQLUISampleLayoutFromRepository(
				Repository,
				TEXT("SQLite full lifecycle repository after clear second layout"),
				SecondDocument.Metadata.LayoutId);
		Result.bLoadsAfterClearFailedAsExpected =
			IsSQLUISampleLoadFailureOrdinaryNotFound(FirstLoadAfterClearResult)
			&& IsSQLUISampleLoadFailureOrdinaryNotFound(SecondLoadAfterClearResult);
		if (!Result.bLoadsAfterClearFailedAsExpected)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite full lifecycle repository smoke failed: LoadLayout after clear did not fail as ordinary not-found results. FirstSucceeded=%s FirstBackendUnavailable=%s FirstError='%s' SecondSucceeded=%s SecondBackendUnavailable=%s SecondError='%s'."),
					FirstLoadAfterClearResult.bSucceeded ? TEXT("true") : TEXT("false"),
					FirstLoadAfterClearResult.bBackendUnavailable ? TEXT("true") : TEXT("false"),
					*FirstLoadAfterClearResult.ErrorMessage,
					SecondLoadAfterClearResult.bSucceeded ? TEXT("true") : TEXT("false"),
					SecondLoadAfterClearResult.bBackendUnavailable ? TEXT("true") : TEXT("false"),
					*SecondLoadAfterClearResult.ErrorMessage));
		}

		FString RowCountErrorMessage;
		const bool bRowCountsQueried = FSQLUISQLiteLayoutReadProbe::CountLayoutSchemaRows(
			Result.DatabasePath,
			Result.TableRowCountsAfterClear,
			RowCountErrorMessage);
		Result.bTablesEmptyAfterClear =
			bRowCountsQueried
			&& Result.TableRowCountsAfterClear.Layouts == 0
			&& Result.TableRowCountsAfterClear.LayoutRevisions == 0
			&& Result.TableRowCountsAfterClear.LayoutTags == 0
			&& Result.TableRowCountsAfterClear.LayoutCheckpoints == 0
			&& Result.TableRowCountsAfterClear.LayoutPreviews == 0;
		if (!Result.bTablesEmptyAfterClear)
		{
			AppendSQLUISampleSQLiteFullLifecycleRepositoryError(
				Result,
				RowCountErrorMessage.IsEmpty()
					? FString::Printf(
						TEXT("SQLUI SQLite full lifecycle repository smoke failed: schema rows remained after clear. Layouts=%d Revisions=%d Tags=%d Checkpoints=%d Previews=%d."),
						Result.TableRowCountsAfterClear.Layouts,
						Result.TableRowCountsAfterClear.LayoutRevisions,
						Result.TableRowCountsAfterClear.LayoutTags,
						Result.TableRowCountsAfterClear.LayoutCheckpoints,
						Result.TableRowCountsAfterClear.LayoutPreviews)
					: RowCountErrorMessage);
		}
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleSQLiteFullLifecycleRepositoryFiles(Result.DatabasePath, Result);

	Result.bSucceeded =
		Result.bDatabasePrepared
		&& Result.bFirstSaveSucceeded
		&& Result.bListAfterFirstSaveSucceeded
		&& Result.bFirstMetadataFoundAfterFirstSave
		&& Result.bLoadAfterFirstSaveSucceeded
		&& Result.bFirstRevisionLoaded
		&& Result.bSecondRevisionSaveSucceeded
		&& Result.bLatestRevisionLoaded
		&& Result.bSecondSaveSucceeded
		&& Result.bListBeforeRemoveSucceeded
		&& Result.bBothMetadataEntriesFoundBeforeRemove
		&& Result.bRemoveSucceeded
		&& Result.bRemoved
		&& Result.bListAfterRemoveSucceeded
		&& Result.bRemovedMetadataAbsentAfterRemove
		&& Result.bSecondMetadataPreservedAfterRemove
		&& Result.bRemovedLoadFailedAsExpected
		&& Result.bSecondLoadAfterRemoveSucceeded
		&& Result.bRevisionHistoryPreservedAfterRemove
		&& Result.bClearSucceeded
		&& Result.bRemovedCountMatchedExpected
		&& Result.bListAfterClearSucceeded
		&& Result.bEmptyAfterClear
		&& Result.bLoadsAfterClearFailedAsExpected
		&& Result.bTablesEmptyAfterClear
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
	FSQLUISampleSQLiteAsyncCallbackRepositorySmokeResult& Result,
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

FString MakeSQLUISampleSQLiteAsyncCallbackRepositoryDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteAsyncCallbackRepository"),
		TEXT("SQLiteAsyncCallbackRepository.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUISampleSQLiteAsyncCallbackRepositoryFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteAsyncCallbackRepositorySmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite async callback repository smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteAsyncCallbackRepositoryDocument()
{
	FSQLUILayoutDocument Document = MakeSQLUISampleSQLiteSaveLayoutRepositoryDocument();
	Document.Version.Label = TEXT("SQLite Async Callback Repository Probe");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.sqlite-async-callback-repository");
	Document.Metadata.DisplayName = TEXT("SQLUI SQLite Async Callback Repository Probe");
	Document.Metadata.Description = TEXT("Smoke/probe layout for SQLite async callback repository mapping.");
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("async-callback"));
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteAsyncCallbackRepository"));
	Document.RootWidgetId = TEXT("SQLUI.SQLite.AsyncCallbackRepository.Root");

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(TEXT("async-callback"));
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteAsyncCallbackRepository"));
	}

	return Document;
}

bool WaitForSQLUISampleSmokeCallback(TFunctionRef<bool()> IsCallbackDelivered)
{
	const double TimeoutSeconds = 5.0;
	const double StartSeconds = FPlatformTime::Seconds();

	while (!IsCallbackDelivered() && (FPlatformTime::Seconds() - StartSeconds) < TimeoutSeconds)
	{
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		if (!IsCallbackDelivered())
		{
			FPlatformProcess::Sleep(0.01f);
		}
	}

	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	return IsCallbackDelivered();
}

struct FSQLUISampleSQLiteAsyncCallbackSaveState
{
	FSQLUILayoutSaveResult Result;
	bool bCallbackDelivered = false;
	bool bDeliveredOnGameThread = false;
};

struct FSQLUISampleSQLiteAsyncCallbackLoadState
{
	FSQLUILayoutLoadResult Result;
	bool bCallbackDelivered = false;
	bool bDeliveredOnGameThread = false;
};

FSQLUISampleSQLiteAsyncCallbackRepositorySmokeResult RunSQLUISampleSQLiteAsyncCallbackRepositorySmoke(
	UObject* Outer)
{
	FSQLUISampleSQLiteAsyncCallbackRepositorySmokeResult Result;
	Result.DatabasePath = MakeSQLUISampleSQLiteAsyncCallbackRepositoryDatabasePath();

	const FSQLUISQLiteLayoutSchemaMigrationProbeResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::RunProbe(Result.DatabasePath, false);
	Result.bDatabasePrepared = SchemaResult.bSucceeded && SchemaResult.bMigrationSucceeded;
	if (!Result.bDatabasePrepared)
	{
		AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
			Result,
			SchemaResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite async callback repository smoke failed: could not prepare probe database.")
				: SchemaResult.ErrorMessage);
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteAsyncCallbackRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
			Result,
			TEXT("SQLUI SQLite async callback repository smoke failed: could not create repository object."));
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteAsyncCallbackRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = Result.DatabasePath;
	RepositorySettings.bReadOnly = false;
	RepositorySettings.bRunCallbackOperationsAsync = true;
	Repository->Configure(RepositorySettings);

	const FSQLUILayoutDocument Document =
		MakeSQLUISampleSQLiteAsyncCallbackRepositoryDocument();

	TSharedRef<FSQLUISampleSQLiteAsyncCallbackSaveState, ESPMode::ThreadSafe> SaveState =
		MakeShared<FSQLUISampleSQLiteAsyncCallbackSaveState, ESPMode::ThreadSafe>();
	SaveState->Result.SavedLayoutId = Document.Metadata.LayoutId;
	Repository->SaveLayout(
		Document,
		FSQLUILayoutSaveCompleteDelegate::CreateLambda(
			[SaveState](const FSQLUILayoutSaveResult& InSaveResult)
			{
				SaveState->Result = InSaveResult;
				SaveState->bDeliveredOnGameThread = IsInGameThread();
				SaveState->bCallbackDelivered = true;
			}));

	Result.bSaveCallbackDelivered =
		WaitForSQLUISampleSmokeCallback(
			[SaveState]()
			{
				return SaveState->bCallbackDelivered;
			});
	if (!Result.bSaveCallbackDelivered)
	{
		SaveState->Result.bSucceeded = false;
		SaveState->Result.ErrorMessage =
			TEXT("SQLUI SQLite async callback repository smoke failed: SaveLayout callback timed out.");
		AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
			Result,
			SaveState->Result.ErrorMessage);
	}

	Result.SavedLayoutId = SaveState->Result.SavedLayoutId;
	Result.bSaveSucceeded =
		Result.bSaveCallbackDelivered
		&& SaveState->Result.bSucceeded
		&& SaveState->Result.SavedLayoutId == Document.Metadata.LayoutId
		&& SaveState->Result.Validation.bIsValid;
	if (Result.bSaveCallbackDelivered && !Result.bSaveSucceeded)
	{
		AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
			Result,
			SaveState->Result.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite async callback repository smoke failed: async SaveLayout failed.")
				: SaveState->Result.ErrorMessage);
	}

	TSharedRef<FSQLUISampleSQLiteAsyncCallbackLoadState, ESPMode::ThreadSafe> LoadState =
		MakeShared<FSQLUISampleSQLiteAsyncCallbackLoadState, ESPMode::ThreadSafe>();
	LoadState->Result.Document.Metadata.LayoutId = Document.Metadata.LayoutId;
	if (Result.bSaveSucceeded)
	{
		Repository->LoadLayout(
			Document.Metadata.LayoutId,
			FSQLUILayoutLoadCompleteDelegate::CreateLambda(
				[LoadState](const FSQLUILayoutLoadResult& InLoadResult)
				{
					LoadState->Result = InLoadResult;
					LoadState->bDeliveredOnGameThread = IsInGameThread();
					LoadState->bCallbackDelivered = true;
				}));

		Result.bLoadCallbackDelivered =
			WaitForSQLUISampleSmokeCallback(
				[LoadState]()
				{
					return LoadState->bCallbackDelivered;
				});
		if (!Result.bLoadCallbackDelivered)
		{
			LoadState->Result.bSucceeded = false;
			LoadState->Result.ErrorMessage =
				TEXT("SQLUI SQLite async callback repository smoke failed: LoadLayout callback timed out.");
			AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
				Result,
				LoadState->Result.ErrorMessage);
		}

		Result.bLoadSucceeded =
			Result.bLoadCallbackDelivered
			&& LoadState->Result.bSucceeded;
		Result.LoadedLayoutId = LoadState->Result.Document.Metadata.LayoutId;
		Result.bLoadedDocumentValid =
			Result.bLoadSucceeded
			&& LoadState->Result.Validation.bIsValid
			&& LoadState->Result.Document.Metadata.LayoutId == Document.Metadata.LayoutId;
		if (Result.bLoadCallbackDelivered && !Result.bLoadSucceeded)
		{
			AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
				Result,
				LoadState->Result.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite async callback repository smoke failed: async LoadLayout failed.")
					: LoadState->Result.ErrorMessage);
		}
		else if (Result.bLoadSucceeded && !Result.bLoadedDocumentValid)
		{
			AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
				Result,
				TEXT("SQLUI SQLite async callback repository smoke failed: loaded document was invalid or did not match the saved layout id."));
		}
	}

	Result.bCallbacksDeliveredOnGameThread =
		Result.bSaveCallbackDelivered
		&& SaveState->bDeliveredOnGameThread
		&& Result.bLoadCallbackDelivered
		&& LoadState->bDeliveredOnGameThread;
	if ((Result.bSaveCallbackDelivered || Result.bLoadCallbackDelivered)
		&& !Result.bCallbacksDeliveredOnGameThread)
	{
		AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
			Result,
			TEXT("SQLUI SQLite async callback repository smoke failed: callbacks were not delivered on the game thread."));
	}

	if (Result.bLoadedDocumentValid)
	{
		const FSQLUILayoutRepositoryListResult ListAfterCallbacksResult =
			Repository->ListLayouts();
		Result.bListAfterAsyncCallbacksSucceeded = ListAfterCallbacksResult.bSucceeded;
		Result.ListedLayoutCount = ListAfterCallbacksResult.Layouts.Num();
		if (!Result.bListAfterAsyncCallbacksSucceeded)
		{
			AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
				Result,
				ListAfterCallbacksResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite async callback repository smoke failed: ListLayouts failed after async callbacks.")
					: ListAfterCallbacksResult.ErrorMessage);
		}
		else
		{
			Result.bListedMetadataFound =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterCallbacksResult.Layouts,
					Document.Metadata);
			if (!Result.bListedMetadataFound)
			{
				AppendSQLUISampleSQLiteAsyncCallbackRepositoryError(
					Result,
					TEXT("SQLUI SQLite async callback repository smoke failed: ListLayouts did not include saved metadata and tags after async callbacks."));
			}
		}
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleSQLiteAsyncCallbackRepositoryFiles(Result.DatabasePath, Result);

	Result.bSucceeded =
		Result.bDatabasePrepared
		&& Result.bSaveCallbackDelivered
		&& Result.bSaveSucceeded
		&& Result.bLoadCallbackDelivered
		&& Result.bLoadSucceeded
		&& Result.bLoadedDocumentValid
		&& Result.bListAfterAsyncCallbacksSucceeded
		&& Result.bListedMetadataFound
		&& Result.bCallbacksDeliveredOnGameThread
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
	FSQLUISampleSQLiteSerializedAsyncCallbackRepositorySmokeResult& Result,
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

FString MakeSQLUISampleSQLiteSerializedAsyncCallbackRepositoryDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteSerializedAsyncCallbackRepository"),
		TEXT("SQLiteSerializedAsyncCallbackRepository.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUISampleSQLiteSerializedAsyncCallbackRepositoryFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteSerializedAsyncCallbackRepositorySmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite serialized async callback repository smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteSerializedAsyncCallbackRepositoryDocument()
{
	FSQLUILayoutDocument Document = MakeSQLUISampleSQLiteSaveLayoutRepositoryDocument();
	Document.Version.Label = TEXT("SQLite Serialized Async Callback Repository Probe");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.sqlite-serialized-async-callback-repository");
	Document.Metadata.DisplayName = TEXT("SQLUI SQLite Serialized Async Callback Repository Probe");
	Document.Metadata.Description = TEXT("Smoke/probe layout for SQLite serialized async callback repository mapping.");
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("serialized-async-callback"));
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteSerializedAsyncCallbackRepository"));
	Document.RootWidgetId = TEXT("SQLUI.SQLite.SerializedAsyncCallbackRepository.Root");

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(TEXT("serialized-async-callback"));
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteSerializedAsyncCallbackRepository"));
	}

	return Document;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteSerializedAsyncCallbackRepositoryUpdatedDocument(
	const FSQLUILayoutDocument& OriginalDocument)
{
	FSQLUILayoutDocument UpdatedDocument = OriginalDocument;
	UpdatedDocument.Version.Label = TEXT("SQLite Serialized Async Callback Repository Probe Updated");
	UpdatedDocument.Metadata.DisplayName = TEXT("SQLUI SQLite Serialized Async Callback Repository Probe Updated");
	UpdatedDocument.Metadata.Description = TEXT("Updated smoke/probe layout for SQLite serialized async callback repository mapping.");
	UpdatedDocument.Metadata.UpdatedAtUtc = TEXT("2026-05-30T00:02:00Z");
	UpdatedDocument.Metadata.Tags.Reset();
	UpdatedDocument.Metadata.Tags.Add(TEXT("sqlite"));
	UpdatedDocument.Metadata.Tags.Add(TEXT("smoke"));
	UpdatedDocument.Metadata.Tags.Add(TEXT("serialized-async-callback"));
	UpdatedDocument.Metadata.Tags.Add(TEXT("updated"));

	if (UpdatedDocument.Nodes.Num() > 0)
	{
		UpdatedDocument.Nodes[0].Properties.Add(TEXT("Text"), UpdatedDocument.Metadata.DisplayName);
		UpdatedDocument.Nodes[0].Tags.Reset();
		UpdatedDocument.Nodes[0].Tags.Add(TEXT("sqlite"));
		UpdatedDocument.Nodes[0].Tags.Add(TEXT("serialized-async-callback"));
		UpdatedDocument.Nodes[0].Tags.Add(TEXT("updated"));
	}

	return UpdatedDocument;
}

struct FSQLUISampleSQLiteSerializedAsyncCallbackRepositoryState
{
	FSQLUILayoutSaveResult FirstSaveResult;
	FSQLUILayoutSaveResult SecondSaveResult;
	FSQLUILayoutLoadResult LoadResult;
	bool bFirstSaveCallbackDelivered = false;
	bool bSecondSaveCallbackDelivered = false;
	bool bLoadCallbackDelivered = false;
	bool bFirstSaveDeliveredOnGameThread = false;
	bool bSecondSaveDeliveredOnGameThread = false;
	bool bLoadDeliveredOnGameThread = false;
	TArray<FString> CallbackOrder;
};

FSQLUISampleSQLiteSerializedAsyncCallbackRepositorySmokeResult
RunSQLUISampleSQLiteSerializedAsyncCallbackRepositorySmoke(UObject* Outer)
{
	FSQLUISampleSQLiteSerializedAsyncCallbackRepositorySmokeResult Result;
	Result.DatabasePath = MakeSQLUISampleSQLiteSerializedAsyncCallbackRepositoryDatabasePath();

	const FSQLUISQLiteLayoutSchemaMigrationProbeResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::RunProbe(Result.DatabasePath, false);
	Result.bDatabasePrepared = SchemaResult.bSucceeded && SchemaResult.bMigrationSucceeded;
	if (!Result.bDatabasePrepared)
	{
		AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
			Result,
			SchemaResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite serialized async callback repository smoke failed: could not prepare probe database.")
				: SchemaResult.ErrorMessage);
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteSerializedAsyncCallbackRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
			Result,
			TEXT("SQLUI SQLite serialized async callback repository smoke failed: could not create repository object."));
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteSerializedAsyncCallbackRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = Result.DatabasePath;
	RepositorySettings.bReadOnly = false;
	RepositorySettings.bRunCallbackOperationsAsync = true;
	Repository->Configure(RepositorySettings);

	const FSQLUILayoutDocument FirstDocument =
		MakeSQLUISampleSQLiteSerializedAsyncCallbackRepositoryDocument();
	const FSQLUILayoutDocument UpdatedDocument =
		MakeSQLUISampleSQLiteSerializedAsyncCallbackRepositoryUpdatedDocument(FirstDocument);

	TSharedRef<FSQLUISampleSQLiteSerializedAsyncCallbackRepositoryState, ESPMode::ThreadSafe> SharedState =
		MakeShared<FSQLUISampleSQLiteSerializedAsyncCallbackRepositoryState, ESPMode::ThreadSafe>();
	SharedState->FirstSaveResult.SavedLayoutId = FirstDocument.Metadata.LayoutId;
	SharedState->SecondSaveResult.SavedLayoutId = UpdatedDocument.Metadata.LayoutId;
	SharedState->LoadResult.Document.Metadata.LayoutId = UpdatedDocument.Metadata.LayoutId;

	Repository->SaveLayout(
		FirstDocument,
		FSQLUILayoutSaveCompleteDelegate::CreateLambda(
			[SharedState](const FSQLUILayoutSaveResult& InSaveResult)
			{
				SharedState->FirstSaveResult = InSaveResult;
				SharedState->bFirstSaveDeliveredOnGameThread = IsInGameThread();
				SharedState->CallbackOrder.Add(TEXT("Save1"));
				SharedState->bFirstSaveCallbackDelivered = true;
			}));

	Repository->SaveLayout(
		UpdatedDocument,
		FSQLUILayoutSaveCompleteDelegate::CreateLambda(
			[SharedState](const FSQLUILayoutSaveResult& InSaveResult)
			{
				SharedState->SecondSaveResult = InSaveResult;
				SharedState->bSecondSaveDeliveredOnGameThread = IsInGameThread();
				SharedState->CallbackOrder.Add(TEXT("Save2"));
				SharedState->bSecondSaveCallbackDelivered = true;
			}));

	Repository->LoadLayout(
		UpdatedDocument.Metadata.LayoutId,
		FSQLUILayoutLoadCompleteDelegate::CreateLambda(
			[SharedState](const FSQLUILayoutLoadResult& InLoadResult)
			{
				SharedState->LoadResult = InLoadResult;
				SharedState->bLoadDeliveredOnGameThread = IsInGameThread();
				SharedState->CallbackOrder.Add(TEXT("Load"));
				SharedState->bLoadCallbackDelivered = true;
			}));

	const bool bAllCallbacksDelivered =
		WaitForSQLUISampleSmokeCallback(
			[SharedState]()
			{
				return SharedState->bFirstSaveCallbackDelivered
					&& SharedState->bSecondSaveCallbackDelivered
					&& SharedState->bLoadCallbackDelivered;
			});
	if (!bAllCallbacksDelivered)
	{
		AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
			Result,
			TEXT("SQLUI SQLite serialized async callback repository smoke failed: one or more callbacks timed out."));
	}

	Result.bFirstSaveCallbackDelivered = SharedState->bFirstSaveCallbackDelivered;
	Result.bSecondSaveCallbackDelivered = SharedState->bSecondSaveCallbackDelivered;
	Result.bLoadCallbackDelivered = SharedState->bLoadCallbackDelivered;
	Result.CallbackOrder = SharedState->CallbackOrder;

	Result.SavedLayoutId = SharedState->SecondSaveResult.SavedLayoutId;
	Result.bFirstSaveSucceeded =
		Result.bFirstSaveCallbackDelivered
		&& SharedState->FirstSaveResult.bSucceeded
		&& SharedState->FirstSaveResult.SavedLayoutId == FirstDocument.Metadata.LayoutId
		&& SharedState->FirstSaveResult.Validation.bIsValid;
	if (Result.bFirstSaveCallbackDelivered && !Result.bFirstSaveSucceeded)
	{
		AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
			Result,
			SharedState->FirstSaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite serialized async callback repository smoke failed: first SaveLayout failed.")
				: SharedState->FirstSaveResult.ErrorMessage);
	}

	Result.bSecondSaveSucceeded =
		Result.bSecondSaveCallbackDelivered
		&& SharedState->SecondSaveResult.bSucceeded
		&& SharedState->SecondSaveResult.SavedLayoutId == UpdatedDocument.Metadata.LayoutId
		&& SharedState->SecondSaveResult.Validation.bIsValid;
	if (Result.bSecondSaveCallbackDelivered && !Result.bSecondSaveSucceeded)
	{
		AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
			Result,
			SharedState->SecondSaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite serialized async callback repository smoke failed: second SaveLayout failed.")
				: SharedState->SecondSaveResult.ErrorMessage);
	}

	Result.bLoadSucceeded =
		Result.bLoadCallbackDelivered
		&& SharedState->LoadResult.bSucceeded;
	Result.LoadedLayoutId = SharedState->LoadResult.Document.Metadata.LayoutId;
	Result.bLatestRevisionLoaded =
		Result.bLoadSucceeded
		&& SharedState->LoadResult.Validation.bIsValid
		&& SharedState->LoadResult.Document.Metadata.LayoutId == UpdatedDocument.Metadata.LayoutId
		&& SharedState->LoadResult.Document.Metadata.DisplayName == UpdatedDocument.Metadata.DisplayName
		&& DoesSQLUISampleLayoutMetadataAndTagsMatch(
			SharedState->LoadResult.Document.Metadata,
			UpdatedDocument.Metadata)
		&& SharedState->LoadResult.Document.Version.Revision == 2;
	if (Result.bLoadCallbackDelivered && !Result.bLoadSucceeded)
	{
		AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
			Result,
			SharedState->LoadResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite serialized async callback repository smoke failed: LoadLayout failed.")
				: SharedState->LoadResult.ErrorMessage);
	}
	else if (Result.bLoadSucceeded && !Result.bLatestRevisionLoaded)
	{
		AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
			Result,
			TEXT("SQLUI SQLite serialized async callback repository smoke failed: LoadLayout did not return the updated revision 2 document."));
	}

	Result.bCallbacksDeliveredInOrder =
		Result.CallbackOrder.Num() == 3
		&& Result.CallbackOrder[0] == TEXT("Save1")
		&& Result.CallbackOrder[1] == TEXT("Save2")
		&& Result.CallbackOrder[2] == TEXT("Load");
	if (bAllCallbacksDelivered && !Result.bCallbacksDeliveredInOrder)
	{
		AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
			Result,
			TEXT("SQLUI SQLite serialized async callback repository smoke failed: callbacks were not delivered in enqueue order."));
	}

	Result.bCallbacksDeliveredOnGameThread =
		Result.bFirstSaveCallbackDelivered
		&& SharedState->bFirstSaveDeliveredOnGameThread
		&& Result.bSecondSaveCallbackDelivered
		&& SharedState->bSecondSaveDeliveredOnGameThread
		&& Result.bLoadCallbackDelivered
		&& SharedState->bLoadDeliveredOnGameThread;
	if (bAllCallbacksDelivered && !Result.bCallbacksDeliveredOnGameThread)
	{
		AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
			Result,
			TEXT("SQLUI SQLite serialized async callback repository smoke failed: callbacks were not delivered on the game thread."));
	}

	if (Result.bLatestRevisionLoaded)
	{
		const FSQLUILayoutRepositoryListResult ListAfterCallbacksResult =
			Repository->ListLayouts();
		Result.bListAfterSerializedCallbacksSucceeded = ListAfterCallbacksResult.bSucceeded;
		Result.ListedLayoutCount = ListAfterCallbacksResult.Layouts.Num();
		if (!Result.bListAfterSerializedCallbacksSucceeded)
		{
			AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
				Result,
				ListAfterCallbacksResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite serialized async callback repository smoke failed: ListLayouts failed after serialized callbacks.")
					: ListAfterCallbacksResult.ErrorMessage);
		}
		else
		{
			Result.bListedUpdatedMetadataFound =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterCallbacksResult.Layouts,
					UpdatedDocument.Metadata);
			if (!Result.bListedUpdatedMetadataFound)
			{
				AppendSQLUISampleSQLiteSerializedAsyncCallbackRepositoryError(
					Result,
					TEXT("SQLUI SQLite serialized async callback repository smoke failed: ListLayouts did not include updated metadata and tags after serialized callbacks."));
			}
		}
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleSQLiteSerializedAsyncCallbackRepositoryFiles(Result.DatabasePath, Result);

	Result.bSucceeded =
		Result.bDatabasePrepared
		&& Result.bFirstSaveCallbackDelivered
		&& Result.bFirstSaveSucceeded
		&& Result.bSecondSaveCallbackDelivered
		&& Result.bSecondSaveSucceeded
		&& Result.bLoadCallbackDelivered
		&& Result.bLoadSucceeded
		&& Result.bCallbacksDeliveredInOrder
		&& Result.bCallbacksDeliveredOnGameThread
		&& Result.bLatestRevisionLoaded
		&& Result.bListAfterSerializedCallbacksSucceeded
		&& Result.bListedUpdatedMetadataFound
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
	FSQLUISampleSQLiteFactoryLayoutRepositorySmokeResult& Result,
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

FString MakeSQLUISampleSQLiteFactoryLayoutRepositoryDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteFactoryRepository"),
		TEXT("SQLiteFactoryRepository.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUISampleSQLiteFactoryLayoutRepositoryFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteFactoryLayoutRepositorySmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite factory layout repository smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteFactoryLayoutRepositoryDocument()
{
	FSQLUILayoutDocument Document = MakeSQLUISampleSQLiteSaveLayoutRepositoryDocument();
	Document.Version.Label = TEXT("SQLite Factory Layout Repository Probe");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.sqlite-factory-layout-repository");
	Document.Metadata.DisplayName = TEXT("SQLUI SQLite Factory Layout Repository Probe");
	Document.Metadata.Description = TEXT("Smoke/probe layout for SQLite factory repository selection.");
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("factory"));
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteFactoryRepository"));
	Document.RootWidgetId = TEXT("SQLUI.SQLite.FactoryRepository.Root");

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(TEXT("factory"));
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteFactoryRepository"));
	}

	return Document;
}

FSQLUISampleSQLiteFactoryLayoutRepositorySmokeResult RunSQLUISampleSQLiteFactoryLayoutRepositorySmoke(
	UObject* Outer)
{
	FSQLUISampleSQLiteFactoryLayoutRepositorySmokeResult Result;
	Result.DatabasePath = MakeSQLUISampleSQLiteFactoryLayoutRepositoryDatabasePath();

	const FSQLUISQLiteLayoutSchemaMigrationProbeResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::RunProbe(Result.DatabasePath, false);
	Result.bDatabasePrepared = SchemaResult.bSucceeded && SchemaResult.bMigrationSucceeded;
	if (!Result.bDatabasePrepared)
	{
		AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
			Result,
			SchemaResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite factory layout repository smoke failed: could not prepare probe database.")
				: SchemaResult.ErrorMessage);
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteFactoryLayoutRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	FSQLUILayoutRepositoryFactorySettings FactorySettings;
	FactorySettings.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	FactorySettings.SQLiteSettings.DatabasePath = Result.DatabasePath;
	FactorySettings.SQLiteSettings.bReadOnly = false;
	FactorySettings.SQLiteSettings.bRunCallbackOperationsAsync = true;

	USQLUILayoutRepository* CreatedRepository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, FactorySettings);
	Result.bCreatedRepository = IsValid(CreatedRepository);
	USQLUISQLiteLayoutRepository* SQLiteRepository =
		Cast<USQLUISQLiteLayoutRepository>(CreatedRepository);
	Result.bCreatedSQLiteRepository = IsValid(SQLiteRepository);
	if (!Result.bCreatedRepository || !Result.bCreatedSQLiteRepository)
	{
		AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
			Result,
			TEXT("SQLUI SQLite factory layout repository smoke failed: factory did not create a SQLite repository."));
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteFactoryLayoutRepositoryFiles(Result.DatabasePath, Result);
		return Result;
	}

	const FSQLUILayoutDocument Document =
		MakeSQLUISampleSQLiteFactoryLayoutRepositoryDocument();

	TSharedRef<FSQLUISampleSQLiteAsyncCallbackSaveState, ESPMode::ThreadSafe> SaveState =
		MakeShared<FSQLUISampleSQLiteAsyncCallbackSaveState, ESPMode::ThreadSafe>();
	SaveState->Result.SavedLayoutId = Document.Metadata.LayoutId;
	CreatedRepository->SaveLayout(
		Document,
		FSQLUILayoutSaveCompleteDelegate::CreateLambda(
			[SaveState](const FSQLUILayoutSaveResult& InSaveResult)
			{
				SaveState->Result = InSaveResult;
				SaveState->bDeliveredOnGameThread = IsInGameThread();
				SaveState->bCallbackDelivered = true;
			}));

	const bool bSaveCallbackDelivered =
		WaitForSQLUISampleSmokeCallback(
			[SaveState]()
			{
				return SaveState->bCallbackDelivered;
			});
	Result.SavedLayoutId = SaveState->Result.SavedLayoutId;
	Result.bSaveSucceeded =
		bSaveCallbackDelivered
		&& SaveState->bDeliveredOnGameThread
		&& SaveState->Result.bSucceeded
		&& SaveState->Result.SavedLayoutId == Document.Metadata.LayoutId
		&& SaveState->Result.Validation.bIsValid;
	if (!Result.bSaveSucceeded)
	{
		AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
			Result,
			SaveState->Result.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite factory layout repository smoke failed: SaveLayout failed or callback was not delivered on the game thread.")
				: SaveState->Result.ErrorMessage);
	}

	if (Result.bSaveSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListResult =
			SQLiteRepository->ListLayouts();
		Result.bListSucceeded = ListResult.bSucceeded;
		Result.ListedLayoutCount = ListResult.Layouts.Num();
		if (!Result.bListSucceeded)
		{
			AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
				Result,
				ListResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory layout repository smoke failed: ListLayouts failed.")
					: ListResult.ErrorMessage);
		}
		else
		{
			Result.bListedMetadataFound =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListResult.Layouts,
					Document.Metadata);
			if (!Result.bListedMetadataFound)
			{
				AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
					Result,
					TEXT("SQLUI SQLite factory layout repository smoke failed: ListLayouts did not include saved metadata and tags."));
			}
		}
	}

	TSharedRef<FSQLUISampleSQLiteAsyncCallbackLoadState, ESPMode::ThreadSafe> LoadState =
		MakeShared<FSQLUISampleSQLiteAsyncCallbackLoadState, ESPMode::ThreadSafe>();
	LoadState->Result.Document.Metadata.LayoutId = Document.Metadata.LayoutId;
	if (Result.bListedMetadataFound)
	{
		CreatedRepository->LoadLayout(
			Document.Metadata.LayoutId,
			FSQLUILayoutLoadCompleteDelegate::CreateLambda(
				[LoadState](const FSQLUILayoutLoadResult& InLoadResult)
				{
					LoadState->Result = InLoadResult;
					LoadState->bDeliveredOnGameThread = IsInGameThread();
					LoadState->bCallbackDelivered = true;
				}));

		const bool bLoadCallbackDelivered =
			WaitForSQLUISampleSmokeCallback(
				[LoadState]()
				{
					return LoadState->bCallbackDelivered;
				});
		Result.bLoadSucceeded =
			bLoadCallbackDelivered
			&& LoadState->bDeliveredOnGameThread
			&& LoadState->Result.bSucceeded;
		Result.LoadedLayoutId = LoadState->Result.Document.Metadata.LayoutId;
		Result.bLoadedDocumentValid =
			Result.bLoadSucceeded
			&& LoadState->Result.Validation.bIsValid
			&& LoadState->Result.Document.Metadata.LayoutId == Document.Metadata.LayoutId;
		if (!Result.bLoadSucceeded)
		{
			AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
				Result,
				LoadState->Result.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory layout repository smoke failed: LoadLayout failed or callback was not delivered on the game thread.")
					: LoadState->Result.ErrorMessage);
		}
		else if (!Result.bLoadedDocumentValid)
		{
			AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
				Result,
				TEXT("SQLUI SQLite factory layout repository smoke failed: loaded document was invalid or did not match the saved layout id."));
		}
	}

	if (Result.bLoadedDocumentValid)
	{
		const FSQLUILayoutRepositoryRemoveResult RemoveResult =
			SQLiteRepository->RemoveLayout(Document.Metadata.LayoutId);
		Result.RemovedLayoutId = RemoveResult.RemovedLayoutId;
		Result.bRemoveSucceeded = RemoveResult.bSucceeded;
		Result.bRemoved =
			RemoveResult.bSucceeded
			&& RemoveResult.bRemoved
			&& RemoveResult.RemovedLayoutId == Document.Metadata.LayoutId;
		if (!Result.bRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
				Result,
				RemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory layout repository smoke failed: RemoveLayout failed.")
					: RemoveResult.ErrorMessage);
		}
		else if (!Result.bRemoved)
		{
			AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
				Result,
				TEXT("SQLUI SQLite factory layout repository smoke failed: RemoveLayout did not report bRemoved=true."));
		}
	}

	if (Result.bRemoved)
	{
		const FSQLUILayoutRepositoryListResult ListAfterRemoveResult =
			SQLiteRepository->ListLayouts();
		Result.bListAfterRemoveSucceeded = ListAfterRemoveResult.bSucceeded;
		Result.ListedLayoutCountAfterRemove = ListAfterRemoveResult.Layouts.Num();
		if (!Result.bListAfterRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
				Result,
				ListAfterRemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory layout repository smoke failed: ListLayouts failed after remove.")
					: ListAfterRemoveResult.ErrorMessage);
		}
		else
		{
			Result.bMetadataAbsentAfterRemove =
				!DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListAfterRemoveResult.Layouts,
					Document.Metadata);
			if (!Result.bMetadataAbsentAfterRemove)
			{
				AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
					Result,
					TEXT("SQLUI SQLite factory layout repository smoke failed: removed metadata remained listed after remove."));
			}
		}
	}

	if (Result.bMetadataAbsentAfterRemove)
	{
		const FSQLUILayoutRepositoryClearResult ClearResult =
			SQLiteRepository->ClearLayouts();
		Result.bClearSucceeded = ClearResult.bSucceeded;
		Result.ClearRemovedCount = ClearResult.RemovedCount;
		if (!Result.bClearSucceeded)
		{
			AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
				Result,
				ClearResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory layout repository smoke failed: ClearLayouts failed.")
					: ClearResult.ErrorMessage);
		}
	}

	const FSQLUILayoutDocument MissingPathDocument =
		MakeSQLUISampleSQLiteFactoryLayoutRepositoryDocument();
	FSQLUILayoutRepositoryFactorySettings MissingPathSettings;
	MissingPathSettings.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	USQLUILayoutRepository* MissingPathRepository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, MissingPathSettings);
	const FSQLUILayoutSaveResult MissingPathSaveResult =
		SaveSQLUISampleLayoutToRepository(
			MissingPathRepository,
			TEXT("SQLite factory missing path repository"),
			MissingPathDocument);
	Result.bMissingPathUnavailable =
		IsValid(MissingPathRepository)
		&& !IsValid(Cast<USQLUISQLiteLayoutRepository>(MissingPathRepository))
		&& !MissingPathSaveResult.bSucceeded
		&& MissingPathSaveResult.bBackendUnavailable
		&& !MissingPathSaveResult.ErrorMessage.IsEmpty();
	if (!Result.bMissingPathUnavailable)
	{
		AppendSQLUISampleSQLiteFactoryLayoutRepositoryError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite factory layout repository smoke failed: missing SQLite database path did not return unavailable behavior. SaveSucceeded=%s BackendUnavailable=%s Error='%s'."),
				MissingPathSaveResult.bSucceeded ? TEXT("true") : TEXT("false"),
				MissingPathSaveResult.bBackendUnavailable ? TEXT("true") : TEXT("false"),
				*MissingPathSaveResult.ErrorMessage));
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleSQLiteFactoryLayoutRepositoryFiles(Result.DatabasePath, Result);

	Result.bSucceeded =
		Result.bDatabasePrepared
		&& Result.bCreatedRepository
		&& Result.bCreatedSQLiteRepository
		&& Result.bSaveSucceeded
		&& Result.bListSucceeded
		&& Result.bListedMetadataFound
		&& Result.bLoadSucceeded
		&& Result.bLoadedDocumentValid
		&& Result.bRemoveSucceeded
		&& Result.bRemoved
		&& Result.bListAfterRemoveSucceeded
		&& Result.bMetadataAbsentAfterRemove
		&& Result.bClearSucceeded
		&& Result.bMissingPathUnavailable
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
	FSQLUISampleSQLiteFactorySchemaInitRepositorySmokeResult& Result,
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

FString MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteFactorySchemaInitRepository"),
		TEXT("SQLiteFactorySchemaInitRepository.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

FString MakeSQLUISampleSQLiteFactorySchemaInitRepositoryMissingDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteFactorySchemaInitRepository"),
		TEXT("SQLiteFactorySchemaInitRepositoryMissing.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DoSQLUISampleSQLiteFactorySchemaInitRepositoryFilesExist(const FString& DatabasePath)
{
	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleSQLiteFactorySchemaInitRepositoryFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteFactorySchemaInitRepositorySmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite factory schema init repository smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument()
{
	FSQLUILayoutDocument Document = MakeSQLUISampleSQLiteFactoryLayoutRepositoryDocument();
	Document.Version.Label = TEXT("SQLite Factory Schema Init Repository Probe");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.sqlite-factory-schema-init-repository");
	Document.Metadata.DisplayName = TEXT("SQLUI SQLite Factory Schema Init Repository Probe");
	Document.Metadata.Description = TEXT("Smoke/probe layout for SQLite factory schema initialization.");
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("schema-init"));
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteFactorySchemaInitRepository"));
	Document.RootWidgetId = TEXT("SQLUI.SQLite.FactorySchemaInitRepository.Root");

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(TEXT("schema-init"));
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteFactorySchemaInitRepository"));
	}

	return Document;
}

FSQLUISampleSQLiteFactorySchemaInitRepositorySmokeResult RunSQLUISampleSQLiteFactorySchemaInitRepositorySmoke(
	UObject* Outer)
{
	FSQLUISampleSQLiteFactorySchemaInitRepositorySmokeResult Result;
	Result.DatabasePath = MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDatabasePath();
	Result.MissingDatabasePath =
		MakeSQLUISampleSQLiteFactorySchemaInitRepositoryMissingDatabasePath();

	const bool bDeletedMainFiles =
		DeleteSQLUISampleSQLiteFactorySchemaInitRepositoryFiles(Result.DatabasePath, Result);
	const bool bDeletedMissingFiles =
		DeleteSQLUISampleSQLiteFactorySchemaInitRepositoryFiles(Result.MissingDatabasePath, Result);
	Result.bDatabaseAbsentBeforeStart =
		bDeletedMainFiles
		&& bDeletedMissingFiles
		&& !DoSQLUISampleSQLiteFactorySchemaInitRepositoryFilesExist(Result.DatabasePath)
		&& !DoSQLUISampleSQLiteFactorySchemaInitRepositoryFilesExist(Result.MissingDatabasePath);
	if (!Result.bDatabaseAbsentBeforeStart)
	{
		AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
			Result,
			TEXT("SQLUI SQLite factory schema init repository smoke failed: probe database files were present before start or could not be removed."));
	}

	FSQLUILayoutRepositoryFactorySettings FactorySettings;
	FactorySettings.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	FactorySettings.SQLiteSettings.DatabasePath = Result.DatabasePath;
	FactorySettings.SQLiteSettings.bReadOnly = false;
	FactorySettings.SQLiteSettings.bRunCallbackOperationsAsync = true;
	FactorySettings.SQLiteSettings.bInitializeSchemaIfMissing = true;
	FactorySettings.SQLiteSettings.bCreateDatabaseIfMissing = true;

	USQLUILayoutRepository* CreatedRepository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, FactorySettings);
	Result.bCreatedRepository = IsValid(CreatedRepository);
	USQLUISQLiteLayoutRepository* SQLiteRepository =
		Cast<USQLUISQLiteLayoutRepository>(CreatedRepository);
	Result.bCreatedSQLiteRepository = IsValid(SQLiteRepository);
	if (!Result.bCreatedRepository || !Result.bCreatedSQLiteRepository)
	{
		AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
			Result,
			TEXT("SQLUI SQLite factory schema init repository smoke failed: factory did not create a SQLite repository."));
		Result.bDatabaseRemoved =
			DeleteSQLUISampleSQLiteFactorySchemaInitRepositoryFiles(Result.DatabasePath, Result)
			&& DeleteSQLUISampleSQLiteFactorySchemaInitRepositoryFiles(Result.MissingDatabasePath, Result);
		return Result;
	}

	const FSQLUILayoutDocument Document =
		MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument();

	TSharedRef<FSQLUISampleSQLiteAsyncCallbackSaveState, ESPMode::ThreadSafe> SaveState =
		MakeShared<FSQLUISampleSQLiteAsyncCallbackSaveState, ESPMode::ThreadSafe>();
	SaveState->Result.SavedLayoutId = Document.Metadata.LayoutId;
	CreatedRepository->SaveLayout(
		Document,
		FSQLUILayoutSaveCompleteDelegate::CreateLambda(
			[SaveState](const FSQLUILayoutSaveResult& InSaveResult)
			{
				SaveState->Result = InSaveResult;
				SaveState->bDeliveredOnGameThread = IsInGameThread();
				SaveState->bCallbackDelivered = true;
			}));

	const bool bSaveCallbackDelivered =
		WaitForSQLUISampleSmokeCallback(
			[SaveState]()
			{
				return SaveState->bCallbackDelivered;
			});
	Result.SavedLayoutId = SaveState->Result.SavedLayoutId;
	Result.bDatabaseCreated =
		!DoSQLUISampleSQLiteFactorySchemaInitRepositoryFilesExist(Result.MissingDatabasePath)
		&& FPaths::FileExists(Result.DatabasePath);
	Result.bSaveSucceeded =
		bSaveCallbackDelivered
		&& SaveState->bDeliveredOnGameThread
		&& SaveState->Result.bSucceeded
		&& SaveState->Result.SavedLayoutId == Document.Metadata.LayoutId
		&& SaveState->Result.Validation.bIsValid;
	if (!Result.bSaveSucceeded)
	{
		AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
			Result,
			SaveState->Result.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite factory schema init repository smoke failed: SaveLayout failed or callback was not delivered on the game thread.")
				: SaveState->Result.ErrorMessage);
	}

	if (Result.bSaveSucceeded && Result.bDatabaseCreated)
	{
		FSQLUISQLiteLayoutSchemaRowCounts RowCounts;
		FString RowCountErrorMessage;
		Result.bSaveInitializedSchema =
			FSQLUISQLiteLayoutReadProbe::CountLayoutSchemaRows(
				Result.DatabasePath,
				RowCounts,
				RowCountErrorMessage)
			&& RowCounts.Layouts == 1
			&& RowCounts.LayoutRevisions == 1
			&& RowCounts.LayoutTags == Document.Metadata.Tags.Num();
		if (!Result.bSaveInitializedSchema)
		{
			AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
				Result,
				RowCountErrorMessage.IsEmpty()
					? FString::Printf(
						TEXT("SQLUI SQLite factory schema init repository smoke failed: schema row counts after save were unexpected. Layouts=%d Revisions=%d Tags=%d."),
						RowCounts.Layouts,
						RowCounts.LayoutRevisions,
						RowCounts.LayoutTags)
					: RowCountErrorMessage);
		}
	}

	if (Result.bSaveInitializedSchema)
	{
		const FSQLUILayoutRepositoryListResult ListResult =
			SQLiteRepository->ListLayouts();
		Result.bListSucceeded = ListResult.bSucceeded;
		Result.ListedLayoutCount = ListResult.Layouts.Num();
		if (!Result.bListSucceeded)
		{
			AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
				Result,
				ListResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory schema init repository smoke failed: ListLayouts failed.")
					: ListResult.ErrorMessage);
		}
		else
		{
			Result.bListedMetadataFound =
				DoesSQLUISampleLayoutMetadataListContainMetadataAndTags(
					ListResult.Layouts,
					Document.Metadata);
			if (!Result.bListedMetadataFound)
			{
				AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
					Result,
					TEXT("SQLUI SQLite factory schema init repository smoke failed: ListLayouts did not include saved metadata and tags."));
			}
		}
	}

	TSharedRef<FSQLUISampleSQLiteAsyncCallbackLoadState, ESPMode::ThreadSafe> LoadState =
		MakeShared<FSQLUISampleSQLiteAsyncCallbackLoadState, ESPMode::ThreadSafe>();
	LoadState->Result.Document.Metadata.LayoutId = Document.Metadata.LayoutId;
	if (Result.bListedMetadataFound)
	{
		CreatedRepository->LoadLayout(
			Document.Metadata.LayoutId,
			FSQLUILayoutLoadCompleteDelegate::CreateLambda(
				[LoadState](const FSQLUILayoutLoadResult& InLoadResult)
				{
					LoadState->Result = InLoadResult;
					LoadState->bDeliveredOnGameThread = IsInGameThread();
					LoadState->bCallbackDelivered = true;
				}));

		const bool bLoadCallbackDelivered =
			WaitForSQLUISampleSmokeCallback(
				[LoadState]()
				{
					return LoadState->bCallbackDelivered;
				});
		Result.bLoadSucceeded =
			bLoadCallbackDelivered
			&& LoadState->bDeliveredOnGameThread
			&& LoadState->Result.bSucceeded;
		Result.LoadedLayoutId = LoadState->Result.Document.Metadata.LayoutId;
		Result.bLoadedDocumentValid =
			Result.bLoadSucceeded
			&& LoadState->Result.Validation.bIsValid
			&& LoadState->Result.Document.Metadata.LayoutId == Document.Metadata.LayoutId;
		if (!Result.bLoadSucceeded)
		{
			AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
				Result,
				LoadState->Result.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory schema init repository smoke failed: LoadLayout failed or callback was not delivered on the game thread.")
					: LoadState->Result.ErrorMessage);
		}
		else if (!Result.bLoadedDocumentValid)
		{
			AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
				Result,
				TEXT("SQLUI SQLite factory schema init repository smoke failed: loaded document was invalid or did not match the saved layout id."));
		}
	}

	if (Result.bLoadedDocumentValid)
	{
		const FSQLUILayoutRepositoryRemoveResult RemoveResult =
			SQLiteRepository->RemoveLayout(Document.Metadata.LayoutId);
		Result.bRemoveSucceeded =
			RemoveResult.bSucceeded
			&& RemoveResult.bRemoved
			&& RemoveResult.RemovedLayoutId == Document.Metadata.LayoutId;
		if (!Result.bRemoveSucceeded)
		{
			AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
				Result,
				RemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory schema init repository smoke failed: RemoveLayout failed.")
					: RemoveResult.ErrorMessage);
		}
	}

	if (Result.bRemoveSucceeded)
	{
		const FSQLUILayoutRepositoryClearResult ClearResult =
			SQLiteRepository->ClearLayouts();
		Result.bClearSucceeded = ClearResult.bSucceeded;
		Result.ClearRemovedCount = ClearResult.RemovedCount;
		if (!Result.bClearSucceeded)
		{
			AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
				Result,
				ClearResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory schema init repository smoke failed: ClearLayouts failed.")
					: ClearResult.ErrorMessage);
		}
	}

	const FSQLUILayoutDocument MissingDbDocument =
		MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument();
	FSQLUILayoutRepositoryFactorySettings MissingDbSettings;
	MissingDbSettings.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	MissingDbSettings.SQLiteSettings.DatabasePath = Result.MissingDatabasePath;
	MissingDbSettings.SQLiteSettings.bReadOnly = false;
	MissingDbSettings.SQLiteSettings.bInitializeSchemaIfMissing = false;
	MissingDbSettings.SQLiteSettings.bCreateDatabaseIfMissing = false;
	USQLUILayoutRepository* MissingDbRepository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, MissingDbSettings);
	const FSQLUILayoutSaveResult MissingDbSaveResult =
		SaveSQLUISampleLayoutToRepository(
			MissingDbRepository,
			TEXT("SQLite factory schema init missing database repository"),
			MissingDbDocument);
	Result.bMissingDbWithoutInitFailed =
		!MissingDbSaveResult.bSucceeded
		&& MissingDbSaveResult.ErrorMessage.Contains(TEXT("does not exist"));
	Result.bMissingDbWithoutInitNotCreated =
		!DoSQLUISampleSQLiteFactorySchemaInitRepositoryFilesExist(Result.MissingDatabasePath);
	if (!Result.bMissingDbWithoutInitFailed)
	{
		AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite factory schema init repository smoke failed: missing DB without init did not fail cleanly. SaveSucceeded=%s Error='%s'."),
				MissingDbSaveResult.bSucceeded ? TEXT("true") : TEXT("false"),
				*MissingDbSaveResult.ErrorMessage));
	}
	if (!Result.bMissingDbWithoutInitNotCreated)
	{
		AppendSQLUISampleSQLiteFactorySchemaInitRepositoryError(
			Result,
			TEXT("SQLUI SQLite factory schema init repository smoke failed: missing DB without init created a database file."));
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleSQLiteFactorySchemaInitRepositoryFiles(Result.DatabasePath, Result)
		&& DeleteSQLUISampleSQLiteFactorySchemaInitRepositoryFiles(Result.MissingDatabasePath, Result);

	Result.bSucceeded =
		Result.bDatabaseAbsentBeforeStart
		&& Result.bCreatedRepository
		&& Result.bCreatedSQLiteRepository
		&& Result.bSaveInitializedSchema
		&& Result.bDatabaseCreated
		&& Result.bSaveSucceeded
		&& Result.bListSucceeded
		&& Result.bListedMetadataFound
		&& Result.bLoadSucceeded
		&& Result.bLoadedDocumentValid
		&& Result.bRemoveSucceeded
		&& Result.bClearSucceeded
		&& Result.bMissingDbWithoutInitFailed
		&& Result.bMissingDbWithoutInitNotCreated
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
	FSQLUISampleLayoutRepositoryRuntimeConfigProbeResult& Result,
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

FString MakeSQLUISampleLayoutRepositoryRuntimeConfigDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutRepositoryRuntimeConfig"),
		TEXT("LayoutRepositoryRuntimeConfig.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

FString MakeSQLUISampleLayoutRepositoryRuntimeConfigSavedRepositoryDirectory()
{
	FString Directory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("LayoutRepositories"));
	FPaths::NormalizeFilename(Directory);
	return FPaths::ConvertRelativePathToFull(Directory);
}

bool DoSQLUISampleLayoutRepositoryRuntimeConfigFilesExist(const FString& DatabasePath)
{
	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleLayoutRepositoryRuntimeConfigFiles(
	const FString& DatabasePath,
	FSQLUISampleLayoutRepositoryRuntimeConfigProbeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI layout repository runtime config probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUISampleLayoutRepositoryRuntimeConfigDocument()
{
	FSQLUILayoutDocument Document = MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument();
	Document.Version.Label = TEXT("Layout Repository Runtime Config Probe");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.layout-repository-runtime-config");
	Document.Metadata.DisplayName = TEXT("SQLUI Layout Repository Runtime Config Probe");
	Document.Metadata.Description =
		TEXT("Smoke/probe layout for runtime repository configuration policy.");
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("runtime-config"));
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutRepositoryRuntimeConfig"));
	Document.RootWidgetId = TEXT("SQLUI.LayoutRepositoryRuntimeConfig.Root");

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(TEXT("runtime-config"));
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutRepositoryRuntimeConfig"));
	}

	return Document;
}

FSQLUISampleLayoutRepositoryRuntimeConfigProbeResult RunSQLUISampleLayoutRepositoryRuntimeConfigProbe(
	UObject* Outer)
{
	FSQLUISampleLayoutRepositoryRuntimeConfigProbeResult Result;
	Result.DatabasePath = MakeSQLUISampleLayoutRepositoryRuntimeConfigDatabasePath();

	const FSQLUILayoutRepositoryRuntimeConfig Defaults =
		FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
	const FSQLUILayoutRepositoryFactorySettings DefaultSettings =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(Defaults);
	Result.bDefaultBackendInMemory =
		Defaults.Backend == ESQLUILayoutRepositoryBackend::InMemory
		&& DefaultSettings.Backend == ESQLUILayoutRepositoryBackend::InMemory
		&& DefaultSettings.SQLiteSettings.DatabasePath.IsEmpty()
		&& !DefaultSettings.SQLiteSettings.bInitializeSchemaIfMissing
		&& !DefaultSettings.SQLiteSettings.bCreateDatabaseIfMissing
		&& !DefaultSettings.SQLiteSettings.bRunCallbackOperationsAsync;
	if (!Result.bDefaultBackendInMemory)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			TEXT("SQLUI layout repository runtime config probe failed: default config did not resolve to in-memory without SQLite side effects."));
	}

	const FSQLUILayoutRepositoryRuntimeConfig JsonConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			TEXT("-SQLUILayoutRepositoryBackend=JsonFile -SQLUIJsonFileLayoutRepositoryDir=C:/SQLUI/JsonLayouts"),
			Defaults);
	const FSQLUILayoutRepositoryFactorySettings JsonSettings =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(JsonConfig);
	Result.bJsonFileBackendParsed =
		JsonConfig.Backend == ESQLUILayoutRepositoryBackend::JsonFile
		&& JsonSettings.Backend == ESQLUILayoutRepositoryBackend::JsonFile
		&& JsonSettings.JsonFileBaseDirectory == TEXT("C:/SQLUI/JsonLayouts");
	if (!Result.bJsonFileBackendParsed)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			TEXT("SQLUI layout repository runtime config probe failed: JSON file backend command-line settings were not parsed."));
	}

	const FSQLUILayoutRepositoryRuntimeConfig SQLiteBackendConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			TEXT("-SQLUILayoutRepositoryBackend=SQLite"),
			Defaults);
	const FSQLUILayoutRepositoryFactorySettings SQLiteBackendSettings =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(SQLiteBackendConfig);
	Result.bSQLiteBackendParsed =
		SQLiteBackendConfig.Backend == ESQLUILayoutRepositoryBackend::SQLite
		&& SQLiteBackendSettings.Backend == ESQLUILayoutRepositoryBackend::SQLite
		&& SQLiteBackendSettings.SQLiteSettings.DatabasePath.IsEmpty();
	if (!Result.bSQLiteBackendParsed)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			TEXT("SQLUI layout repository runtime config probe failed: SQLite backend command-line setting was not parsed."));
	}

	const FString RelativeSQLiteFilename = TEXT("RuntimeConfigRelative.db");
	const FSQLUILayoutRepositoryRuntimeConfig RelativeSQLiteConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			TEXT("-SQLUILayoutRepositoryBackend=SQLite -SQLUISQLiteLayoutRepositoryPath=RuntimeConfigRelative.db"),
			Defaults);
	const FSQLUILayoutRepositoryFactorySettings RelativeSQLiteSettings =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(RelativeSQLiteConfig);
	const FString SavedRepositoryDirectory =
		MakeSQLUISampleLayoutRepositoryRuntimeConfigSavedRepositoryDirectory();
	Result.bRelativeSQLitePathResolvedUnderSaved =
		RelativeSQLiteSettings.Backend == ESQLUILayoutRepositoryBackend::SQLite
		&& RelativeSQLiteSettings.SQLiteSettings.DatabasePath.StartsWith(SavedRepositoryDirectory)
		&& RelativeSQLiteSettings.SQLiteSettings.DatabasePath.EndsWith(RelativeSQLiteFilename);
	if (!Result.bRelativeSQLitePathResolvedUnderSaved)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI layout repository runtime config probe failed: relative SQLite path resolved to '%s' instead of under '%s'."),
				*RelativeSQLiteSettings.SQLiteSettings.DatabasePath,
				*SavedRepositoryDirectory));
	}

	FString AbsoluteSQLitePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutRepositoryRuntimeConfig"),
		TEXT("AbsoluteRuntimeConfig.db"));
	FPaths::NormalizeFilename(AbsoluteSQLitePath);
	AbsoluteSQLitePath = FPaths::ConvertRelativePathToFull(AbsoluteSQLitePath);
	const FString AbsoluteSQLiteCommandLine = FString::Printf(
		TEXT("-SQLUILayoutRepositoryBackend=SQLite -SQLUISQLiteLayoutRepositoryPath=\"%s\""),
		*AbsoluteSQLitePath);
	const FSQLUILayoutRepositoryRuntimeConfig AbsoluteSQLiteConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			*AbsoluteSQLiteCommandLine,
			Defaults);
	const FSQLUILayoutRepositoryFactorySettings AbsoluteSQLiteSettings =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(AbsoluteSQLiteConfig);
	Result.bAbsoluteSQLitePathPreserved =
		AbsoluteSQLiteSettings.Backend == ESQLUILayoutRepositoryBackend::SQLite
		&& AbsoluteSQLiteSettings.SQLiteSettings.DatabasePath == AbsoluteSQLitePath;
	if (!Result.bAbsoluteSQLitePathPreserved)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI layout repository runtime config probe failed: absolute SQLite path resolved to '%s' instead of '%s'."),
				*AbsoluteSQLiteSettings.SQLiteSettings.DatabasePath,
				*AbsoluteSQLitePath));
	}

	const FSQLUILayoutRepositoryRuntimeConfig SQLiteFlagsConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			TEXT("-SQLUILayoutRepositoryBackend=SQLite -SQLUISQLiteLayoutRepositoryPath=RuntimeConfigFlags.db -SQLUISQLiteLayoutRepositoryReadOnly -SQLUISQLiteLayoutRepositoryInitializeSchema -SQLUISQLiteLayoutRepositoryCreateDatabase -SQLUISQLiteLayoutRepositoryAsyncCallbacks"),
			Defaults);
	const FSQLUILayoutRepositoryFactorySettings SQLiteFlagsSettings =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(SQLiteFlagsConfig);
	Result.bSQLiteFlagsParsed =
		SQLiteFlagsSettings.Backend == ESQLUILayoutRepositoryBackend::SQLite
		&& SQLiteFlagsSettings.SQLiteSettings.bReadOnly
		&& SQLiteFlagsSettings.SQLiteSettings.bInitializeSchemaIfMissing
		&& SQLiteFlagsSettings.SQLiteSettings.bCreateDatabaseIfMissing
		&& SQLiteFlagsSettings.SQLiteSettings.bRunCallbackOperationsAsync;
	if (!Result.bSQLiteFlagsParsed)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			TEXT("SQLUI layout repository runtime config probe failed: SQLite command-line flags were not parsed."));
	}

	const FString RuntimeConfigSeedPath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutRepositoryRuntimeConfig"),
		TEXT("SeedRuntimeConfig.db"));
	const FString SQLiteSeedFlagsCommandLine = FString::Printf(
		TEXT("-SQLUILayoutRepositoryBackend=SQLite -SQLUISQLiteLayoutRepositoryPath=RuntimeConfigSeedTarget.db -SQLUISQLiteLayoutRepositorySeedPath=\"%s\" -SQLUISQLiteLayoutRepositoryCopySeedIfMissing -SQLUISQLiteLayoutRepositoryOverwriteFromSeed"),
		*RuntimeConfigSeedPath);
	const FSQLUILayoutRepositoryRuntimeConfig SQLiteSeedFlagsConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			*SQLiteSeedFlagsCommandLine,
			Defaults);
	const FSQLUISQLiteSeedDatabaseCopyRequest SeedCopyRequest =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToSeedDatabaseCopyRequest(
			SQLiteSeedFlagsConfig);
	Result.bSQLiteSeedFlagsParsed =
		SQLiteSeedFlagsConfig.Backend == ESQLUILayoutRepositoryBackend::SQLite
		&& SQLiteSeedFlagsConfig.SQLiteSeedDatabasePath == RuntimeConfigSeedPath
		&& SQLiteSeedFlagsConfig.bSQLiteCopySeedIfMissing
		&& SQLiteSeedFlagsConfig.bSQLiteOverwriteDatabaseFromSeed;
	Result.bSQLiteSeedCopyRequestMapped =
		SeedCopyRequest.SeedDatabasePath == FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteSeedDatabasePath(RuntimeConfigSeedPath)
		&& SeedCopyRequest.TargetDatabasePath == FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(TEXT("RuntimeConfigSeedTarget.db"))
		&& SeedCopyRequest.bCopyIfTargetMissing
		&& SeedCopyRequest.bOverwriteTarget
		&& SeedCopyRequest.bCreateTargetDirectory;
	if (!Result.bSQLiteSeedFlagsParsed || !Result.bSQLiteSeedCopyRequestMapped)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			TEXT("SQLUI layout repository runtime config probe failed: SQLite seed database copy command-line settings were not parsed or mapped."));
	}

	USQLUILayoutRepository* MissingPathRepository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(
			Outer,
			SQLiteBackendSettings);
	const FSQLUILayoutDocument MissingPathDocument =
		MakeSQLUISampleLayoutRepositoryRuntimeConfigDocument();
	const FSQLUILayoutSaveResult MissingPathSaveResult =
		SaveSQLUISampleLayoutToRepository(
			MissingPathRepository,
			TEXT("layout repository runtime config missing SQLite path repository"),
			MissingPathDocument);
	Result.bSQLiteMissingPathUnavailable =
		SQLiteBackendSettings.Backend == ESQLUILayoutRepositoryBackend::SQLite
		&& SQLiteBackendSettings.SQLiteSettings.DatabasePath.IsEmpty()
		&& IsValid(MissingPathRepository)
		&& !IsValid(Cast<USQLUISQLiteLayoutRepository>(MissingPathRepository))
		&& !MissingPathSaveResult.bSucceeded
		&& MissingPathSaveResult.bBackendUnavailable;
	if (!Result.bSQLiteMissingPathUnavailable)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI layout repository runtime config probe failed: SQLite missing path did not remain unavailable. SaveSucceeded=%s BackendUnavailable=%s Error='%s'."),
				MissingPathSaveResult.bSucceeded ? TEXT("true") : TEXT("false"),
				MissingPathSaveResult.bBackendUnavailable ? TEXT("true") : TEXT("false"),
				*MissingPathSaveResult.ErrorMessage));
	}

	ESQLUILayoutRepositoryBackend ParsedInvalidBackend = ESQLUILayoutRepositoryBackend::SQLite;
	const bool bInvalidBackendParsed =
		FSQLUILayoutRepositoryRuntimeConfigResolver::TryParseBackend(
			TEXT("DefinitelyNotABackend"),
			ParsedInvalidBackend);
	const FSQLUILayoutRepositoryRuntimeConfig InvalidBackendConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			TEXT("-SQLUILayoutRepositoryBackend=DefinitelyNotABackend"),
			Defaults);
	Result.bInvalidBackendFallsBackToDefault =
		!bInvalidBackendParsed
		&& ParsedInvalidBackend == ESQLUILayoutRepositoryBackend::SQLite
		&& InvalidBackendConfig.Backend == Defaults.Backend;
	if (!Result.bInvalidBackendFallsBackToDefault)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			TEXT("SQLUI layout repository runtime config probe failed: invalid backend text did not fall back to defaults."));
	}

	DeleteSQLUISampleLayoutRepositoryRuntimeConfigFiles(Result.DatabasePath, Result);
	const FString FactorySQLiteCommandLine = FString::Printf(
		TEXT("-SQLUILayoutRepositoryBackend=SQLite -SQLUISQLiteLayoutRepositoryPath=\"%s\" -SQLUISQLiteLayoutRepositoryInitializeSchema -SQLUISQLiteLayoutRepositoryCreateDatabase -SQLUISQLiteLayoutRepositoryAsyncCallbacks"),
		*Result.DatabasePath);
	const FSQLUILayoutRepositoryRuntimeConfig FactorySQLiteConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			*FactorySQLiteCommandLine,
			Defaults);
	const FSQLUILayoutRepositoryFactorySettings FactorySQLiteSettings =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(FactorySQLiteConfig);
	USQLUILayoutRepository* CreatedRepository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, FactorySQLiteSettings);
	USQLUISQLiteLayoutRepository* CreatedSQLiteRepository =
		Cast<USQLUISQLiteLayoutRepository>(CreatedRepository);
	Result.bFactoryCreatedSQLiteRepository = IsValid(CreatedSQLiteRepository);
	if (!Result.bFactoryCreatedSQLiteRepository)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			TEXT("SQLUI layout repository runtime config probe failed: explicit SQLite factory settings did not create a SQLite repository."));
	}

	if (Result.bFactoryCreatedSQLiteRepository)
	{
		const FSQLUILayoutDocument Document =
			MakeSQLUISampleLayoutRepositoryRuntimeConfigDocument();
		TSharedRef<FSQLUISampleSQLiteAsyncCallbackSaveState, ESPMode::ThreadSafe> SaveState =
			MakeShared<FSQLUISampleSQLiteAsyncCallbackSaveState, ESPMode::ThreadSafe>();
		SaveState->Result.SavedLayoutId = Document.Metadata.LayoutId;
		CreatedRepository->SaveLayout(
			Document,
			FSQLUILayoutSaveCompleteDelegate::CreateLambda(
				[SaveState](const FSQLUILayoutSaveResult& InSaveResult)
				{
					SaveState->Result = InSaveResult;
					SaveState->bDeliveredOnGameThread = IsInGameThread();
					SaveState->bCallbackDelivered = true;
				}));

		const bool bSaveCallbackDelivered =
			WaitForSQLUISampleSmokeCallback(
				[SaveState]()
				{
					return SaveState->bCallbackDelivered;
				});
		Result.bFactorySQLiteSaveSucceeded =
			bSaveCallbackDelivered
			&& SaveState->bDeliveredOnGameThread
			&& SaveState->Result.bSucceeded
			&& SaveState->Result.SavedLayoutId == Document.Metadata.LayoutId
			&& SaveState->Result.Validation.bIsValid
			&& FPaths::FileExists(Result.DatabasePath);
		if (!Result.bFactorySQLiteSaveSucceeded)
		{
			AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
				Result,
				SaveState->Result.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime config probe failed: factory-created SQLite repository SaveLayout failed or callback was not delivered on the game thread.")
					: SaveState->Result.ErrorMessage);
		}
	}

	Result.bDatabaseRemoved =
		DeleteSQLUISampleLayoutRepositoryRuntimeConfigFiles(Result.DatabasePath, Result)
		&& !DoSQLUISampleLayoutRepositoryRuntimeConfigFilesExist(Result.DatabasePath);
	if (!Result.bDatabaseRemoved)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeConfigProbeError(
			Result,
			TEXT("SQLUI layout repository runtime config probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bDefaultBackendInMemory
		&& Result.bJsonFileBackendParsed
		&& Result.bSQLiteBackendParsed
		&& Result.bRelativeSQLitePathResolvedUnderSaved
		&& Result.bAbsoluteSQLitePathPreserved
		&& Result.bSQLiteFlagsParsed
		&& Result.bSQLiteSeedFlagsParsed
		&& Result.bSQLiteSeedCopyRequestMapped
		&& Result.bSQLiteMissingPathUnavailable
		&& Result.bInvalidBackendFallsBackToDefault
		&& Result.bFactoryCreatedSQLiteRepository
		&& Result.bFactorySQLiteSaveSucceeded
		&& Result.bDatabaseRemoved;

	return Result;
}

void AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
	FSQLUISampleLayoutRepositoryRuntimeIntegrationProbeResult& Result,
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

FString MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
	const TCHAR* SubDirectory,
	const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutRepositoryRuntimeIntegration"));
	if (SubDirectory && FCString::Strlen(SubDirectory) > 0)
	{
		DatabasePath = FPaths::Combine(DatabasePath, SubDirectory);
	}
	DatabasePath = FPaths::Combine(DatabasePath, DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISampleLayoutRepositoryRuntimeIntegrationDatabasePaths()
{
	TArray<FString> DatabasePaths;
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
		TEXT(""),
		TEXT("RuntimeIntegration.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
		TEXT("Seed"),
		TEXT("SeedRuntimeIntegration.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
		TEXT("Runtime"),
		TEXT("SeedCopiedRuntimeIntegration.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
		TEXT("Runtime"),
		TEXT("MissingPathShouldNotExist.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
		TEXT("Seed"),
		TEXT("MissingSeedRuntimeIntegration.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
		TEXT("Runtime"),
		TEXT("MissingSeedTargetRuntimeIntegration.db")));
	return DatabasePaths;
}

bool DoSQLUISampleLayoutRepositoryRuntimeIntegrationFilesExist(
	const FString& DatabasePath)
{
	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleLayoutRepositoryRuntimeIntegrationFiles(
	const FString& DatabasePath,
	FSQLUISampleLayoutRepositoryRuntimeIntegrationProbeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI layout repository runtime integration probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISampleLayoutRepositoryRuntimeIntegrationFiles(
	FSQLUISampleLayoutRepositoryRuntimeIntegrationProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& DatabasePath : MakeSQLUISampleLayoutRepositoryRuntimeIntegrationDatabasePaths())
	{
		bRemoved =
			DeleteSQLUISampleLayoutRepositoryRuntimeIntegrationFiles(DatabasePath, Result)
			&& bRemoved;
	}

	return bRemoved;
}

bool DoesAnySQLUISampleLayoutRepositoryRuntimeIntegrationFileExist()
{
	for (const FString& DatabasePath : MakeSQLUISampleLayoutRepositoryRuntimeIntegrationDatabasePaths())
	{
		if (DoSQLUISampleLayoutRepositoryRuntimeIntegrationFilesExist(DatabasePath))
		{
			return true;
		}
	}

	return false;
}

FSQLUILayoutDocument MakeSQLUISampleLayoutRepositoryRuntimeIntegrationDocument(
	const TCHAR* LayoutId,
	const TCHAR* DisplayName,
	const TCHAR* ProbeTag)
{
	FSQLUILayoutDocument Document =
		MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument();
	Document.Version.Label = DisplayName;
	Document.Metadata.LayoutId = LayoutId;
	Document.Metadata.DisplayName = DisplayName;
	Document.Metadata.Description =
		TEXT("Smoke/probe layout for runtime repository integration policy.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-06-04T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = Document.Metadata.CreatedAtUtc;
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(ProbeTag);
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutRepositoryRuntimeIntegration"));
	Document.RootWidgetId = FString::Printf(TEXT("%s.Root"), LayoutId);

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(ProbeTag);
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutRepositoryRuntimeIntegration"));
	}

	return Document;
}

USQLUISQLiteLayoutRepository* CreateSQLUISampleLayoutRepositoryRuntimeIntegrationSQLiteRepository(
	UObject* Outer,
	const FString& DatabasePath,
	bool bReadOnly)
{
	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		return nullptr;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = DatabasePath;
	RepositorySettings.bReadOnly = bReadOnly;
	RepositorySettings.bInitializeSchemaIfMissing = !bReadOnly;
	RepositorySettings.bCreateDatabaseIfMissing = !bReadOnly;
	Repository->Configure(RepositorySettings);
	return Repository;
}

bool PrepareSQLUISampleLayoutRepositoryRuntimeIntegrationSeedDatabase(
	UObject* Outer,
	const FString& DatabasePath,
	const FSQLUILayoutDocument& Document,
	FSQLUISampleLayoutRepositoryRuntimeIntegrationProbeResult& Result)
{
	DeleteSQLUISampleLayoutRepositoryRuntimeIntegrationFiles(DatabasePath, Result);

	USQLUISQLiteLayoutRepository* Repository =
		CreateSQLUISampleLayoutRepositoryRuntimeIntegrationSQLiteRepository(
			Outer,
			DatabasePath,
			false);
	if (!IsValid(Repository))
	{
		AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
			Result,
			TEXT("SQLUI layout repository runtime integration probe failed: could not create seed SQLite repository."));
		return false;
	}

	const FSQLUILayoutSaveResult SaveResult =
		SaveSQLUISampleLayoutToRepository(
			Repository,
			TEXT("layout repository runtime integration seed repository"),
			Document);
	if (!SaveResult.bSucceeded || SaveResult.SavedLayoutId != Document.Metadata.LayoutId)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
			Result,
			SaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime integration probe failed: could not save seed layout.")
				: SaveResult.ErrorMessage);
		return false;
	}

	return FPaths::FileExists(DatabasePath);
}

bool LoadSQLUISampleLayoutRepositoryRuntimeIntegrationLayout(
	UObject* Outer,
	const FString& DatabasePath,
	const FSQLUILayoutDocument& Document,
	FSQLUISampleLayoutRepositoryRuntimeIntegrationProbeResult& Result)
{
	USQLUISQLiteLayoutRepository* Repository =
		CreateSQLUISampleLayoutRepositoryRuntimeIntegrationSQLiteRepository(
			Outer,
			DatabasePath,
			true);
	if (!IsValid(Repository))
	{
		AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
			Result,
			TEXT("SQLUI layout repository runtime integration probe failed: could not create read-only SQLite repository."));
		return false;
	}

	const FSQLUILayoutLoadResult LoadResult =
		LoadSQLUISampleLayoutFromRepository(
			Repository,
			TEXT("layout repository runtime integration read-only repository"),
			Document.Metadata.LayoutId);
	if (LoadResult.bSucceeded
		&& LoadResult.Validation.bIsValid
		&& LoadResult.Document.Metadata.LayoutId == Document.Metadata.LayoutId)
	{
		return true;
	}

	AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
		Result,
		LoadResult.ErrorMessage.IsEmpty()
			? TEXT("SQLUI layout repository runtime integration probe failed: could not load expected layout.")
			: LoadResult.ErrorMessage);
	return false;
}

FSQLUISampleLayoutRepositoryRuntimeIntegrationProbeResult
RunSQLUISampleLayoutRepositoryRuntimeIntegrationProbe(UObject* Outer)
{
	FSQLUISampleLayoutRepositoryRuntimeIntegrationProbeResult Result;
	Result.DatabasePath = MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
		TEXT(""),
		TEXT("RuntimeIntegration.db"));
	Result.SeedDatabasePath = MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
		TEXT("Seed"),
		TEXT("SeedRuntimeIntegration.db"));
	Result.SeedTargetDatabasePath = MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
		TEXT("Runtime"),
		TEXT("SeedCopiedRuntimeIntegration.db"));
	const FString MissingPathMarkerDatabasePath =
		MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
			TEXT("Runtime"),
			TEXT("MissingPathShouldNotExist.db"));
	const FString MissingSeedPath =
		MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
			TEXT("Seed"),
			TEXT("MissingSeedRuntimeIntegration.db"));
	const FString MissingSeedTargetPath =
		MakeSQLUISampleLayoutRepositoryRuntimeIntegrationPath(
			TEXT("Runtime"),
			TEXT("MissingSeedTargetRuntimeIntegration.db"));

	DeleteSQLUISampleLayoutRepositoryRuntimeIntegrationFiles(Result);

	FSQLUILayoutRepositoryRuntimeIntegrationRequest DefaultRequest;
	DefaultRequest.RuntimeConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
	const FSQLUILayoutRepositoryRuntimeIntegrationResult DefaultIntegrationResult =
		FSQLUILayoutRepositoryRuntimeIntegration::CreateRepository(
			Outer,
			DefaultRequest);
	Result.bDefaultCreatedRepository =
		DefaultIntegrationResult.bRepositoryCreated
		&& IsValid(DefaultIntegrationResult.Repository.Get());
	Result.bDefaultBackendInMemory =
		DefaultIntegrationResult.bSucceeded
		&& DefaultIntegrationResult.Backend == ESQLUILayoutRepositoryBackend::InMemory;
	Result.bDefaultNotSQLite =
		!IsValid(Cast<USQLUISQLiteLayoutRepository>(
			DefaultIntegrationResult.Repository.Get()));
	if (!Result.bDefaultCreatedRepository
		|| !Result.bDefaultBackendInMemory
		|| !Result.bDefaultNotSQLite)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
			Result,
			DefaultIntegrationResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime integration probe failed: default config did not create an in-memory non-SQLite repository.")
				: DefaultIntegrationResult.ErrorMessage);
	}

	const FSQLUILayoutDocument RuntimeDocument =
		MakeSQLUISampleLayoutRepositoryRuntimeIntegrationDocument(
			TEXT("sqlui.smoke.runtime-integration.sqlite"),
			TEXT("SQLUI Runtime Integration SQLite Layout"),
			TEXT("runtime-integration"));
	FSQLUILayoutRepositoryRuntimeIntegrationRequest SQLiteRequest;
	SQLiteRequest.RuntimeConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	SQLiteRequest.RuntimeConfig.SQLiteDatabasePath = Result.DatabasePath;
	SQLiteRequest.RuntimeConfig.bSQLiteReadOnly = false;
	SQLiteRequest.RuntimeConfig.bSQLiteInitializeSchemaIfMissing = true;
	SQLiteRequest.RuntimeConfig.bSQLiteCreateDatabaseIfMissing = true;
	const FSQLUILayoutRepositoryRuntimeIntegrationResult SQLiteIntegrationResult =
		FSQLUILayoutRepositoryRuntimeIntegration::CreateRepository(
			Outer,
			SQLiteRequest);
	Result.bSQLiteCreatedRepository =
		SQLiteIntegrationResult.bRepositoryCreated
		&& IsValid(SQLiteIntegrationResult.Repository.Get());
	USQLUISQLiteLayoutRepository* SQLiteRepository =
		Cast<USQLUISQLiteLayoutRepository>(SQLiteIntegrationResult.Repository.Get());
	Result.bSQLiteRepositoryCreated = IsValid(SQLiteRepository);
	if (!Result.bSQLiteCreatedRepository || !Result.bSQLiteRepositoryCreated)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
			Result,
			SQLiteIntegrationResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime integration probe failed: explicit SQLite config did not create a SQLite repository.")
				: SQLiteIntegrationResult.ErrorMessage);
	}

	if (Result.bSQLiteRepositoryCreated)
	{
		const FSQLUILayoutSaveResult SaveResult =
			SaveSQLUISampleLayoutToRepository(
				SQLiteRepository,
				TEXT("layout repository runtime integration SQLite repository"),
				RuntimeDocument);
		Result.bSQLiteSaveSucceeded =
			SaveResult.bSucceeded
			&& SaveResult.SavedLayoutId == RuntimeDocument.Metadata.LayoutId
			&& SaveResult.Validation.bIsValid;
		Result.bSQLiteDatabaseCreated =
			Result.bSQLiteSaveSucceeded
			&& FPaths::FileExists(Result.DatabasePath);
		if (!Result.bSQLiteSaveSucceeded)
		{
			AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
				Result,
				SaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime integration probe failed: explicit SQLite repository SaveLayout failed.")
					: SaveResult.ErrorMessage);
		}

		const FSQLUILayoutRepositoryListResult ListResult =
			SQLiteRepository->ListLayouts();
		Result.bSQLiteListSucceeded = ListResult.bSucceeded;
		Result.bSQLiteListedMetadataFound =
			ListResult.bSucceeded
			&& DoesSQLUISampleLayoutMetadataAndTagsListContain(
				ListResult.Layouts,
				RuntimeDocument.Metadata);
		if (!Result.bSQLiteListSucceeded || !Result.bSQLiteListedMetadataFound)
		{
			AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
				Result,
				ListResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime integration probe failed: explicit SQLite repository ListLayouts did not include saved metadata and tags.")
					: ListResult.ErrorMessage);
		}
	}

	FSQLUILayoutRepositoryRuntimeIntegrationRequest MissingPathRequest;
	MissingPathRequest.RuntimeConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	const FSQLUILayoutRepositoryRuntimeIntegrationResult MissingPathResult =
		FSQLUILayoutRepositoryRuntimeIntegration::CreateRepository(
			Outer,
			MissingPathRequest);
	Result.bSQLiteMissingPathUnavailable =
		!MissingPathResult.bSucceeded
		&& MissingPathResult.bBackendUnavailable
		&& MissingPathResult.bRepositoryCreated
		&& IsValid(MissingPathResult.Repository.Get())
		&& !IsValid(Cast<USQLUISQLiteLayoutRepository>(
			MissingPathResult.Repository.Get()));
	Result.bSQLiteMissingPathDidNotCreateDb =
		!DoSQLUISampleLayoutRepositoryRuntimeIntegrationFilesExist(
			MissingPathMarkerDatabasePath);
	if (!Result.bSQLiteMissingPathUnavailable
		|| !Result.bSQLiteMissingPathDidNotCreateDb)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
			Result,
			MissingPathResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime integration probe failed: explicit SQLite missing path did not stay unavailable without creating DB files.")
				: MissingPathResult.ErrorMessage);
	}

	const FSQLUILayoutDocument SeedDocument =
		MakeSQLUISampleLayoutRepositoryRuntimeIntegrationDocument(
			TEXT("sqlui.smoke.runtime-integration.seed"),
			TEXT("SQLUI Runtime Integration Seed Layout"),
			TEXT("runtime-integration-seed"));
	Result.bSeedDatabasePrepared =
		PrepareSQLUISampleLayoutRepositoryRuntimeIntegrationSeedDatabase(
			Outer,
			Result.SeedDatabasePath,
			SeedDocument,
			Result);
	if (Result.bSeedDatabasePrepared)
	{
		FSQLUILayoutRepositoryRuntimeIntegrationRequest SeedCopyRequest;
		SeedCopyRequest.RuntimeConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
		SeedCopyRequest.RuntimeConfig.SQLiteDatabasePath = Result.SeedTargetDatabasePath;
		SeedCopyRequest.RuntimeConfig.SQLiteSeedDatabasePath = Result.SeedDatabasePath;
		SeedCopyRequest.RuntimeConfig.bSQLiteCopySeedIfMissing = true;
		SeedCopyRequest.RuntimeConfig.bSQLiteReadOnly = true;
		const FSQLUILayoutRepositoryRuntimeIntegrationResult SeedCopyResult =
			FSQLUILayoutRepositoryRuntimeIntegration::CreateRepository(
				Outer,
				SeedCopyRequest);
		Result.bSeedCopyRequested = SeedCopyResult.bSeedCopyRequested;
		Result.bSeedCopySucceeded =
			SeedCopyResult.bSeedCopyRequested
			&& SeedCopyResult.bSeedCopySucceeded
			&& SeedCopyResult.bRepositoryCreated
			&& IsValid(Cast<USQLUISQLiteLayoutRepository>(
				SeedCopyResult.Repository.Get()))
			&& FPaths::FileExists(Result.SeedTargetDatabasePath);
		if (!Result.bSeedCopySucceeded)
		{
			AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
				Result,
				SeedCopyResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime integration probe failed: seed copy did not create a SQLite runtime repository target.")
					: SeedCopyResult.ErrorMessage);
		}

		USQLUISQLiteLayoutRepository* SeedCopyRepository =
			Cast<USQLUISQLiteLayoutRepository>(SeedCopyResult.Repository.Get());
		if (IsValid(SeedCopyRepository))
		{
			const FSQLUILayoutRepositoryListResult SeedTargetListResult =
				SeedCopyRepository->ListLayouts();
			Result.bSeedCopiedTargetReadable =
				SeedTargetListResult.bSucceeded
				&& DoesSQLUISampleLayoutMetadataAndTagsListContain(
					SeedTargetListResult.Layouts,
					SeedDocument.Metadata);
			if (!Result.bSeedCopiedTargetReadable)
			{
				AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
					Result,
					SeedTargetListResult.ErrorMessage.IsEmpty()
						? TEXT("SQLUI layout repository runtime integration probe failed: copied seed target did not list expected metadata and tags.")
						: SeedTargetListResult.ErrorMessage);
			}

			const FSQLUILayoutLoadResult SeedTargetLoadResult =
				LoadSQLUISampleLayoutFromRepository(
					SeedCopyRepository,
					TEXT("layout repository runtime integration seed-copied repository"),
					SeedDocument.Metadata.LayoutId);
			Result.bSeedCopiedTargetLoadedLayout =
				SeedTargetLoadResult.bSucceeded
				&& SeedTargetLoadResult.Validation.bIsValid
				&& SeedTargetLoadResult.Document.Metadata.LayoutId == SeedDocument.Metadata.LayoutId;
			if (!Result.bSeedCopiedTargetLoadedLayout)
			{
				AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
					Result,
					SeedTargetLoadResult.ErrorMessage.IsEmpty()
						? TEXT("SQLUI layout repository runtime integration probe failed: copied seed target did not load expected layout.")
						: SeedTargetLoadResult.ErrorMessage);
			}
		}

		Result.bSeedDatabaseLeftIntact =
			LoadSQLUISampleLayoutRepositoryRuntimeIntegrationLayout(
				Outer,
				Result.SeedDatabasePath,
				SeedDocument,
				Result);
	}
	else
	{
		AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
			Result,
			TEXT("SQLUI layout repository runtime integration probe failed: seed database was not prepared."));
	}

	FSQLUILayoutRepositoryRuntimeIntegrationRequest MissingSeedRequest;
	MissingSeedRequest.RuntimeConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	MissingSeedRequest.RuntimeConfig.SQLiteDatabasePath = MissingSeedTargetPath;
	MissingSeedRequest.RuntimeConfig.SQLiteSeedDatabasePath = MissingSeedPath;
	MissingSeedRequest.RuntimeConfig.bSQLiteCopySeedIfMissing = true;
	const FSQLUILayoutRepositoryRuntimeIntegrationResult MissingSeedResult =
		FSQLUILayoutRepositoryRuntimeIntegration::CreateRepository(
			Outer,
			MissingSeedRequest);
	Result.bSeedCopyFailureFatal =
		!MissingSeedResult.bSucceeded
		&& MissingSeedResult.bSeedCopyRequested
		&& !MissingSeedResult.bSeedCopySucceeded
		&& MissingSeedResult.ErrorMessage.Contains(TEXT("does not exist"));
	Result.bSeedCopyFailureDidNotCreateRepository =
		!MissingSeedResult.bRepositoryCreated;
	Result.bSeedCopyFailureDidNotCreateTarget =
		!DoSQLUISampleLayoutRepositoryRuntimeIntegrationFilesExist(MissingSeedTargetPath);
	if (!Result.bSeedCopyFailureFatal
		|| !Result.bSeedCopyFailureDidNotCreateRepository
		|| !Result.bSeedCopyFailureDidNotCreateTarget)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
			Result,
			MissingSeedResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime integration probe failed: missing seed did not fail before repository creation without creating target.")
				: MissingSeedResult.ErrorMessage);
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISampleLayoutRepositoryRuntimeIntegrationFiles(Result)
		&& !DoesAnySQLUISampleLayoutRepositoryRuntimeIntegrationFileExist();
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeIntegrationProbeError(
			Result,
			TEXT("SQLUI layout repository runtime integration probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bDefaultCreatedRepository
		&& Result.bDefaultBackendInMemory
		&& Result.bDefaultNotSQLite
		&& Result.bSQLiteCreatedRepository
		&& Result.bSQLiteRepositoryCreated
		&& Result.bSQLiteSaveSucceeded
		&& Result.bSQLiteDatabaseCreated
		&& Result.bSQLiteListSucceeded
		&& Result.bSQLiteListedMetadataFound
		&& Result.bSQLiteMissingPathUnavailable
		&& Result.bSQLiteMissingPathDidNotCreateDb
		&& Result.bSeedDatabasePrepared
		&& Result.bSeedCopyRequested
		&& Result.bSeedCopySucceeded
		&& Result.bSeedCopiedTargetReadable
		&& Result.bSeedCopiedTargetLoadedLayout
		&& Result.bSeedDatabaseLeftIntact
		&& Result.bSeedCopyFailureFatal
		&& Result.bSeedCopyFailureDidNotCreateRepository
		&& Result.bSeedCopyFailureDidNotCreateTarget
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
	FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult& Result,
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

FString MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
	const TCHAR* SubDirectory,
	const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutRepositoryRuntimeProvider"));
	if (SubDirectory && FCString::Strlen(SubDirectory) > 0)
	{
		DatabasePath = FPaths::Combine(DatabasePath, SubDirectory);
	}
	DatabasePath = FPaths::Combine(DatabasePath, DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISampleLayoutRepositoryRuntimeProviderDatabasePaths()
{
	TArray<FString> DatabasePaths;
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT(""),
		TEXT("RuntimeProvider.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT("Runtime"),
		TEXT("CommandLineRuntimeProvider.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT("Seed"),
		TEXT("SeedRuntimeProvider.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT("Runtime"),
		TEXT("SeedCopiedRuntimeProvider.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT("Runtime"),
		TEXT("MissingPathShouldNotExist.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT("Seed"),
		TEXT("MissingSeedRuntimeProvider.db")));
	DatabasePaths.Add(MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT("Runtime"),
		TEXT("MissingSeedTargetRuntimeProvider.db")));
	return DatabasePaths;
}

bool DoSQLUISampleLayoutRepositoryRuntimeProviderFilesExist(const FString& DatabasePath)
{
	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleLayoutRepositoryRuntimeProviderFiles(
	const FString& DatabasePath,
	FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI layout repository runtime provider probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISampleLayoutRepositoryRuntimeProviderFiles(
	FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& DatabasePath : MakeSQLUISampleLayoutRepositoryRuntimeProviderDatabasePaths())
	{
		bRemoved =
			DeleteSQLUISampleLayoutRepositoryRuntimeProviderFiles(DatabasePath, Result)
			&& bRemoved;
	}

	return bRemoved;
}

bool DoesAnySQLUISampleLayoutRepositoryRuntimeProviderFileExist()
{
	for (const FString& DatabasePath : MakeSQLUISampleLayoutRepositoryRuntimeProviderDatabasePaths())
	{
		if (DoSQLUISampleLayoutRepositoryRuntimeProviderFilesExist(DatabasePath))
		{
			return true;
		}
	}

	return false;
}

FSQLUILayoutDocument MakeSQLUISampleLayoutRepositoryRuntimeProviderDocument(
	const TCHAR* LayoutId,
	const TCHAR* DisplayName,
	const TCHAR* ProbeTag)
{
	FSQLUILayoutDocument Document =
		MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument();
	Document.Version.Label = DisplayName;
	Document.Metadata.LayoutId = LayoutId;
	Document.Metadata.DisplayName = DisplayName;
	Document.Metadata.Description =
		TEXT("Smoke/probe layout for runtime repository provider policy.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-06-05T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = Document.Metadata.CreatedAtUtc;
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(ProbeTag);
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutRepositoryRuntimeProvider"));
	Document.RootWidgetId = FString::Printf(TEXT("%s.Root"), LayoutId);

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(ProbeTag);
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutRepositoryRuntimeProvider"));
	}

	return Document;
}

USQLUILayoutRepositoryRuntimeProvider* CreateSQLUISampleLayoutRepositoryRuntimeProvider(
	UObject* Outer)
{
	return NewObject<USQLUILayoutRepositoryRuntimeProvider>(
		IsValid(Outer) ? Outer : GetTransientPackage());
}

USQLUISQLiteLayoutRepository* CreateSQLUISampleLayoutRepositoryRuntimeProviderSQLiteRepository(
	UObject* Outer,
	const FString& DatabasePath,
	bool bReadOnly)
{
	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		return nullptr;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = DatabasePath;
	RepositorySettings.bReadOnly = bReadOnly;
	RepositorySettings.bInitializeSchemaIfMissing = !bReadOnly;
	RepositorySettings.bCreateDatabaseIfMissing = !bReadOnly;
	Repository->Configure(RepositorySettings);
	return Repository;
}

bool PrepareSQLUISampleLayoutRepositoryRuntimeProviderSeedDatabase(
	UObject* Outer,
	const FString& DatabasePath,
	const FSQLUILayoutDocument& Document,
	FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult& Result)
{
	DeleteSQLUISampleLayoutRepositoryRuntimeProviderFiles(DatabasePath, Result);

	USQLUISQLiteLayoutRepository* Repository =
		CreateSQLUISampleLayoutRepositoryRuntimeProviderSQLiteRepository(
			Outer,
			DatabasePath,
			false);
	if (!IsValid(Repository))
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			TEXT("SQLUI layout repository runtime provider probe failed: could not create seed SQLite repository."));
		return false;
	}

	const FSQLUILayoutSaveResult SaveResult =
		SaveSQLUISampleLayoutToRepository(
			Repository,
			TEXT("layout repository runtime provider seed repository"),
			Document);
	if (!SaveResult.bSucceeded || SaveResult.SavedLayoutId != Document.Metadata.LayoutId)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			SaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime provider probe failed: could not save seed layout.")
				: SaveResult.ErrorMessage);
		return false;
	}

	return FPaths::FileExists(DatabasePath);
}

bool LoadSQLUISampleLayoutRepositoryRuntimeProviderLayout(
	UObject* Outer,
	const FString& DatabasePath,
	const FSQLUILayoutDocument& Document,
	FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult& Result)
{
	USQLUISQLiteLayoutRepository* Repository =
		CreateSQLUISampleLayoutRepositoryRuntimeProviderSQLiteRepository(
			Outer,
			DatabasePath,
			true);
	if (!IsValid(Repository))
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			TEXT("SQLUI layout repository runtime provider probe failed: could not create read-only SQLite repository."));
		return false;
	}

	const FSQLUILayoutLoadResult LoadResult =
		LoadSQLUISampleLayoutFromRepository(
			Repository,
			TEXT("layout repository runtime provider read-only repository"),
			Document.Metadata.LayoutId);
	if (LoadResult.bSucceeded
		&& LoadResult.Validation.bIsValid
		&& LoadResult.Document.Metadata.LayoutId == Document.Metadata.LayoutId)
	{
		return true;
	}

	AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
		Result,
		LoadResult.ErrorMessage.IsEmpty()
			? TEXT("SQLUI layout repository runtime provider probe failed: could not load expected layout.")
			: LoadResult.ErrorMessage);
	return false;
}

bool DoesSQLUISampleLayoutRepositoryRuntimeProviderListContain(
	USQLUILayoutRepositoryRuntimeProvider* Provider,
	const FSQLUILayoutDocument& Document,
	const TCHAR* RepositoryName,
	FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult& Result)
{
	USQLUISQLiteLayoutRepository* SQLiteRepository =
		IsValid(Provider) ? Cast<USQLUISQLiteLayoutRepository>(Provider->GetRepository()) : nullptr;
	if (!IsValid(SQLiteRepository))
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI layout repository runtime provider probe failed: %s was not SQLite."),
				RepositoryName));
		return false;
	}

	const FSQLUILayoutRepositoryListResult ListResult = SQLiteRepository->ListLayouts();
	if (ListResult.bSucceeded
		&& DoesSQLUISampleLayoutMetadataAndTagsListContain(
			ListResult.Layouts,
			Document.Metadata))
	{
		return true;
	}

	AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
		Result,
		ListResult.ErrorMessage.IsEmpty()
			? FString::Printf(
				TEXT("SQLUI layout repository runtime provider probe failed: %s did not list expected metadata and tags."),
				RepositoryName)
			: ListResult.ErrorMessage);
	return false;
}

bool DoesSQLUISampleLayoutRepositoryRuntimeProviderLoadLayout(
	USQLUILayoutRepositoryRuntimeProvider* Provider,
	const FSQLUILayoutDocument& Document,
	const TCHAR* RepositoryName,
	FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult& Result)
{
	const FSQLUILayoutLoadResult LoadResult =
		LoadSQLUISampleLayoutFromRepository(
			IsValid(Provider) ? Provider->GetRepository() : nullptr,
			RepositoryName,
			Document.Metadata.LayoutId);
	if (LoadResult.bSucceeded
		&& LoadResult.Validation.bIsValid
		&& LoadResult.Document.Metadata.LayoutId == Document.Metadata.LayoutId)
	{
		return true;
	}

	AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
		Result,
		LoadResult.ErrorMessage.IsEmpty()
			? FString::Printf(
				TEXT("SQLUI layout repository runtime provider probe failed: %s did not load expected layout."),
				RepositoryName)
			: LoadResult.ErrorMessage);
	return false;
}

FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult
RunSQLUISampleLayoutRepositoryRuntimeProviderProbe(UObject* Outer)
{
	FSQLUISampleLayoutRepositoryRuntimeProviderProbeResult Result;
	Result.DatabasePath = MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT(""),
		TEXT("RuntimeProvider.db"));
	Result.CommandLineDatabasePath = MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT("Runtime"),
		TEXT("CommandLineRuntimeProvider.db"));
	Result.SeedDatabasePath = MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT("Seed"),
		TEXT("SeedRuntimeProvider.db"));
	Result.SeedTargetDatabasePath = MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
		TEXT("Runtime"),
		TEXT("SeedCopiedRuntimeProvider.db"));
	const FString MissingPathMarkerDatabasePath =
		MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
			TEXT("Runtime"),
			TEXT("MissingPathShouldNotExist.db"));
	const FString MissingSeedPath =
		MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
			TEXT("Seed"),
			TEXT("MissingSeedRuntimeProvider.db"));
	const FString MissingSeedTargetPath =
		MakeSQLUISampleLayoutRepositoryRuntimeProviderPath(
			TEXT("Runtime"),
			TEXT("MissingSeedTargetRuntimeProvider.db"));

	DeleteSQLUISampleLayoutRepositoryRuntimeProviderFiles(Result);

	USQLUILayoutRepositoryRuntimeProvider* Provider =
		CreateSQLUISampleLayoutRepositoryRuntimeProvider(Outer);
	Result.bProviderCreated = IsValid(Provider);
	if (!Result.bProviderCreated)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			TEXT("SQLUI layout repository runtime provider probe failed: provider was not created."));
		return Result;
	}

	Result.bDefaultInitializationSucceeded =
		Provider->InitializeRepositoryFromRuntimeConfig(
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault());
	Result.bDefaultBackendInMemory =
		Provider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::InMemory;
	Result.bDefaultRepositoryAvailable = Provider->HasRepository();
	Result.bDefaultRepositoryNotSQLite =
		!IsValid(Cast<USQLUISQLiteLayoutRepository>(Provider->GetRepository()));
	const FSQLUILayoutDocument DefaultDocument =
		MakeSQLUISampleLayoutRepositoryRuntimeProviderDocument(
			TEXT("sqlui.smoke.runtime-provider.default"),
			TEXT("SQLUI Runtime Provider Default Layout"),
			TEXT("runtime-provider-default"));
	const FSQLUILayoutSaveResult DefaultSaveResult =
		SaveSQLUISampleLayoutToRepository(
			Provider->GetRepository(),
			TEXT("layout repository runtime provider default repository"),
			DefaultDocument);
	const FSQLUILayoutLoadResult DefaultLoadResult =
		LoadSQLUISampleLayoutFromRepository(
			Provider->GetRepository(),
			TEXT("layout repository runtime provider default repository"),
			DefaultDocument.Metadata.LayoutId);
	if (!Result.bDefaultInitializationSucceeded
		|| !Result.bDefaultBackendInMemory
		|| !Result.bDefaultRepositoryAvailable
		|| !Result.bDefaultRepositoryNotSQLite
		|| !DefaultSaveResult.bSucceeded
		|| !DefaultLoadResult.bSucceeded
		|| DefaultLoadResult.Document.Metadata.LayoutId != DefaultDocument.Metadata.LayoutId)
	{
		const FSQLUILayoutRepositoryRuntimeIntegrationResult DefaultIntegrationResult =
			Provider->GetLastIntegrationResult();
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			DefaultIntegrationResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime provider probe failed: default initialization did not create a usable in-memory repository.")
				: DefaultIntegrationResult.ErrorMessage);
	}

	Provider->ResetRepository();
	Result.bResetClearedRepository =
		!Provider->HasRepository()
		&& !Provider->WasLastInitializationSuccessful()
		&& Provider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::Unavailable;
	Result.bReinitializeAfterResetSucceeded =
		Provider->InitializeRepositoryFromRuntimeConfig(
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault())
		&& Provider->HasRepository()
		&& Provider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::InMemory;
	if (!Result.bResetClearedRepository || !Result.bReinitializeAfterResetSucceeded)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			TEXT("SQLUI layout repository runtime provider probe failed: reset/reinitialize did not behave as expected."));
	}

	const FSQLUILayoutDocument SQLiteDocument =
		MakeSQLUISampleLayoutRepositoryRuntimeProviderDocument(
			TEXT("sqlui.smoke.runtime-provider.sqlite"),
			TEXT("SQLUI Runtime Provider SQLite Layout"),
			TEXT("runtime-provider-sqlite"));
	FSQLUILayoutRepositoryRuntimeConfig SQLiteConfig;
	SQLiteConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	SQLiteConfig.SQLiteDatabasePath = Result.DatabasePath;
	SQLiteConfig.bSQLiteReadOnly = false;
	SQLiteConfig.bSQLiteInitializeSchemaIfMissing = true;
	SQLiteConfig.bSQLiteCreateDatabaseIfMissing = true;
	Result.bSQLiteInitializationSucceeded =
		Provider->InitializeRepositoryFromRuntimeConfig(SQLiteConfig);
	Result.bSQLiteBackendSelected =
		Provider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::SQLite;
	Result.bSQLiteRepositoryAvailable =
		Provider->HasRepository()
		&& IsValid(Cast<USQLUISQLiteLayoutRepository>(Provider->GetRepository()));
	if (!Result.bSQLiteInitializationSucceeded
		|| !Result.bSQLiteBackendSelected
		|| !Result.bSQLiteRepositoryAvailable)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			Provider->GetLastIntegrationResult().ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime provider probe failed: SQLite initialization did not create a SQLite repository.")
				: Provider->GetLastIntegrationResult().ErrorMessage);
	}

	if (Result.bSQLiteRepositoryAvailable)
	{
		const FSQLUILayoutSaveResult SaveResult =
			SaveSQLUISampleLayoutToRepository(
				Provider->GetRepository(),
				TEXT("layout repository runtime provider SQLite repository"),
				SQLiteDocument);
		Result.bSQLiteSaveSucceeded =
			SaveResult.bSucceeded
			&& SaveResult.SavedLayoutId == SQLiteDocument.Metadata.LayoutId
			&& SaveResult.Validation.bIsValid
			&& FPaths::FileExists(Result.DatabasePath);
		Result.bSQLiteListSucceeded =
			DoesSQLUISampleLayoutRepositoryRuntimeProviderListContain(
				Provider,
				SQLiteDocument,
				TEXT("SQLite repository"),
				Result);
		Result.bSQLiteLoadSucceeded =
			DoesSQLUISampleLayoutRepositoryRuntimeProviderLoadLayout(
				Provider,
				SQLiteDocument,
				TEXT("layout repository runtime provider SQLite repository"),
				Result);
		if (!Result.bSQLiteSaveSucceeded)
		{
			AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
				Result,
				SaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime provider probe failed: SQLite SaveLayout failed.")
					: SaveResult.ErrorMessage);
		}
	}

	FSQLUILayoutRepositoryRuntimeConfig MissingPathConfig;
	MissingPathConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	const bool bMissingPathInitialized =
		Provider->InitializeRepositoryFromRuntimeConfig(MissingPathConfig);
	const FSQLUILayoutRepositoryRuntimeIntegrationResult MissingPathResult =
		Provider->GetLastIntegrationResult();
	Result.bSQLiteMissingPathHandled =
		!bMissingPathInitialized
		&& MissingPathResult.bBackendUnavailable
		&& !IsValid(Cast<USQLUISQLiteLayoutRepository>(Provider->GetRepository()));
	Result.bSQLiteMissingPathDidNotCreateDb =
		!DoSQLUISampleLayoutRepositoryRuntimeProviderFilesExist(MissingPathMarkerDatabasePath);
	if (!Result.bSQLiteMissingPathHandled
		|| !Result.bSQLiteMissingPathDidNotCreateDb)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			MissingPathResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime provider probe failed: missing SQLite path did not surface unavailable behavior without creating DB files.")
				: MissingPathResult.ErrorMessage);
	}

	const FSQLUILayoutDocument CommandLineDocument =
		MakeSQLUISampleLayoutRepositoryRuntimeProviderDocument(
			TEXT("sqlui.smoke.runtime-provider.command-line"),
			TEXT("SQLUI Runtime Provider Command Line Layout"),
			TEXT("runtime-provider-command-line"));
	const FString CommandLine = FString::Printf(
		TEXT("-SQLUILayoutRepositoryBackend=SQLite -SQLUISQLiteLayoutRepositoryPath=\"%s\" -SQLUISQLiteLayoutRepositoryInitializeSchema -SQLUISQLiteLayoutRepositoryCreateDatabase"),
		*Result.CommandLineDatabasePath);
	Result.bCommandLineInitializationSucceeded =
		Provider->InitializeRepositoryFromCommandLine(
			CommandLine,
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault())
		&& Provider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::SQLite
		&& IsValid(Cast<USQLUISQLiteLayoutRepository>(Provider->GetRepository()));
	if (Result.bCommandLineInitializationSucceeded)
	{
		const FSQLUILayoutSaveResult CommandLineSaveResult =
			SaveSQLUISampleLayoutToRepository(
				Provider->GetRepository(),
				TEXT("layout repository runtime provider command-line SQLite repository"),
				CommandLineDocument);
		Result.bCommandLineSQLiteSaveSucceeded =
			CommandLineSaveResult.bSucceeded
			&& CommandLineSaveResult.SavedLayoutId == CommandLineDocument.Metadata.LayoutId
			&& CommandLineSaveResult.Validation.bIsValid
			&& DoesSQLUISampleLayoutRepositoryRuntimeProviderListContain(
				Provider,
				CommandLineDocument,
				TEXT("command-line SQLite repository"),
				Result)
			&& DoesSQLUISampleLayoutRepositoryRuntimeProviderLoadLayout(
				Provider,
				CommandLineDocument,
				TEXT("layout repository runtime provider command-line SQLite repository"),
				Result);
		if (!Result.bCommandLineSQLiteSaveSucceeded)
		{
			AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
				Result,
				CommandLineSaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime provider probe failed: command-line SQLite SaveLayout/ListLayouts/LoadLayout failed.")
					: CommandLineSaveResult.ErrorMessage);
		}
	}
	else
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			Provider->GetLastIntegrationResult().ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime provider probe failed: command-line SQLite initialization failed.")
				: Provider->GetLastIntegrationResult().ErrorMessage);
	}

	const FSQLUILayoutDocument SeedDocument =
		MakeSQLUISampleLayoutRepositoryRuntimeProviderDocument(
			TEXT("sqlui.smoke.runtime-provider.seed"),
			TEXT("SQLUI Runtime Provider Seed Layout"),
			TEXT("runtime-provider-seed"));
	Result.bSeedDatabasePrepared =
		PrepareSQLUISampleLayoutRepositoryRuntimeProviderSeedDatabase(
			Outer,
			Result.SeedDatabasePath,
			SeedDocument,
			Result);
	if (Result.bSeedDatabasePrepared)
	{
		FSQLUILayoutRepositoryRuntimeConfig SeedConfig;
		SeedConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
		SeedConfig.SQLiteDatabasePath = Result.SeedTargetDatabasePath;
		SeedConfig.SQLiteSeedDatabasePath = Result.SeedDatabasePath;
		SeedConfig.bSQLiteCopySeedIfMissing = true;
		SeedConfig.bSQLiteReadOnly = true;
		Result.bSeedCopyInitializationSucceeded =
			Provider->InitializeRepositoryFromRuntimeConfig(SeedConfig);
		const FSQLUILayoutRepositoryRuntimeIntegrationResult SeedCopyResult =
			Provider->GetLastIntegrationResult();
		Result.bSeedCopyRequested = SeedCopyResult.bSeedCopyRequested;
		Result.bSeedCopySucceeded = SeedCopyResult.bSeedCopySucceeded;
		Result.bSeedCopiedTargetReadable =
			Result.bSeedCopyInitializationSucceeded
			&& IsValid(Cast<USQLUISQLiteLayoutRepository>(Provider->GetRepository()))
			&& DoesSQLUISampleLayoutRepositoryRuntimeProviderListContain(
				Provider,
				SeedDocument,
				TEXT("seed-copied SQLite repository"),
				Result)
			&& DoesSQLUISampleLayoutRepositoryRuntimeProviderLoadLayout(
				Provider,
				SeedDocument,
				TEXT("layout repository runtime provider seed-copied repository"),
				Result);
		Result.bSeedDatabaseLeftIntact =
			LoadSQLUISampleLayoutRepositoryRuntimeProviderLayout(
				Outer,
				Result.SeedDatabasePath,
				SeedDocument,
				Result);
	}
	else
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			TEXT("SQLUI layout repository runtime provider probe failed: seed database was not prepared."));
	}

	FSQLUILayoutRepositoryRuntimeIntegrationRequest MissingSeedRequest;
	MissingSeedRequest.RuntimeConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	MissingSeedRequest.RuntimeConfig.SQLiteDatabasePath = MissingSeedTargetPath;
	MissingSeedRequest.RuntimeConfig.SQLiteSeedDatabasePath = MissingSeedPath;
	MissingSeedRequest.RuntimeConfig.bSQLiteCopySeedIfMissing = true;
	MissingSeedRequest.bTreatSeedCopyFailureAsFatal = true;
	const bool bMissingSeedInitialized = Provider->InitializeRepository(MissingSeedRequest);
	const FSQLUILayoutRepositoryRuntimeIntegrationResult MissingSeedResult =
		Provider->GetLastIntegrationResult();
	Result.bSeedCopyFailureFatal =
		!bMissingSeedInitialized
		&& !MissingSeedResult.bSucceeded
		&& MissingSeedResult.bSeedCopyRequested
		&& !MissingSeedResult.bSeedCopySucceeded
		&& MissingSeedResult.ErrorMessage.Contains(TEXT("does not exist"));
	Result.bSeedCopyFailureDidNotCreateRepository =
		!Provider->HasRepository()
		&& !MissingSeedResult.bRepositoryCreated;
	Result.bSeedCopyFailureDidNotCreateTarget =
		!DoSQLUISampleLayoutRepositoryRuntimeProviderFilesExist(MissingSeedTargetPath);
	if (!Result.bSeedCopyFailureFatal
		|| !Result.bSeedCopyFailureDidNotCreateRepository
		|| !Result.bSeedCopyFailureDidNotCreateTarget)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			MissingSeedResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository runtime provider probe failed: missing seed did not fail before repository creation without creating target.")
				: MissingSeedResult.ErrorMessage);
	}

	Provider->ResetRepository();
	Result.bDatabaseFilesRemoved =
		DeleteSQLUISampleLayoutRepositoryRuntimeProviderFiles(Result)
		&& !DoesAnySQLUISampleLayoutRepositoryRuntimeProviderFileExist();
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeProviderProbeError(
			Result,
			TEXT("SQLUI layout repository runtime provider probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bProviderCreated
		&& Result.bDefaultInitializationSucceeded
		&& Result.bDefaultBackendInMemory
		&& Result.bDefaultRepositoryAvailable
		&& Result.bDefaultRepositoryNotSQLite
		&& Result.bResetClearedRepository
		&& Result.bReinitializeAfterResetSucceeded
		&& Result.bSQLiteInitializationSucceeded
		&& Result.bSQLiteBackendSelected
		&& Result.bSQLiteRepositoryAvailable
		&& Result.bSQLiteSaveSucceeded
		&& Result.bSQLiteListSucceeded
		&& Result.bSQLiteLoadSucceeded
		&& Result.bSQLiteMissingPathHandled
		&& Result.bSQLiteMissingPathDidNotCreateDb
		&& Result.bCommandLineInitializationSucceeded
		&& Result.bCommandLineSQLiteSaveSucceeded
		&& Result.bSeedDatabasePrepared
		&& Result.bSeedCopyInitializationSucceeded
		&& Result.bSeedCopyRequested
		&& Result.bSeedCopySucceeded
		&& Result.bSeedCopiedTargetReadable
		&& Result.bSeedDatabaseLeftIntact
		&& Result.bSeedCopyFailureFatal
		&& Result.bSeedCopyFailureDidNotCreateRepository
		&& Result.bSeedCopyFailureDidNotCreateTarget
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
	FSQLUISampleLayoutRepositoryRuntimeSettingsProbeResult& Result,
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

FString MakeSQLUISampleLayoutRepositoryRuntimeSettingsPath(const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutRepositoryRuntimeSettings"),
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISampleLayoutRepositoryRuntimeSettingsDatabasePaths()
{
	return {
		MakeSQLUISampleLayoutRepositoryRuntimeSettingsPath(TEXT("RuntimeSettings.db")),
		MakeSQLUISampleLayoutRepositoryRuntimeSettingsPath(TEXT("CommandLineOverrideRuntimeSettings.db")),
		MakeSQLUISampleLayoutRepositoryRuntimeSettingsPath(TEXT("MissingPathShouldNotExist.db"))
	};
}

bool DoSQLUISampleLayoutRepositoryRuntimeSettingsFilesExist(const FString& DatabasePath)
{
	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleLayoutRepositoryRuntimeSettingsFiles(
	const FString& DatabasePath,
	FSQLUISampleLayoutRepositoryRuntimeSettingsProbeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI layout repository runtime settings probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISampleLayoutRepositoryRuntimeSettingsFiles(
	FSQLUISampleLayoutRepositoryRuntimeSettingsProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& DatabasePath : MakeSQLUISampleLayoutRepositoryRuntimeSettingsDatabasePaths())
	{
		bRemoved =
			DeleteSQLUISampleLayoutRepositoryRuntimeSettingsFiles(DatabasePath, Result)
			&& bRemoved;
	}

	return bRemoved;
}

bool DoesAnySQLUISampleLayoutRepositoryRuntimeSettingsFileExist()
{
	for (const FString& DatabasePath : MakeSQLUISampleLayoutRepositoryRuntimeSettingsDatabasePaths())
	{
		if (DoSQLUISampleLayoutRepositoryRuntimeSettingsFilesExist(DatabasePath))
		{
			return true;
		}
	}

	return false;
}

USQLUILayoutRepositoryRuntimeSettings* CreateSQLUISampleLayoutRepositoryRuntimeSettings(UObject* Outer)
{
	return NewObject<USQLUILayoutRepositoryRuntimeSettings>(
		IsValid(Outer) ? Outer : GetTransientPackage());
}

FSQLUILayoutDocument MakeSQLUISampleLayoutRepositoryRuntimeSettingsDocument(
	const TCHAR* LayoutId,
	const TCHAR* DisplayName,
	const TCHAR* ProbeTag)
{
	FSQLUILayoutDocument Document =
		MakeSQLUISampleLayoutRepositoryRuntimeProviderDocument(LayoutId, DisplayName, ProbeTag);
	Document.Metadata.Description =
		TEXT("Smoke/probe layout for runtime repository settings policy.");
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutRepositoryRuntimeSettings"));

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutRepositoryRuntimeSettings"));
	}

	return Document;
}

bool DoesSQLUISampleLayoutRepositoryRuntimeSettingsListContain(
	USQLUILayoutRepositoryRuntimeProvider* Provider,
	const FSQLUILayoutDocument& Document,
	const TCHAR* RepositoryName,
	FSQLUISampleLayoutRepositoryRuntimeSettingsProbeResult& Result)
{
	USQLUISQLiteLayoutRepository* SQLiteRepository =
		IsValid(Provider) ? Cast<USQLUISQLiteLayoutRepository>(Provider->GetRepository()) : nullptr;
	if (!IsValid(SQLiteRepository))
	{
		AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI layout repository runtime settings probe failed: %s was not SQLite."),
				RepositoryName));
		return false;
	}

	const FSQLUILayoutRepositoryListResult ListResult = SQLiteRepository->ListLayouts();
	if (ListResult.bSucceeded
		&& DoesSQLUISampleLayoutMetadataAndTagsListContain(
			ListResult.Layouts,
			Document.Metadata))
	{
		return true;
	}

	AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
		Result,
		ListResult.ErrorMessage.IsEmpty()
			? FString::Printf(
				TEXT("SQLUI layout repository runtime settings probe failed: %s did not list expected metadata and tags."),
				RepositoryName)
			: ListResult.ErrorMessage);
	return false;
}

bool DoesSQLUISampleLayoutRepositoryRuntimeSettingsLoadLayout(
	USQLUILayoutRepositoryRuntimeProvider* Provider,
	const FSQLUILayoutDocument& Document,
	const TCHAR* RepositoryName,
	FSQLUISampleLayoutRepositoryRuntimeSettingsProbeResult& Result)
{
	const FSQLUILayoutLoadResult LoadResult =
		LoadSQLUISampleLayoutFromRepository(
			IsValid(Provider) ? Provider->GetRepository() : nullptr,
			RepositoryName,
			Document.Metadata.LayoutId);
	if (LoadResult.bSucceeded
		&& LoadResult.Validation.bIsValid
		&& LoadResult.Document.Metadata.LayoutId == Document.Metadata.LayoutId)
	{
		return true;
	}

	AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
		Result,
		LoadResult.ErrorMessage.IsEmpty()
			? FString::Printf(
				TEXT("SQLUI layout repository runtime settings probe failed: %s did not load expected layout."),
				RepositoryName)
			: LoadResult.ErrorMessage);
	return false;
}

bool DoesSQLUISampleLayoutRepositoryRuntimeSettingsSaveListLoad(
	USQLUILayoutRepositoryRuntimeProvider* Provider,
	const FSQLUILayoutDocument& Document,
	const TCHAR* RepositoryName,
	FSQLUISampleLayoutRepositoryRuntimeSettingsProbeResult& Result,
	bool& bOutSaveSucceeded,
	bool& bOutListSucceeded,
	bool& bOutLoadSucceeded)
{
	const FSQLUILayoutSaveResult SaveResult =
		SaveSQLUISampleLayoutToRepository(
			IsValid(Provider) ? Provider->GetRepository() : nullptr,
			RepositoryName,
			Document);
	bOutSaveSucceeded =
		SaveResult.bSucceeded
		&& SaveResult.SavedLayoutId == Document.Metadata.LayoutId
		&& SaveResult.Validation.bIsValid;
	if (!bOutSaveSucceeded)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
			Result,
			SaveResult.ErrorMessage.IsEmpty()
				? FString::Printf(
					TEXT("SQLUI layout repository runtime settings probe failed: %s SaveLayout failed."),
					RepositoryName)
				: SaveResult.ErrorMessage);
	}

	bOutListSucceeded =
		DoesSQLUISampleLayoutRepositoryRuntimeSettingsListContain(
			Provider,
			Document,
			RepositoryName,
			Result);
	bOutLoadSucceeded =
		DoesSQLUISampleLayoutRepositoryRuntimeSettingsLoadLayout(
			Provider,
			Document,
			RepositoryName,
			Result);

	return bOutSaveSucceeded && bOutListSucceeded && bOutLoadSucceeded;
}

FSQLUISampleLayoutRepositoryRuntimeSettingsProbeResult
RunSQLUISampleLayoutRepositoryRuntimeSettingsProbe(UObject* Outer)
{
	FSQLUISampleLayoutRepositoryRuntimeSettingsProbeResult Result;
	Result.DatabasePath =
		MakeSQLUISampleLayoutRepositoryRuntimeSettingsPath(TEXT("RuntimeSettings.db"));
	Result.CommandLineDatabasePath =
		MakeSQLUISampleLayoutRepositoryRuntimeSettingsPath(
			TEXT("CommandLineOverrideRuntimeSettings.db"));
	Result.MissingPathMarkerDatabasePath =
		MakeSQLUISampleLayoutRepositoryRuntimeSettingsPath(
			TEXT("MissingPathShouldNotExist.db"));

	DeleteSQLUISampleLayoutRepositoryRuntimeSettingsFiles(Result);

	const USQLUILayoutRepositoryRuntimeSettings* DefaultSettings =
		GetDefault<USQLUILayoutRepositoryRuntimeSettings>();
	const FSQLUILayoutRepositoryRuntimeIntegrationRequest DefaultRequest =
		FSQLUILayoutRepositoryRuntimeSettingsPolicy::
			MakeRuntimeIntegrationRequestFromSettingsAndCommandLine(
				DefaultSettings,
				TEXT(""));
	Result.bDefaultSettingsSafe =
		DefaultSettings
		&& !DefaultSettings->bAutoInitializeProvider
		&& DefaultSettings->bRunSQLiteSeedCopyPolicy
		&& DefaultSettings->bTreatSeedCopyFailureAsFatal
		&& DefaultSettings->bAllowCommandLineOverrides
		&& DefaultSettings->RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::InMemory
		&& DefaultSettings->RuntimeConfig.SQLiteDatabasePath.IsEmpty()
		&& !DefaultSettings->RuntimeConfig.bSQLiteInitializeSchemaIfMissing
		&& !DefaultSettings->RuntimeConfig.bSQLiteCreateDatabaseIfMissing
		&& !DefaultSettings->RuntimeConfig.bSQLiteRunCallbackOperationsAsync
		&& !DefaultSettings->RuntimeConfig.bSQLiteCopySeedIfMissing
		&& !DefaultSettings->RuntimeConfig.bSQLiteOverwriteDatabaseFromSeed
		&& DefaultRequest.RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::InMemory
		&& DefaultRequest.RuntimeConfig.SQLiteDatabasePath.IsEmpty()
		&& DefaultRequest.bRunSQLiteSeedCopyPolicy
		&& DefaultRequest.bTreatSeedCopyFailureAsFatal;
	Result.bDefaultDoesNotAutoInitialize =
		!FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
			DefaultSettings,
			TEXT(""));
	if (!Result.bDefaultSettingsSafe || !Result.bDefaultDoesNotAutoInitialize)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
			Result,
			TEXT("SQLUI layout repository runtime settings probe failed: default settings were not passive/in-memory safe."));
	}

	USQLUILayoutRepositoryRuntimeSettings* InMemorySettings =
		CreateSQLUISampleLayoutRepositoryRuntimeSettings(Outer);
	if (IsValid(InMemorySettings))
	{
		InMemorySettings->bAutoInitializeProvider = true;
		InMemorySettings->RuntimeConfig =
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
		const FSQLUILayoutRepositoryRuntimeIntegrationRequest InMemoryRequest =
			FSQLUILayoutRepositoryRuntimeSettingsPolicy::
				MakeRuntimeIntegrationRequestFromSettingsAndCommandLine(
					InMemorySettings,
					TEXT(""));
		Result.bSettingsInMemoryAutoInitResolved =
			FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
				InMemorySettings,
				TEXT(""))
			&& InMemoryRequest.RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::InMemory;

		USQLUILayoutRepositoryRuntimeProvider* InMemoryProvider =
			CreateSQLUISampleLayoutRepositoryRuntimeProvider(Outer);
		const bool bInMemoryInitialized =
			IsValid(InMemoryProvider)
			&& InMemoryProvider->InitializeRepository(InMemoryRequest);
		Result.bSettingsInMemoryRepositoryCreated =
			bInMemoryInitialized
			&& InMemoryProvider->HasRepository()
			&& InMemoryProvider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::InMemory;
		Result.bSettingsInMemoryRepositoryNotSQLite =
			IsValid(InMemoryProvider)
			&& !IsValid(Cast<USQLUISQLiteLayoutRepository>(InMemoryProvider->GetRepository()));
		if (IsValid(InMemoryProvider))
		{
			InMemoryProvider->ResetRepository();
		}
	}
	if (!Result.bSettingsInMemoryAutoInitResolved
		|| !Result.bSettingsInMemoryRepositoryCreated
		|| !Result.bSettingsInMemoryRepositoryNotSQLite)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
			Result,
			TEXT("SQLUI layout repository runtime settings probe failed: settings-driven InMemory auto-init did not resolve/create correctly."));
	}

	USQLUILayoutRepositoryRuntimeSettings* SQLiteSettings =
		CreateSQLUISampleLayoutRepositoryRuntimeSettings(Outer);
	if (IsValid(SQLiteSettings))
	{
		SQLiteSettings->bAutoInitializeProvider = true;
		SQLiteSettings->RuntimeConfig =
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
		SQLiteSettings->RuntimeConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
		SQLiteSettings->RuntimeConfig.SQLiteDatabasePath = Result.DatabasePath;
		SQLiteSettings->RuntimeConfig.bSQLiteReadOnly = false;
		SQLiteSettings->RuntimeConfig.bSQLiteInitializeSchemaIfMissing = true;
		SQLiteSettings->RuntimeConfig.bSQLiteCreateDatabaseIfMissing = true;
		const FSQLUILayoutRepositoryRuntimeIntegrationRequest SQLiteRequest =
			FSQLUILayoutRepositoryRuntimeSettingsPolicy::
				MakeRuntimeIntegrationRequestFromSettingsAndCommandLine(
					SQLiteSettings,
					TEXT(""));
		Result.bSettingsSQLiteResolved =
			FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
				SQLiteSettings,
				TEXT(""))
			&& SQLiteRequest.RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::SQLite
			&& SQLiteRequest.RuntimeConfig.SQLiteDatabasePath == Result.DatabasePath
			&& SQLiteRequest.RuntimeConfig.bSQLiteInitializeSchemaIfMissing
			&& SQLiteRequest.RuntimeConfig.bSQLiteCreateDatabaseIfMissing;

		USQLUILayoutRepositoryRuntimeProvider* SQLiteProvider =
			CreateSQLUISampleLayoutRepositoryRuntimeProvider(Outer);
		Result.bSettingsSQLiteRepositoryCreated =
			IsValid(SQLiteProvider)
			&& SQLiteProvider->InitializeRepository(SQLiteRequest)
			&& SQLiteProvider->HasRepository()
			&& SQLiteProvider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::SQLite
			&& IsValid(Cast<USQLUISQLiteLayoutRepository>(SQLiteProvider->GetRepository()));
		if (Result.bSettingsSQLiteRepositoryCreated)
		{
			const FSQLUILayoutDocument SQLiteDocument =
				MakeSQLUISampleLayoutRepositoryRuntimeSettingsDocument(
					TEXT("sqlui.smoke.runtime-settings.sqlite"),
					TEXT("SQLUI Runtime Settings SQLite Layout"),
					TEXT("runtime-settings-sqlite"));
			DoesSQLUISampleLayoutRepositoryRuntimeSettingsSaveListLoad(
				SQLiteProvider,
				SQLiteDocument,
				TEXT("layout repository runtime settings SQLite repository"),
				Result,
				Result.bSettingsSQLiteSaveSucceeded,
				Result.bSettingsSQLiteListSucceeded,
				Result.bSettingsSQLiteLoadSucceeded);
			Result.bSettingsSQLiteSaveSucceeded =
				Result.bSettingsSQLiteSaveSucceeded
				&& FPaths::FileExists(Result.DatabasePath);
		}
		if (IsValid(SQLiteProvider))
		{
			SQLiteProvider->ResetRepository();
		}
	}
	if (!Result.bSettingsSQLiteResolved
		|| !Result.bSettingsSQLiteRepositoryCreated
		|| !Result.bSettingsSQLiteSaveSucceeded
		|| !Result.bSettingsSQLiteListSucceeded
		|| !Result.bSettingsSQLiteLoadSucceeded)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
			Result,
			TEXT("SQLUI layout repository runtime settings probe failed: settings-driven SQLite did not resolve/create/save/list/load correctly."));
	}

	USQLUILayoutRepositoryRuntimeSettings* OverrideSettings =
		CreateSQLUISampleLayoutRepositoryRuntimeSettings(Outer);
	if (IsValid(OverrideSettings))
	{
		OverrideSettings->bAutoInitializeProvider = false;
		OverrideSettings->bAllowCommandLineOverrides = true;
		OverrideSettings->RuntimeConfig =
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
		const FString OverrideCommandLine = FString::Printf(
			TEXT("-SQLUILayoutRepositoryProviderAutoInit -SQLUILayoutRepositoryBackend=SQLite -SQLUISQLiteLayoutRepositoryPath=\"%s\" -SQLUISQLiteLayoutRepositoryInitializeSchema -SQLUISQLiteLayoutRepositoryCreateDatabase"),
			*Result.CommandLineDatabasePath);
		const FSQLUILayoutRepositoryRuntimeIntegrationRequest OverrideRequest =
			FSQLUILayoutRepositoryRuntimeSettingsPolicy::
				MakeRuntimeIntegrationRequestFromSettingsAndCommandLine(
					OverrideSettings,
					*OverrideCommandLine);
		Result.bCommandLineOverrideResolvedSQLite =
			FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
				OverrideSettings,
				*OverrideCommandLine)
			&& OverrideRequest.RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::SQLite
			&& OverrideRequest.RuntimeConfig.SQLiteDatabasePath == Result.CommandLineDatabasePath
			&& OverrideRequest.RuntimeConfig.bSQLiteInitializeSchemaIfMissing
			&& OverrideRequest.RuntimeConfig.bSQLiteCreateDatabaseIfMissing;

		USQLUILayoutRepositoryRuntimeProvider* OverrideProvider =
			CreateSQLUISampleLayoutRepositoryRuntimeProvider(Outer);
		const bool bOverrideRepositoryCreated =
			IsValid(OverrideProvider)
			&& OverrideProvider->InitializeRepository(OverrideRequest)
			&& IsValid(Cast<USQLUISQLiteLayoutRepository>(OverrideProvider->GetRepository()));
		bool bOverrideListSucceeded = false;
		bool bOverrideLoadSucceeded = false;
		if (bOverrideRepositoryCreated)
		{
			const FSQLUILayoutDocument OverrideDocument =
				MakeSQLUISampleLayoutRepositoryRuntimeSettingsDocument(
					TEXT("sqlui.smoke.runtime-settings.command-line"),
					TEXT("SQLUI Runtime Settings Command-Line Override Layout"),
					TEXT("runtime-settings-command-line"));
			DoesSQLUISampleLayoutRepositoryRuntimeSettingsSaveListLoad(
				OverrideProvider,
				OverrideDocument,
				TEXT("layout repository runtime settings command-line SQLite repository"),
				Result,
				Result.bCommandLineOverrideSaveSucceeded,
				bOverrideListSucceeded,
				bOverrideLoadSucceeded);
			Result.bCommandLineOverrideSaveSucceeded =
				Result.bCommandLineOverrideSaveSucceeded
				&& bOverrideListSucceeded
				&& bOverrideLoadSucceeded
				&& FPaths::FileExists(Result.CommandLineDatabasePath);
		}
		if (IsValid(OverrideProvider))
		{
			OverrideProvider->ResetRepository();
		}
	}
	if (!Result.bCommandLineOverrideResolvedSQLite
		|| !Result.bCommandLineOverrideSaveSucceeded)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
			Result,
			TEXT("SQLUI layout repository runtime settings probe failed: command-line override did not select usable SQLite."));
	}

	USQLUILayoutRepositoryRuntimeSettings* DisabledOverrideSettings =
		CreateSQLUISampleLayoutRepositoryRuntimeSettings(Outer);
	if (IsValid(DisabledOverrideSettings))
	{
		DisabledOverrideSettings->bAutoInitializeProvider = false;
		DisabledOverrideSettings->bAllowCommandLineOverrides = false;
		DisabledOverrideSettings->RuntimeConfig =
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
		const FString DisabledOverrideCommandLine = FString::Printf(
			TEXT("-SQLUILayoutRepositoryProviderAutoInit -SQLUILayoutRepositoryBackend=SQLite -SQLUISQLiteLayoutRepositoryPath=\"%s\" -SQLUISQLiteLayoutRepositoryInitializeSchema -SQLUISQLiteLayoutRepositoryCreateDatabase"),
			*Result.MissingPathMarkerDatabasePath);
		const FSQLUILayoutRepositoryRuntimeIntegrationRequest DisabledOverrideRequest =
			FSQLUILayoutRepositoryRuntimeSettingsPolicy::
				MakeRuntimeIntegrationRequestFromSettingsAndCommandLine(
					DisabledOverrideSettings,
					*DisabledOverrideCommandLine);
		Result.bCommandLineOverrideDisabledPreservedSettings =
			FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
				DisabledOverrideSettings,
				*DisabledOverrideCommandLine)
			&& DisabledOverrideRequest.RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::InMemory
			&& DisabledOverrideRequest.RuntimeConfig.SQLiteDatabasePath.IsEmpty();

		USQLUILayoutRepositoryRuntimeProvider* DisabledOverrideProvider =
			CreateSQLUISampleLayoutRepositoryRuntimeProvider(Outer);
		const bool bDisabledOverrideInitialized =
			IsValid(DisabledOverrideProvider)
			&& DisabledOverrideProvider->InitializeRepository(DisabledOverrideRequest)
			&& DisabledOverrideProvider->HasRepository()
			&& DisabledOverrideProvider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::InMemory;
		Result.bCommandLineOverrideDisabledPreservedSettings =
			Result.bCommandLineOverrideDisabledPreservedSettings
			&& bDisabledOverrideInitialized;
		Result.bCommandLineOverrideDisabledDidNotCreateDb =
			!DoSQLUISampleLayoutRepositoryRuntimeSettingsFilesExist(
				Result.MissingPathMarkerDatabasePath);
		if (IsValid(DisabledOverrideProvider))
		{
			DisabledOverrideProvider->ResetRepository();
		}
	}
	if (!Result.bCommandLineOverrideDisabledPreservedSettings
		|| !Result.bCommandLineOverrideDisabledDidNotCreateDb)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
			Result,
			TEXT("SQLUI layout repository runtime settings probe failed: disabled command-line override did not preserve InMemory settings without creating DB files."));
	}

	USQLUILayoutRepositoryRuntimeSettings* MissingPathSettings =
		CreateSQLUISampleLayoutRepositoryRuntimeSettings(Outer);
	if (IsValid(MissingPathSettings))
	{
		MissingPathSettings->bAutoInitializeProvider = true;
		MissingPathSettings->RuntimeConfig =
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
		MissingPathSettings->RuntimeConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
		MissingPathSettings->RuntimeConfig.SQLiteDatabasePath.Empty();
		MissingPathSettings->RuntimeConfig.bSQLiteReadOnly = false;
		MissingPathSettings->RuntimeConfig.bSQLiteInitializeSchemaIfMissing = true;
		MissingPathSettings->RuntimeConfig.bSQLiteCreateDatabaseIfMissing = true;
		const FSQLUILayoutRepositoryRuntimeIntegrationRequest MissingPathRequest =
			FSQLUILayoutRepositoryRuntimeSettingsPolicy::
				MakeRuntimeIntegrationRequestFromSettingsAndCommandLine(
					MissingPathSettings,
					TEXT(""));
		USQLUILayoutRepositoryRuntimeProvider* MissingPathProvider =
			CreateSQLUISampleLayoutRepositoryRuntimeProvider(Outer);
		const bool bMissingPathInitialized =
			IsValid(MissingPathProvider)
			&& MissingPathProvider->InitializeRepository(MissingPathRequest);
		const FSQLUILayoutRepositoryRuntimeIntegrationResult MissingPathResult =
			IsValid(MissingPathProvider)
				? MissingPathProvider->GetLastIntegrationResult()
				: FSQLUILayoutRepositoryRuntimeIntegrationResult();
		Result.bSQLiteMissingPathUnavailable =
			!bMissingPathInitialized
			&& MissingPathRequest.RuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::SQLite
			&& MissingPathRequest.RuntimeConfig.SQLiteDatabasePath.IsEmpty()
			&& MissingPathResult.bBackendUnavailable
			&& IsValid(MissingPathProvider)
			&& !IsValid(Cast<USQLUISQLiteLayoutRepository>(MissingPathProvider->GetRepository()));
		Result.bSQLiteMissingPathDidNotCreateDb =
			!DoSQLUISampleLayoutRepositoryRuntimeSettingsFilesExist(
				Result.MissingPathMarkerDatabasePath);
		if (IsValid(MissingPathProvider))
		{
			MissingPathProvider->ResetRepository();
		}
	}
	if (!Result.bSQLiteMissingPathUnavailable
		|| !Result.bSQLiteMissingPathDidNotCreateDb)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
			Result,
			TEXT("SQLUI layout repository runtime settings probe failed: explicit SQLite missing path did not stay unavailable without creating DB files."));
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISampleLayoutRepositoryRuntimeSettingsFiles(Result)
		&& !DoesAnySQLUISampleLayoutRepositoryRuntimeSettingsFileExist();
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISampleLayoutRepositoryRuntimeSettingsProbeError(
			Result,
			TEXT("SQLUI layout repository runtime settings probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bDefaultSettingsSafe
		&& Result.bDefaultDoesNotAutoInitialize
		&& Result.bSettingsInMemoryAutoInitResolved
		&& Result.bSettingsInMemoryRepositoryCreated
		&& Result.bSettingsInMemoryRepositoryNotSQLite
		&& Result.bSettingsSQLiteResolved
		&& Result.bSettingsSQLiteRepositoryCreated
		&& Result.bSettingsSQLiteSaveSucceeded
		&& Result.bSettingsSQLiteListSucceeded
		&& Result.bSettingsSQLiteLoadSucceeded
		&& Result.bCommandLineOverrideResolvedSQLite
		&& Result.bCommandLineOverrideSaveSucceeded
		&& Result.bCommandLineOverrideDisabledPreservedSettings
		&& Result.bCommandLineOverrideDisabledDidNotCreateDb
		&& Result.bSQLiteMissingPathUnavailable
		&& Result.bSQLiteMissingPathDidNotCreateDb
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISampleLayoutPersistenceWorkflowProbeError(
	FSQLUISampleLayoutPersistenceWorkflowProbeResult& Result,
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

FString MakeSQLUISampleLayoutPersistenceWorkflowPath(const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutPersistenceWorkflow"),
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISampleLayoutPersistenceWorkflowDatabasePaths(
	const FSQLUISampleLayoutPersistenceWorkflowProbeResult& Result)
{
	TArray<FString> DatabasePaths;
	DatabasePaths.Add(Result.DatabasePath);
	DatabasePaths.Add(Result.MissingRepositoryMarkerDatabasePath);
	return DatabasePaths;
}

bool DoSQLUISampleLayoutPersistenceWorkflowFilesExist(const FString& DatabasePath)
{
	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleLayoutPersistenceWorkflowFiles(
	const FString& DatabasePath,
	FSQLUISampleLayoutPersistenceWorkflowProbeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleLayoutPersistenceWorkflowProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI layout persistence workflow probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISampleLayoutPersistenceWorkflowFiles(
	FSQLUISampleLayoutPersistenceWorkflowProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& DatabasePath :
		MakeSQLUISampleLayoutPersistenceWorkflowDatabasePaths(Result))
	{
		bRemoved =
			DeleteSQLUISampleLayoutPersistenceWorkflowFiles(DatabasePath, Result)
			&& bRemoved;
	}

	return bRemoved;
}

bool DoesAnySQLUISampleLayoutPersistenceWorkflowFileExist(
	const FSQLUISampleLayoutPersistenceWorkflowProbeResult& Result)
{
	for (const FString& DatabasePath :
		MakeSQLUISampleLayoutPersistenceWorkflowDatabasePaths(Result))
	{
		if (DoSQLUISampleLayoutPersistenceWorkflowFilesExist(DatabasePath))
		{
			return true;
		}
	}

	return false;
}

UGameInstance* CreateSQLUISampleLayoutPersistenceWorkflowGameInstance(UObject* Outer)
{
	return NewObject<UGameInstance>(
		IsValid(Outer) ? Outer : GetTransientPackage());
}

USQLUILayoutRepositoryRuntimeSubsystem*
CreateSQLUISampleLayoutPersistenceWorkflowSubsystem(UGameInstance* GameInstance)
{
	return IsValid(GameInstance)
		? NewObject<USQLUILayoutRepositoryRuntimeSubsystem>(GameInstance)
		: nullptr;
}

FSQLUILayoutDocument MakeSQLUISampleLayoutPersistenceWorkflowDocument(
	const TCHAR* LayoutId,
	const TCHAR* DisplayName,
	const TCHAR* ProbeTag)
{
	FSQLUILayoutDocument Document =
		MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument();
	Document.Version.Label = DisplayName;
	Document.Metadata.LayoutId = LayoutId;
	Document.Metadata.DisplayName = DisplayName;
	Document.Metadata.Description =
		TEXT("Smoke/probe layout for runtime persistence workflow.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-06-05T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = Document.Metadata.CreatedAtUtc;
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("runtime"));
	Document.Metadata.Tags.Add(TEXT("workflow"));
	Document.Metadata.Tags.Add(ProbeTag);
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutPersistenceWorkflow"));
	Document.RootWidgetId = FString::Printf(TEXT("%s.Root"), LayoutId);

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("runtime"));
		Document.Nodes[0].Tags.Add(ProbeTag);
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("LayoutPersistenceWorkflow"));
	}

	return Document;
}

FSQLUISampleLayoutPersistenceWorkflowProbeResult
RunSQLUISampleLayoutPersistenceWorkflowProbe(UObject* Outer)
{
	FSQLUISampleLayoutPersistenceWorkflowProbeResult Result;
	Result.DatabasePath =
		MakeSQLUISampleLayoutPersistenceWorkflowPath(
			TEXT("LayoutPersistenceWorkflow.db"));
	Result.MissingRepositoryMarkerDatabasePath =
		MakeSQLUISampleLayoutPersistenceWorkflowPath(
			TEXT("MissingRepositoryShouldNotExist.db"));

	DeleteSQLUISampleLayoutPersistenceWorkflowFiles(Result);

	UGameInstance* WorkflowGameInstance =
		CreateSQLUISampleLayoutPersistenceWorkflowGameInstance(Outer);
	if (!IsValid(WorkflowGameInstance))
	{
		AppendSQLUISampleLayoutPersistenceWorkflowProbeError(
			Result,
			TEXT("SQLUI layout persistence workflow probe failed: could not create transient GameInstance for subsystem outer."));
	}

	const FSQLUILayoutDocument NullSubsystemDocument =
		MakeSQLUISampleLayoutPersistenceWorkflowDocument(
			TEXT("sqlui.smoke.persistence-workflow.null-subsystem"),
			TEXT("SQLUI Persistence Workflow Null Subsystem Layout"),
			TEXT("persistence-workflow-null-subsystem"));
	const FSQLUILayoutSaveResult NullSubsystemSaveResult =
		FSQLUILayoutPersistenceWorkflow::SaveLayout(nullptr, NullSubsystemDocument);
	const FSQLUILayoutRepositoryListResult NullSubsystemListResult =
		FSQLUILayoutPersistenceWorkflow::ListLayouts(nullptr);
	const FSQLUILayoutLoadResult NullSubsystemLoadResult =
		FSQLUILayoutPersistenceWorkflow::LoadLayout(
			nullptr,
			NullSubsystemDocument.Metadata.LayoutId);
	Result.bNullSubsystemSaveFailed =
		!NullSubsystemSaveResult.bSucceeded
		&& NullSubsystemSaveResult.bBackendUnavailable
		&& !NullSubsystemSaveResult.ErrorMessage.IsEmpty();
	Result.bNullSubsystemListFailed =
		!NullSubsystemListResult.bSucceeded
		&& !NullSubsystemListResult.ErrorMessage.IsEmpty();
	Result.bNullSubsystemLoadFailed =
		!NullSubsystemLoadResult.bSucceeded
		&& NullSubsystemLoadResult.bBackendUnavailable
		&& !NullSubsystemLoadResult.ErrorMessage.IsEmpty();
	if (!Result.bNullSubsystemSaveFailed
		|| !Result.bNullSubsystemListFailed
		|| !Result.bNullSubsystemLoadFailed)
	{
		AppendSQLUISampleLayoutPersistenceWorkflowProbeError(
			Result,
			TEXT("SQLUI layout persistence workflow probe failed: null subsystem did not fail clearly."));
	}

	USQLUILayoutRepositoryRuntimeSubsystem* MissingRepositorySubsystem =
		CreateSQLUISampleLayoutPersistenceWorkflowSubsystem(WorkflowGameInstance);
	const FSQLUILayoutDocument MissingRepositoryDocument =
		MakeSQLUISampleLayoutPersistenceWorkflowDocument(
			TEXT("sqlui.smoke.persistence-workflow.missing-repository"),
			TEXT("SQLUI Persistence Workflow Missing Repository Layout"),
			TEXT("persistence-workflow-missing-repository"));
	const FSQLUILayoutSaveResult MissingRepositorySaveResult =
		FSQLUILayoutPersistenceWorkflow::SaveLayout(
			MissingRepositorySubsystem,
			MissingRepositoryDocument);
	const FSQLUILayoutRepositoryListResult MissingRepositoryListResult =
		FSQLUILayoutPersistenceWorkflow::ListLayouts(MissingRepositorySubsystem);
	const FSQLUILayoutLoadResult MissingRepositoryLoadResult =
		FSQLUILayoutPersistenceWorkflow::LoadLayout(
			MissingRepositorySubsystem,
			MissingRepositoryDocument.Metadata.LayoutId);
	Result.bMissingRepositorySaveFailed =
		IsValid(MissingRepositorySubsystem)
		&& !FSQLUILayoutPersistenceWorkflow::HasRepository(MissingRepositorySubsystem)
		&& !MissingRepositorySaveResult.bSucceeded
		&& MissingRepositorySaveResult.bBackendUnavailable
		&& !MissingRepositorySaveResult.ErrorMessage.IsEmpty();
	Result.bMissingRepositoryListFailed =
		!MissingRepositoryListResult.bSucceeded
		&& !MissingRepositoryListResult.ErrorMessage.IsEmpty();
	Result.bMissingRepositoryLoadFailed =
		!MissingRepositoryLoadResult.bSucceeded
		&& MissingRepositoryLoadResult.bBackendUnavailable
		&& !MissingRepositoryLoadResult.ErrorMessage.IsEmpty();
	Result.bMissingRepositoryDidNotCreateDb =
		!DoSQLUISampleLayoutPersistenceWorkflowFilesExist(
			Result.MissingRepositoryMarkerDatabasePath);
	if (!Result.bMissingRepositorySaveFailed
		|| !Result.bMissingRepositoryListFailed
		|| !Result.bMissingRepositoryLoadFailed
		|| !Result.bMissingRepositoryDidNotCreateDb)
	{
		AppendSQLUISampleLayoutPersistenceWorkflowProbeError(
			Result,
			TEXT("SQLUI layout persistence workflow probe failed: missing repository did not fail clearly without creating DB files."));
	}

	USQLUILayoutRepositoryRuntimeSubsystem* InMemorySubsystem =
		CreateSQLUISampleLayoutPersistenceWorkflowSubsystem(WorkflowGameInstance);
	const FSQLUILayoutDocument InMemoryDocument =
		MakeSQLUISampleLayoutPersistenceWorkflowDocument(
			TEXT("sqlui.smoke.persistence-workflow.in-memory"),
			TEXT("SQLUI Persistence Workflow InMemory Layout"),
			TEXT("persistence-workflow-in-memory"));
	Result.bInMemoryInitialized =
		IsValid(InMemorySubsystem)
		&& InMemorySubsystem->InitializeRepositoryFromRuntimeConfig(
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault())
		&& FSQLUILayoutPersistenceWorkflow::HasRepository(InMemorySubsystem)
		&& InMemorySubsystem->GetActiveBackend() == ESQLUILayoutRepositoryBackend::InMemory;
	if (Result.bInMemoryInitialized)
	{
		const FSQLUILayoutSaveResult InMemorySaveResult =
			FSQLUILayoutPersistenceWorkflow::SaveLayout(
				InMemorySubsystem,
				InMemoryDocument);
		const FSQLUILayoutRepositoryListResult InMemoryListResult =
			FSQLUILayoutPersistenceWorkflow::ListLayouts(InMemorySubsystem);
		const FSQLUILayoutLoadResult InMemoryLoadResult =
			FSQLUILayoutPersistenceWorkflow::LoadLayout(
				InMemorySubsystem,
				InMemoryDocument.Metadata.LayoutId);
		Result.bInMemorySaveSucceeded =
			InMemorySaveResult.bSucceeded
			&& InMemorySaveResult.SavedLayoutId == InMemoryDocument.Metadata.LayoutId
			&& InMemorySaveResult.Validation.bIsValid;
		Result.bInMemoryListSucceeded = InMemoryListResult.bSucceeded;
		Result.bInMemoryListedMetadataFound =
			InMemoryListResult.bSucceeded
			&& DoesSQLUISampleLayoutMetadataAndTagsListContain(
				InMemoryListResult.Layouts,
				InMemoryDocument.Metadata);
		Result.bInMemoryLoadSucceeded = InMemoryLoadResult.bSucceeded;
		Result.bInMemoryLoadedDocumentValid = InMemoryLoadResult.Validation.bIsValid;
		Result.bInMemoryLoadedLayoutIdMatched =
			InMemoryLoadResult.Document.Metadata.LayoutId
			== InMemoryDocument.Metadata.LayoutId;
		if (IsValid(InMemorySubsystem))
		{
			InMemorySubsystem->ResetRepository();
		}
	}
	if (!Result.bInMemoryInitialized
		|| !Result.bInMemorySaveSucceeded
		|| !Result.bInMemoryListSucceeded
		|| !Result.bInMemoryListedMetadataFound
		|| !Result.bInMemoryLoadSucceeded
		|| !Result.bInMemoryLoadedDocumentValid
		|| !Result.bInMemoryLoadedLayoutIdMatched)
	{
		AppendSQLUISampleLayoutPersistenceWorkflowProbeError(
			Result,
			TEXT("SQLUI layout persistence workflow probe failed: InMemory workflow did not save/list/load through the runtime subsystem."));
	}

	USQLUILayoutRepositoryRuntimeSubsystem* SQLiteSubsystem =
		CreateSQLUISampleLayoutPersistenceWorkflowSubsystem(WorkflowGameInstance);
	FSQLUILayoutRepositoryRuntimeConfig SQLiteConfig;
	SQLiteConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	SQLiteConfig.SQLiteDatabasePath = Result.DatabasePath;
	SQLiteConfig.bSQLiteReadOnly = false;
	SQLiteConfig.bSQLiteInitializeSchemaIfMissing = true;
	SQLiteConfig.bSQLiteCreateDatabaseIfMissing = true;
	const FSQLUILayoutDocument SQLiteDocument =
		MakeSQLUISampleLayoutPersistenceWorkflowDocument(
			TEXT("sqlui.smoke.persistence-workflow.sqlite"),
			TEXT("SQLUI Persistence Workflow SQLite Layout"),
			TEXT("persistence-workflow-sqlite"));
	Result.bSQLiteInitialized =
		IsValid(SQLiteSubsystem)
		&& SQLiteSubsystem->InitializeRepositoryFromRuntimeConfig(SQLiteConfig)
		&& FSQLUILayoutPersistenceWorkflow::HasRepository(SQLiteSubsystem)
		&& SQLiteSubsystem->GetActiveBackend() == ESQLUILayoutRepositoryBackend::SQLite;
	if (Result.bSQLiteInitialized)
	{
		const FSQLUILayoutSaveResult SQLiteSaveResult =
			FSQLUILayoutPersistenceWorkflow::SaveLayout(
				SQLiteSubsystem,
				SQLiteDocument);
		const FSQLUILayoutRepositoryListResult SQLiteListResult =
			FSQLUILayoutPersistenceWorkflow::ListLayouts(SQLiteSubsystem);
		const FSQLUILayoutLoadResult SQLiteLoadResult =
			FSQLUILayoutPersistenceWorkflow::LoadLayout(
				SQLiteSubsystem,
				SQLiteDocument.Metadata.LayoutId);
		Result.bSQLiteSaveSucceeded =
			SQLiteSaveResult.bSucceeded
			&& SQLiteSaveResult.SavedLayoutId == SQLiteDocument.Metadata.LayoutId
			&& SQLiteSaveResult.Validation.bIsValid;
		Result.bSQLiteDatabaseCreated = FPaths::FileExists(Result.DatabasePath);
		Result.bSQLiteListSucceeded = SQLiteListResult.bSucceeded;
		Result.bSQLiteListedMetadataFound =
			SQLiteListResult.bSucceeded
			&& DoesSQLUISampleLayoutMetadataAndTagsListContain(
				SQLiteListResult.Layouts,
				SQLiteDocument.Metadata);
		Result.bSQLiteLoadSucceeded = SQLiteLoadResult.bSucceeded;
		Result.bSQLiteLoadedDocumentValid = SQLiteLoadResult.Validation.bIsValid;
		Result.bSQLiteLoadedLayoutIdMatched =
			SQLiteLoadResult.Document.Metadata.LayoutId
			== SQLiteDocument.Metadata.LayoutId;
		if (IsValid(SQLiteSubsystem))
		{
			SQLiteSubsystem->ResetRepository();
		}
	}
	if (!Result.bSQLiteInitialized
		|| !Result.bSQLiteSaveSucceeded
		|| !Result.bSQLiteDatabaseCreated
		|| !Result.bSQLiteListSucceeded
		|| !Result.bSQLiteListedMetadataFound
		|| !Result.bSQLiteLoadSucceeded
		|| !Result.bSQLiteLoadedDocumentValid
		|| !Result.bSQLiteLoadedLayoutIdMatched)
	{
		AppendSQLUISampleLayoutPersistenceWorkflowProbeError(
			Result,
			TEXT("SQLUI layout persistence workflow probe failed: SQLite workflow did not save/list/load through the runtime subsystem."));
	}

	USQLUILayoutRepositoryRuntimeSubsystem* SQLiteUnavailableSubsystem =
		CreateSQLUISampleLayoutPersistenceWorkflowSubsystem(WorkflowGameInstance);
	FSQLUILayoutRepositoryRuntimeConfig SQLiteUnavailableConfig;
	SQLiteUnavailableConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	const bool bSQLiteUnavailableInitialized =
		IsValid(SQLiteUnavailableSubsystem)
		&& SQLiteUnavailableSubsystem->InitializeRepositoryFromRuntimeConfig(
			SQLiteUnavailableConfig);
	const FSQLUILayoutRepositoryRuntimeIntegrationResult SQLiteUnavailableResult =
		IsValid(SQLiteUnavailableSubsystem)
			? SQLiteUnavailableSubsystem->GetLastIntegrationResult()
			: FSQLUILayoutRepositoryRuntimeIntegrationResult();
	const FSQLUILayoutDocument SQLiteUnavailableDocument =
		MakeSQLUISampleLayoutPersistenceWorkflowDocument(
			TEXT("sqlui.smoke.persistence-workflow.sqlite-unavailable"),
			TEXT("SQLUI Persistence Workflow SQLite Unavailable Layout"),
			TEXT("persistence-workflow-sqlite-unavailable"));
	const FSQLUILayoutSaveResult SQLiteUnavailableSaveResult =
		FSQLUILayoutPersistenceWorkflow::SaveLayout(
			SQLiteUnavailableSubsystem,
			SQLiteUnavailableDocument);
	const FSQLUILayoutRepositoryListResult SQLiteUnavailableListResult =
		FSQLUILayoutPersistenceWorkflow::ListLayouts(SQLiteUnavailableSubsystem);
	const FSQLUILayoutLoadResult SQLiteUnavailableLoadResult =
		FSQLUILayoutPersistenceWorkflow::LoadLayout(
			SQLiteUnavailableSubsystem,
			SQLiteUnavailableDocument.Metadata.LayoutId);
	Result.bSQLiteUnavailableHandled =
		!bSQLiteUnavailableInitialized
		&& SQLiteUnavailableResult.bBackendUnavailable;
	Result.bSQLiteUnavailableWorkflowFailedClearly =
		!SQLiteUnavailableSaveResult.bSucceeded
		&& SQLiteUnavailableSaveResult.bBackendUnavailable
		&& !SQLiteUnavailableSaveResult.ErrorMessage.IsEmpty()
		&& !SQLiteUnavailableListResult.bSucceeded
		&& !SQLiteUnavailableListResult.ErrorMessage.IsEmpty()
		&& !SQLiteUnavailableLoadResult.bSucceeded
		&& SQLiteUnavailableLoadResult.bBackendUnavailable
		&& !SQLiteUnavailableLoadResult.ErrorMessage.IsEmpty();
	Result.bSQLiteUnavailableDidNotCreateDb =
		!DoSQLUISampleLayoutPersistenceWorkflowFilesExist(
			Result.MissingRepositoryMarkerDatabasePath);
	if (IsValid(SQLiteUnavailableSubsystem))
	{
		SQLiteUnavailableSubsystem->ResetRepository();
	}
	if (!Result.bSQLiteUnavailableHandled
		|| !Result.bSQLiteUnavailableWorkflowFailedClearly
		|| !Result.bSQLiteUnavailableDidNotCreateDb)
	{
		AppendSQLUISampleLayoutPersistenceWorkflowProbeError(
			Result,
			SQLiteUnavailableResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout persistence workflow probe failed: SQLite unavailable path did not fail clearly without creating DB files.")
				: SQLiteUnavailableResult.ErrorMessage);
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISampleLayoutPersistenceWorkflowFiles(Result)
		&& !DoesAnySQLUISampleLayoutPersistenceWorkflowFileExist(Result);
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISampleLayoutPersistenceWorkflowProbeError(
			Result,
			TEXT("SQLUI layout persistence workflow probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bNullSubsystemSaveFailed
		&& Result.bNullSubsystemListFailed
		&& Result.bNullSubsystemLoadFailed
		&& Result.bMissingRepositorySaveFailed
		&& Result.bMissingRepositoryListFailed
		&& Result.bMissingRepositoryLoadFailed
		&& Result.bMissingRepositoryDidNotCreateDb
		&& Result.bInMemoryInitialized
		&& Result.bInMemorySaveSucceeded
		&& Result.bInMemoryListSucceeded
		&& Result.bInMemoryListedMetadataFound
		&& Result.bInMemoryLoadSucceeded
		&& Result.bInMemoryLoadedDocumentValid
		&& Result.bInMemoryLoadedLayoutIdMatched
		&& Result.bSQLiteInitialized
		&& Result.bSQLiteSaveSucceeded
		&& Result.bSQLiteDatabaseCreated
		&& Result.bSQLiteListSucceeded
		&& Result.bSQLiteListedMetadataFound
		&& Result.bSQLiteLoadSucceeded
		&& Result.bSQLiteLoadedDocumentValid
		&& Result.bSQLiteLoadedLayoutIdMatched
		&& Result.bSQLiteUnavailableHandled
		&& Result.bSQLiteUnavailableWorkflowFailedClearly
		&& Result.bSQLiteUnavailableDidNotCreateDb
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
	FSQLUISampleLayoutRepositoryDatabaseManagementProbeResult& Result,
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

FString MakeSQLUISampleLayoutRepositoryDatabaseManagementPath(
	const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutRepositoryDatabaseManagement"),
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISampleLayoutRepositoryDatabaseManagementDatabasePaths(
	const FSQLUISampleLayoutRepositoryDatabaseManagementProbeResult& Result)
{
	TArray<FString> DatabasePaths;
	DatabasePaths.Add(Result.DatabasePath);
	DatabasePaths.Add(Result.SidecarDatabasePath);
	DatabasePaths.Add(Result.ResolvedRelativeDatabasePath);
	return DatabasePaths;
}

bool DoSQLUISampleLayoutRepositoryDatabaseManagementFilesExist(
	const FString& DatabasePath)
{
	if (DatabasePath.IsEmpty())
	{
		return false;
	}

	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleLayoutRepositoryDatabaseManagementFiles(
	const FString& DatabasePath,
	FSQLUISampleLayoutRepositoryDatabaseManagementProbeResult& Result)
{
	if (DatabasePath.IsEmpty())
	{
		return true;
	}

	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI layout repository database management probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISampleLayoutRepositoryDatabaseManagementFiles(
	FSQLUISampleLayoutRepositoryDatabaseManagementProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& DatabasePath :
		MakeSQLUISampleLayoutRepositoryDatabaseManagementDatabasePaths(Result))
	{
		bRemoved =
			DeleteSQLUISampleLayoutRepositoryDatabaseManagementFiles(DatabasePath, Result)
			&& bRemoved;
	}

	return bRemoved;
}

bool DoesAnySQLUISampleLayoutRepositoryDatabaseManagementFileExist(
	const FSQLUISampleLayoutRepositoryDatabaseManagementProbeResult& Result)
{
	for (const FString& DatabasePath :
		MakeSQLUISampleLayoutRepositoryDatabaseManagementDatabasePaths(Result))
	{
		if (DoSQLUISampleLayoutRepositoryDatabaseManagementFilesExist(DatabasePath))
		{
			return true;
		}
	}

	return false;
}

bool WriteSQLUISampleLayoutRepositoryDatabaseManagementFile(
	const FString& Path,
	FSQLUISampleLayoutRepositoryDatabaseManagementProbeResult& Result)
{
	const FString Directory = FPaths::GetPath(Path);
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI layout repository database management probe failed: could not create directory '%s'."),
				*Directory));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(TEXT("SQLUI probe sidecar"), *Path))
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI layout repository database management probe failed: could not create file '%s'."),
				*Path));
		return false;
	}

	return true;
}

FSQLUILayoutDocument MakeSQLUISampleLayoutRepositoryDatabaseManagementDocument()
{
	FSQLUILayoutDocument Document =
		MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument();
	Document.Version.Label = TEXT("Layout Repository Database Management Probe");
	Document.Metadata.LayoutId =
		TEXT("sqlui.smoke.layout-repository-database-management");
	Document.Metadata.DisplayName =
		TEXT("SQLUI Layout Repository Database Management Probe");
	Document.Metadata.Description =
		TEXT("Smoke/probe layout for runtime SQLite database management policy.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-06-06T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = Document.Metadata.CreatedAtUtc;
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("database-management"));
	Document.Metadata.SearchMetadata.Add(
		TEXT("Probe"),
		TEXT("LayoutRepositoryDatabaseManagement"));
	Document.RootWidgetId = TEXT("SQLUI.LayoutRepositoryDatabaseManagement.Root");

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), Document.Metadata.DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(TEXT("database-management"));
		Document.Nodes[0].SearchMetadata.Add(
			TEXT("Probe"),
			TEXT("LayoutRepositoryDatabaseManagement"));
	}

	return Document;
}

FSQLUILayoutRepositoryRuntimeConfig
MakeSQLUISampleLayoutRepositoryDatabaseManagementSQLiteConfig(
	const FString& DatabasePath)
{
	FSQLUILayoutRepositoryRuntimeConfig Config;
	Config.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	Config.SQLiteDatabasePath = DatabasePath;
	Config.bSQLiteReadOnly = false;
	Config.bSQLiteInitializeSchemaIfMissing = true;
	Config.bSQLiteCreateDatabaseIfMissing = true;
	return Config;
}

FSQLUISampleLayoutRepositoryDatabaseManagementProbeResult
RunSQLUISampleLayoutRepositoryDatabaseManagementProbe(UObject* Outer)
{
	FSQLUISampleLayoutRepositoryDatabaseManagementProbeResult Result;
	Result.DatabasePath =
		MakeSQLUISampleLayoutRepositoryDatabaseManagementPath(
			TEXT("LayoutRepositoryDatabaseManagement.db"));
	Result.SidecarDatabasePath =
		MakeSQLUISampleLayoutRepositoryDatabaseManagementPath(
			TEXT("LayoutRepositoryDatabaseManagementSidecars.db"));
	Result.RelativeDatabasePath =
		TEXT("LayoutRepositoryDatabaseManagement/RelativeDatabaseManagement.db");
	Result.ResolvedRelativeDatabasePath =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(
			Result.RelativeDatabasePath);

	DeleteSQLUISampleLayoutRepositoryDatabaseManagementFiles(Result);

	FSQLUILayoutRepositoryRuntimeConfig NonSQLiteConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
	NonSQLiteConfig.SQLiteDatabasePath = Result.DatabasePath;
	FSQLUILayoutRepositoryDatabaseStatusRequest NonSQLiteStatusRequest;
	NonSQLiteStatusRequest.RuntimeConfig = NonSQLiteConfig;
	const FSQLUILayoutRepositoryDatabaseStatusResult NonSQLiteStatus =
		FSQLUILayoutRepositoryDatabaseManagement::GetStatus(NonSQLiteStatusRequest);
	FSQLUILayoutRepositoryDatabaseResetRequest NonSQLiteResetRequest;
	NonSQLiteResetRequest.RuntimeConfig = NonSQLiteConfig;
	const FSQLUILayoutRepositoryDatabaseResetResult NonSQLiteReset =
		FSQLUILayoutRepositoryDatabaseManagement::ResetDatabase(NonSQLiteResetRequest);
	Result.bNonSQLiteStatusSafe =
		NonSQLiteStatus.bSucceeded
		&& !NonSQLiteStatus.bBackendIsSQLite
		&& !NonSQLiteStatus.bDatabasePathResolved
		&& !DoSQLUISampleLayoutRepositoryDatabaseManagementFilesExist(Result.DatabasePath);
	Result.bNonSQLiteResetSafe =
		NonSQLiteReset.bSucceeded
		&& !NonSQLiteReset.bBackendIsSQLite
		&& !NonSQLiteReset.bDatabasePathResolved
		&& !DoSQLUISampleLayoutRepositoryDatabaseManagementFilesExist(Result.DatabasePath);
	if (!Result.bNonSQLiteStatusSafe || !Result.bNonSQLiteResetSafe)
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			TEXT("SQLUI layout repository database management probe failed: non-SQLite status/reset was not a safe no-op."));
	}

	FSQLUILayoutRepositoryRuntimeConfig EmptySQLiteConfig;
	EmptySQLiteConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	FSQLUILayoutRepositoryDatabaseStatusRequest EmptyStatusRequest;
	EmptyStatusRequest.RuntimeConfig = EmptySQLiteConfig;
	const FSQLUILayoutRepositoryDatabaseStatusResult EmptyStatus =
		FSQLUILayoutRepositoryDatabaseManagement::GetStatus(EmptyStatusRequest);
	FSQLUILayoutRepositoryDatabaseResetRequest EmptyResetRequest;
	EmptyResetRequest.RuntimeConfig = EmptySQLiteConfig;
	const FSQLUILayoutRepositoryDatabaseResetResult EmptyReset =
		FSQLUILayoutRepositoryDatabaseManagement::ResetDatabase(EmptyResetRequest);
	Result.bSQLiteEmptyPathStatusHandled =
		EmptyStatus.bSucceeded
		&& EmptyStatus.bBackendIsSQLite
		&& !EmptyStatus.bDatabasePathResolved
		&& !EmptyStatus.ErrorMessage.IsEmpty();
	Result.bSQLiteEmptyPathResetFailedClearly =
		!EmptyReset.bSucceeded
		&& EmptyReset.bBackendIsSQLite
		&& !EmptyReset.bDatabasePathResolved
		&& !EmptyReset.ErrorMessage.IsEmpty();
	if (!Result.bSQLiteEmptyPathStatusHandled
		|| !Result.bSQLiteEmptyPathResetFailedClearly)
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			TEXT("SQLUI layout repository database management probe failed: SQLite empty path was not handled clearly."));
	}

	const FSQLUILayoutRepositoryRuntimeConfig SQLiteConfig =
		MakeSQLUISampleLayoutRepositoryDatabaseManagementSQLiteConfig(
			Result.DatabasePath);
	FSQLUILayoutRepositoryDatabaseStatusRequest SQLiteStatusRequest;
	SQLiteStatusRequest.RuntimeConfig = SQLiteConfig;
	const FSQLUILayoutRepositoryDatabaseStatusResult StatusBeforeCreate =
		FSQLUILayoutRepositoryDatabaseManagement::GetStatus(SQLiteStatusRequest);
	Result.bSQLiteStatusBeforeCreateSucceeded =
		StatusBeforeCreate.bSucceeded
		&& StatusBeforeCreate.bBackendIsSQLite
		&& StatusBeforeCreate.bDatabasePathResolved
		&& StatusBeforeCreate.DatabasePath == Result.DatabasePath;
	Result.bSQLiteStatusBeforeCreateAbsent =
		Result.bSQLiteStatusBeforeCreateSucceeded
		&& !StatusBeforeCreate.bAnyDatabaseFileExists
		&& !StatusBeforeCreate.bDatabaseExists;
	if (!Result.bSQLiteStatusBeforeCreateSucceeded
		|| !Result.bSQLiteStatusBeforeCreateAbsent)
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			TEXT("SQLUI layout repository database management probe failed: SQLite status before create was not absent and resolved."));
	}

	FSQLUILayoutRepositoryFactorySettings FactorySettings =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(SQLiteConfig);
	USQLUILayoutRepository* Repository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, FactorySettings);
	const FSQLUILayoutDocument Document =
		MakeSQLUISampleLayoutRepositoryDatabaseManagementDocument();
	const FSQLUILayoutSaveResult SaveResult =
		SaveSQLUISampleLayoutToRepository(
			Repository,
			TEXT("SQLite database management repository"),
			Document);
	if (!SaveResult.bSucceeded)
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			SaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository database management probe failed: SaveLayout did not create the SQLite database.")
				: SaveResult.ErrorMessage);
	}

	const FSQLUILayoutRepositoryDatabaseStatusResult StatusAfterSave =
		FSQLUILayoutRepositoryDatabaseManagement::GetStatus(SQLiteStatusRequest);
	Result.bSQLiteStatusAfterSaveDetectedDatabase =
		StatusAfterSave.bSucceeded
		&& StatusAfterSave.bDatabaseExists
		&& StatusAfterSave.bAnyDatabaseFileExists;
	Result.bSQLiteStatusAfterSaveSizePositive =
		StatusAfterSave.bDatabaseExists
		&& StatusAfterSave.DatabaseFileSizeBytes > 0;
	if (!Result.bSQLiteStatusAfterSaveDetectedDatabase
		|| !Result.bSQLiteStatusAfterSaveSizePositive)
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			TEXT("SQLUI layout repository database management probe failed: status after save did not detect a non-empty database file."));
	}

	FSQLUILayoutRepositoryDatabaseResetRequest SQLiteResetRequest;
	SQLiteResetRequest.RuntimeConfig = SQLiteConfig;
	const FSQLUILayoutRepositoryDatabaseResetResult ResetAfterSave =
		FSQLUILayoutRepositoryDatabaseManagement::ResetDatabase(SQLiteResetRequest);
	Result.bSQLiteResetSucceeded = ResetAfterSave.bSucceeded;
	Result.bSQLiteResetRemovedDatabase =
		ResetAfterSave.bDatabaseRemoved
		&& ResetAfterSave.bAllDatabaseFilesAbsent
		&& !DoSQLUISampleLayoutRepositoryDatabaseManagementFilesExist(Result.DatabasePath);
	const FSQLUILayoutRepositoryDatabaseResetResult ResetAgain =
		FSQLUILayoutRepositoryDatabaseManagement::ResetDatabase(SQLiteResetRequest);
	Result.bSQLiteResetIdempotent =
		ResetAgain.bSucceeded
		&& ResetAgain.bDatabaseRemoved
		&& ResetAgain.bAllDatabaseFilesAbsent;
	const FSQLUILayoutRepositoryDatabaseStatusResult StatusAfterReset =
		FSQLUILayoutRepositoryDatabaseManagement::GetStatus(SQLiteStatusRequest);
	Result.bSQLiteStatusAfterResetAbsent =
		StatusAfterReset.bSucceeded
		&& !StatusAfterReset.bAnyDatabaseFileExists
		&& !StatusAfterReset.bDatabaseExists;
	if (!Result.bSQLiteResetSucceeded
		|| !Result.bSQLiteResetRemovedDatabase
		|| !Result.bSQLiteResetIdempotent
		|| !Result.bSQLiteStatusAfterResetAbsent)
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			ResetAfterSave.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository database management probe failed: SQLite reset was not successful and idempotent.")
				: ResetAfterSave.ErrorMessage);
	}

	const bool bSidecarsCreated =
		WriteSQLUISampleLayoutRepositoryDatabaseManagementFile(
			Result.SidecarDatabasePath + TEXT("-journal"),
			Result)
		&& WriteSQLUISampleLayoutRepositoryDatabaseManagementFile(
			Result.SidecarDatabasePath + TEXT("-wal"),
			Result)
		&& WriteSQLUISampleLayoutRepositoryDatabaseManagementFile(
			Result.SidecarDatabasePath + TEXT("-shm"),
			Result);
	FSQLUILayoutRepositoryRuntimeConfig SidecarConfig =
		MakeSQLUISampleLayoutRepositoryDatabaseManagementSQLiteConfig(
			Result.SidecarDatabasePath);
	FSQLUILayoutRepositoryDatabaseResetRequest SidecarResetRequest;
	SidecarResetRequest.RuntimeConfig = SidecarConfig;
	const FSQLUILayoutRepositoryDatabaseResetResult SidecarReset =
		FSQLUILayoutRepositoryDatabaseManagement::ResetDatabase(SidecarResetRequest);
	Result.bSQLiteSidecarRemovalSucceeded =
		bSidecarsCreated
		&& SidecarReset.bSucceeded
		&& SidecarReset.bJournalRemoved
		&& SidecarReset.bWalRemoved
		&& SidecarReset.bShmRemoved
		&& !DoSQLUISampleLayoutRepositoryDatabaseManagementFilesExist(
			Result.SidecarDatabasePath);
	if (!Result.bSQLiteSidecarRemovalSucceeded)
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			SidecarReset.ErrorMessage.IsEmpty()
				? TEXT("SQLUI layout repository database management probe failed: fake sidecar files were not removed.")
				: SidecarReset.ErrorMessage);
	}

	FSQLUILayoutRepositoryRuntimeConfig RelativeConfig =
		MakeSQLUISampleLayoutRepositoryDatabaseManagementSQLiteConfig(
			Result.RelativeDatabasePath);
	FSQLUILayoutRepositoryDatabaseStatusRequest RelativeStatusRequest;
	RelativeStatusRequest.RuntimeConfig = RelativeConfig;
	const FSQLUILayoutRepositoryDatabaseStatusResult RelativeStatus =
		FSQLUILayoutRepositoryDatabaseManagement::GetStatus(RelativeStatusRequest);
	FString ExpectedRelativeBaseDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("LayoutRepositories"));
	FPaths::NormalizeFilename(ExpectedRelativeBaseDirectory);
	ExpectedRelativeBaseDirectory =
		FPaths::ConvertRelativePathToFull(ExpectedRelativeBaseDirectory);
	Result.bRelativePathResolvedUnderSaved =
		RelativeStatus.bSucceeded
		&& RelativeStatus.bDatabasePathResolved
		&& RelativeStatus.DatabasePath == Result.ResolvedRelativeDatabasePath
		&& RelativeStatus.DatabasePath.StartsWith(ExpectedRelativeBaseDirectory)
		&& !RelativeStatus.bAnyDatabaseFileExists;
	if (!Result.bRelativePathResolvedUnderSaved)
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			TEXT("SQLUI layout repository database management probe failed: relative SQLite path did not resolve under Saved/SQLUI/LayoutRepositories without creating files."));
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISampleLayoutRepositoryDatabaseManagementFiles(Result)
		&& !DoesAnySQLUISampleLayoutRepositoryDatabaseManagementFileExist(Result);
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISampleLayoutRepositoryDatabaseManagementProbeError(
			Result,
			TEXT("SQLUI layout repository database management probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bNonSQLiteStatusSafe
		&& Result.bNonSQLiteResetSafe
		&& Result.bSQLiteEmptyPathStatusHandled
		&& Result.bSQLiteEmptyPathResetFailedClearly
		&& Result.bSQLiteStatusBeforeCreateSucceeded
		&& Result.bSQLiteStatusBeforeCreateAbsent
		&& Result.bSQLiteStatusAfterSaveDetectedDatabase
		&& Result.bSQLiteStatusAfterSaveSizePositive
		&& Result.bSQLiteResetSucceeded
		&& Result.bSQLiteResetRemovedDatabase
		&& Result.bSQLiteResetIdempotent
		&& Result.bSQLiteStatusAfterResetAbsent
		&& Result.bSQLiteSidecarRemovalSucceeded
		&& Result.bRelativePathResolvedUnderSaved
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISamplePersistenceStatusSurfaceProbeError(
	FSQLUISamplePersistenceStatusSurfaceProbeResult& Result,
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

FString MakeSQLUISamplePersistenceStatusSurfacePath(
	const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("PersistenceStatusSurface"),
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISamplePersistenceStatusSurfaceFiles(
	const FString& DatabasePath)
{
	return {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};
}

bool DoesSQLUISamplePersistenceStatusSurfaceFileExist(
	const FString& DatabasePath)
{
	for (const FString& Path : MakeSQLUISamplePersistenceStatusSurfaceFiles(DatabasePath))
	{
		if (FPaths::FileExists(Path))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISamplePersistenceStatusSurfaceFiles(
	const FString& DatabasePath,
	FSQLUISamplePersistenceStatusSurfaceProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& Path : MakeSQLUISamplePersistenceStatusSurfaceFiles(DatabasePath))
	{
		if (FPaths::FileExists(Path)
			&& !IFileManager::Get().Delete(*Path, false, true, true))
		{
			AppendSQLUISamplePersistenceStatusSurfaceProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI persistence status surface probe failed: could not remove '%s'."),
					*Path));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISamplePersistenceStatusSurfaceFiles(
	FSQLUISamplePersistenceStatusSurfaceProbeResult& Result)
{
	return DeleteSQLUISamplePersistenceStatusSurfaceFiles(Result.DatabasePath, Result)
		&& DeleteSQLUISamplePersistenceStatusSurfaceFiles(Result.SidecarDatabasePath, Result);
}

bool DoesAnySQLUISamplePersistenceStatusSurfaceFileExist(
	const FSQLUISamplePersistenceStatusSurfaceProbeResult& Result)
{
	return DoesSQLUISamplePersistenceStatusSurfaceFileExist(Result.DatabasePath)
		|| DoesSQLUISamplePersistenceStatusSurfaceFileExist(Result.SidecarDatabasePath);
}

bool WriteSQLUISamplePersistenceStatusSurfaceSidecar(
	const FString& Path,
	FSQLUISamplePersistenceStatusSurfaceProbeResult& Result)
{
	const FString Directory = FPaths::GetPath(Path);
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		AppendSQLUISamplePersistenceStatusSurfaceProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI persistence status surface probe failed: could not create directory '%s'."),
				*Directory));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(TEXT("SQLUI persistence status sidecar probe"), *Path))
	{
		AppendSQLUISamplePersistenceStatusSurfaceProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI persistence status surface probe failed: could not create sidecar '%s'."),
				*Path));
		return false;
	}

	return true;
}

FSQLUILayoutRepositoryRuntimeConfig MakeSQLUISamplePersistenceStatusSurfaceSQLiteConfig(
	const FString& DatabasePath)
{
	FSQLUILayoutRepositoryRuntimeConfig Config;
	Config.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	Config.SQLiteDatabasePath = DatabasePath;
	return Config;
}

FSQLUISamplePersistenceStatusSurfaceProbeResult
RunSQLUISamplePersistenceStatusSurfaceProbe(UObject* Outer)
{
	FSQLUISamplePersistenceStatusSurfaceProbeResult Result;
	Result.DatabasePath =
		MakeSQLUISamplePersistenceStatusSurfacePath(TEXT("PersistenceStatusSurface.db"));
	Result.SidecarDatabasePath =
		MakeSQLUISamplePersistenceStatusSurfacePath(TEXT("PersistenceStatusSidecarOnly.db"));

	DeleteSQLUISamplePersistenceStatusSurfaceFiles(Result);

	const FSQLUILayoutRepositoryRuntimeConfig DefaultConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
	const FSQLUIPersistenceStatusSnapshot DefaultStatus =
		USQLUIPersistenceStatusLibrary::GetPersistenceStatusForProvider(
			DefaultConfig,
			nullptr);
	Result.bDefaultStatusSucceeded = DefaultStatus.bSucceeded;
	Result.bDefaultBackendInMemory =
		DefaultStatus.ConfiguredBackend == ESQLUILayoutRepositoryBackend::InMemory
		&& DefaultStatus.ConfiguredBackendName == TEXT("InMemory");
	Result.bDefaultProviderNotInitialized = !DefaultStatus.bProviderInitialized;
	Result.bDefaultRepositoryNotActive = !DefaultStatus.bRepositoryActive;
	Result.bDefaultStatusDidNotCreateDb =
		!DoesAnySQLUISamplePersistenceStatusSurfaceFileExist(Result);
	if (!Result.bDefaultStatusSucceeded
		|| !Result.bDefaultBackendInMemory
		|| !Result.bDefaultProviderNotInitialized
		|| !Result.bDefaultRepositoryNotActive
		|| !Result.bDefaultStatusDidNotCreateDb)
	{
		AppendSQLUISamplePersistenceStatusSurfaceProbeError(
			Result,
			TEXT("SQLUI persistence status surface probe failed: default InMemory status was not safe/read-only."));
	}

	const FSQLUISQLiteLayoutSchemaInitializationResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(
			Result.DatabasePath,
			true);
	Result.bSQLiteDatabasePrepared =
		SchemaResult.bSucceeded
		&& FPaths::FileExists(Result.DatabasePath);
	if (!Result.bSQLiteDatabasePrepared)
	{
		AppendSQLUISamplePersistenceStatusSurfaceProbeError(
			Result,
			SchemaResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI persistence status surface probe failed: could not prepare SQLite status database.")
				: SchemaResult.ErrorMessage);
	}

	const int64 DatabaseSizeBeforeStatus =
		FPaths::FileExists(Result.DatabasePath)
			? IFileManager::Get().FileSize(*Result.DatabasePath)
			: 0;
	const FSQLUILayoutRepositoryRuntimeConfig SQLiteConfig =
		MakeSQLUISamplePersistenceStatusSurfaceSQLiteConfig(Result.DatabasePath);
	const FSQLUIPersistenceStatusSnapshot SQLiteStatus =
		USQLUIPersistenceStatusLibrary::GetPersistenceStatusFromRuntimeConfig(
			Outer,
			SQLiteConfig);
	const int64 DatabaseSizeAfterStatus =
		FPaths::FileExists(Result.DatabasePath)
			? IFileManager::Get().FileSize(*Result.DatabasePath)
			: 0;

	Result.bSQLiteStatusSucceeded = SQLiteStatus.bSucceeded;
	Result.bSQLitePathResolved =
		SQLiteStatus.bSQLiteDatabasePathResolved
		&& SQLiteStatus.ResolvedSQLiteDatabasePath == Result.DatabasePath;
	Result.bSQLiteDatabaseDetected = SQLiteStatus.bSQLiteDatabaseExists;
	Result.bSQLiteDatabaseSizePositive = SQLiteStatus.SQLiteDatabaseSizeBytes > 0;
	Result.bSQLiteMigrationStatusChecked = SQLiteStatus.bMigrationStatusChecked;
	Result.bSQLiteMigrationStatusSucceeded = SQLiteStatus.bMigrationStatusSucceeded;
	Result.bSQLiteSchemaReady = SQLiteStatus.bSQLiteSchemaObjectsReady;
	Result.bSQLiteStatusReadOnly =
		DatabaseSizeBeforeStatus > 0
		&& DatabaseSizeAfterStatus == DatabaseSizeBeforeStatus
		&& FPaths::FileExists(Result.DatabasePath);
	if (!Result.bSQLiteStatusSucceeded
		|| !Result.bSQLitePathResolved
		|| !Result.bSQLiteDatabaseDetected
		|| !Result.bSQLiteDatabaseSizePositive
		|| !Result.bSQLiteMigrationStatusChecked
		|| !Result.bSQLiteMigrationStatusSucceeded
		|| !Result.bSQLiteSchemaReady
		|| !Result.bSQLiteStatusReadOnly)
	{
		AppendSQLUISamplePersistenceStatusSurfaceProbeError(
			Result,
			SQLiteStatus.ErrorMessage.IsEmpty()
				? TEXT("SQLUI persistence status surface probe failed: SQLite status snapshot did not report the prepared database read-only.")
				: SQLiteStatus.ErrorMessage);
	}

	const bool bSidecarCreated =
		WriteSQLUISamplePersistenceStatusSurfaceSidecar(
			Result.SidecarDatabasePath + TEXT("-wal"),
			Result);
	const FSQLUILayoutRepositoryRuntimeConfig SidecarConfig =
		MakeSQLUISamplePersistenceStatusSurfaceSQLiteConfig(Result.SidecarDatabasePath);
	const FSQLUIPersistenceStatusSnapshot SidecarStatus =
		USQLUIPersistenceStatusLibrary::GetPersistenceStatusForProvider(
			SidecarConfig,
			nullptr);
	Result.bSQLiteSidecarsDetected =
		bSidecarCreated
		&& SidecarStatus.bSucceeded
		&& !SidecarStatus.bSQLiteDatabaseExists
		&& SidecarStatus.bSQLiteSidecarsPresent
		&& SidecarStatus.bSQLiteWalExists
		&& !SidecarStatus.bMigrationStatusChecked
		&& FPaths::FileExists(Result.SidecarDatabasePath + TEXT("-wal"));
	if (!Result.bSQLiteSidecarsDetected)
	{
		AppendSQLUISamplePersistenceStatusSurfaceProbeError(
			Result,
			TEXT("SQLUI persistence status surface probe failed: sidecar-only status was not reported without mutation."));
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISamplePersistenceStatusSurfaceFiles(Result)
		&& !DoesAnySQLUISamplePersistenceStatusSurfaceFileExist(Result);
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISamplePersistenceStatusSurfaceProbeError(
			Result,
			TEXT("SQLUI persistence status surface probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bDefaultStatusSucceeded
		&& Result.bDefaultBackendInMemory
		&& Result.bDefaultProviderNotInitialized
		&& Result.bDefaultRepositoryNotActive
		&& Result.bDefaultStatusDidNotCreateDb
		&& Result.bSQLiteDatabasePrepared
		&& Result.bSQLiteStatusSucceeded
		&& Result.bSQLitePathResolved
		&& Result.bSQLiteDatabaseDetected
		&& Result.bSQLiteDatabaseSizePositive
		&& Result.bSQLiteSidecarsDetected
		&& Result.bSQLiteMigrationStatusChecked
		&& Result.bSQLiteMigrationStatusSucceeded
		&& Result.bSQLiteSchemaReady
		&& Result.bSQLiteStatusReadOnly
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISamplePersistenceStatusDisplayRowsProbeError(
	FSQLUISamplePersistenceStatusDisplayRowsProbeResult& Result,
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

FString MakeSQLUISamplePersistenceStatusDisplayRowsPath(
	const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("PersistenceStatusDisplayRows"),
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISamplePersistenceStatusDisplayRowsFiles(
	const FString& DatabasePath)
{
	return {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};
}

bool DoesSQLUISamplePersistenceStatusDisplayRowsFileExist(
	const FString& DatabasePath)
{
	for (const FString& Path : MakeSQLUISamplePersistenceStatusDisplayRowsFiles(DatabasePath))
	{
		if (FPaths::FileExists(Path))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISamplePersistenceStatusDisplayRowsFiles(
	const FString& DatabasePath,
	FSQLUISamplePersistenceStatusDisplayRowsProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& Path : MakeSQLUISamplePersistenceStatusDisplayRowsFiles(DatabasePath))
	{
		if (FPaths::FileExists(Path)
			&& !IFileManager::Get().Delete(*Path, false, true, true))
		{
			AppendSQLUISamplePersistenceStatusDisplayRowsProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI persistence status display rows probe failed: could not remove '%s'."),
					*Path));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISamplePersistenceStatusDisplayRowsFiles(
	FSQLUISamplePersistenceStatusDisplayRowsProbeResult& Result)
{
	return DeleteSQLUISamplePersistenceStatusDisplayRowsFiles(Result.DatabasePath, Result)
		&& DeleteSQLUISamplePersistenceStatusDisplayRowsFiles(Result.SidecarDatabasePath, Result);
}

bool DoesAnySQLUISamplePersistenceStatusDisplayRowsFileExist(
	const FSQLUISamplePersistenceStatusDisplayRowsProbeResult& Result)
{
	return DoesSQLUISamplePersistenceStatusDisplayRowsFileExist(Result.DatabasePath)
		|| DoesSQLUISamplePersistenceStatusDisplayRowsFileExist(Result.SidecarDatabasePath);
}

bool WriteSQLUISamplePersistenceStatusDisplayRowsSidecar(
	const FString& Path,
	FSQLUISamplePersistenceStatusDisplayRowsProbeResult& Result)
{
	const FString Directory = FPaths::GetPath(Path);
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		AppendSQLUISamplePersistenceStatusDisplayRowsProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI persistence status display rows probe failed: could not create directory '%s'."),
				*Directory));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(TEXT("SQLUI persistence status display sidecar probe"), *Path))
	{
		AppendSQLUISamplePersistenceStatusDisplayRowsProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI persistence status display rows probe failed: could not create sidecar '%s'."),
				*Path));
		return false;
	}

	return true;
}

const FSQLUIPersistenceStatusDisplayRow* FindSQLUISamplePersistenceStatusDisplayRow(
	const TArray<FSQLUIPersistenceStatusDisplayRow>& Rows,
	const TCHAR* Label)
{
	for (const FSQLUIPersistenceStatusDisplayRow& Row : Rows)
	{
		if (Row.Label.ToString() == Label)
		{
			return &Row;
		}
	}

	return nullptr;
}

bool DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
	const TArray<FSQLUIPersistenceStatusDisplayRow>& Rows,
	const TCHAR* Label,
	const TCHAR* ExpectedValue)
{
	const FSQLUIPersistenceStatusDisplayRow* Row =
		FindSQLUISamplePersistenceStatusDisplayRow(Rows, Label);
	return Row != nullptr && Row->Value.ToString() == ExpectedValue;
}

bool DoesSQLUISamplePersistenceStatusDisplayRowContainValue(
	const TArray<FSQLUIPersistenceStatusDisplayRow>& Rows,
	const TCHAR* Label,
	const FString& ExpectedValue)
{
	const FSQLUIPersistenceStatusDisplayRow* Row =
		FindSQLUISamplePersistenceStatusDisplayRow(Rows, Label);
	return Row != nullptr && Row->Value.ToString().Contains(ExpectedValue);
}

FSQLUILayoutRepositoryRuntimeConfig MakeSQLUISamplePersistenceStatusDisplayRowsSQLiteConfig(
	const FString& DatabasePath)
{
	FSQLUILayoutRepositoryRuntimeConfig Config;
	Config.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	Config.SQLiteDatabasePath = DatabasePath;
	return Config;
}

FSQLUISamplePersistenceStatusDisplayRowsProbeResult
RunSQLUISamplePersistenceStatusDisplayRowsProbe(UObject* Outer)
{
	FSQLUISamplePersistenceStatusDisplayRowsProbeResult Result;
	Result.DatabasePath =
		MakeSQLUISamplePersistenceStatusDisplayRowsPath(TEXT("PersistenceStatusDisplayRows.db"));
	Result.SidecarDatabasePath =
		MakeSQLUISamplePersistenceStatusDisplayRowsPath(TEXT("PersistenceStatusDisplaySidecarOnly.db"));

	DeleteSQLUISamplePersistenceStatusDisplayRowsFiles(Result);

	const FSQLUILayoutRepositoryRuntimeConfig DefaultConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
	const TArray<FSQLUIPersistenceStatusDisplayRow> DefaultRows =
		USQLUIPersistenceStatusDisplayLibrary::GetPersistenceStatusDisplayRowsFromRuntimeConfig(
			Outer,
			DefaultConfig);
	Result.bDefaultRowsGenerated = DefaultRows.Num() >= 8;
	Result.bDefaultBackendRowFound =
		DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			DefaultRows,
			TEXT("Backend"),
			TEXT("InMemory"));
	Result.bDefaultProviderRowFound =
		DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			DefaultRows,
			TEXT("Provider initialized"),
			TEXT("No"));
	Result.bDefaultRepositoryRowFound =
		DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			DefaultRows,
			TEXT("Repository active"),
			TEXT("No"));
	Result.bDefaultSQLiteRowsGraceful =
		DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			DefaultRows,
			TEXT("SQLite database path"),
			TEXT("Not applicable"))
		&& DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			DefaultRows,
			TEXT("SQLite database exists"),
			TEXT("Not applicable"))
		&& DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			DefaultRows,
			TEXT("Schema status"),
			TEXT("Not applicable"));
	Result.bDefaultRowsDidNotCreateDb =
		!DoesAnySQLUISamplePersistenceStatusDisplayRowsFileExist(Result);
	if (!Result.bDefaultRowsGenerated
		|| !Result.bDefaultBackendRowFound
		|| !Result.bDefaultProviderRowFound
		|| !Result.bDefaultRepositoryRowFound
		|| !Result.bDefaultSQLiteRowsGraceful
		|| !Result.bDefaultRowsDidNotCreateDb)
	{
		AppendSQLUISamplePersistenceStatusDisplayRowsProbeError(
			Result,
			TEXT("SQLUI persistence status display rows probe failed: default display rows were not safe/read-only."));
	}

	const FSQLUISQLiteLayoutSchemaInitializationResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(
			Result.DatabasePath,
			true);
	Result.bSQLiteDatabasePrepared =
		SchemaResult.bSucceeded
		&& FPaths::FileExists(Result.DatabasePath);
	if (!Result.bSQLiteDatabasePrepared)
	{
		AppendSQLUISamplePersistenceStatusDisplayRowsProbeError(
			Result,
			SchemaResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI persistence status display rows probe failed: could not prepare SQLite display database.")
				: SchemaResult.ErrorMessage);
	}

	const int64 DatabaseSizeBeforeRows =
		FPaths::FileExists(Result.DatabasePath)
			? IFileManager::Get().FileSize(*Result.DatabasePath)
			: 0;
	const FSQLUILayoutRepositoryRuntimeConfig SQLiteConfig =
		MakeSQLUISamplePersistenceStatusDisplayRowsSQLiteConfig(Result.DatabasePath);
	const TArray<FSQLUIPersistenceStatusDisplayRow> SQLiteRows =
		USQLUIPersistenceStatusDisplayLibrary::GetPersistenceStatusDisplayRowsFromRuntimeConfig(
			Outer,
			SQLiteConfig);
	const int64 DatabaseSizeAfterRows =
		FPaths::FileExists(Result.DatabasePath)
			? IFileManager::Get().FileSize(*Result.DatabasePath)
			: 0;

	Result.bSQLiteRowsGenerated = SQLiteRows.Num() >= 8;
	Result.bSQLitePathRowFound =
		DoesSQLUISamplePersistenceStatusDisplayRowContainValue(
			SQLiteRows,
			TEXT("SQLite database path"),
			Result.DatabasePath);
	Result.bSQLiteDatabaseExistsRowFound =
		DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			SQLiteRows,
			TEXT("SQLite database exists"),
			TEXT("Yes"));
	Result.bSQLiteDatabaseSizeRowFound =
		FindSQLUISamplePersistenceStatusDisplayRow(
			SQLiteRows,
			TEXT("SQLite database size")) != nullptr;
	Result.bSQLiteSchemaRowFound =
		DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			SQLiteRows,
			TEXT("Schema status"),
			TEXT("Ready"));
	Result.bSQLiteRowsReadOnly =
		DatabaseSizeBeforeRows > 0
		&& DatabaseSizeAfterRows == DatabaseSizeBeforeRows
		&& FPaths::FileExists(Result.DatabasePath);

	const bool bSidecarCreated =
		WriteSQLUISamplePersistenceStatusDisplayRowsSidecar(
			Result.SidecarDatabasePath + TEXT("-wal"),
			Result);
	const FSQLUILayoutRepositoryRuntimeConfig SidecarConfig =
		MakeSQLUISamplePersistenceStatusDisplayRowsSQLiteConfig(Result.SidecarDatabasePath);
	const TArray<FSQLUIPersistenceStatusDisplayRow> SidecarRows =
		USQLUIPersistenceStatusDisplayLibrary::GetPersistenceStatusDisplayRowsFromRuntimeConfig(
			Outer,
			SidecarConfig);
	Result.bSQLiteSidecarsRowFound =
		bSidecarCreated
		&& DoesSQLUISamplePersistenceStatusDisplayRowContainValue(
			SidecarRows,
			TEXT("SQLite sidecars"),
			TEXT("wal"))
		&& FPaths::FileExists(Result.SidecarDatabasePath + TEXT("-wal"));

	if (!Result.bSQLiteRowsGenerated
		|| !Result.bSQLitePathRowFound
		|| !Result.bSQLiteDatabaseExistsRowFound
		|| !Result.bSQLiteDatabaseSizeRowFound
		|| !Result.bSQLiteSidecarsRowFound
		|| !Result.bSQLiteSchemaRowFound
		|| !Result.bSQLiteRowsReadOnly)
	{
		AppendSQLUISamplePersistenceStatusDisplayRowsProbeError(
			Result,
			TEXT("SQLUI persistence status display rows probe failed: SQLite display rows were incomplete or not read-only."));
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISamplePersistenceStatusDisplayRowsFiles(Result)
		&& !DoesAnySQLUISamplePersistenceStatusDisplayRowsFileExist(Result);
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISamplePersistenceStatusDisplayRowsProbeError(
			Result,
			TEXT("SQLUI persistence status display rows probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bDefaultRowsGenerated
		&& Result.bDefaultBackendRowFound
		&& Result.bDefaultProviderRowFound
		&& Result.bDefaultRepositoryRowFound
		&& Result.bDefaultSQLiteRowsGraceful
		&& Result.bDefaultRowsDidNotCreateDb
		&& Result.bSQLiteDatabasePrepared
		&& Result.bSQLiteRowsGenerated
		&& Result.bSQLitePathRowFound
		&& Result.bSQLiteDatabaseExistsRowFound
		&& Result.bSQLiteDatabaseSizeRowFound
		&& Result.bSQLiteSidecarsRowFound
		&& Result.bSQLiteSchemaRowFound
		&& Result.bSQLiteRowsReadOnly
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
	FSQLUISamplePersistenceStatusSampleSurfaceProbeResult& Result,
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

FString MakeSQLUISamplePersistenceStatusSampleSurfacePath(
	const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("PersistenceStatusSampleSurface"),
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISamplePersistenceStatusSampleSurfaceFiles(
	const FString& DatabasePath)
{
	return {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};
}

bool DoesSQLUISamplePersistenceStatusSampleSurfaceFileExist(
	const FString& DatabasePath)
{
	for (const FString& Path : MakeSQLUISamplePersistenceStatusSampleSurfaceFiles(DatabasePath))
	{
		if (FPaths::FileExists(Path))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISamplePersistenceStatusSampleSurfaceFiles(
	const FString& DatabasePath,
	FSQLUISamplePersistenceStatusSampleSurfaceProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& Path : MakeSQLUISamplePersistenceStatusSampleSurfaceFiles(DatabasePath))
	{
		if (FPaths::FileExists(Path)
			&& !IFileManager::Get().Delete(*Path, false, true, true))
		{
			AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI persistence status sample surface probe failed: could not remove '%s'."),
					*Path));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISamplePersistenceStatusSampleSurfaceFiles(
	FSQLUISamplePersistenceStatusSampleSurfaceProbeResult& Result)
{
	return DeleteSQLUISamplePersistenceStatusSampleSurfaceFiles(Result.DatabasePath, Result)
		&& DeleteSQLUISamplePersistenceStatusSampleSurfaceFiles(Result.SidecarDatabasePath, Result);
}

bool DoesAnySQLUISamplePersistenceStatusSampleSurfaceFileExist(
	const FSQLUISamplePersistenceStatusSampleSurfaceProbeResult& Result)
{
	return DoesSQLUISamplePersistenceStatusSampleSurfaceFileExist(Result.DatabasePath)
		|| DoesSQLUISamplePersistenceStatusSampleSurfaceFileExist(Result.SidecarDatabasePath);
}

bool WriteSQLUISamplePersistenceStatusSampleSurfaceSidecar(
	const FString& Path,
	FSQLUISamplePersistenceStatusSampleSurfaceProbeResult& Result)
{
	const FString Directory = FPaths::GetPath(Path);
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI persistence status sample surface probe failed: could not create directory '%s'."),
				*Directory));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(TEXT("SQLUI persistence status sample surface sidecar probe"), *Path))
	{
		AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI persistence status sample surface probe failed: could not create sidecar '%s'."),
				*Path));
		return false;
	}

	return true;
}

bool DoesSQLUISamplePersistenceStatusSampleSurfaceLineStartWith(
	const TArray<FString>& Lines,
	const FString& ExpectedPrefix)
{
	for (const FString& Line : Lines)
	{
		if (Line.StartsWith(ExpectedPrefix))
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUISamplePersistenceStatusSampleSurfaceLineContain(
	const TArray<FString>& Lines,
	const FString& ExpectedText)
{
	for (const FString& Line : Lines)
	{
		if (Line.Contains(ExpectedText))
		{
			return true;
		}
	}

	return false;
}

bool AreSQLUISamplePersistenceStatusSampleSurfaceLinesEqual(
	const TArray<FString>& Left,
	const TArray<FString>& Right)
{
	if (Left.Num() != Right.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < Left.Num(); ++Index)
	{
		if (Left[Index] != Right[Index])
		{
			return false;
		}
	}

	return true;
}

bool IsSQLUISamplePersistenceStatusPresenterFunctionBlueprintCallable(
	const UClass* PresenterClass,
	const FName FunctionName)
{
	const UFunction* Function =
		PresenterClass ? PresenterClass->FindFunctionByName(FunctionName) : nullptr;
	return Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable);
}

bool IsSQLUISamplePersistenceStatusPresenterFunctionNotBlueprintPure(
	const UClass* PresenterClass,
	const FName FunctionName)
{
	const UFunction* Function =
		PresenterClass ? PresenterClass->FindFunctionByName(FunctionName) : nullptr;
	return Function && !Function->HasAnyFunctionFlags(FUNC_BlueprintPure);
}

bool IsSQLUISamplePersistenceStatusPanelAdapterFunctionBlueprintCallable(
	const UClass* AdapterClass,
	const FName FunctionName)
{
	const UFunction* Function =
		AdapterClass ? AdapterClass->FindFunctionByName(FunctionName) : nullptr;
	return Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable);
}

bool IsSQLUISamplePersistenceStatusPanelAdapterFunctionNotBlueprintPure(
	const UClass* AdapterClass,
	const FName FunctionName)
{
	const UFunction* Function =
		AdapterClass ? AdapterClass->FindFunctionByName(FunctionName) : nullptr;
	return Function && !Function->HasAnyFunctionFlags(FUNC_BlueprintPure);
}

bool IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintCallable(
	const UClass* WidgetClass,
	const FName FunctionName)
{
	const UFunction* Function =
		WidgetClass ? WidgetClass->FindFunctionByName(FunctionName) : nullptr;
	return Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable);
}

bool IsSQLUISamplePersistenceStatusPanelWidgetFunctionNotBlueprintPure(
	const UClass* WidgetClass,
	const FName FunctionName)
{
	const UFunction* Function =
		WidgetClass ? WidgetClass->FindFunctionByName(FunctionName) : nullptr;
	return Function && !Function->HasAnyFunctionFlags(FUNC_BlueprintPure);
}

bool IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintPure(
	const UClass* WidgetClass,
	const FName FunctionName)
{
	const UFunction* Function =
		WidgetClass ? WidgetClass->FindFunctionByName(FunctionName) : nullptr;
	return Function && Function->HasAnyFunctionFlags(FUNC_BlueprintPure);
}

bool IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
	const UClass* WidgetClass,
	const FName PropertyName)
{
	const FProperty* Property =
		WidgetClass ? WidgetClass->FindPropertyByName(PropertyName) : nullptr;
	return Property && Property->HasAnyPropertyFlags(CPF_BlueprintVisible);
}

bool IsSQLUISamplePersistenceStatusRefreshResultReflected()
{
	const UScriptStruct* RefreshResultStruct =
		FSQLUISamplePersistenceStatusRefreshResult::StaticStruct();
	return RefreshResultStruct
		&& RefreshResultStruct->FindPropertyByName(
			GET_MEMBER_NAME_CHECKED(
				FSQLUISamplePersistenceStatusRefreshResult,
				bSucceeded))
		&& RefreshResultStruct->FindPropertyByName(
			GET_MEMBER_NAME_CHECKED(
				FSQLUISamplePersistenceStatusRefreshResult,
				Rows))
		&& RefreshResultStruct->FindPropertyByName(
			GET_MEMBER_NAME_CHECKED(
				FSQLUISamplePersistenceStatusRefreshResult,
				FormattedLines))
		&& RefreshResultStruct->FindPropertyByName(
			GET_MEMBER_NAME_CHECKED(
				FSQLUISamplePersistenceStatusRefreshResult,
				SummaryText));
}

FSQLUILayoutRepositoryRuntimeConfig MakeSQLUISamplePersistenceStatusSampleSurfaceSQLiteConfig(
	const FString& DatabasePath)
{
	FSQLUILayoutRepositoryRuntimeConfig Config;
	Config.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	Config.SQLiteDatabasePath = DatabasePath;
	return Config;
}

FSQLUISamplePersistenceStatusSampleSurfaceProbeResult
RunSQLUISamplePersistenceStatusSampleSurfaceProbe(UObject* Outer)
{
	FSQLUISamplePersistenceStatusSampleSurfaceProbeResult Result;
	Result.DatabasePath =
		MakeSQLUISamplePersistenceStatusSampleSurfacePath(TEXT("PersistenceStatusSampleSurface.db"));
	Result.SidecarDatabasePath =
		MakeSQLUISamplePersistenceStatusSampleSurfacePath(TEXT("PersistenceStatusSampleSurfaceSidecarOnly.db"));

	DeleteSQLUISamplePersistenceStatusSampleSurfaceFiles(Result);

	USQLUISamplePersistenceStatusPresenter* Presenter =
		NewObject<USQLUISamplePersistenceStatusPresenter>(Outer);
	Result.bPresenterCreated = IsValid(Presenter);
	if (!Result.bPresenterCreated)
	{
		AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
			Result,
			TEXT("SQLUI persistence status sample surface probe failed: presenter was not created."));
		Result.bDatabaseFilesRemoved =
			DeleteSQLUISamplePersistenceStatusSampleSurfaceFiles(Result)
			&& !DoesAnySQLUISamplePersistenceStatusSampleSurfaceFileExist(Result);
		return Result;
	}

	USQLUISamplePersistenceStatusPanelAdapter* PanelAdapter =
		NewObject<USQLUISamplePersistenceStatusPanelAdapter>(Outer);
	Result.bPanelAdapterCreated = IsValid(PanelAdapter);
	if (!Result.bPanelAdapterCreated)
	{
		AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
			Result,
			TEXT("SQLUI persistence status sample surface probe failed: panel adapter was not created."));
	}

	const UClass* PresenterClass = Presenter->GetClass();
	const UClass* PanelWidgetClass =
		USQLUISamplePersistenceStatusPanelWidget::StaticClass();
	Result.bPanelWidgetClassDerivedFromUserWidget =
		PanelWidgetClass && PanelWidgetClass->IsChildOf(UUserWidget::StaticClass());
	Result.bBlueprintRefreshFunctionCallable =
		IsSQLUISamplePersistenceStatusPresenterFunctionBlueprintCallable(
			PresenterClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPresenter,
				RefreshPersistenceStatus));
	Result.bBlueprintRuntimeConfigRefreshFunctionCallable =
		IsSQLUISamplePersistenceStatusPresenterFunctionBlueprintCallable(
			PresenterClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPresenter,
				RefreshPersistenceStatusFromRuntimeConfig));
	Result.bPresenterRefreshFunctionsNotBlueprintPure =
		IsSQLUISamplePersistenceStatusPresenterFunctionNotBlueprintPure(
			PresenterClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPresenter,
				RefreshPersistenceStatus))
		&& IsSQLUISamplePersistenceStatusPresenterFunctionNotBlueprintPure(
			PresenterClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPresenter,
				RefreshPersistenceStatusFromRuntimeConfig));
	Result.bBlueprintRefreshResultReflected =
		IsSQLUISamplePersistenceStatusRefreshResultReflected();
	const UClass* PanelAdapterClass =
		PanelAdapter ? PanelAdapter->GetClass() : nullptr;
	Result.bPanelAdapterBlueprintRefreshFunctionCallable =
		IsSQLUISamplePersistenceStatusPanelAdapterFunctionBlueprintCallable(
			PanelAdapterClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelAdapter,
				RefreshPersistenceStatusPanel));
	Result.bPanelAdapterBlueprintRuntimeConfigRefreshFunctionCallable =
		IsSQLUISamplePersistenceStatusPanelAdapterFunctionBlueprintCallable(
			PanelAdapterClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelAdapter,
				RefreshPersistenceStatusPanelFromRuntimeConfig));
	Result.bPanelAdapterRefreshFunctionsNotBlueprintPure =
		IsSQLUISamplePersistenceStatusPanelAdapterFunctionNotBlueprintPure(
			PanelAdapterClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelAdapter,
				RefreshPersistenceStatusPanel))
		&& IsSQLUISamplePersistenceStatusPanelAdapterFunctionNotBlueprintPure(
			PanelAdapterClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelAdapter,
				RefreshPersistenceStatusPanelFromRuntimeConfig));
	Result.bPanelWidgetBlueprintRefreshFunctionCallable =
		IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintCallable(
			PanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelWidget,
				RefreshPersistenceStatusPanel));
	Result.bPanelWidgetBlueprintRuntimeConfigRefreshFunctionCallable =
		IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintCallable(
			PanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelWidget,
				RefreshPersistenceStatusPanelFromRuntimeConfig));
	Result.bPanelWidgetRefreshFunctionsNotBlueprintPure =
		IsSQLUISamplePersistenceStatusPanelWidgetFunctionNotBlueprintPure(
			PanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelWidget,
				RefreshPersistenceStatusPanel))
		&& IsSQLUISamplePersistenceStatusPanelWidgetFunctionNotBlueprintPure(
			PanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelWidget,
				RefreshPersistenceStatusPanelFromRuntimeConfig));
	Result.bPanelWidgetCachedGetterFunctionsBlueprintPure =
		IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintPure(
			PanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelWidget,
				GetRows))
		&& IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintPure(
			PanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelWidget,
				GetFormattedLines))
		&& IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintPure(
			PanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelWidget,
				GetLastRefreshResult))
		&& IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintPure(
			PanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceStatusPanelWidget,
				GetSummaryText));
	Result.bPanelWidgetRowsPropertyBlueprintVisible =
		IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			PanelWidgetClass,
			TEXT("Rows"));
	Result.bPanelWidgetFormattedLinesPropertyBlueprintVisible =
		IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			PanelWidgetClass,
			TEXT("FormattedLines"));
	Result.bPanelWidgetRefreshResultPropertyBlueprintVisible =
		IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			PanelWidgetClass,
			TEXT("LastRefreshResult"));
	Result.bPanelWidgetSummaryTextPropertyBlueprintVisible =
		IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			PanelWidgetClass,
			TEXT("SummaryText"));
	// Reflection keeps the commandlet proof independent from widget blueprint
	// assets, maps, viewport attachment, and startup wiring.
	Result.bPanelWidgetContractValidatedWithoutAssetOrViewport =
		PanelWidgetClass
		&& Result.bPanelWidgetClassDerivedFromUserWidget
		&& Result.bPanelWidgetBlueprintRefreshFunctionCallable
		&& Result.bPanelWidgetBlueprintRuntimeConfigRefreshFunctionCallable
		&& Result.bPanelWidgetRefreshFunctionsNotBlueprintPure
		&& Result.bPanelWidgetCachedGetterFunctionsBlueprintPure
		&& Result.bPanelWidgetRowsPropertyBlueprintVisible
		&& Result.bPanelWidgetFormattedLinesPropertyBlueprintVisible
		&& Result.bPanelWidgetRefreshResultPropertyBlueprintVisible
		&& Result.bPanelWidgetSummaryTextPropertyBlueprintVisible;

	const auto AppendReflectionFailure =
		[&Result](const bool bPassed, const TCHAR* FailureMessage)
		{
			if (!bPassed)
			{
				AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
					Result,
					FailureMessage);
			}
		};
	AppendReflectionFailure(
		Result.bBlueprintRefreshFunctionCallable,
		TEXT("SQLUI persistence status sample surface probe failed: presenter RefreshPersistenceStatus was not BlueprintCallable."));
	AppendReflectionFailure(
		Result.bBlueprintRuntimeConfigRefreshFunctionCallable,
		TEXT("SQLUI persistence status sample surface probe failed: presenter RefreshPersistenceStatusFromRuntimeConfig was not BlueprintCallable."));
	AppendReflectionFailure(
		Result.bPresenterRefreshFunctionsNotBlueprintPure,
		TEXT("SQLUI persistence status sample surface probe failed: presenter refresh functions were unexpectedly BlueprintPure."));
	AppendReflectionFailure(
		Result.bPanelAdapterBlueprintRefreshFunctionCallable,
		TEXT("SQLUI persistence status sample surface probe failed: panel adapter RefreshPersistenceStatusPanel was not BlueprintCallable."));
	AppendReflectionFailure(
		Result.bPanelAdapterBlueprintRuntimeConfigRefreshFunctionCallable,
		TEXT("SQLUI persistence status sample surface probe failed: panel adapter RefreshPersistenceStatusPanelFromRuntimeConfig was not BlueprintCallable."));
	AppendReflectionFailure(
		Result.bPanelAdapterRefreshFunctionsNotBlueprintPure,
		TEXT("SQLUI persistence status sample surface probe failed: panel adapter refresh functions were unexpectedly BlueprintPure."));
	AppendReflectionFailure(
		Result.bPanelWidgetClassDerivedFromUserWidget,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget shell did not derive from UUserWidget."));
	AppendReflectionFailure(
		Result.bPanelWidgetBlueprintRefreshFunctionCallable,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget RefreshPersistenceStatusPanel was not BlueprintCallable."));
	AppendReflectionFailure(
		Result.bPanelWidgetBlueprintRuntimeConfigRefreshFunctionCallable,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget RefreshPersistenceStatusPanelFromRuntimeConfig was not BlueprintCallable."));
	AppendReflectionFailure(
		Result.bPanelWidgetRefreshFunctionsNotBlueprintPure,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget refresh functions were unexpectedly BlueprintPure."));
	AppendReflectionFailure(
		Result.bPanelWidgetCachedGetterFunctionsBlueprintPure,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget cached getter functions were not BlueprintPure."));
	AppendReflectionFailure(
		Result.bPanelWidgetRowsPropertyBlueprintVisible,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget Rows property was not Blueprint-visible."));
	AppendReflectionFailure(
		Result.bPanelWidgetFormattedLinesPropertyBlueprintVisible,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget FormattedLines property was not Blueprint-visible."));
	AppendReflectionFailure(
		Result.bPanelWidgetRefreshResultPropertyBlueprintVisible,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget LastRefreshResult property was not Blueprint-visible."));
	AppendReflectionFailure(
		Result.bPanelWidgetSummaryTextPropertyBlueprintVisible,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget SummaryText property was not Blueprint-visible."));
	AppendReflectionFailure(
		Result.bPanelWidgetContractValidatedWithoutAssetOrViewport,
		TEXT("SQLUI persistence status sample surface probe failed: panel widget shell contract could not be validated by reflection without an asset or viewport."));
	AppendReflectionFailure(
		Result.bBlueprintRefreshResultReflected,
		TEXT("SQLUI persistence status sample surface probe failed: refresh result struct was not reflected as expected."));

	const FSQLUILayoutRepositoryRuntimeConfig DefaultConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
	const FSQLUISamplePersistenceStatusRefreshResult DefaultRefresh =
		Presenter->RefreshPersistenceStatusFromRuntimeConfig(Outer, DefaultConfig);
	const TArray<FSQLUIPersistenceStatusDisplayRow> DefaultRows =
		Presenter->GetRows();
	const TArray<FString> DefaultLines = Presenter->GetFormattedLines();
	Result.bExplicitRefreshResultSucceeded =
		DefaultRefresh.bSucceeded
		&& DefaultRefresh.Rows.Num() == DefaultRows.Num()
		&& DefaultRefresh.FormattedLines.Num() == DefaultLines.Num()
		&& DefaultRefresh.SummaryText.Contains(TEXT("Refreshed"));
	const FSQLUISamplePersistenceStatusRefreshResult PanelAdapterRefresh =
		PanelAdapter
			? PanelAdapter->RefreshPersistenceStatusPanelFromRuntimeConfig(
				Outer,
				DefaultConfig)
			: FSQLUISamplePersistenceStatusRefreshResult();
	const TArray<FSQLUIPersistenceStatusDisplayRow> PanelAdapterRows =
		PanelAdapter ? PanelAdapter->GetRows() : TArray<FSQLUIPersistenceStatusDisplayRow>();
	const TArray<FString> PanelAdapterLines =
		PanelAdapter ? PanelAdapter->GetFormattedLines() : TArray<FString>();
	const FSQLUISamplePersistenceStatusRefreshResult PanelAdapterLastRefresh =
		PanelAdapter
			? PanelAdapter->GetLastRefreshResult()
			: FSQLUISamplePersistenceStatusRefreshResult();
	Result.bPanelAdapterRefreshSucceeded =
		PanelAdapter
		&& PanelAdapterRefresh.bSucceeded
		&& PanelAdapterRefresh.Rows.Num() == PanelAdapterRows.Num()
		&& PanelAdapterRefresh.FormattedLines.Num() == PanelAdapterLines.Num()
		&& PanelAdapterLastRefresh.Rows.Num() == PanelAdapterRows.Num()
		&& PanelAdapterLastRefresh.FormattedLines.Num() == PanelAdapterLines.Num()
		&& PanelAdapter->GetSummaryText() == PanelAdapterRefresh.SummaryText
		&& PanelAdapterRefresh.SummaryText.Contains(TEXT("Refreshed"));
	Result.bPanelAdapterRowsMatchedPresenter =
		AreSQLUISamplePersistenceStatusSampleSurfaceLinesEqual(
			DefaultLines,
			PanelAdapterLines);
	Result.bPanelAdapterDidNotCreateDb =
		!DoesAnySQLUISamplePersistenceStatusSampleSurfaceFileExist(Result);
	Result.bDefaultRowsPresented = DefaultRows.Num() >= 8;
	Result.bDefaultFormattedLinesGenerated =
		DefaultLines.Num() == DefaultRows.Num()
		&& DefaultLines.Num() > 0;
	Result.bDefaultBackendLineFound =
		DoesSQLUISamplePersistenceStatusSampleSurfaceLineStartWith(
			DefaultLines,
			TEXT("Backend: InMemory"));
	Result.bDefaultProviderLineFound =
		DoesSQLUISamplePersistenceStatusSampleSurfaceLineStartWith(
			DefaultLines,
			TEXT("Provider initialized: No"));
	Result.bDefaultRepositoryLineFound =
		DoesSQLUISamplePersistenceStatusSampleSurfaceLineStartWith(
			DefaultLines,
			TEXT("Repository active: No"));
	Result.bDefaultSQLiteRowsGraceful =
		DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			DefaultRows,
			TEXT("SQLite database path"),
			TEXT("Not applicable"))
		&& DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			DefaultRows,
			TEXT("SQLite database exists"),
			TEXT("Not applicable"))
		&& DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			DefaultRows,
			TEXT("Schema status"),
			TEXT("Not applicable"));
	Result.bDefaultSurfaceDidNotCreateDb =
		!DoesAnySQLUISamplePersistenceStatusSampleSurfaceFileExist(Result);

	const FSQLUISamplePersistenceStatusRefreshResult RepeatedRefresh =
		Presenter->RefreshPersistenceStatusFromRuntimeConfig(Outer, DefaultConfig);
	const TArray<FString> RepeatedLines = Presenter->GetFormattedLines();
	Result.bRepeatedRefreshSucceeded =
		RepeatedRefresh.bSucceeded
		&& RepeatedRefresh.Rows.Num() == DefaultRows.Num()
		&& RepeatedRefresh.FormattedLines.Num() == DefaultLines.Num();
	Result.bRepeatedRefreshDeterministic =
		AreSQLUISamplePersistenceStatusSampleSurfaceLinesEqual(
			DefaultLines,
			RepeatedLines);
	Result.bRepeatedRefreshDidNotCreateDb =
		!DoesAnySQLUISamplePersistenceStatusSampleSurfaceFileExist(Result);
	const FSQLUISamplePersistenceStatusRefreshResult PanelAdapterRepeatedRefresh =
		PanelAdapter
			? PanelAdapter->RefreshPersistenceStatusPanelFromRuntimeConfig(
				Outer,
				DefaultConfig)
			: FSQLUISamplePersistenceStatusRefreshResult();
	const TArray<FString> PanelAdapterRepeatedLines =
		PanelAdapter ? PanelAdapter->GetFormattedLines() : TArray<FString>();
	Result.bPanelAdapterRepeatedRefreshSucceeded =
		PanelAdapterRepeatedRefresh.bSucceeded
		&& PanelAdapterRepeatedRefresh.Rows.Num() == PanelAdapterRows.Num()
		&& PanelAdapterRepeatedRefresh.FormattedLines.Num() == PanelAdapterLines.Num();
	Result.bPanelAdapterRepeatedRefreshDeterministic =
		AreSQLUISamplePersistenceStatusSampleSurfaceLinesEqual(
			PanelAdapterLines,
			PanelAdapterRepeatedLines);

	if (!Result.bDefaultRowsPresented
		|| !Result.bExplicitRefreshResultSucceeded
		|| !Result.bPanelAdapterRefreshSucceeded
		|| !Result.bPanelAdapterRowsMatchedPresenter
		|| !Result.bPanelAdapterDidNotCreateDb
		|| !Result.bDefaultFormattedLinesGenerated
		|| !Result.bDefaultBackendLineFound
		|| !Result.bDefaultProviderLineFound
		|| !Result.bDefaultRepositoryLineFound
		|| !Result.bDefaultSQLiteRowsGraceful
		|| !Result.bDefaultSurfaceDidNotCreateDb
		|| !Result.bRepeatedRefreshSucceeded
		|| !Result.bRepeatedRefreshDeterministic
		|| !Result.bRepeatedRefreshDidNotCreateDb
		|| !Result.bPanelAdapterRepeatedRefreshSucceeded
		|| !Result.bPanelAdapterRepeatedRefreshDeterministic)
	{
		AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
			Result,
			TEXT("SQLUI persistence status sample surface probe failed: default presenter or panel adapter refresh rows were not safe/read-only."));
	}

	const FSQLUILayoutRepositoryRuntimeConfig MissingSQLiteConfig =
		MakeSQLUISamplePersistenceStatusSampleSurfaceSQLiteConfig(Result.DatabasePath);
	Presenter->RefreshPersistenceStatusFromRuntimeConfig(Outer, MissingSQLiteConfig);
	const TArray<FSQLUIPersistenceStatusDisplayRow> MissingSQLiteRows =
		Presenter->GetRows();
	const TArray<FString> MissingSQLiteLines = Presenter->GetFormattedLines();
	Result.bMissingSQLiteRowsPresented = MissingSQLiteRows.Num() >= 8;
	Result.bMissingSQLitePathLineFound =
		DoesSQLUISamplePersistenceStatusSampleSurfaceLineContain(
			MissingSQLiteLines,
			Result.DatabasePath);
	Result.bMissingSQLiteDatabaseAbsentLineFound =
		DoesSQLUISamplePersistenceStatusDisplayRowHaveValue(
			MissingSQLiteRows,
			TEXT("SQLite database exists"),
			TEXT("No"));
	Result.bMissingSQLiteSurfaceDidNotCreateDb =
		!DoesSQLUISamplePersistenceStatusSampleSurfaceFileExist(Result.DatabasePath);

	if (!Result.bMissingSQLiteRowsPresented
		|| !Result.bMissingSQLitePathLineFound
		|| !Result.bMissingSQLiteDatabaseAbsentLineFound
		|| !Result.bMissingSQLiteSurfaceDidNotCreateDb)
	{
		AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
			Result,
			TEXT("SQLUI persistence status sample surface probe failed: missing SQLite database rows were not graceful/read-only."));
	}

	const FString SidecarPath = Result.SidecarDatabasePath + TEXT("-wal");
	const bool bSidecarCreated =
		WriteSQLUISamplePersistenceStatusSampleSurfaceSidecar(SidecarPath, Result);
	const FSQLUILayoutRepositoryRuntimeConfig SidecarConfig =
		MakeSQLUISamplePersistenceStatusSampleSurfaceSQLiteConfig(Result.SidecarDatabasePath);
	Presenter->RefreshPersistenceStatusFromRuntimeConfig(Outer, SidecarConfig);
	Result.bSidecarStillPresentAfterRefresh =
		bSidecarCreated
		&& FPaths::FileExists(SidecarPath)
		&& !FPaths::FileExists(Result.SidecarDatabasePath)
		&& DoesSQLUISamplePersistenceStatusSampleSurfaceLineContain(
			Presenter->GetFormattedLines(),
			TEXT("wal"));
	if (!Result.bSidecarStillPresentAfterRefresh)
	{
		AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
			Result,
			TEXT("SQLUI persistence status sample surface probe failed: presenter refresh deleted or hid a smoke-owned sidecar."));
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISamplePersistenceStatusSampleSurfaceFiles(Result)
		&& !DoesAnySQLUISamplePersistenceStatusSampleSurfaceFileExist(Result);
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISamplePersistenceStatusSampleSurfaceProbeError(
			Result,
			TEXT("SQLUI persistence status sample surface probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bPresenterCreated
		&& Result.bPanelAdapterCreated
		&& Result.bPanelWidgetClassDerivedFromUserWidget
		&& Result.bBlueprintRefreshFunctionCallable
		&& Result.bBlueprintRuntimeConfigRefreshFunctionCallable
		&& Result.bPresenterRefreshFunctionsNotBlueprintPure
		&& Result.bPanelAdapterBlueprintRefreshFunctionCallable
		&& Result.bPanelAdapterBlueprintRuntimeConfigRefreshFunctionCallable
		&& Result.bPanelAdapterRefreshFunctionsNotBlueprintPure
		&& Result.bPanelWidgetBlueprintRefreshFunctionCallable
		&& Result.bPanelWidgetBlueprintRuntimeConfigRefreshFunctionCallable
		&& Result.bPanelWidgetRefreshFunctionsNotBlueprintPure
		&& Result.bPanelWidgetCachedGetterFunctionsBlueprintPure
		&& Result.bPanelWidgetRowsPropertyBlueprintVisible
		&& Result.bPanelWidgetFormattedLinesPropertyBlueprintVisible
		&& Result.bPanelWidgetRefreshResultPropertyBlueprintVisible
		&& Result.bPanelWidgetSummaryTextPropertyBlueprintVisible
		&& Result.bPanelWidgetContractValidatedWithoutAssetOrViewport
		&& Result.bBlueprintRefreshResultReflected
		&& Result.bDefaultRowsPresented
		&& Result.bExplicitRefreshResultSucceeded
		&& Result.bPanelAdapterRefreshSucceeded
		&& Result.bPanelAdapterRowsMatchedPresenter
		&& Result.bPanelAdapterDidNotCreateDb
		&& Result.bDefaultFormattedLinesGenerated
		&& Result.bDefaultBackendLineFound
		&& Result.bDefaultProviderLineFound
		&& Result.bDefaultRepositoryLineFound
		&& Result.bDefaultSQLiteRowsGraceful
		&& Result.bDefaultSurfaceDidNotCreateDb
		&& Result.bRepeatedRefreshSucceeded
		&& Result.bRepeatedRefreshDeterministic
		&& Result.bRepeatedRefreshDidNotCreateDb
		&& Result.bPanelAdapterRepeatedRefreshSucceeded
		&& Result.bPanelAdapterRepeatedRefreshDeterministic
		&& Result.bMissingSQLiteRowsPresented
		&& Result.bMissingSQLitePathLineFound
		&& Result.bMissingSQLiteDatabaseAbsentLineFound
		&& Result.bMissingSQLiteSurfaceDidNotCreateDb
		&& Result.bSidecarStillPresentAfterRefresh
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISamplePersistenceSettingsDraftProbeError(
	FSQLUISamplePersistenceSettingsDraftProbeResult& Result,
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

FString MakeSQLUISamplePersistenceSettingsDraftPath(
	const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("PersistenceSettingsDraft"),
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISamplePersistenceSettingsDraftFiles(
	const FString& DatabasePath)
{
	return {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};
}

bool DoesSQLUISamplePersistenceSettingsDraftFileExist(
	const FString& DatabasePath)
{
	for (const FString& Path : MakeSQLUISamplePersistenceSettingsDraftFiles(DatabasePath))
	{
		if (FPaths::FileExists(Path))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISamplePersistenceSettingsDraftFiles(
	const FString& DatabasePath,
	FSQLUISamplePersistenceSettingsDraftProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& Path : MakeSQLUISamplePersistenceSettingsDraftFiles(DatabasePath))
	{
		if (FPaths::FileExists(Path)
			&& !IFileManager::Get().Delete(*Path, false, true, true))
		{
			AppendSQLUISamplePersistenceSettingsDraftProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI persistence settings draft probe failed: could not remove '%s'."),
					*Path));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISamplePersistenceSettingsDraftFiles(
	FSQLUISamplePersistenceSettingsDraftProbeResult& Result)
{
	return DeleteSQLUISamplePersistenceSettingsDraftFiles(Result.DatabasePath, Result)
		&& DeleteSQLUISamplePersistenceSettingsDraftFiles(Result.SidecarDatabasePath, Result);
}

bool DoesAnySQLUISamplePersistenceSettingsDraftFileExist(
	const FSQLUISamplePersistenceSettingsDraftProbeResult& Result)
{
	return DoesSQLUISamplePersistenceSettingsDraftFileExist(Result.DatabasePath)
		|| DoesSQLUISamplePersistenceSettingsDraftFileExist(Result.SidecarDatabasePath);
}

bool WriteSQLUISamplePersistenceSettingsDraftSidecar(
	const FString& Path,
	FSQLUISamplePersistenceSettingsDraftProbeResult& Result)
{
	const FString Directory = FPaths::GetPath(Path);
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI persistence settings draft probe failed: could not create directory '%s'."),
				*Directory));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(TEXT("SQLUI persistence settings draft sidecar probe"), *Path))
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI persistence settings draft probe failed: could not create sidecar '%s'."),
				*Path));
		return false;
	}

	return true;
}

bool DoesSQLUIPersistenceSettingsValidationHaveSeverity(
	const FSQLUIPersistenceSettingsValidationResult& Validation,
	const ESQLUIPersistenceSettingsValidationMessageSeverity Severity)
{
	for (const FSQLUIPersistenceSettingsValidationMessage& Message : Validation.Messages)
	{
		if (Message.Severity == Severity)
		{
			return true;
		}
	}

	return false;
}

bool AreSQLUIPersistenceSettingsValidationResultsEquivalent(
	const FSQLUIPersistenceSettingsValidationResult& First,
	const FSQLUIPersistenceSettingsValidationResult& Second)
{
	return First.bIsValid == Second.bIsValid
		&& First.bWouldChangeBackend == Second.bWouldChangeBackend
		&& First.bWouldChangeSQLitePath == Second.bWouldChangeSQLitePath
		&& First.bWouldChangeProviderAutoInitialize == Second.bWouldChangeProviderAutoInitialize
		&& First.bRequiresRestartOrReinitialize == Second.bRequiresRestartOrReinitialize
		&& First.bSQLitePathResolved == Second.bSQLitePathResolved
		&& First.ResolvedSQLiteDatabasePath == Second.ResolvedSQLiteDatabasePath
		&& First.Messages.Num() == Second.Messages.Num()
		&& First.SummaryText == Second.SummaryText;
}

bool AreSQLUIPersistenceSettingsValidationMessagesEquivalent(
	const FSQLUIPersistenceSettingsValidationMessage& First,
	const FSQLUIPersistenceSettingsValidationMessage& Second)
{
	return First.Severity == Second.Severity
		&& First.Message == Second.Message
		&& First.DetailText == Second.DetailText;
}

bool AreSQLUIPersistenceSettingsApplyPreviewsEquivalent(
	const FSQLUIPersistenceSettingsApplyPreviewResult& First,
	const FSQLUIPersistenceSettingsApplyPreviewResult& Second)
{
	if (First.bCanApplyInFuture != Second.bCanApplyInFuture
		|| First.bIsValid != Second.bIsValid
		|| First.bHasChanges != Second.bHasChanges
		|| First.bWouldChangeBackend != Second.bWouldChangeBackend
		|| First.bWouldChangeSQLitePath != Second.bWouldChangeSQLitePath
		|| First.bWouldChangeSQLiteConfig != Second.bWouldChangeSQLiteConfig
		|| First.bWouldChangeProviderAutoInitialize
			!= Second.bWouldChangeProviderAutoInitialize
		|| First.bRequiresRestartOrReinitialize
			!= Second.bRequiresRestartOrReinitialize
		|| First.bWouldNeedProviderReinitialize
			!= Second.bWouldNeedProviderReinitialize
		|| First.bWouldNeedRepositoryReopen
			!= Second.bWouldNeedRepositoryReopen
		|| First.Messages.Num() != Second.Messages.Num()
		|| First.SummaryText != Second.SummaryText
		|| !AreSQLUIPersistenceSettingsValidationResultsEquivalent(
			First.ValidationResult,
			Second.ValidationResult))
	{
		return false;
	}

	for (int32 Index = 0; Index < First.Messages.Num(); ++Index)
	{
		if (!AreSQLUIPersistenceSettingsValidationMessagesEquivalent(
			First.Messages[Index],
			Second.Messages[Index]))
		{
			return false;
		}
	}

	return true;
}

bool DoesSQLUIPersistenceSettingsApplyPreviewContainText(
	const FSQLUIPersistenceSettingsApplyPreviewResult& Preview,
	const FString& ExpectedText)
{
	if (Preview.SummaryText.Contains(ExpectedText))
	{
		return true;
	}

	for (const FSQLUIPersistenceSettingsValidationMessage& Message : Preview.Messages)
	{
		if (Message.Message.Contains(ExpectedText)
			|| Message.DetailText.Contains(ExpectedText))
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUIPersistenceSettingsValidationDisplayContainField(
	const FSQLUIPersistenceSettingsValidationDisplaySummary& Display,
	const FName FieldKey)
{
	for (const FSQLUIPersistenceSettingsValidationDisplayRow& Row : Display.Rows)
	{
		if (Row.FieldKey == FieldKey)
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUIPersistenceSettingsValidationDisplayContainState(
	const FSQLUIPersistenceSettingsValidationDisplaySummary& Display,
	const ESQLUIPersistenceSettingsValidationDisplayState State)
{
	for (const FSQLUIPersistenceSettingsValidationDisplayRow& Row : Display.Rows)
	{
		if (Row.State == State)
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUIPersistenceSettingsValidationDisplayContainText(
	const FSQLUIPersistenceSettingsValidationDisplaySummary& Display,
	const FString& ExpectedText)
{
	for (const FSQLUIPersistenceSettingsValidationDisplayRow& Row : Display.Rows)
	{
		if (Row.Label.ToString().Contains(ExpectedText)
			|| Row.Value.ToString().Contains(ExpectedText)
			|| Row.DetailText.ToString().Contains(ExpectedText))
		{
			return true;
		}
	}

	return Display.SummaryText.ToString().Contains(ExpectedText);
}

bool AreSQLUIPersistenceSettingsValidationDisplayRowsEquivalent(
	const FSQLUIPersistenceSettingsValidationDisplayRow& First,
	const FSQLUIPersistenceSettingsValidationDisplayRow& Second)
{
	return First.FieldKey == Second.FieldKey
		&& First.Label.ToString() == Second.Label.ToString()
		&& First.Value.ToString() == Second.Value.ToString()
		&& First.State == Second.State
		&& First.DetailText.ToString() == Second.DetailText.ToString();
}

bool AreSQLUIPersistenceSettingsValidationDisplaysEquivalent(
	const FSQLUIPersistenceSettingsValidationDisplaySummary& First,
	const FSQLUIPersistenceSettingsValidationDisplaySummary& Second)
{
	if (First.bIsValid != Second.bIsValid
		|| First.bHasErrors != Second.bHasErrors
		|| First.bHasWarnings != Second.bHasWarnings
		|| First.bRequiresRestartOrReinitialize != Second.bRequiresRestartOrReinitialize
		|| First.SummaryText.ToString() != Second.SummaryText.ToString()
		|| First.Rows.Num() != Second.Rows.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < First.Rows.Num(); ++Index)
	{
		if (!AreSQLUIPersistenceSettingsValidationDisplayRowsEquivalent(
			First.Rows[Index],
			Second.Rows[Index]))
		{
			return false;
		}
	}

	return true;
}

bool DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainField(
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary& Display,
	const FName FieldKey)
{
	for (const FSQLUIPersistenceSettingsApplyPreviewDisplayRow& Row :
		Display.Rows)
	{
		if (Row.FieldKey == FieldKey)
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainState(
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary& Display,
	const ESQLUIPersistenceSettingsValidationDisplayState State)
{
	for (const FSQLUIPersistenceSettingsApplyPreviewDisplayRow& Row :
		Display.Rows)
	{
		if (Row.State == State)
		{
			return true;
		}
	}

	return false;
}

bool DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary& Display,
	const FString& ExpectedText)
{
	for (const FSQLUIPersistenceSettingsApplyPreviewDisplayRow& Row :
		Display.Rows)
	{
		if (Row.Label.ToString().Contains(ExpectedText)
			|| Row.Value.ToString().Contains(ExpectedText)
			|| Row.DetailText.ToString().Contains(ExpectedText))
		{
			return true;
		}
	}

	return Display.SummaryText.ToString().Contains(ExpectedText);
}

bool AreSQLUIPersistenceSettingsApplyPreviewDisplayRowsEquivalent(
	const FSQLUIPersistenceSettingsApplyPreviewDisplayRow& First,
	const FSQLUIPersistenceSettingsApplyPreviewDisplayRow& Second)
{
	return First.FieldKey == Second.FieldKey
		&& First.Label.ToString() == Second.Label.ToString()
		&& First.Value.ToString() == Second.Value.ToString()
		&& First.State == Second.State
		&& First.DetailText.ToString() == Second.DetailText.ToString();
}

bool AreSQLUIPersistenceSettingsApplyPreviewDisplaysEquivalent(
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary& First,
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary& Second)
{
	if (First.bCanApplyInFuture != Second.bCanApplyInFuture
		|| First.bIsValid != Second.bIsValid
		|| First.bHasChanges != Second.bHasChanges
		|| First.bHasErrors != Second.bHasErrors
		|| First.bHasWarnings != Second.bHasWarnings
		|| First.bRequiresRestartOrReinitialize
			!= Second.bRequiresRestartOrReinitialize
		|| First.bWouldNeedProviderReinitialize
			!= Second.bWouldNeedProviderReinitialize
		|| First.bWouldNeedRepositoryReopen != Second.bWouldNeedRepositoryReopen
		|| First.SummaryText.ToString() != Second.SummaryText.ToString()
		|| First.Rows.Num() != Second.Rows.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < First.Rows.Num(); ++Index)
	{
		if (!AreSQLUIPersistenceSettingsApplyPreviewDisplayRowsEquivalent(
			First.Rows[Index],
			Second.Rows[Index]))
		{
			return false;
		}
	}

	return true;
}

FSQLUISamplePersistenceSettingsDraftProbeResult
RunSQLUISamplePersistenceSettingsDraftProbe(UObject* Outer)
{
	FSQLUISamplePersistenceSettingsDraftProbeResult Result;
	Result.DatabasePath =
		MakeSQLUISamplePersistenceSettingsDraftPath(TEXT("PersistenceSettingsDraft.db"));
	Result.SidecarDatabasePath =
		MakeSQLUISamplePersistenceSettingsDraftPath(TEXT("PersistenceSettingsDraftSidecarOnly.db"));

	DeleteSQLUISamplePersistenceSettingsDraftFiles(Result);

	const USQLUILayoutRepositoryRuntimeSettings* DefaultSettings =
		GetDefault<USQLUILayoutRepositoryRuntimeSettings>();
	const bool bAutoInitBefore =
		FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
			DefaultSettings,
			TEXT(""));

	const FSQLUIPersistenceSettingsDraft DefaultDraft =
		USQLUIPersistenceSettingsDraftLibrary::MakeDefaultPersistenceSettingsDraft();
	Result.bDefaultDraftCreated =
		DefaultDraft.CurrentRuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::InMemory
		&& DefaultDraft.PendingRuntimeConfig.Backend == ESQLUILayoutRepositoryBackend::InMemory
		&& !DefaultDraft.bCurrentProviderAutoInitialize
		&& !DefaultDraft.bPendingProviderAutoInitialize;

	const FSQLUIPersistenceSettingsValidationResult DefaultValidation =
		USQLUIPersistenceSettingsDraftLibrary::ValidatePersistenceSettingsDraft(DefaultDraft);
	const FSQLUIPersistenceSettingsValidationDisplaySummary DefaultDisplay =
		USQLUIPersistenceSettingsDraftDisplayLibrary::
			MakePersistenceSettingsValidationDisplay(DefaultDraft, DefaultValidation);
	const FSQLUIPersistenceSettingsApplyPreviewResult DefaultApplyPreview =
		USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
			DefaultDraft);
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
		DefaultApplyPreviewDisplay =
			USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
				PreviewAndMakePersistenceSettingsApplyPreviewDisplay(DefaultDraft);
	Result.bDefaultDraftValidated = DefaultValidation.bIsValid;
	Result.bDefaultInMemorySafe =
		DefaultValidation.bIsValid
		&& !DefaultValidation.bWouldChangeBackend
		&& !DefaultValidation.bWouldChangeSQLitePath
		&& !DefaultValidation.bWouldChangeProviderAutoInitialize
		&& !DefaultValidation.bRequiresRestartOrReinitialize
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bDefaultDisplayGenerated =
		DefaultDisplay.Rows.Num() > 0
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainField(
			DefaultDisplay,
			TEXT("Backend"))
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainField(
			DefaultDisplay,
			TEXT("Summary"));
	Result.bDefaultDisplaySafe =
		DefaultDisplay.bIsValid
		&& !DefaultDisplay.bHasErrors
		&& !DefaultDisplay.bHasWarnings
		&& !DefaultDisplay.bRequiresRestartOrReinitialize
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
			DefaultDisplay,
			TEXT("InMemory"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bDefaultApplyPreviewSafe =
		DefaultApplyPreview.bIsValid
		&& !DefaultApplyPreview.bHasChanges
		&& !DefaultApplyPreview.bCanApplyInFuture
		&& !DefaultApplyPreview.bRequiresRestartOrReinitialize
		&& DoesSQLUIPersistenceSettingsApplyPreviewContainText(
			DefaultApplyPreview,
			TEXT("No changes to apply"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bDefaultApplyPreviewDisplaySafe =
		DefaultApplyPreviewDisplay.bIsValid
		&& !DefaultApplyPreviewDisplay.bHasErrors
		&& !DefaultApplyPreviewDisplay.bHasWarnings
		&& !DefaultApplyPreviewDisplay.bCanApplyInFuture
		&& !DefaultApplyPreviewDisplay.bHasChanges
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainField(
			DefaultApplyPreviewDisplay,
			TEXT("ApplyPreviewSummary"))
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainField(
			DefaultApplyPreviewDisplay,
			TEXT("HasChanges"))
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			DefaultApplyPreviewDisplay,
			TEXT("No changes to apply"))
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			DefaultApplyPreviewDisplay,
			TEXT("No settings were applied or saved"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	if (!Result.bDefaultDraftCreated
		|| !Result.bDefaultDraftValidated
		|| !Result.bDefaultInMemorySafe
		|| !Result.bDefaultDisplayGenerated
		|| !Result.bDefaultDisplaySafe
		|| !Result.bDefaultApplyPreviewSafe
		|| !Result.bDefaultApplyPreviewDisplaySafe)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: default InMemory draft/display/preview/display-preview was not safe and valid."));
	}

	const FSQLUIPersistenceSettingsDraft CurrentDraft =
		USQLUIPersistenceSettingsDraftLibrary::MakeCurrentPersistenceSettingsDraft();
	const FSQLUIPersistenceSettingsValidationResult CurrentValidation =
		USQLUIPersistenceSettingsDraftLibrary::ValidatePersistenceSettingsDraft(CurrentDraft);
	const FSQLUIPersistenceSettingsApplyPreviewResult CurrentApplyPreview =
		USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
			CurrentDraft);
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
		CurrentApplyPreviewDisplay =
			USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
				MakePersistenceSettingsApplyPreviewDisplay(CurrentApplyPreview);
	Result.bCurrentDraftValidated =
		CurrentValidation.bIsValid
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bCurrentApplyPreviewNoChanges =
		CurrentApplyPreview.bIsValid
		&& !CurrentApplyPreview.bHasChanges
		&& !CurrentApplyPreview.bCanApplyInFuture
		&& DoesSQLUIPersistenceSettingsApplyPreviewContainText(
			CurrentApplyPreview,
			TEXT("No changes to apply"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bCurrentApplyPreviewDisplayNoChanges =
		CurrentApplyPreviewDisplay.bIsValid
		&& !CurrentApplyPreviewDisplay.bHasChanges
		&& !CurrentApplyPreviewDisplay.bCanApplyInFuture
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			CurrentApplyPreviewDisplay,
			TEXT("No changes to apply"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	if (!Result.bCurrentDraftValidated
		|| !Result.bCurrentApplyPreviewNoChanges
		|| !Result.bCurrentApplyPreviewDisplayNoChanges)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: current settings draft validation/preview/display-preview was not safe."));
	}

	FSQLUIPersistenceSettingsDraft UnknownBackendDraft = DefaultDraft;
	UnknownBackendDraft.PendingRuntimeConfig.Backend =
		static_cast<ESQLUILayoutRepositoryBackend>(255);
	UnknownBackendDraft.bHasBackendOverride = true;
	const FSQLUIPersistenceSettingsValidationResult UnknownBackendValidation =
		USQLUIPersistenceSettingsDraftLibrary::ValidatePersistenceSettingsDraft(
			UnknownBackendDraft);
	const FSQLUIPersistenceSettingsValidationDisplaySummary UnknownBackendDisplay =
		USQLUIPersistenceSettingsDraftDisplayLibrary::
			MakePersistenceSettingsValidationDisplay(
				UnknownBackendDraft,
				UnknownBackendValidation);
	const FSQLUIPersistenceSettingsApplyPreviewResult UnknownBackendApplyPreview =
		USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
			UnknownBackendDraft);
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
		UnknownBackendApplyPreviewDisplay =
			USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
				MakePersistenceSettingsApplyPreviewDisplay(
					UnknownBackendApplyPreview);
	Result.bUnknownBackendRejected =
		!UnknownBackendValidation.bIsValid
		&& DoesSQLUIPersistenceSettingsValidationHaveSeverity(
			UnknownBackendValidation,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error)
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bUnknownBackendDisplayShowsError =
		UnknownBackendDisplay.bHasErrors
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainState(
			UnknownBackendDisplay,
			ESQLUIPersistenceSettingsValidationDisplayState::Error)
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
			UnknownBackendDisplay,
			TEXT("not a selectable SQLUI persistence backend"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bUnknownBackendApplyPreviewRejected =
		!UnknownBackendApplyPreview.bIsValid
		&& !UnknownBackendApplyPreview.bCanApplyInFuture
		&& DoesSQLUIPersistenceSettingsApplyPreviewContainText(
			UnknownBackendApplyPreview,
			TEXT("Future Apply would be blocked"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bUnknownBackendApplyPreviewDisplayShowsError =
		UnknownBackendApplyPreviewDisplay.bHasErrors
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainState(
			UnknownBackendApplyPreviewDisplay,
			ESQLUIPersistenceSettingsValidationDisplayState::Error)
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			UnknownBackendApplyPreviewDisplay,
			TEXT("Future Apply would be blocked"))
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			UnknownBackendApplyPreviewDisplay,
			TEXT("not a selectable SQLUI persistence backend"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	if (!Result.bUnknownBackendRejected)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: unknown backend was not rejected without mutation."));
	}
	if (!Result.bUnknownBackendDisplayShowsError)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: unknown backend display did not show a user-readable error."));
	}
	if (!Result.bUnknownBackendApplyPreviewRejected)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: unknown backend apply preview did not reject safely."));
	}
	if (!Result.bUnknownBackendApplyPreviewDisplayShowsError)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: unknown backend apply preview display did not show a user-readable error."));
	}

	FSQLUIPersistenceSettingsDraft SQLiteDraft = DefaultDraft;
	SQLiteDraft.PendingRuntimeConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	SQLiteDraft.PendingRuntimeConfig.SQLiteDatabasePath = Result.DatabasePath;
	SQLiteDraft.bHasBackendOverride = true;
	SQLiteDraft.bHasSQLiteDatabasePathOverride = true;
	const FSQLUIPersistenceSettingsValidationResult SQLiteValidation =
		USQLUIPersistenceSettingsDraftLibrary::ValidatePersistenceSettingsDraft(SQLiteDraft);
	const FSQLUIPersistenceSettingsValidationDisplaySummary SQLiteDisplay =
		USQLUIPersistenceSettingsDraftDisplayLibrary::
			MakePersistenceSettingsValidationDisplay(SQLiteDraft, SQLiteValidation);
	const FSQLUIPersistenceSettingsApplyPreviewResult SQLiteApplyPreview =
		USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
			SQLiteDraft);
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
		SQLiteApplyPreviewDisplay =
			USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
				MakePersistenceSettingsApplyPreviewDisplay(SQLiteApplyPreview);
	Result.bSQLiteDraftRepresented =
		SQLiteValidation.bIsValid
		&& SQLiteValidation.bWouldChangeBackend
		&& SQLiteValidation.bWouldChangeSQLitePath
		&& SQLiteValidation.bRequiresRestartOrReinitialize
		&& SQLiteValidation.bSQLitePathResolved
		&& SQLiteValidation.ResolvedSQLiteDatabasePath == Result.DatabasePath;
	Result.bSQLiteDraftDisplayGenerated =
		SQLiteDisplay.bIsValid
		&& SQLiteDisplay.bRequiresRestartOrReinitialize
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainField(
			SQLiteDisplay,
			TEXT("SQLitePath"))
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
			SQLiteDisplay,
			Result.DatabasePath);
	Result.bSQLiteDraftDidNotCreateDb =
		!DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bSQLiteDisplayDidNotCreateDb =
		!DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bBackendChangeApplyPreviewDetected =
		SQLiteApplyPreview.bIsValid
		&& SQLiteApplyPreview.bCanApplyInFuture
		&& SQLiteApplyPreview.bHasChanges
		&& SQLiteApplyPreview.bWouldChangeBackend
		&& SQLiteApplyPreview.bWouldNeedRepositoryReopen
		&& SQLiteApplyPreview.bWouldNeedProviderReinitialize
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bSQLiteApplyPreviewSafe =
		SQLiteApplyPreview.bIsValid
		&& SQLiteApplyPreview.bCanApplyInFuture
		&& SQLiteApplyPreview.bWouldChangeSQLitePath
		&& SQLiteApplyPreview.bWouldChangeSQLiteConfig
		&& SQLiteApplyPreview.bRequiresRestartOrReinitialize
		&& DoesSQLUIPersistenceSettingsApplyPreviewContainText(
			SQLiteApplyPreview,
			TEXT("Not applied"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bBackendChangeApplyPreviewDisplayDetected =
		SQLiteApplyPreviewDisplay.bIsValid
		&& SQLiteApplyPreviewDisplay.bCanApplyInFuture
		&& SQLiteApplyPreviewDisplay.bHasChanges
		&& SQLiteApplyPreviewDisplay.bWouldNeedRepositoryReopen
		&& SQLiteApplyPreviewDisplay.bWouldNeedProviderReinitialize
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainField(
			SQLiteApplyPreviewDisplay,
			TEXT("BackendChange"))
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			SQLiteApplyPreviewDisplay,
			TEXT("Would change"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bSQLiteApplyPreviewDisplaySafe =
		SQLiteApplyPreviewDisplay.bIsValid
		&& SQLiteApplyPreviewDisplay.bCanApplyInFuture
		&& SQLiteApplyPreviewDisplay.bRequiresRestartOrReinitialize
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainField(
			SQLiteApplyPreviewDisplay,
			TEXT("SQLitePathOrPolicyChange"))
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			SQLiteApplyPreviewDisplay,
			TEXT("SQLite remains opt-in"))
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			SQLiteApplyPreviewDisplay,
			TEXT("Not applied"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	if (!Result.bSQLiteDraftRepresented
		|| !Result.bSQLiteDraftDisplayGenerated
		|| !Result.bSQLiteDraftDidNotCreateDb
		|| !Result.bSQLiteDisplayDidNotCreateDb
		|| !Result.bBackendChangeApplyPreviewDetected
		|| !Result.bSQLiteApplyPreviewSafe
		|| !Result.bBackendChangeApplyPreviewDisplayDetected
		|| !Result.bSQLiteApplyPreviewDisplaySafe)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: SQLite draft/display/preview/display-preview representation was not validation/preview-only."));
	}

	FSQLUIPersistenceSettingsDraft EmptySQLitePathDraft = DefaultDraft;
	EmptySQLitePathDraft.PendingRuntimeConfig.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	EmptySQLitePathDraft.PendingRuntimeConfig.SQLiteDatabasePath.Empty();
	EmptySQLitePathDraft.bHasBackendOverride = true;
	EmptySQLitePathDraft.bHasSQLiteDatabasePathOverride = true;
	const FSQLUIPersistenceSettingsValidationResult EmptySQLitePathValidation =
		USQLUIPersistenceSettingsDraftLibrary::ValidatePersistenceSettingsDraft(
			EmptySQLitePathDraft);
	const FSQLUIPersistenceSettingsValidationDisplaySummary EmptySQLitePathDisplay =
		USQLUIPersistenceSettingsDraftDisplayLibrary::
			MakePersistenceSettingsValidationDisplay(
				EmptySQLitePathDraft,
				EmptySQLitePathValidation);
	const FSQLUIPersistenceSettingsApplyPreviewResult EmptySQLitePathApplyPreview =
		USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
			EmptySQLitePathDraft);
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
		EmptySQLitePathApplyPreviewDisplay =
			USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
				MakePersistenceSettingsApplyPreviewDisplay(
					EmptySQLitePathApplyPreview);
	Result.bSQLiteEmptyPathRejected =
		!EmptySQLitePathValidation.bIsValid
		&& DoesSQLUIPersistenceSettingsValidationHaveSeverity(
			EmptySQLitePathValidation,
			ESQLUIPersistenceSettingsValidationMessageSeverity::Error)
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bSQLiteEmptyPathDisplayShowsError =
		EmptySQLitePathDisplay.bHasErrors
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainState(
			EmptySQLitePathDisplay,
			ESQLUIPersistenceSettingsValidationDisplayState::Error)
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
			EmptySQLitePathDisplay,
			TEXT("SQLite requires a database path"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bSQLiteEmptyPathApplyPreviewRejected =
		!EmptySQLitePathApplyPreview.bIsValid
		&& !EmptySQLitePathApplyPreview.bCanApplyInFuture
		&& DoesSQLUIPersistenceSettingsApplyPreviewContainText(
			EmptySQLitePathApplyPreview,
			TEXT("SQLite requires a database path"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bSQLiteEmptyPathApplyPreviewDisplayShowsError =
		EmptySQLitePathApplyPreviewDisplay.bHasErrors
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainState(
			EmptySQLitePathApplyPreviewDisplay,
			ESQLUIPersistenceSettingsValidationDisplayState::Error)
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			EmptySQLitePathApplyPreviewDisplay,
			TEXT("SQLite requires a database path"))
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			EmptySQLitePathApplyPreviewDisplay,
			TEXT("Future Apply would be blocked"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	if (!Result.bSQLiteEmptyPathRejected)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: empty SQLite path was not rejected safely."));
	}
	if (!Result.bSQLiteEmptyPathDisplayShowsError)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: empty SQLite path display did not show a user-readable error."));
	}
	if (!Result.bSQLiteEmptyPathApplyPreviewRejected)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: empty SQLite path apply preview did not reject safely."));
	}
	if (!Result.bSQLiteEmptyPathApplyPreviewDisplayShowsError)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: empty SQLite path apply preview display did not show a user-readable error."));
	}

	FSQLUIPersistenceSettingsDraft ProviderAutoInitDraft = DefaultDraft;
	ProviderAutoInitDraft.bPendingProviderAutoInitialize = true;
	ProviderAutoInitDraft.bHasProviderAutoInitializeOverride = true;
	const FSQLUIPersistenceSettingsValidationResult ProviderAutoInitValidation =
		USQLUIPersistenceSettingsDraftLibrary::ValidatePersistenceSettingsDraft(
			ProviderAutoInitDraft);
	const FSQLUIPersistenceSettingsValidationDisplaySummary ProviderAutoInitDisplay =
		USQLUIPersistenceSettingsDraftDisplayLibrary::
			MakePersistenceSettingsValidationDisplay(
				ProviderAutoInitDraft,
				ProviderAutoInitValidation);
	const FSQLUIPersistenceSettingsApplyPreviewResult ProviderAutoInitApplyPreview =
		USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
			ProviderAutoInitDraft);
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
		ProviderAutoInitApplyPreviewDisplay =
			USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
				MakePersistenceSettingsApplyPreviewDisplay(
					ProviderAutoInitApplyPreview);
	const bool bAutoInitAfter =
		FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
			DefaultSettings,
			TEXT(""));
	Result.bProviderAutoInitPendingValidated =
		ProviderAutoInitValidation.bIsValid
		&& ProviderAutoInitValidation.bWouldChangeProviderAutoInitialize
		&& ProviderAutoInitValidation.bRequiresRestartOrReinitialize
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bProviderAutoInitPolicyUnchanged =
		!bAutoInitBefore
		&& !bAutoInitAfter;
	Result.bProviderAutoInitDisplayPending =
		ProviderAutoInitDisplay.bIsValid
		&& ProviderAutoInitDisplay.bHasWarnings
		&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
			ProviderAutoInitDisplay,
			TEXT("Enabled (pending)"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bProviderAutoInitApplyPreviewDetected =
		ProviderAutoInitApplyPreview.bIsValid
		&& ProviderAutoInitApplyPreview.bCanApplyInFuture
		&& ProviderAutoInitApplyPreview.bWouldChangeProviderAutoInitialize
		&& ProviderAutoInitApplyPreview.bRequiresRestartOrReinitialize
		&& DoesSQLUIPersistenceSettingsApplyPreviewContainText(
			ProviderAutoInitApplyPreview,
			TEXT("Startup behavior is unchanged"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bProviderAutoInitApplyPreviewDisplayPending =
		ProviderAutoInitApplyPreviewDisplay.bIsValid
		&& ProviderAutoInitApplyPreviewDisplay.bHasWarnings
		&& ProviderAutoInitApplyPreviewDisplay.bRequiresRestartOrReinitialize
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainField(
			ProviderAutoInitApplyPreviewDisplay,
			TEXT("ProviderAutoInitChange"))
		&& DoesSQLUIPersistenceSettingsApplyPreviewDisplayContainText(
			ProviderAutoInitApplyPreviewDisplay,
			TEXT("Startup behavior is unchanged"))
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	if (!Result.bProviderAutoInitPendingValidated
		|| !Result.bProviderAutoInitPolicyUnchanged
		|| !Result.bProviderAutoInitDisplayPending
		|| !Result.bProviderAutoInitApplyPreviewDetected
		|| !Result.bProviderAutoInitApplyPreviewDisplayPending)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: provider auto-init draft/display/preview/display-preview changed policy or failed validation."));
	}

	USQLUISamplePersistenceSettingsDraftPresenter* DraftPresenter =
		NewObject<USQLUISamplePersistenceSettingsDraftPresenter>(
			Outer ? Outer : GetTransientPackage());
	if (!IsValid(DraftPresenter))
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: sample draft presenter was not created."));
	}
	else
	{
		const FSQLUISamplePersistenceSettingsDraftRefreshResult
			DefaultAdapterResult =
				DraftPresenter->RefreshDefaultPersistenceSettingsDraftDisplay();
		Result.bSampleAdapterDefaultDisplayGenerated =
			DefaultAdapterResult.bSucceeded
			&& DefaultAdapterResult.Rows.Num() > 0
			&& DefaultAdapterResult.FormattedLines.Num()
				== DefaultAdapterResult.Rows.Num()
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainField(
				DefaultAdapterResult.DisplaySummary,
				TEXT("Backend"))
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainField(
				DefaultAdapterResult.DisplaySummary,
				TEXT("Summary"));
		Result.bSampleAdapterDefaultDisplaySafe =
			DefaultAdapterResult.bIsValid
			&& !DefaultAdapterResult.bHasErrors
			&& !DefaultAdapterResult.bHasWarnings
			&& !DefaultAdapterResult.bRequiresRestartOrReinitialize
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
				DefaultAdapterResult.DisplaySummary,
				TEXT("InMemory"))
			&& DraftPresenter->GetRows().Num()
				== DefaultAdapterResult.Rows.Num()
			&& DraftPresenter->GetFormattedLines().Num()
				== DefaultAdapterResult.FormattedLines.Num()
			&& DraftPresenter->GetSummaryText()
				== DefaultAdapterResult.SummaryText
			&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);

		const FSQLUISamplePersistenceSettingsDraftRefreshResult
			UnknownBackendAdapterResult =
				DraftPresenter->BuildPersistenceSettingsDraftValidationDisplay(
					UnknownBackendDraft);
		Result.bSampleAdapterUnknownBackendDisplayShowsError =
			UnknownBackendAdapterResult.bHasErrors
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainState(
				UnknownBackendAdapterResult.DisplaySummary,
				ESQLUIPersistenceSettingsValidationDisplayState::Error)
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
				UnknownBackendAdapterResult.DisplaySummary,
				TEXT("not a selectable SQLUI persistence backend"))
			&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);

		const FSQLUISamplePersistenceSettingsDraftRefreshResult
			SQLiteAdapterResult =
				DraftPresenter->BuildPersistenceSettingsDraftValidationDisplay(
					SQLiteDraft);
		Result.bSampleAdapterSQLiteDraftDisplayGenerated =
			SQLiteAdapterResult.bSucceeded
			&& SQLiteAdapterResult.bIsValid
			&& SQLiteAdapterResult.bRequiresRestartOrReinitialize
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainField(
				SQLiteAdapterResult.DisplaySummary,
				TEXT("SQLitePath"))
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
				SQLiteAdapterResult.DisplaySummary,
				Result.DatabasePath);
		Result.bSampleAdapterSQLiteDisplayDidNotCreateDb =
			!DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);

		const FSQLUISamplePersistenceSettingsDraftRefreshResult
			EmptySQLitePathAdapterResult =
				DraftPresenter->BuildPersistenceSettingsDraftValidationDisplay(
					EmptySQLitePathDraft);
		Result.bSampleAdapterSQLiteEmptyPathDisplayShowsError =
			EmptySQLitePathAdapterResult.bHasErrors
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainState(
				EmptySQLitePathAdapterResult.DisplaySummary,
				ESQLUIPersistenceSettingsValidationDisplayState::Error)
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
				EmptySQLitePathAdapterResult.DisplaySummary,
				TEXT("SQLite requires a database path"))
			&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);

		const FSQLUISamplePersistenceSettingsDraftRefreshResult
			ProviderAutoInitAdapterResult =
				DraftPresenter->BuildPersistenceSettingsDraftValidationDisplay(
					ProviderAutoInitDraft);
		Result.bSampleAdapterProviderAutoInitDisplayPending =
			ProviderAutoInitAdapterResult.bSucceeded
			&& ProviderAutoInitAdapterResult.bHasWarnings
			&& DoesSQLUIPersistenceSettingsValidationDisplayContainText(
				ProviderAutoInitAdapterResult.DisplaySummary,
				TEXT("Enabled (pending)"))
			&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);

		const FSQLUISamplePersistenceSettingsDraftRefreshResult
			RepeatedSQLiteAdapterResult =
				DraftPresenter->BuildPersistenceSettingsDraftValidationDisplay(
					SQLiteDraft);
		Result.bSampleAdapterRepeatedDisplayDeterministic =
			AreSQLUIPersistenceSettingsValidationDisplaysEquivalent(
				SQLiteAdapterResult.DisplaySummary,
				RepeatedSQLiteAdapterResult.DisplaySummary)
			&& RepeatedSQLiteAdapterResult.FormattedLines
				== SQLiteAdapterResult.FormattedLines
			&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);

		if (!Result.bSampleAdapterDefaultDisplayGenerated
			|| !Result.bSampleAdapterDefaultDisplaySafe
			|| !Result.bSampleAdapterUnknownBackendDisplayShowsError
			|| !Result.bSampleAdapterSQLiteDraftDisplayGenerated
			|| !Result.bSampleAdapterSQLiteDisplayDidNotCreateDb
			|| !Result.bSampleAdapterSQLiteEmptyPathDisplayShowsError
			|| !Result.bSampleAdapterProviderAutoInitDisplayPending
			|| !Result.bSampleAdapterRepeatedDisplayDeterministic)
		{
			AppendSQLUISamplePersistenceSettingsDraftProbeError(
				Result,
				TEXT("SQLUI persistence settings draft probe failed: sample adapter did not preserve validation-only display behavior."));
		}
	}

	const UClass* DraftPanelWidgetClass =
		USQLUISamplePersistenceSettingsDraftPanelWidget::StaticClass();
	Result.bPanelWidgetClassDerivedFromUserWidget =
		DraftPanelWidgetClass
		&& DraftPanelWidgetClass->IsChildOf(UUserWidget::StaticClass());
	Result.bPanelWidgetBlueprintDefaultRefreshFunctionCallable =
		IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintCallable(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				RefreshDefaultPersistenceSettingsDraftPanel));
	Result.bPanelWidgetBlueprintCurrentRefreshFunctionCallable =
		IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintCallable(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				RefreshCurrentPersistenceSettingsDraftPanel));
	Result.bPanelWidgetBlueprintBuildFunctionCallable =
		IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintCallable(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				BuildPersistenceSettingsDraftPanel));
	Result.bPanelWidgetRefreshFunctionsNotBlueprintPure =
		IsSQLUISamplePersistenceStatusPanelWidgetFunctionNotBlueprintPure(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				RefreshDefaultPersistenceSettingsDraftPanel))
		&& IsSQLUISamplePersistenceStatusPanelWidgetFunctionNotBlueprintPure(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				RefreshCurrentPersistenceSettingsDraftPanel))
		&& IsSQLUISamplePersistenceStatusPanelWidgetFunctionNotBlueprintPure(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				BuildPersistenceSettingsDraftPanel));
	Result.bPanelWidgetCachedGetterFunctionsBlueprintPure =
		IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintPure(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				GetRows))
		&& IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintPure(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				GetFormattedLines))
		&& IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintPure(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				GetLastRefreshResult))
		&& IsSQLUISamplePersistenceStatusPanelWidgetFunctionBlueprintPure(
			DraftPanelWidgetClass,
			GET_FUNCTION_NAME_CHECKED(
				USQLUISamplePersistenceSettingsDraftPanelWidget,
				GetSummaryText));
	Result.bPanelWidgetRowsPropertyBlueprintVisible =
		IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			DraftPanelWidgetClass,
			TEXT("Rows"));
	Result.bPanelWidgetFormattedLinesPropertyBlueprintVisible =
		IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			DraftPanelWidgetClass,
			TEXT("FormattedLines"));
	Result.bPanelWidgetRefreshResultPropertyBlueprintVisible =
		IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			DraftPanelWidgetClass,
			TEXT("LastRefreshResult"));
	Result.bPanelWidgetSummaryTextPropertyBlueprintVisible =
		IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			DraftPanelWidgetClass,
			TEXT("SummaryText"));
	Result.bPanelWidgetValidationFlagsBlueprintVisible =
		IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			DraftPanelWidgetClass,
			TEXT("bIsValid"))
		&& IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			DraftPanelWidgetClass,
			TEXT("bHasErrors"))
		&& IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			DraftPanelWidgetClass,
			TEXT("bHasWarnings"))
		&& IsSQLUISamplePersistenceStatusPanelWidgetPropertyBlueprintVisible(
			DraftPanelWidgetClass,
			TEXT("bRequiresRestartOrReinitialize"));
	// Reflection keeps this proof independent from widget blueprint assets,
	// maps, viewport attachment, widget construction, and startup wiring.
	Result.bPanelWidgetContractValidatedWithoutAssetOrViewport =
		DraftPanelWidgetClass
		&& Result.bPanelWidgetClassDerivedFromUserWidget
		&& Result.bPanelWidgetBlueprintDefaultRefreshFunctionCallable
		&& Result.bPanelWidgetBlueprintCurrentRefreshFunctionCallable
		&& Result.bPanelWidgetBlueprintBuildFunctionCallable
		&& Result.bPanelWidgetRefreshFunctionsNotBlueprintPure
		&& Result.bPanelWidgetCachedGetterFunctionsBlueprintPure
		&& Result.bPanelWidgetRowsPropertyBlueprintVisible
		&& Result.bPanelWidgetFormattedLinesPropertyBlueprintVisible
		&& Result.bPanelWidgetRefreshResultPropertyBlueprintVisible
		&& Result.bPanelWidgetSummaryTextPropertyBlueprintVisible
		&& Result.bPanelWidgetValidationFlagsBlueprintVisible;

	const auto AppendPanelWidgetReflectionFailure =
		[&Result](const bool bPassed, const TCHAR* FailureMessage)
		{
			if (!bPassed)
			{
				AppendSQLUISamplePersistenceSettingsDraftProbeError(
					Result,
					FailureMessage);
			}
		};
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetClassDerivedFromUserWidget,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget shell did not derive from UUserWidget."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetBlueprintDefaultRefreshFunctionCallable,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget default refresh was not BlueprintCallable."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetBlueprintCurrentRefreshFunctionCallable,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget current refresh was not BlueprintCallable."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetBlueprintBuildFunctionCallable,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget build function was not BlueprintCallable."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetRefreshFunctionsNotBlueprintPure,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget refresh/build functions were unexpectedly BlueprintPure."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetCachedGetterFunctionsBlueprintPure,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget cached getter functions were not BlueprintPure."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetRowsPropertyBlueprintVisible,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget Rows property was not Blueprint-visible."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetFormattedLinesPropertyBlueprintVisible,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget FormattedLines property was not Blueprint-visible."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetRefreshResultPropertyBlueprintVisible,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget LastRefreshResult property was not Blueprint-visible."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetSummaryTextPropertyBlueprintVisible,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget SummaryText property was not Blueprint-visible."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetValidationFlagsBlueprintVisible,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget validation flag properties were not Blueprint-visible."));
	AppendPanelWidgetReflectionFailure(
		Result.bPanelWidgetContractValidatedWithoutAssetOrViewport,
		TEXT("SQLUI persistence settings draft probe failed: draft panel widget shell contract could not be validated by reflection without an asset or viewport."));

	const FSQLUIPersistenceSettingsValidationResult RepeatedSQLiteValidation =
		USQLUIPersistenceSettingsDraftLibrary::ValidatePersistenceSettingsDraft(SQLiteDraft);
	const FSQLUIPersistenceSettingsValidationDisplaySummary RepeatedSQLiteDisplay =
		USQLUIPersistenceSettingsDraftDisplayLibrary::
			MakePersistenceSettingsValidationDisplay(
				SQLiteDraft,
				RepeatedSQLiteValidation);
	const FSQLUIPersistenceSettingsApplyPreviewResult RepeatedSQLiteApplyPreview =
		USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
			SQLiteDraft);
	const FSQLUIPersistenceSettingsApplyPreviewDisplaySummary
		RepeatedSQLiteApplyPreviewDisplay =
			USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
				MakePersistenceSettingsApplyPreviewDisplay(
					RepeatedSQLiteApplyPreview);
	Result.bRepeatedValidationDeterministic =
		AreSQLUIPersistenceSettingsValidationResultsEquivalent(
			SQLiteValidation,
			RepeatedSQLiteValidation)
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bRepeatedDisplayDeterministic =
		AreSQLUIPersistenceSettingsValidationDisplaysEquivalent(
			SQLiteDisplay,
			RepeatedSQLiteDisplay)
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bRepeatedApplyPreviewDeterministic =
		AreSQLUIPersistenceSettingsApplyPreviewsEquivalent(
			SQLiteApplyPreview,
			RepeatedSQLiteApplyPreview)
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	Result.bRepeatedApplyPreviewDisplayDeterministic =
		AreSQLUIPersistenceSettingsApplyPreviewDisplaysEquivalent(
			SQLiteApplyPreviewDisplay,
			RepeatedSQLiteApplyPreviewDisplay)
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	if (!Result.bRepeatedValidationDeterministic
		|| !Result.bRepeatedDisplayDeterministic
		|| !Result.bRepeatedApplyPreviewDeterministic
		|| !Result.bRepeatedApplyPreviewDisplayDeterministic)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: repeated validation/display/apply preview/apply preview display was not deterministic."));
	}

	const FString SidecarPath = Result.SidecarDatabasePath + TEXT("-wal");
	const bool bSidecarCreated =
		WriteSQLUISamplePersistenceSettingsDraftSidecar(SidecarPath, Result);
	FSQLUIPersistenceSettingsDraft SidecarDraft = SQLiteDraft;
	SidecarDraft.PendingRuntimeConfig.SQLiteDatabasePath = Result.SidecarDatabasePath;
	USQLUIPersistenceSettingsDraftDisplayLibrary::
		ValidateAndMakePersistenceSettingsValidationDisplay(SidecarDraft);
	USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply(
		SidecarDraft);
	USQLUIPersistenceSettingsApplyPreviewDisplayLibrary::
		PreviewAndMakePersistenceSettingsApplyPreviewDisplay(SidecarDraft);
	Result.bSidecarPreservedDuringValidation =
		bSidecarCreated
		&& FPaths::FileExists(SidecarPath);
	Result.bSidecarPreservedDuringApplyPreview =
		bSidecarCreated
		&& FPaths::FileExists(SidecarPath);
	Result.bSidecarPreservedDuringApplyPreviewDisplay =
		bSidecarCreated
		&& FPaths::FileExists(SidecarPath);
	if (!Result.bSidecarPreservedDuringValidation
		|| !Result.bSidecarPreservedDuringApplyPreview
		|| !Result.bSidecarPreservedDuringApplyPreviewDisplay)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: validation, apply preview, or apply preview display deleted a smoke-owned sidecar file."));
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISamplePersistenceSettingsDraftFiles(Result)
		&& !DoesAnySQLUISamplePersistenceSettingsDraftFileExist(Result);
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISamplePersistenceSettingsDraftProbeError(
			Result,
			TEXT("SQLUI persistence settings draft probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bDefaultDraftCreated
		&& Result.bDefaultDraftValidated
		&& Result.bDefaultInMemorySafe
		&& Result.bCurrentDraftValidated
		&& Result.bUnknownBackendRejected
		&& Result.bUnknownBackendDisplayShowsError
		&& Result.bSQLiteDraftRepresented
		&& Result.bSQLiteDraftDisplayGenerated
		&& Result.bSQLiteDraftDidNotCreateDb
		&& Result.bSQLiteDisplayDidNotCreateDb
		&& Result.bSQLiteEmptyPathRejected
		&& Result.bSQLiteEmptyPathDisplayShowsError
		&& Result.bProviderAutoInitPendingValidated
		&& Result.bProviderAutoInitPolicyUnchanged
		&& Result.bProviderAutoInitDisplayPending
		&& Result.bDefaultApplyPreviewSafe
		&& Result.bCurrentApplyPreviewNoChanges
		&& Result.bBackendChangeApplyPreviewDetected
		&& Result.bSQLiteApplyPreviewSafe
		&& Result.bUnknownBackendApplyPreviewRejected
		&& Result.bSQLiteEmptyPathApplyPreviewRejected
		&& Result.bProviderAutoInitApplyPreviewDetected
		&& Result.bDefaultApplyPreviewDisplaySafe
		&& Result.bCurrentApplyPreviewDisplayNoChanges
		&& Result.bBackendChangeApplyPreviewDisplayDetected
		&& Result.bSQLiteApplyPreviewDisplaySafe
		&& Result.bUnknownBackendApplyPreviewDisplayShowsError
		&& Result.bSQLiteEmptyPathApplyPreviewDisplayShowsError
		&& Result.bProviderAutoInitApplyPreviewDisplayPending
		&& Result.bSampleAdapterDefaultDisplayGenerated
		&& Result.bSampleAdapterDefaultDisplaySafe
		&& Result.bSampleAdapterUnknownBackendDisplayShowsError
		&& Result.bSampleAdapterSQLiteDraftDisplayGenerated
		&& Result.bSampleAdapterSQLiteDisplayDidNotCreateDb
		&& Result.bSampleAdapterSQLiteEmptyPathDisplayShowsError
		&& Result.bSampleAdapterProviderAutoInitDisplayPending
		&& Result.bSampleAdapterRepeatedDisplayDeterministic
		&& Result.bPanelWidgetClassDerivedFromUserWidget
		&& Result.bPanelWidgetBlueprintDefaultRefreshFunctionCallable
		&& Result.bPanelWidgetBlueprintCurrentRefreshFunctionCallable
		&& Result.bPanelWidgetBlueprintBuildFunctionCallable
		&& Result.bPanelWidgetRefreshFunctionsNotBlueprintPure
		&& Result.bPanelWidgetCachedGetterFunctionsBlueprintPure
		&& Result.bPanelWidgetRowsPropertyBlueprintVisible
		&& Result.bPanelWidgetFormattedLinesPropertyBlueprintVisible
		&& Result.bPanelWidgetRefreshResultPropertyBlueprintVisible
		&& Result.bPanelWidgetSummaryTextPropertyBlueprintVisible
		&& Result.bPanelWidgetValidationFlagsBlueprintVisible
		&& Result.bPanelWidgetContractValidatedWithoutAssetOrViewport
		&& Result.bRepeatedValidationDeterministic
		&& Result.bRepeatedDisplayDeterministic
		&& Result.bRepeatedApplyPreviewDeterministic
		&& Result.bRepeatedApplyPreviewDisplayDeterministic
		&& Result.bSidecarPreservedDuringValidation
		&& Result.bSidecarPreservedDuringApplyPreview
		&& Result.bSidecarPreservedDuringApplyPreviewDisplay
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
	FSQLUISampleSQLiteSeedDatabaseCopyPolicyProbeResult& Result,
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

FString MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(
	const TCHAR* DirectoryName,
	const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteSeedDatabaseCopyPolicy"),
		DirectoryName,
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyDatabasePaths()
{
	return {
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Seed"), TEXT("SeedLayoutRepository.db")),
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("RuntimeLayoutRepository.db")),
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("ExistingLayoutRepository.db")),
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("OverwriteLayoutRepository.db")),
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Seed"), TEXT("MissingSeedLayoutRepository.db")),
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("MissingSeedTargetLayoutRepository.db")),
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("RuntimeConfigSeedCopyTarget.db"))
	};
}

bool DoSQLUISampleSQLiteSeedDatabaseCopyPolicyFilesExist(const FString& DatabasePath)
{
	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleSQLiteSeedDatabaseCopyPolicyFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteSeedDatabaseCopyPolicyProbeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite seed database copy policy probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISampleSQLiteSeedDatabaseCopyPolicyFiles(
	FSQLUISampleSQLiteSeedDatabaseCopyPolicyProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& DatabasePath : MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyDatabasePaths())
	{
		bRemoved =
			DeleteSQLUISampleSQLiteSeedDatabaseCopyPolicyFiles(DatabasePath, Result)
			&& bRemoved;
	}

	return bRemoved;
}

bool DoesAnySQLUISampleSQLiteSeedDatabaseCopyPolicyFileExist()
{
	for (const FString& DatabasePath : MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyDatabasePaths())
	{
		if (DoSQLUISampleSQLiteSeedDatabaseCopyPolicyFilesExist(DatabasePath))
		{
			return true;
		}
	}

	return false;
}

FSQLUILayoutDocument MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyDocument(
	const TCHAR* LayoutId,
	const TCHAR* DisplayName,
	const TCHAR* ProbeTag)
{
	FSQLUILayoutDocument Document =
		MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument();
	Document.Version.Label = DisplayName;
	Document.Metadata.LayoutId = LayoutId;
	Document.Metadata.DisplayName = DisplayName;
	Document.Metadata.Description = TEXT("Smoke/probe layout for SQLite seed database copy policy.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-06-04T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = Document.Metadata.CreatedAtUtc;
	Document.Metadata.Tags.Reset();
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(ProbeTag);
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteSeedDatabaseCopyPolicy"));
	Document.RootWidgetId = FString::Printf(TEXT("%s.Root"), LayoutId);

	if (Document.Nodes.Num() > 0)
	{
		Document.Nodes[0].WidgetId = Document.RootWidgetId;
		Document.Nodes[0].Properties.Add(TEXT("Text"), DisplayName);
		Document.Nodes[0].Tags.Reset();
		Document.Nodes[0].Tags.Add(TEXT("sqlite"));
		Document.Nodes[0].Tags.Add(ProbeTag);
		Document.Nodes[0].SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteSeedDatabaseCopyPolicy"));
	}

	return Document;
}

USQLUISQLiteLayoutRepository* CreateSQLUISampleSQLiteSeedDatabaseCopyPolicyRepository(
	UObject* Outer,
	const FString& DatabasePath,
	bool bReadOnly)
{
	USQLUISQLiteLayoutRepository* Repository =
		NewObject<USQLUISQLiteLayoutRepository>(IsValid(Outer) ? Outer : GetTransientPackage());
	if (!IsValid(Repository))
	{
		return nullptr;
	}

	FSQLUISQLiteLayoutRepositorySettings RepositorySettings;
	RepositorySettings.DatabasePath = DatabasePath;
	RepositorySettings.bReadOnly = bReadOnly;
	RepositorySettings.bInitializeSchemaIfMissing = !bReadOnly;
	RepositorySettings.bCreateDatabaseIfMissing = !bReadOnly;
	Repository->Configure(RepositorySettings);
	return Repository;
}

bool PrepareSQLUISampleSQLiteSeedDatabaseCopyPolicyDatabase(
	UObject* Outer,
	const FString& DatabasePath,
	const FSQLUILayoutDocument& Document,
	FSQLUISampleSQLiteSeedDatabaseCopyPolicyProbeResult& Result)
{
	DeleteSQLUISampleSQLiteSeedDatabaseCopyPolicyFiles(DatabasePath, Result);

	USQLUISQLiteLayoutRepository* Repository =
		CreateSQLUISampleSQLiteSeedDatabaseCopyPolicyRepository(Outer, DatabasePath, false);
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
			Result,
			TEXT("SQLUI SQLite seed database copy policy probe failed: could not create SQLite repository."));
		return false;
	}

	const FSQLUILayoutSaveResult SaveResult =
		SaveSQLUISampleLayoutToRepository(
			Repository,
			TEXT("SQLite seed database copy policy repository"),
			Document);
	if (!SaveResult.bSucceeded || SaveResult.SavedLayoutId != Document.Metadata.LayoutId)
	{
		AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
			Result,
			SaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite seed database copy policy probe failed: could not save probe layout.")
				: SaveResult.ErrorMessage);
		return false;
	}

	return FPaths::FileExists(DatabasePath);
}

bool ListSQLUISampleSQLiteSeedDatabaseCopyPolicyLayouts(
	UObject* Outer,
	const FString& DatabasePath,
	FSQLUILayoutRepositoryListResult& OutListResult,
	FSQLUISampleSQLiteSeedDatabaseCopyPolicyProbeResult& Result)
{
	USQLUISQLiteLayoutRepository* Repository =
		CreateSQLUISampleSQLiteSeedDatabaseCopyPolicyRepository(Outer, DatabasePath, true);
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
			Result,
			TEXT("SQLUI SQLite seed database copy policy probe failed: could not create read-only SQLite repository."));
		return false;
	}

	OutListResult = Repository->ListLayouts();
	if (OutListResult.bSucceeded)
	{
		return true;
	}

	AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
		Result,
		OutListResult.ErrorMessage.IsEmpty()
			? TEXT("SQLUI SQLite seed database copy policy probe failed: ListLayouts failed.")
			: OutListResult.ErrorMessage);
	return false;
}

bool LoadSQLUISampleSQLiteSeedDatabaseCopyPolicyLayout(
	UObject* Outer,
	const FString& DatabasePath,
	const FSQLUILayoutDocument& Document,
	FSQLUISampleSQLiteSeedDatabaseCopyPolicyProbeResult& Result)
{
	USQLUISQLiteLayoutRepository* Repository =
		CreateSQLUISampleSQLiteSeedDatabaseCopyPolicyRepository(Outer, DatabasePath, true);
	if (!IsValid(Repository))
	{
		AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
			Result,
			TEXT("SQLUI SQLite seed database copy policy probe failed: could not create read-only SQLite repository for load."));
		return false;
	}

	const FSQLUILayoutLoadResult LoadResult =
		LoadSQLUISampleLayoutFromRepository(
			Repository,
			TEXT("SQLite seed database copy policy repository"),
			Document.Metadata.LayoutId);
	const bool bLoaded =
		LoadResult.bSucceeded
		&& LoadResult.Validation.bIsValid
		&& LoadResult.Document.Metadata.LayoutId == Document.Metadata.LayoutId;
	if (!bLoaded)
	{
		AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
			Result,
			LoadResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite seed database copy policy probe failed: LoadLayout failed.")
				: LoadResult.ErrorMessage);
	}

	return bLoaded;
}

FSQLUISampleSQLiteSeedDatabaseCopyPolicyProbeResult
RunSQLUISampleSQLiteSeedDatabaseCopyPolicyProbe(UObject* Outer)
{
	FSQLUISampleSQLiteSeedDatabaseCopyPolicyProbeResult Result;
	const FString SeedPath =
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Seed"), TEXT("SeedLayoutRepository.db"));
	const FString MissingTargetPath =
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("RuntimeLayoutRepository.db"));
	const FString ExistingTargetPath =
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("ExistingLayoutRepository.db"));
	const FString OverwriteTargetPath =
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("OverwriteLayoutRepository.db"));
	const FString MissingSeedPath =
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Seed"), TEXT("MissingSeedLayoutRepository.db"));
	const FString MissingSeedTargetPath =
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("MissingSeedTargetLayoutRepository.db"));
	const FString RuntimeConfigTargetPath =
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyPath(TEXT("Runtime"), TEXT("RuntimeConfigSeedCopyTarget.db"));
	Result.SeedDatabasePath = SeedPath;
	Result.TargetDatabasePath = MissingTargetPath;

	DeleteSQLUISampleSQLiteSeedDatabaseCopyPolicyFiles(Result);

	const FSQLUILayoutDocument SeedDocument =
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyDocument(
			TEXT("sqlui.smoke.sqlite-seed-copy.seed"),
			TEXT("SQLUI SQLite Seed Copy Policy Seed Layout"),
			TEXT("seed-copy-seed"));
	const FSQLUILayoutDocument ExistingDocument =
		MakeSQLUISampleSQLiteSeedDatabaseCopyPolicyDocument(
			TEXT("sqlui.smoke.sqlite-seed-copy.existing"),
			TEXT("SQLUI SQLite Seed Copy Policy Existing Layout"),
			TEXT("seed-copy-existing"));

	Result.bSeedDatabaseCreated =
		PrepareSQLUISampleSQLiteSeedDatabaseCopyPolicyDatabase(
			Outer,
			SeedPath,
			SeedDocument,
			Result);

	if (Result.bSeedDatabaseCreated)
	{
		FSQLUISQLiteSeedDatabaseCopyRequest MissingTargetRequest;
		MissingTargetRequest.SeedDatabasePath = SeedPath;
		MissingTargetRequest.TargetDatabasePath = MissingTargetPath;
		MissingTargetRequest.bCopyIfTargetMissing = true;
		const FSQLUISQLiteSeedDatabaseCopyResult MissingTargetCopyResult =
			FSQLUISQLiteSeedDatabaseCopy::CopySeedDatabase(MissingTargetRequest);
		Result.bMissingTargetCopied =
			MissingTargetCopyResult.bSucceeded
			&& MissingTargetCopyResult.bSeedDatabaseFound
			&& !MissingTargetCopyResult.bTargetDatabaseAlreadyExisted
			&& MissingTargetCopyResult.bTargetDatabaseCopied
			&& MissingTargetCopyResult.bTargetDatabaseReady
			&& FPaths::FileExists(MissingTargetPath);
		if (!Result.bMissingTargetCopied)
		{
			AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
				Result,
				MissingTargetCopyResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite seed database copy policy probe failed: missing target was not copied.")
					: MissingTargetCopyResult.ErrorMessage);
		}

		FSQLUILayoutRepositoryListResult CopiedTargetListResult;
		Result.bCopiedTargetReadable =
			ListSQLUISampleSQLiteSeedDatabaseCopyPolicyLayouts(
				Outer,
				MissingTargetPath,
				CopiedTargetListResult,
				Result)
			&& DoesSQLUISampleLayoutMetadataAndTagsListContain(
				CopiedTargetListResult.Layouts,
				SeedDocument.Metadata);
		Result.bCopiedTargetLoadedSeedLayout =
			LoadSQLUISampleSQLiteSeedDatabaseCopyPolicyLayout(
				Outer,
				MissingTargetPath,
				SeedDocument,
				Result);
	}

	if (Result.bSeedDatabaseCreated
		&& PrepareSQLUISampleSQLiteSeedDatabaseCopyPolicyDatabase(
			Outer,
			ExistingTargetPath,
			ExistingDocument,
			Result))
	{
		FSQLUISQLiteSeedDatabaseCopyRequest ExistingTargetRequest;
		ExistingTargetRequest.SeedDatabasePath = SeedPath;
		ExistingTargetRequest.TargetDatabasePath = ExistingTargetPath;
		ExistingTargetRequest.bCopyIfTargetMissing = true;
		ExistingTargetRequest.bOverwriteTarget = false;
		const FSQLUISQLiteSeedDatabaseCopyResult ExistingTargetCopyResult =
			FSQLUISQLiteSeedDatabaseCopy::CopySeedDatabase(ExistingTargetRequest);

		FSQLUILayoutRepositoryListResult ExistingTargetListResult;
		const bool bExistingTargetReadable =
			ListSQLUISampleSQLiteSeedDatabaseCopyPolicyLayouts(
				Outer,
				ExistingTargetPath,
				ExistingTargetListResult,
				Result);
		Result.bExistingTargetPreservedWithoutOverwrite =
			ExistingTargetCopyResult.bSucceeded
			&& ExistingTargetCopyResult.bTargetDatabaseAlreadyExisted
			&& !ExistingTargetCopyResult.bTargetDatabaseCopied
			&& ExistingTargetCopyResult.bTargetDatabaseReady
			&& bExistingTargetReadable
			&& DoesSQLUISampleLayoutMetadataAndTagsListContain(
				ExistingTargetListResult.Layouts,
				ExistingDocument.Metadata)
			&& !DoesSQLUISampleLayoutMetadataAndTagsListContain(
				ExistingTargetListResult.Layouts,
				SeedDocument.Metadata);
		if (!Result.bExistingTargetPreservedWithoutOverwrite)
		{
			AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
				Result,
				ExistingTargetCopyResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite seed database copy policy probe failed: existing target was not preserved without overwrite.")
					: ExistingTargetCopyResult.ErrorMessage);
		}
	}

	if (Result.bSeedDatabaseCreated
		&& PrepareSQLUISampleSQLiteSeedDatabaseCopyPolicyDatabase(
			Outer,
			OverwriteTargetPath,
			ExistingDocument,
			Result))
	{
		FSQLUISQLiteSeedDatabaseCopyRequest OverwriteTargetRequest;
		OverwriteTargetRequest.SeedDatabasePath = SeedPath;
		OverwriteTargetRequest.TargetDatabasePath = OverwriteTargetPath;
		OverwriteTargetRequest.bCopyIfTargetMissing = true;
		OverwriteTargetRequest.bOverwriteTarget = true;
		const FSQLUISQLiteSeedDatabaseCopyResult OverwriteTargetCopyResult =
			FSQLUISQLiteSeedDatabaseCopy::CopySeedDatabase(OverwriteTargetRequest);

		FSQLUILayoutRepositoryListResult OverwriteTargetListResult;
		const bool bOverwriteTargetReadable =
			ListSQLUISampleSQLiteSeedDatabaseCopyPolicyLayouts(
				Outer,
				OverwriteTargetPath,
				OverwriteTargetListResult,
				Result);
		Result.bOverwriteTargetCopiedSeed =
			OverwriteTargetCopyResult.bSucceeded
			&& OverwriteTargetCopyResult.bTargetDatabaseAlreadyExisted
			&& OverwriteTargetCopyResult.bTargetDatabaseCopied
			&& OverwriteTargetCopyResult.bTargetDatabaseReady
			&& bOverwriteTargetReadable
			&& DoesSQLUISampleLayoutMetadataAndTagsListContain(
				OverwriteTargetListResult.Layouts,
				SeedDocument.Metadata);
		Result.bOverwriteRemovedExistingLayout =
			bOverwriteTargetReadable
			&& !DoesSQLUISampleLayoutMetadataAndTagsListContain(
				OverwriteTargetListResult.Layouts,
				ExistingDocument.Metadata);
		if (!Result.bOverwriteTargetCopiedSeed || !Result.bOverwriteRemovedExistingLayout)
		{
			AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
				Result,
				OverwriteTargetCopyResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite seed database copy policy probe failed: overwrite target did not contain only the seed layout.")
					: OverwriteTargetCopyResult.ErrorMessage);
		}
	}

	FSQLUISQLiteSeedDatabaseCopyRequest MissingSeedRequest;
	MissingSeedRequest.SeedDatabasePath = MissingSeedPath;
	MissingSeedRequest.TargetDatabasePath = MissingSeedTargetPath;
	MissingSeedRequest.bCopyIfTargetMissing = true;
	const FSQLUISQLiteSeedDatabaseCopyResult MissingSeedCopyResult =
		FSQLUISQLiteSeedDatabaseCopy::CopySeedDatabase(MissingSeedRequest);
	Result.bMissingSeedFailed =
		!MissingSeedCopyResult.bSucceeded
		&& !MissingSeedCopyResult.bSeedDatabaseFound
		&& MissingSeedCopyResult.ErrorMessage.Contains(TEXT("does not exist"));
	Result.bMissingSeedDidNotCreateTarget =
		!DoSQLUISampleSQLiteSeedDatabaseCopyPolicyFilesExist(MissingSeedTargetPath);
	if (!Result.bMissingSeedFailed || !Result.bMissingSeedDidNotCreateTarget)
	{
		AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
			Result,
			MissingSeedCopyResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite seed database copy policy probe failed: missing seed did not fail cleanly.")
				: MissingSeedCopyResult.ErrorMessage);
	}

	if (Result.bSeedDatabaseCreated)
	{
		FSQLUISQLiteSeedDatabaseCopyRequest SamePathRequest;
		SamePathRequest.SeedDatabasePath = SeedPath;
		SamePathRequest.TargetDatabasePath = SeedPath;
		const FSQLUISQLiteSeedDatabaseCopyResult SamePathCopyResult =
			FSQLUISQLiteSeedDatabaseCopy::CopySeedDatabase(SamePathRequest);
		Result.bSamePathFailed =
			!SamePathCopyResult.bSucceeded
			&& SamePathCopyResult.ErrorMessage.Contains(TEXT("same"));
		Result.bSamePathLeftSeedIntact =
			LoadSQLUISampleSQLiteSeedDatabaseCopyPolicyLayout(
				Outer,
				SeedPath,
				SeedDocument,
				Result);
		if (!Result.bSamePathFailed || !Result.bSamePathLeftSeedIntact)
		{
			AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
				Result,
				SamePathCopyResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite seed database copy policy probe failed: same-path case did not fail without mutating seed.")
					: SamePathCopyResult.ErrorMessage);
		}
	}

	const FSQLUILayoutRepositoryRuntimeConfig Defaults =
		FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
	const FString RuntimeConfigCommandLine = FString::Printf(
		TEXT("-SQLUILayoutRepositoryBackend=SQLite -SQLUISQLiteLayoutRepositoryPath=\"%s\" -SQLUISQLiteLayoutRepositorySeedPath=\"%s\" -SQLUISQLiteLayoutRepositoryCopySeedIfMissing -SQLUISQLiteLayoutRepositoryOverwriteFromSeed"),
		*RuntimeConfigTargetPath,
		*SeedPath);
	const FSQLUILayoutRepositoryRuntimeConfig RuntimeConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			*RuntimeConfigCommandLine,
			Defaults);
	const FSQLUISQLiteSeedDatabaseCopyRequest RuntimeConfigCopyRequest =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ToSeedDatabaseCopyRequest(RuntimeConfig);
	Result.bRuntimeConfigSeedFlagsParsed =
		RuntimeConfig.SQLiteSeedDatabasePath == SeedPath
		&& RuntimeConfig.bSQLiteCopySeedIfMissing
		&& RuntimeConfig.bSQLiteOverwriteDatabaseFromSeed
		&& RuntimeConfigCopyRequest.SeedDatabasePath == FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteSeedDatabasePath(SeedPath)
		&& RuntimeConfigCopyRequest.TargetDatabasePath == FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(RuntimeConfigTargetPath)
		&& RuntimeConfigCopyRequest.bCopyIfTargetMissing
		&& RuntimeConfigCopyRequest.bOverwriteTarget
		&& !DoSQLUISampleSQLiteSeedDatabaseCopyPolicyFilesExist(RuntimeConfigTargetPath);
	if (!Result.bRuntimeConfigSeedFlagsParsed)
	{
		AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
			Result,
			TEXT("SQLUI SQLite seed database copy policy probe failed: runtime config seed flags were not parsed or mapped without copying."));
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISampleSQLiteSeedDatabaseCopyPolicyFiles(Result)
		&& !DoesAnySQLUISampleSQLiteSeedDatabaseCopyPolicyFileExist();
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISampleSQLiteSeedDatabaseCopyPolicyError(
			Result,
			TEXT("SQLUI SQLite seed database copy policy probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bSeedDatabaseCreated
		&& Result.bMissingTargetCopied
		&& Result.bCopiedTargetReadable
		&& Result.bCopiedTargetLoadedSeedLayout
		&& Result.bExistingTargetPreservedWithoutOverwrite
		&& Result.bOverwriteTargetCopiedSeed
		&& Result.bOverwriteRemovedExistingLayout
		&& Result.bMissingSeedFailed
		&& Result.bMissingSeedDidNotCreateTarget
		&& Result.bSamePathFailed
		&& Result.bSamePathLeftSeedIntact
		&& Result.bRuntimeConfigSeedFlagsParsed
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteSchemaInitHardeningError(
	FSQLUISampleSQLiteSchemaInitHardeningSmokeResult& Result,
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

FString MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteSchemaInitHardening"),
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePaths()
{
	TArray<FString> DatabasePaths;
	DatabasePaths.Add(MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("MissingCreateDisabled.db")));
	DatabasePaths.Add(MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("EmptyCreateEnabled.db")));
	DatabasePaths.Add(MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("AlreadyInitialized.db")));
	DatabasePaths.Add(MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("CompleteSchemaMissingMigration.db")));
	DatabasePaths.Add(MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("PartialSchema.db")));
	DatabasePaths.Add(MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("ReadOnlyInitBlocked.db")));
	return DatabasePaths;
}

bool DoSQLUISampleSQLiteSchemaInitHardeningFilesExist(const FString& DatabasePath)
{
	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleSQLiteSchemaInitHardeningFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteSchemaInitHardeningSmokeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteSchemaInitHardeningError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite schema init hardening smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISampleSQLiteSchemaInitHardeningFiles(
	FSQLUISampleSQLiteSchemaInitHardeningSmokeResult& Result)
{
	bool bRemoved = true;
	for (const FString& DatabasePath : MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePaths())
	{
		bRemoved =
			DeleteSQLUISampleSQLiteSchemaInitHardeningFiles(DatabasePath, Result)
			&& bRemoved;
	}

	return bRemoved;
}

bool DoesAnySQLUISampleSQLiteSchemaInitHardeningFileExist()
{
	for (const FString& DatabasePath : MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePaths())
	{
		if (DoSQLUISampleSQLiteSchemaInitHardeningFilesExist(DatabasePath))
		{
			return true;
		}
	}

	return false;
}

bool CountSQLUISampleSQLiteSchemaInitHardeningMigrationRows(
	const FString& DatabasePath,
	int32& OutRecordCount,
	FSQLUISampleSQLiteSchemaInitHardeningSmokeResult& Result)
{
	FString ErrorMessage;
	if (FSQLUISQLiteLayoutSchemaMigration::CountInitialSchemaMigrationRecords(
		DatabasePath,
		OutRecordCount,
		ErrorMessage))
	{
		return true;
	}

	AppendSQLUISampleSQLiteSchemaInitHardeningError(
		Result,
		ErrorMessage.IsEmpty()
			? TEXT("SQLUI SQLite schema init hardening smoke failed: could not count initial schema migration rows.")
			: ErrorMessage);
	return false;
}

bool PrepareSQLUISampleSQLiteSchemaInitHardeningPartialSchema(
	const FString& DatabasePath,
	FSQLUISampleSQLiteSchemaInitHardeningSmokeResult& Result)
{
	FSQLUISQLiteMigrationStep Step;
	Step.MigrationId = TEXT("001_initial_layout_schema");
	Step.Description =
		TEXT("Smoke-only intentionally partial initial schema for hardening coverage.");
	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS layouts (")
		TEXT("layout_id TEXT PRIMARY KEY, ")
		TEXT("display_name TEXT NOT NULL, ")
		TEXT("schema_version INTEGER NOT NULL, ")
		TEXT("b_deleted INTEGER NOT NULL DEFAULT 0")
		TEXT(");"));

	TArray<FSQLUISQLiteMigrationStep> Steps;
	Steps.Add(Step);
	const FSQLUISQLiteMigrationResult MigrationResult =
		FSQLUISQLiteMigrationRunner::RunMigrations(DatabasePath, Steps, false);
	if (MigrationResult.bSucceeded)
	{
		return true;
	}

	AppendSQLUISampleSQLiteSchemaInitHardeningError(
		Result,
		MigrationResult.ErrorMessage.IsEmpty()
			? TEXT("SQLUI SQLite schema init hardening smoke failed: could not prepare partial schema database.")
			: MigrationResult.ErrorMessage);
	return false;
}

FSQLUISampleSQLiteSchemaInitHardeningSmokeResult RunSQLUISampleSQLiteSchemaInitHardeningSmoke(
	UObject* Outer)
{
	FSQLUISampleSQLiteSchemaInitHardeningSmokeResult Result;
	DeleteSQLUISampleSQLiteSchemaInitHardeningFiles(Result);

	const FString MissingCreateDisabledPath =
		MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("MissingCreateDisabled.db"));
	const FSQLUISQLiteLayoutSchemaInitializationResult MissingCreateDisabledResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(MissingCreateDisabledPath, false);
	Result.bMissingDbCreateDisabledFailed =
		!MissingCreateDisabledResult.bSucceeded
		&& !MissingCreateDisabledResult.bSchemaReady
		&& MissingCreateDisabledResult.ErrorMessage.Contains(TEXT("does not exist"))
		&& MissingCreateDisabledResult.ErrorMessage.Contains(TEXT("creation is disabled"));
	Result.bMissingDbCreateDisabledNotCreated =
		!DoSQLUISampleSQLiteSchemaInitHardeningFilesExist(MissingCreateDisabledPath);
	if (!Result.bMissingDbCreateDisabledFailed)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite schema init hardening smoke failed: missing DB create disabled did not fail clearly. Succeeded=%s SchemaReady=%s Error='%s'."),
				MissingCreateDisabledResult.bSucceeded ? TEXT("true") : TEXT("false"),
				MissingCreateDisabledResult.bSchemaReady ? TEXT("true") : TEXT("false"),
				*MissingCreateDisabledResult.ErrorMessage));
	}
	if (!Result.bMissingDbCreateDisabledNotCreated)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			TEXT("SQLUI SQLite schema init hardening smoke failed: missing DB create disabled created a database file."));
	}

	const FString EmptyCreateEnabledPath =
		MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("EmptyCreateEnabled.db"));
	const FSQLUISQLiteLayoutSchemaInitializationResult EmptyCreateEnabledResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(EmptyCreateEnabledPath, true);
	Result.bEmptyDbCreateEnabledSucceeded =
		EmptyCreateEnabledResult.bSucceeded
		&& EmptyCreateEnabledResult.bMigrationApplied
		&& FPaths::FileExists(EmptyCreateEnabledPath);
	Result.bEmptyDbSchemaReady = EmptyCreateEnabledResult.bSchemaReady;
	if (!Result.bEmptyDbCreateEnabledSucceeded || !Result.bEmptyDbSchemaReady)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			EmptyCreateEnabledResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite schema init hardening smoke failed: empty DB create enabled did not initialize schema.")
				: EmptyCreateEnabledResult.ErrorMessage);
	}

	const FString AlreadyInitializedPath =
		MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("AlreadyInitialized.db"));
	const FSQLUISQLiteLayoutSchemaInitializationResult AlreadyInitializedFirstResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(AlreadyInitializedPath, true);
	int32 AlreadyInitializedMigrationRowCountBefore = 0;
	const bool bCountedAlreadyInitializedBefore =
		CountSQLUISampleSQLiteSchemaInitHardeningMigrationRows(
			AlreadyInitializedPath,
			AlreadyInitializedMigrationRowCountBefore,
			Result);
	const FSQLUISQLiteLayoutSchemaInitializationResult AlreadyInitializedSecondResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(AlreadyInitializedPath, true);
	int32 AlreadyInitializedMigrationRowCountAfter = 0;
	const bool bCountedAlreadyInitializedAfter =
		CountSQLUISampleSQLiteSchemaInitHardeningMigrationRows(
			AlreadyInitializedPath,
			AlreadyInitializedMigrationRowCountAfter,
			Result);
	Result.bAlreadyInitializedSucceeded =
		AlreadyInitializedFirstResult.bSucceeded
		&& AlreadyInitializedSecondResult.bSucceeded
		&& AlreadyInitializedSecondResult.bSchemaReady;
	Result.bAlreadyInitializedDetected =
		AlreadyInitializedSecondResult.bMigrationAlreadyApplied;
	Result.bMigrationRowNotDuplicated =
		bCountedAlreadyInitializedBefore
		&& bCountedAlreadyInitializedAfter
		&& AlreadyInitializedMigrationRowCountBefore == 1
		&& AlreadyInitializedMigrationRowCountAfter == 1;
	if (!Result.bAlreadyInitializedSucceeded || !Result.bAlreadyInitializedDetected)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			AlreadyInitializedSecondResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite schema init hardening smoke failed: already-initialized DB did not succeed or report already-applied migration.")
				: AlreadyInitializedSecondResult.ErrorMessage);
	}
	if (!Result.bMigrationRowNotDuplicated)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite schema init hardening smoke failed: migration row count was duplicated or unexpected. Before=%d After=%d."),
				AlreadyInitializedMigrationRowCountBefore,
				AlreadyInitializedMigrationRowCountAfter));
	}

	const FString CompleteSchemaMissingMigrationPath =
		MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("CompleteSchemaMissingMigration.db"));
	const FSQLUISQLiteLayoutSchemaInitializationResult CompleteSchemaInitialResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(
			CompleteSchemaMissingMigrationPath,
			true);
	FString DeleteMigrationErrorMessage;
	const bool bDeletedMigrationRecord =
		CompleteSchemaInitialResult.bSucceeded
		&& FSQLUISQLiteLayoutSchemaMigration::DeleteInitialSchemaMigrationRecordForSmokeTest(
			CompleteSchemaMissingMigrationPath,
			DeleteMigrationErrorMessage);
	int32 MissingMigrationRowCountBefore = 0;
	const bool bCountedMissingMigrationBefore =
		bDeletedMigrationRecord
		&& CountSQLUISampleSQLiteSchemaInitHardeningMigrationRows(
			CompleteSchemaMissingMigrationPath,
			MissingMigrationRowCountBefore,
			Result);
	const FSQLUISQLiteLayoutSchemaInitializationResult CompleteSchemaRepairResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(
			CompleteSchemaMissingMigrationPath,
			true);
	int32 MissingMigrationRowCountAfter = 0;
	const bool bCountedMissingMigrationAfter =
		CountSQLUISampleSQLiteSchemaInitHardeningMigrationRows(
			CompleteSchemaMissingMigrationPath,
			MissingMigrationRowCountAfter,
			Result);
	Result.bCompleteSchemaMissingMigrationSucceeded =
		CompleteSchemaInitialResult.bSucceeded
		&& bDeletedMigrationRecord
		&& bCountedMissingMigrationBefore
		&& MissingMigrationRowCountBefore == 0
		&& CompleteSchemaRepairResult.bSucceeded
		&& CompleteSchemaRepairResult.bSchemaReady;
	Result.bCompleteSchemaMissingMigrationRecorded =
		bCountedMissingMigrationAfter
		&& MissingMigrationRowCountAfter == 1
		&& CompleteSchemaRepairResult.bMigrationApplied;
	if (!bDeletedMigrationRecord && !DeleteMigrationErrorMessage.IsEmpty())
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(Result, DeleteMigrationErrorMessage);
	}
	if (!Result.bCompleteSchemaMissingMigrationSucceeded)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			CompleteSchemaRepairResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite schema init hardening smoke failed: complete schema missing migration row did not succeed.")
				: CompleteSchemaRepairResult.ErrorMessage);
	}
	if (!Result.bCompleteSchemaMissingMigrationRecorded)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite schema init hardening smoke failed: missing migration row was not restored. Before=%d After=%d MigrationApplied=%s."),
				MissingMigrationRowCountBefore,
				MissingMigrationRowCountAfter,
				CompleteSchemaRepairResult.bMigrationApplied ? TEXT("true") : TEXT("false")));
	}

	const FString PartialSchemaPath =
		MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("PartialSchema.db"));
	const bool bPartialSchemaPrepared =
		PrepareSQLUISampleSQLiteSchemaInitHardeningPartialSchema(
			PartialSchemaPath,
			Result);
	const FSQLUISQLiteLayoutSchemaInitializationResult PartialSchemaResult =
		bPartialSchemaPrepared
			? FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(PartialSchemaPath, true)
			: FSQLUISQLiteLayoutSchemaInitializationResult();
	Result.bPartialSchemaFailedClearly =
		bPartialSchemaPrepared
		&& !PartialSchemaResult.bSucceeded
		&& !PartialSchemaResult.bSchemaReady;
	Result.bPartialSchemaReportedMissingObjects =
		Result.bPartialSchemaFailedClearly
		&& PartialSchemaResult.ErrorMessage.Contains(TEXT("missing expected schema object"));
	if (!Result.bPartialSchemaFailedClearly || !Result.bPartialSchemaReportedMissingObjects)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite schema init hardening smoke failed: partial schema did not fail clearly. Prepared=%s Succeeded=%s SchemaReady=%s Error='%s'."),
				bPartialSchemaPrepared ? TEXT("true") : TEXT("false"),
				PartialSchemaResult.bSucceeded ? TEXT("true") : TEXT("false"),
				PartialSchemaResult.bSchemaReady ? TEXT("true") : TEXT("false"),
				*PartialSchemaResult.ErrorMessage));
	}

	const FString ReadOnlyInitBlockedPath =
		MakeSQLUISampleSQLiteSchemaInitHardeningDatabasePath(TEXT("ReadOnlyInitBlocked.db"));
	FSQLUILayoutRepositoryFactorySettings ReadOnlyFactorySettings;
	ReadOnlyFactorySettings.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	ReadOnlyFactorySettings.SQLiteSettings.DatabasePath = ReadOnlyInitBlockedPath;
	ReadOnlyFactorySettings.SQLiteSettings.bReadOnly = true;
	ReadOnlyFactorySettings.SQLiteSettings.bInitializeSchemaIfMissing = true;
	ReadOnlyFactorySettings.SQLiteSettings.bCreateDatabaseIfMissing = true;
	USQLUILayoutRepository* ReadOnlyRepository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, ReadOnlyFactorySettings);
	const FSQLUILayoutDocument ReadOnlyDocument =
		MakeSQLUISampleSQLiteFactorySchemaInitRepositoryDocument();
	const FSQLUILayoutSaveResult ReadOnlySaveResult =
		SaveSQLUISampleLayoutToRepository(
			ReadOnlyRepository,
			TEXT("SQLite schema init hardening read-only repository"),
			ReadOnlyDocument);
	Result.bReadOnlyInitBlocked =
		!ReadOnlySaveResult.bSucceeded
		&& ReadOnlySaveResult.ErrorMessage.Contains(TEXT("read-only"), ESearchCase::IgnoreCase);
	Result.bReadOnlyInitDidNotCreateDb =
		!DoSQLUISampleSQLiteSchemaInitHardeningFilesExist(ReadOnlyInitBlockedPath);
	if (!Result.bReadOnlyInitBlocked)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite schema init hardening smoke failed: read-only repository did not reject SaveLayout before schema init. SaveSucceeded=%s Error='%s'."),
				ReadOnlySaveResult.bSucceeded ? TEXT("true") : TEXT("false"),
				*ReadOnlySaveResult.ErrorMessage));
	}
	if (!Result.bReadOnlyInitDidNotCreateDb)
	{
		AppendSQLUISampleSQLiteSchemaInitHardeningError(
			Result,
			TEXT("SQLUI SQLite schema init hardening smoke failed: read-only schema init created a database file."));
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISampleSQLiteSchemaInitHardeningFiles(Result)
		&& !DoesAnySQLUISampleSQLiteSchemaInitHardeningFileExist();

	Result.bSucceeded =
		Result.bMissingDbCreateDisabledFailed
		&& Result.bMissingDbCreateDisabledNotCreated
		&& Result.bEmptyDbCreateEnabledSucceeded
		&& Result.bEmptyDbSchemaReady
		&& Result.bAlreadyInitializedSucceeded
		&& Result.bAlreadyInitializedDetected
		&& Result.bMigrationRowNotDuplicated
		&& Result.bCompleteSchemaMissingMigrationSucceeded
		&& Result.bCompleteSchemaMissingMigrationRecorded
		&& Result.bPartialSchemaFailedClearly
		&& Result.bPartialSchemaReportedMissingObjects
		&& Result.bReadOnlyInitBlocked
		&& Result.bReadOnlyInitDidNotCreateDb
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

void AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
	FSQLUISampleSQLiteMigrationVersioningPolicyProbeResult& Result,
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

FString MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(
	const TCHAR* DatabaseFileName)
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("SQLiteMigrationVersioningPolicy"),
		DatabaseFileName);
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

TArray<FString> MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePaths()
{
	return {
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("LayoutSchemaCurrent.db")),
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("LayoutSchemaMissingRecord.db")),
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("LayoutSchemaPartial.db")),
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("SmokeOrderedMigrations.db")),
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("SmokePendingMigration.db")),
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("SmokeFailingMigration.db"))
	};
}

bool DoSQLUISampleSQLiteMigrationVersioningPolicyFilesExist(const FString& DatabasePath)
{
	const TArray<FString> PathsToCheck = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	for (const FString& PathToCheck : PathsToCheck)
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUISampleSQLiteMigrationVersioningPolicyFiles(
	const FString& DatabasePath,
	FSQLUISampleSQLiteMigrationVersioningPolicyProbeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite migration versioning policy probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DeleteSQLUISampleSQLiteMigrationVersioningPolicyFiles(
	FSQLUISampleSQLiteMigrationVersioningPolicyProbeResult& Result)
{
	bool bRemoved = true;
	for (const FString& DatabasePath : MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePaths())
	{
		bRemoved =
			DeleteSQLUISampleSQLiteMigrationVersioningPolicyFiles(DatabasePath, Result)
			&& bRemoved;
	}

	return bRemoved;
}

bool DoesAnySQLUISampleSQLiteMigrationVersioningPolicyFileExist()
{
	for (const FString& DatabasePath : MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePaths())
	{
		if (DoSQLUISampleSQLiteMigrationVersioningPolicyFilesExist(DatabasePath))
		{
			return true;
		}
	}

	return false;
}

FSQLUISQLiteMigrationStep MakeSQLUISampleSQLiteMigrationVersioningPolicySmokeStepA()
{
	FSQLUISQLiteMigrationStep Step;
	Step.MigrationId = TEXT("001_smoke_versioning_a");
	Step.Description =
		TEXT("Smoke-only versioning migration A. This is not a production layout schema migration.");
	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS sqlui_smoke_versioning_a (")
		TEXT("probe_id TEXT PRIMARY KEY")
		TEXT(");"));
	Step.Statements.Add(
		TEXT("INSERT OR IGNORE INTO sqlui_smoke_versioning_a (probe_id) VALUES ('a');"));
	return Step;
}

FSQLUISQLiteMigrationStep MakeSQLUISampleSQLiteMigrationVersioningPolicySmokeStepB()
{
	FSQLUISQLiteMigrationStep Step;
	Step.MigrationId = TEXT("002_smoke_versioning_b");
	Step.Description =
		TEXT("Smoke-only versioning migration B. This is not a production layout schema migration.");
	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS sqlui_smoke_versioning_b (")
		TEXT("probe_id TEXT PRIMARY KEY")
		TEXT(");"));
	Step.Statements.Add(
		TEXT("INSERT OR IGNORE INTO sqlui_smoke_versioning_b (probe_id) VALUES ('b');"));
	return Step;
}

FSQLUISQLiteMigrationStep MakeSQLUISampleSQLiteMigrationVersioningPolicyFailingStep()
{
	FSQLUISQLiteMigrationStep Step;
	Step.MigrationId = TEXT("003_smoke_versioning_fails");
	Step.Description =
		TEXT("Smoke-only intentionally failing versioning migration.");
	Step.Statements.Add(TEXT("THIS IS NOT VALID SQL;"));
	return Step;
}

TArray<FString> MakeSQLUISampleSQLiteMigrationVersioningPolicySmokeMigrationIds()
{
	return {
		TEXT("001_smoke_versioning_a"),
		TEXT("002_smoke_versioning_b")
	};
}

bool DoSQLUISampleSQLiteMigrationVersioningPolicyIdsMatch(
	const TArray<FString>& ActualIds,
	const TArray<FString>& ExpectedIds)
{
	if (ActualIds.Num() != ExpectedIds.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < ExpectedIds.Num(); ++Index)
	{
		if (ActualIds[Index] != ExpectedIds[Index])
		{
			return false;
		}
	}

	return true;
}

bool PrepareSQLUISampleSQLiteMigrationVersioningPolicyPartialSchema(
	const FString& DatabasePath,
	FSQLUISampleSQLiteMigrationVersioningPolicyProbeResult& Result)
{
	FSQLUISQLiteMigrationStep Step;
	Step.MigrationId =
		FSQLUISQLiteLayoutSchemaMigration::GetInitialLayoutSchemaMigrationId();
	Step.Description =
		TEXT("Smoke-only intentionally partial initial schema for versioning coverage.");
	Step.Statements.Add(
		TEXT("CREATE TABLE IF NOT EXISTS layouts (")
		TEXT("layout_id TEXT PRIMARY KEY, ")
		TEXT("display_name TEXT NOT NULL, ")
		TEXT("schema_version INTEGER NOT NULL, ")
		TEXT("b_deleted INTEGER NOT NULL DEFAULT 0")
		TEXT(");"));

	TArray<FSQLUISQLiteMigrationStep> Steps;
	Steps.Add(Step);
	const FSQLUISQLiteMigrationResult MigrationResult =
		FSQLUISQLiteMigrationRunner::RunMigrations(DatabasePath, Steps, false);
	if (MigrationResult.bSucceeded)
	{
		return true;
	}

	AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
		Result,
		MigrationResult.ErrorMessage.IsEmpty()
			? TEXT("SQLUI SQLite migration versioning policy probe failed: could not prepare partial schema database.")
			: MigrationResult.ErrorMessage);
	return false;
}

FSQLUISampleSQLiteMigrationVersioningPolicyProbeResult
RunSQLUISampleSQLiteMigrationVersioningPolicyProbe()
{
	FSQLUISampleSQLiteMigrationVersioningPolicyProbeResult Result;
	DeleteSQLUISampleSQLiteMigrationVersioningPolicyFiles(Result);

	const FString InitialMigrationId =
		FSQLUISQLiteLayoutSchemaMigration::GetInitialLayoutSchemaMigrationId();
	const FString LatestKnownMigrationId =
		FSQLUISQLiteLayoutSchemaVersioning::GetLatestKnownLayoutSchemaMigrationId();

	const FString CurrentSchemaPath =
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("LayoutSchemaCurrent.db"));
	const FSQLUISQLiteLayoutSchemaVersionStatus CurrentApplyStatus =
		FSQLUISQLiteLayoutSchemaVersioning::ApplyKnownLayoutSchemaMigrations(
			CurrentSchemaPath,
			true);
	const FSQLUISQLiteLayoutSchemaVersionStatus CurrentStatus =
		FSQLUISQLiteLayoutSchemaVersioning::GetLayoutSchemaVersionStatus(CurrentSchemaPath);
	Result.bCurrentInitialSchemaStatusSucceeded =
		CurrentApplyStatus.bSucceeded
		&& CurrentStatus.bSucceeded
		&& CurrentStatus.bInitialSchemaRecorded
		&& CurrentStatus.bSchemaObjectsReady
		&& CurrentStatus.AppliedMigrationIds.Contains(InitialMigrationId);
	Result.bLatestKnownMigrationMatched =
		LatestKnownMigrationId == InitialMigrationId
		&& CurrentStatus.LatestKnownMigrationId == InitialMigrationId;
	Result.bNoPendingKnownMigrations =
		CurrentStatus.bSucceeded
		&& !CurrentStatus.bHasPendingMigrations
		&& CurrentStatus.PendingMigrationIds.Num() == 0;
	if (!Result.bCurrentInitialSchemaStatusSucceeded
		|| !Result.bLatestKnownMigrationMatched
		|| !Result.bNoPendingKnownMigrations)
	{
		AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
			Result,
			CurrentStatus.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite migration versioning policy probe failed: current initial schema status was not current.")
				: CurrentStatus.ErrorMessage);
	}

	const FString MissingRecordPath =
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("LayoutSchemaMissingRecord.db"));
	const FSQLUISQLiteLayoutSchemaInitializationResult MissingRecordInitialResult =
		FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema(MissingRecordPath, true);
	FString DeleteRecordErrorMessage;
	const bool bDeletedMissingRecord =
		MissingRecordInitialResult.bSucceeded
		&& FSQLUISQLiteLayoutSchemaMigration::DeleteInitialSchemaMigrationRecordForSmokeTest(
			MissingRecordPath,
			DeleteRecordErrorMessage);
	const FSQLUISQLiteLayoutSchemaVersionStatus MissingRecordStatusBefore =
		FSQLUISQLiteLayoutSchemaVersioning::GetLayoutSchemaVersionStatus(MissingRecordPath);
	Result.bCompleteSchemaMissingRecordDetected =
		bDeletedMissingRecord
		&& MissingRecordStatusBefore.bSucceeded
		&& MissingRecordStatusBefore.bSchemaObjectsReady
		&& !MissingRecordStatusBefore.bInitialSchemaRecorded
		&& MissingRecordStatusBefore.bHasPendingMigrations
		&& MissingRecordStatusBefore.PendingMigrationIds.Contains(InitialMigrationId);
	const FSQLUISQLiteLayoutSchemaVersionStatus MissingRecordRepairStatus =
		FSQLUISQLiteLayoutSchemaVersioning::ApplyKnownLayoutSchemaMigrations(
			MissingRecordPath,
			true);
	int32 MissingRecordCountAfterRepair = 0;
	FString MissingRecordCountErrorMessage;
	const bool bCountedMissingRecordAfterRepair =
		FSQLUISQLiteLayoutSchemaMigration::CountInitialSchemaMigrationRecords(
			MissingRecordPath,
			MissingRecordCountAfterRepair,
			MissingRecordCountErrorMessage);
	Result.bMissingRecordRepairedNonDestructively =
		Result.bCompleteSchemaMissingRecordDetected
		&& MissingRecordRepairStatus.bSucceeded
		&& MissingRecordRepairStatus.bInitialSchemaRecorded
		&& MissingRecordRepairStatus.bSchemaObjectsReady
		&& !MissingRecordRepairStatus.bHasPendingMigrations
		&& bCountedMissingRecordAfterRepair
		&& MissingRecordCountAfterRepair == 1;
	if (!bDeletedMissingRecord && !DeleteRecordErrorMessage.IsEmpty())
	{
		AppendSQLUISampleSQLiteMigrationVersioningPolicyError(Result, DeleteRecordErrorMessage);
	}
	if (!Result.bCompleteSchemaMissingRecordDetected
		|| !Result.bMissingRecordRepairedNonDestructively)
	{
		AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
			Result,
			MissingRecordRepairStatus.ErrorMessage.IsEmpty()
				? FString::Printf(
					TEXT("SQLUI SQLite migration versioning policy probe failed: missing migration record was not detected/repaired. CountAfter=%d Counted=%s CountError='%s'."),
					MissingRecordCountAfterRepair,
					bCountedMissingRecordAfterRepair ? TEXT("true") : TEXT("false"),
					*MissingRecordCountErrorMessage)
				: MissingRecordRepairStatus.ErrorMessage);
	}

	const FString PartialSchemaPath =
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("LayoutSchemaPartial.db"));
	const bool bPartialSchemaPrepared =
		PrepareSQLUISampleSQLiteMigrationVersioningPolicyPartialSchema(
			PartialSchemaPath,
			Result);
	const FSQLUISQLiteLayoutSchemaVersionStatus PartialSchemaStatus =
		bPartialSchemaPrepared
			? FSQLUISQLiteLayoutSchemaVersioning::GetLayoutSchemaVersionStatus(PartialSchemaPath)
			: FSQLUISQLiteLayoutSchemaVersionStatus();
	const FSQLUISQLiteLayoutSchemaVersionStatus PartialSchemaApplyStatus =
		bPartialSchemaPrepared
			? FSQLUISQLiteLayoutSchemaVersioning::ApplyKnownLayoutSchemaMigrations(PartialSchemaPath, true)
			: FSQLUISQLiteLayoutSchemaVersionStatus();
	Result.bPartialSchemaFailedClearly =
		bPartialSchemaPrepared
		&& (!PartialSchemaStatus.bSucceeded || !PartialSchemaApplyStatus.bSucceeded)
		&& (PartialSchemaStatus.ErrorMessage.Contains(TEXT("missing expected schema object"))
			|| PartialSchemaApplyStatus.ErrorMessage.Contains(TEXT("missing expected schema object")));
	if (!Result.bPartialSchemaFailedClearly)
	{
		AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite migration versioning policy probe failed: partial schema did not fail clearly. Prepared=%s StatusSucceeded=%s ApplySucceeded=%s StatusError='%s' ApplyError='%s'."),
				bPartialSchemaPrepared ? TEXT("true") : TEXT("false"),
				PartialSchemaStatus.bSucceeded ? TEXT("true") : TEXT("false"),
				PartialSchemaApplyStatus.bSucceeded ? TEXT("true") : TEXT("false"),
				*PartialSchemaStatus.ErrorMessage,
				*PartialSchemaApplyStatus.ErrorMessage));
	}

	const FString OrderedMigrationsPath =
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("SmokeOrderedMigrations.db"));
	TArray<FSQLUISQLiteMigrationStep> OrderedSteps;
	OrderedSteps.Add(MakeSQLUISampleSQLiteMigrationVersioningPolicySmokeStepA());
	OrderedSteps.Add(MakeSQLUISampleSQLiteMigrationVersioningPolicySmokeStepB());
	const TArray<FString> SmokeMigrationIds =
		MakeSQLUISampleSQLiteMigrationVersioningPolicySmokeMigrationIds();
	const FSQLUISQLiteMigrationResult OrderedMigrationResult =
		FSQLUISQLiteMigrationRunner::RunMigrations(OrderedMigrationsPath, OrderedSteps, false);
	const FSQLUISQLiteLayoutSchemaVersionStatus OrderedMigrationStatus =
		FSQLUISQLiteLayoutSchemaVersioning::GetMigrationVersionStatus(
			OrderedMigrationsPath,
			SmokeMigrationIds,
			false);
	Result.bSmokeMigrationsAppliedInOrder =
		OrderedMigrationResult.bSucceeded
		&& OrderedMigrationStatus.bSucceeded
		&& DoSQLUISampleSQLiteMigrationVersioningPolicyIdsMatch(
			OrderedMigrationStatus.AppliedMigrationIds,
			SmokeMigrationIds)
		&& !OrderedMigrationStatus.bHasPendingMigrations;
	const FSQLUISQLiteMigrationResult OrderedMigrationRerunResult =
		FSQLUISQLiteMigrationRunner::RunMigrations(OrderedMigrationsPath, OrderedSteps, false);
	const FSQLUISQLiteLayoutSchemaVersionStatus OrderedMigrationRerunStatus =
		FSQLUISQLiteLayoutSchemaVersioning::GetMigrationVersionStatus(
			OrderedMigrationsPath,
			SmokeMigrationIds,
			false);
	Result.bSmokeMigrationsIdempotent =
		OrderedMigrationRerunResult.bSucceeded
		&& OrderedMigrationRerunResult.AppliedMigrationCount == 0
		&& DoSQLUISampleSQLiteMigrationVersioningPolicyIdsMatch(
			OrderedMigrationRerunStatus.AppliedMigrationIds,
			SmokeMigrationIds)
		&& !OrderedMigrationRerunStatus.bHasPendingMigrations;
	if (!Result.bSmokeMigrationsAppliedInOrder || !Result.bSmokeMigrationsIdempotent)
	{
		AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
			Result,
			OrderedMigrationResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite migration versioning policy probe failed: smoke migrations were not applied in order or rerun idempotently.")
				: OrderedMigrationResult.ErrorMessage);
	}

	const FString PendingMigrationPath =
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("SmokePendingMigration.db"));
	TArray<FSQLUISQLiteMigrationStep> FirstPendingStepOnly;
	FirstPendingStepOnly.Add(MakeSQLUISampleSQLiteMigrationVersioningPolicySmokeStepA());
	const FSQLUISQLiteMigrationResult PendingFirstMigrationResult =
		FSQLUISQLiteMigrationRunner::RunMigrations(
			PendingMigrationPath,
			FirstPendingStepOnly,
			false);
	const FSQLUISQLiteLayoutSchemaVersionStatus PendingStatusBefore =
		FSQLUISQLiteLayoutSchemaVersioning::GetMigrationVersionStatus(
			PendingMigrationPath,
			SmokeMigrationIds,
			false);
	Result.bSmokePendingMigrationDetected =
		PendingFirstMigrationResult.bSucceeded
		&& PendingStatusBefore.bSucceeded
		&& PendingStatusBefore.bHasPendingMigrations
		&& PendingStatusBefore.PendingMigrationIds.Num() == 1
		&& PendingStatusBefore.PendingMigrationIds.Contains(TEXT("002_smoke_versioning_b"));
	const FSQLUISQLiteMigrationResult PendingApplyAllResult =
		FSQLUISQLiteMigrationRunner::RunMigrations(
			PendingMigrationPath,
			OrderedSteps,
			false);
	const FSQLUISQLiteLayoutSchemaVersionStatus PendingStatusAfter =
		FSQLUISQLiteLayoutSchemaVersioning::GetMigrationVersionStatus(
			PendingMigrationPath,
			SmokeMigrationIds,
			false);
	Result.bSmokePendingMigrationApplied =
		PendingApplyAllResult.bSucceeded
		&& PendingApplyAllResult.AppliedMigrationCount == 1
		&& PendingStatusAfter.bSucceeded
		&& !PendingStatusAfter.bHasPendingMigrations
		&& DoSQLUISampleSQLiteMigrationVersioningPolicyIdsMatch(
			PendingStatusAfter.AppliedMigrationIds,
			SmokeMigrationIds);
	if (!Result.bSmokePendingMigrationDetected || !Result.bSmokePendingMigrationApplied)
	{
		AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
			Result,
			PendingApplyAllResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite migration versioning policy probe failed: pending smoke migration was not detected/applied.")
				: PendingApplyAllResult.ErrorMessage);
	}

	const FString FailingMigrationPath =
		MakeSQLUISampleSQLiteMigrationVersioningPolicyDatabasePath(TEXT("SmokeFailingMigration.db"));
	TArray<FSQLUISQLiteMigrationStep> FailingSteps;
	const FSQLUISQLiteMigrationStep FailingStep =
		MakeSQLUISampleSQLiteMigrationVersioningPolicyFailingStep();
	FailingSteps.Add(FailingStep);
	const FSQLUISQLiteMigrationResult FailingMigrationResult =
		FSQLUISQLiteMigrationRunner::RunMigrations(
			FailingMigrationPath,
			FailingSteps,
			false);
	const FSQLUISQLiteLayoutSchemaVersionStatus FailingMigrationStatus =
		FSQLUISQLiteLayoutSchemaVersioning::GetMigrationVersionStatus(
			FailingMigrationPath,
			{ FailingStep.MigrationId },
			false);
	Result.bFailingMigrationFailedClearly =
		!FailingMigrationResult.bSucceeded
		&& FailingMigrationResult.ErrorMessage.Contains(TEXT("failed"));
	Result.bFailingMigrationNotRecorded =
		FailingMigrationStatus.bSucceeded
		&& !FailingMigrationStatus.AppliedMigrationIds.Contains(FailingStep.MigrationId)
		&& FailingMigrationStatus.PendingMigrationIds.Contains(FailingStep.MigrationId);
	if (!Result.bFailingMigrationFailedClearly || !Result.bFailingMigrationNotRecorded)
	{
		AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
			Result,
			FailingMigrationResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI SQLite migration versioning policy probe failed: failing migration was not reported or was recorded.")
				: FailingMigrationResult.ErrorMessage);
	}

	Result.bDatabaseFilesRemoved =
		DeleteSQLUISampleSQLiteMigrationVersioningPolicyFiles(Result)
		&& !DoesAnySQLUISampleSQLiteMigrationVersioningPolicyFileExist();
	if (!Result.bDatabaseFilesRemoved)
	{
		AppendSQLUISampleSQLiteMigrationVersioningPolicyError(
			Result,
			TEXT("SQLUI SQLite migration versioning policy probe failed: probe database files were not removed."));
	}

	Result.bSucceeded =
		Result.bCurrentInitialSchemaStatusSucceeded
		&& Result.bLatestKnownMigrationMatched
		&& Result.bNoPendingKnownMigrations
		&& Result.bCompleteSchemaMissingRecordDetected
		&& Result.bMissingRecordRepairedNonDestructively
		&& Result.bPartialSchemaFailedClearly
		&& Result.bSmokeMigrationsAppliedInOrder
		&& Result.bSmokeMigrationsIdempotent
		&& Result.bSmokePendingMigrationDetected
		&& Result.bSmokePendingMigrationApplied
		&& Result.bFailingMigrationFailedClearly
		&& Result.bFailingMigrationNotRecorded
		&& Result.bDatabaseFilesRemoved;

	return Result;
}

FSQLUISampleSmokeTestLayoutResult ResolveSQLUISampleSmokeTestLayoutDocument(
	UObject* Outer,
	const FSQLUISampleSmokeTestRequest& Request)
{
	FSQLUISampleSmokeTestLayoutResult Result;
	Result.bUsedInMemoryLayoutRepository = Request.bUseInMemoryLayoutRepository;
	Result.bUsedJsonFileLayoutRepository = Request.bUseJsonFileLayoutRepository;
	Result.bUsedJsonLayoutFixture =
		Request.bUseJsonLayoutFixture
		|| Request.bUseInMemoryLayoutRepository
		|| Request.bUseJsonFileLayoutRepository;

	if (!Result.bUsedJsonLayoutFixture)
	{
		Result.Document = MakeSQLUISampleSmokeTestDocument(Request);
		return Result;
	}

	const bool bParsedAndValidated = FSQLUILayoutJson::FromJsonString(
		FSQLUISampleLayoutJsonFixture::GetJsonString(),
		Result.Document,
		Result.JsonLayoutFixtureValidation);

	Result.bJsonLayoutFixtureParseSucceeded =
		!DidSQLUISampleLayoutJsonParseFail(Result.JsonLayoutFixtureValidation);
	Result.bJsonLayoutFixtureValidationSucceeded =
		Result.bJsonLayoutFixtureParseSucceeded && Result.JsonLayoutFixtureValidation.bIsValid;
	Result.bSucceeded = bParsedAndValidated;
	if (!Result.bSucceeded)
	{
		return Result;
	}

	if (Request.bUseInMemoryLayoutRepository || Request.bUseJsonFileLayoutRepository)
	{
		RunSQLUISampleUnavailableRepositorySelectionSmoke(Outer, Result.Document, Result);
		if (!Result.bUnavailableRepositorySelectionSucceeded)
		{
			Result.bSucceeded = false;
			return Result;
		}
	}

	if (Request.bUseInMemoryLayoutRepository)
	{
		FSQLUILayoutRepositoryFactorySettings RepositorySettings;
		RepositorySettings.Backend = ESQLUILayoutRepositoryBackend::InMemory;
		USQLUIInMemoryLayoutRepository* LayoutRepository = Cast<USQLUIInMemoryLayoutRepository>(
			USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, RepositorySettings));
		if (!IsValid(LayoutRepository))
		{
			Result.RepositorySaveErrorMessage =
				TEXT("SQLUI sample smoke test failed: layout repository factory did not create an in-memory layout repository.");
			Result.bSucceeded = false;
			return Result;
		}

		FSQLUILayoutSaveResult SaveResult;
		FSQLUILayoutLoadResult LoadResult;
		RunSQLUISampleLayoutRepositorySmoke(
			LayoutRepository,
			TEXT("in-memory layout repository"),
			Result.Document,
			SaveResult,
			LoadResult,
			Result.RepositoryOperationSmoke);

		Result.bRepositorySaveSucceeded = SaveResult.bSucceeded;
		Result.SavedLayoutId = SaveResult.SavedLayoutId;
		Result.RepositorySaveErrorMessage = SaveResult.ErrorMessage;
		Result.RepositorySaveValidation = SaveResult.Validation;

		if (!SaveResult.bSucceeded)
		{
			Result.bSucceeded = false;
			return Result;
		}

		Result.bRepositoryLoadSucceeded =
			LoadResult.bSucceeded && LoadResult.Validation.bIsValid;
		Result.LoadedLayoutId = LoadResult.Document.Metadata.LayoutId;
		Result.RepositoryLoadErrorMessage = LoadResult.ErrorMessage;
		Result.RepositoryLoadValidation = LoadResult.Validation;

		if (!Result.bRepositoryLoadSucceeded)
		{
			Result.bSucceeded = false;
			if (Result.RepositoryLoadErrorMessage.IsEmpty())
			{
				Result.RepositoryLoadErrorMessage =
					TEXT("SQLUI sample smoke test failed: in-memory layout repository loaded an invalid layout document.");
			}
			return Result;
		}

		if (!DidSQLUISampleRepositoryOperationSmokeSucceed(Result.RepositoryOperationSmoke))
		{
			Result.bSucceeded = false;
			return Result;
		}

		Result.Document = LoadResult.Document;
	}

	if (Request.bUseJsonFileLayoutRepository)
	{
		FSQLUILayoutRepositoryFactorySettings RepositorySettings;
		RepositorySettings.Backend = ESQLUILayoutRepositoryBackend::JsonFile;
		RepositorySettings.JsonFileBaseDirectory = MakeSQLUISampleJsonFileLayoutRepositoryBaseDirectory();
		USQLUIJsonFileLayoutRepository* LayoutRepository = Cast<USQLUIJsonFileLayoutRepository>(
			USQLUILayoutRepositoryFactory::CreateLayoutRepository(Outer, RepositorySettings));
		if (!IsValid(LayoutRepository))
		{
			Result.JsonFileRepositorySaveErrorMessage =
				TEXT("SQLUI sample smoke test failed: layout repository factory did not create a JSON file layout repository.");
			Result.bSucceeded = false;
			return Result;
		}

		FSQLUILayoutSaveResult SaveResult;
		FSQLUILayoutLoadResult LoadResult;
		RunSQLUISampleLayoutRepositorySmoke(
			LayoutRepository,
			TEXT("JSON file layout repository"),
			Result.Document,
			SaveResult,
			LoadResult,
			Result.JsonFileRepositoryOperationSmoke);

		Result.bJsonFileRepositorySaveSucceeded = SaveResult.bSucceeded;
		Result.JsonFileRepositorySavedLayoutId = SaveResult.SavedLayoutId;
		Result.JsonFileRepositorySaveErrorMessage = SaveResult.ErrorMessage;
		Result.JsonFileRepositorySaveValidation = SaveResult.Validation;

		if (!SaveResult.bSucceeded)
		{
			Result.bSucceeded = false;
			return Result;
		}

		Result.bJsonFileRepositoryLoadSucceeded =
			LoadResult.bSucceeded && LoadResult.Validation.bIsValid;
		Result.JsonFileRepositoryLoadedLayoutId = LoadResult.Document.Metadata.LayoutId;
		Result.JsonFileRepositoryLoadErrorMessage = LoadResult.ErrorMessage;
		Result.JsonFileRepositoryLoadValidation = LoadResult.Validation;

		if (!Result.bJsonFileRepositoryLoadSucceeded)
		{
			Result.bSucceeded = false;
			if (Result.JsonFileRepositoryLoadErrorMessage.IsEmpty())
			{
				Result.JsonFileRepositoryLoadErrorMessage =
					TEXT("SQLUI sample smoke test failed: JSON file layout repository loaded an invalid layout document.");
			}
			return Result;
		}

		if (!DidSQLUISampleRepositoryOperationSmokeSucceed(Result.JsonFileRepositoryOperationSmoke))
		{
			Result.bSucceeded = false;
			return Result;
		}

		Result.Document = LoadResult.Document;
	}

	return Result;
}

void ApplySQLUISampleLayoutResult(
	FSQLUISampleSmokeTestResult& Result,
	const FSQLUISampleSmokeTestLayoutResult& LayoutResult)
{
	Result.bUsedJsonLayoutFixture = LayoutResult.bUsedJsonLayoutFixture;
	Result.bJsonLayoutFixtureParseSucceeded = LayoutResult.bJsonLayoutFixtureParseSucceeded;
	Result.bJsonLayoutFixtureValidationSucceeded = LayoutResult.bJsonLayoutFixtureValidationSucceeded;
	Result.JsonLayoutFixtureValidation = LayoutResult.JsonLayoutFixtureValidation;
	Result.bRepositorySelectionSmokeTested = LayoutResult.bRepositorySelectionSmokeTested;
	Result.bUnavailableRepositorySelectionSucceeded = LayoutResult.bUnavailableRepositorySelectionSucceeded;
	Result.bUnavailableRepositorySaveBackendUnavailable = LayoutResult.bUnavailableRepositorySaveBackendUnavailable;
	Result.bUnavailableRepositoryLoadBackendUnavailable = LayoutResult.bUnavailableRepositoryLoadBackendUnavailable;
	Result.UnavailableRepositorySelectionErrorMessage = LayoutResult.UnavailableRepositorySelectionErrorMessage;
	Result.bUsedInMemoryLayoutRepository = LayoutResult.bUsedInMemoryLayoutRepository;
	Result.bRepositorySaveSucceeded = LayoutResult.bRepositorySaveSucceeded;
	Result.bRepositoryLoadSucceeded = LayoutResult.bRepositoryLoadSucceeded;
	Result.SavedLayoutId = LayoutResult.SavedLayoutId;
	Result.LoadedLayoutId = LayoutResult.LoadedLayoutId;
	Result.RepositorySaveErrorMessage = LayoutResult.RepositorySaveErrorMessage;
	Result.RepositoryLoadErrorMessage = LayoutResult.RepositoryLoadErrorMessage;
	Result.RepositorySaveValidation = LayoutResult.RepositorySaveValidation;
	Result.RepositoryLoadValidation = LayoutResult.RepositoryLoadValidation;
	Result.RepositoryOperationSmoke = LayoutResult.RepositoryOperationSmoke;
	Result.bUsedJsonFileLayoutRepository = LayoutResult.bUsedJsonFileLayoutRepository;
	Result.bJsonFileRepositorySaveSucceeded = LayoutResult.bJsonFileRepositorySaveSucceeded;
	Result.bJsonFileRepositoryLoadSucceeded = LayoutResult.bJsonFileRepositoryLoadSucceeded;
	Result.JsonFileRepositorySavedLayoutId = LayoutResult.JsonFileRepositorySavedLayoutId;
	Result.JsonFileRepositoryLoadedLayoutId = LayoutResult.JsonFileRepositoryLoadedLayoutId;
	Result.JsonFileRepositorySaveErrorMessage = LayoutResult.JsonFileRepositorySaveErrorMessage;
	Result.JsonFileRepositoryLoadErrorMessage = LayoutResult.JsonFileRepositoryLoadErrorMessage;
	Result.JsonFileRepositoryOperationSmoke = LayoutResult.JsonFileRepositoryOperationSmoke;
}

void AddSQLUISampleLayoutValidationMessages(
	FSQLUISampleSmokeTestResult& Result,
	const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		AddSQLUISampleSmokeTestError(Result, Error);
	}

	for (const FString& Warning : Validation.Warnings)
	{
		Result.Warnings.Add(Warning);
	}
}

void AddSQLUISampleRepositoryOperationMessages(
	FSQLUISampleSmokeTestResult& Result,
	const FSQLUISampleRepositoryOperationSmokeResult& OperationResult)
{
	AddSQLUISampleSmokeTestError(Result, OperationResult.ListAfterSaveErrorMessage);
	AddSQLUISampleSmokeTestError(Result, OperationResult.RemoveErrorMessage);
	AddSQLUISampleSmokeTestError(Result, OperationResult.ListAfterRemoveErrorMessage);
	AddSQLUISampleSmokeTestError(Result, OperationResult.ClearErrorMessage);
}

bool DidSQLUISampleDatabaseAsyncProbeSucceed(const FSQLUIDatabaseAsyncResult& Result)
{
	return Result.bSucceeded
		&& Result.bBackgroundWorkCompleted
		&& Result.bRanOnBackgroundThread
		&& Result.bDeliveredOnGameThread;
}

FString MakeSQLUISampleDatabaseAsyncProbeFailureMessage(
	const FSQLUIDatabaseAsyncResult& Result)
{
	if (!Result.ErrorMessage.IsEmpty())
	{
		return Result.ErrorMessage;
	}

	return FString::Printf(
		TEXT("SQLUI database async probe failed. BackgroundWorkCompleted=%s RanOnBackgroundThread=%s DeliveredOnGameThread=%s"),
		Result.bBackgroundWorkCompleted ? TEXT("true") : TEXT("false"),
		Result.bRanOnBackgroundThread ? TEXT("true") : TEXT("false"),
		Result.bDeliveredOnGameThread ? TEXT("true") : TEXT("false"));
}

bool DidSQLUISampleSQLiteMigrationProbeSucceed(const FSQLUISQLiteMigrationResult& Result)
{
	return Result.bSucceeded
		&& Result.bDatabaseOpened
		&& Result.bMigrationTableCreated
		&& Result.bMigrationApplied
		&& Result.bMigrationRecorded
		&& Result.bDatabaseClosed
		&& Result.bDatabaseRemoved;
}

FString MakeSQLUISampleSQLiteMigrationProbeFailureMessage(
	const FSQLUISQLiteMigrationResult& Result)
{
	if (!Result.ErrorMessage.IsEmpty())
	{
		return Result.ErrorMessage;
	}

	return FString::Printf(
		TEXT("SQLUI SQLite migration probe failed. DatabaseOpened=%s MigrationTableCreated=%s MigrationApplied=%s MigrationRecorded=%s DatabaseClosed=%s DatabaseRemoved=%s"),
		Result.bDatabaseOpened ? TEXT("true") : TEXT("false"),
		Result.bMigrationTableCreated ? TEXT("true") : TEXT("false"),
		Result.bMigrationApplied ? TEXT("true") : TEXT("false"),
		Result.bMigrationRecorded ? TEXT("true") : TEXT("false"),
		Result.bDatabaseClosed ? TEXT("true") : TEXT("false"),
		Result.bDatabaseRemoved ? TEXT("true") : TEXT("false"));
}

bool DidSQLUISampleSQLiteLayoutSchemaMigrationProbeSucceed(
	const FSQLUISQLiteLayoutSchemaMigrationProbeResult& Result)
{
	return Result.bSucceeded
		&& Result.bMigrationSucceeded
		&& Result.bLayoutsTableExists
		&& Result.bLayoutRevisionsTableExists
		&& Result.bLayoutTagsTableExists
		&& Result.bLayoutCheckpointsTableExists
		&& Result.bLayoutPreviewsTableExists
		&& Result.bExpectedIndexesExist
		&& Result.bDatabaseRemoved;
}

FString MakeSQLUISampleSQLiteLayoutSchemaMigrationProbeFailureMessage(
	const FSQLUISQLiteLayoutSchemaMigrationProbeResult& Result)
{
	if (!Result.ErrorMessage.IsEmpty())
	{
		return Result.ErrorMessage;
	}

	return FString::Printf(
		TEXT("SQLUI SQLite layout schema migration probe failed. MigrationSucceeded=%s LayoutsTable=%s LayoutRevisionsTable=%s LayoutTagsTable=%s LayoutCheckpointsTable=%s LayoutPreviewsTable=%s ExpectedIndexes=%s DatabaseRemoved=%s"),
		Result.bMigrationSucceeded ? TEXT("true") : TEXT("false"),
		Result.bLayoutsTableExists ? TEXT("true") : TEXT("false"),
		Result.bLayoutRevisionsTableExists ? TEXT("true") : TEXT("false"),
		Result.bLayoutTagsTableExists ? TEXT("true") : TEXT("false"),
		Result.bLayoutCheckpointsTableExists ? TEXT("true") : TEXT("false"),
		Result.bLayoutPreviewsTableExists ? TEXT("true") : TEXT("false"),
		Result.bExpectedIndexesExist ? TEXT("true") : TEXT("false"),
		Result.bDatabaseRemoved ? TEXT("true") : TEXT("false"));
}

bool DidSQLUISampleSQLiteLayoutReadProbeSucceed(
	const FSQLUISQLiteLayoutReadProbeResult& Result)
{
	return Result.bSucceeded
		&& Result.bSchemaMigrationSucceeded
		&& Result.bSeedInserted
		&& Result.bListSucceeded
		&& Result.bListedMetadataFound
		&& Result.bLoadSucceeded
		&& Result.bLoadedDocumentValid
		&& Result.bDatabaseRemoved;
}

FString MakeSQLUISampleSQLiteLayoutReadProbeFailureMessage(
	const FSQLUISQLiteLayoutReadProbeResult& Result)
{
	if (!Result.ErrorMessage.IsEmpty())
	{
		return Result.ErrorMessage;
	}

	return FString::Printf(
		TEXT("SQLUI SQLite layout read probe failed. SchemaMigrationSucceeded=%s SeedInserted=%s ListSucceeded=%s ListedMetadataFound=%s LoadSucceeded=%s LoadedDocumentValid=%s DatabaseRemoved=%s"),
		Result.bSchemaMigrationSucceeded ? TEXT("true") : TEXT("false"),
		Result.bSeedInserted ? TEXT("true") : TEXT("false"),
		Result.bListSucceeded ? TEXT("true") : TEXT("false"),
		Result.bListedMetadataFound ? TEXT("true") : TEXT("false"),
		Result.bLoadSucceeded ? TEXT("true") : TEXT("false"),
		Result.bLoadedDocumentValid ? TEXT("true") : TEXT("false"),
		Result.bDatabaseRemoved ? TEXT("true") : TEXT("false"));
}

struct FSQLUISampleDatabaseAsyncProbeState
{
	FSQLUIDatabaseAsyncResult Result;
	bool bCallbackDelivered = false;
};

FSQLUIDatabaseAsyncResult RunSQLUISampleDatabaseAsyncProbe()
{
	FSQLUIDatabaseAsyncRequest Request;
	Request.RequestId = TEXT("sqlui.smoke.database-async-probe");
	Request.DebugName = TEXT("SQLUI Database Async Smoke Probe");
	Request.bSimulateSuccess = true;

	TSharedRef<FSQLUISampleDatabaseAsyncProbeState, ESPMode::ThreadSafe> SharedState =
		MakeShared<FSQLUISampleDatabaseAsyncProbeState, ESPMode::ThreadSafe>();

	FSQLUIDatabaseAsyncRunner::RunAsync(
		Request,
		FSQLUIDatabaseAsyncCompleteDelegate::CreateLambda(
			[SharedState](const FSQLUIDatabaseAsyncResult& InResult)
			{
				SharedState->Result = InResult;
				SharedState->bCallbackDelivered = true;
			}));

	const double TimeoutSeconds = 5.0;
	const double StartSeconds = FPlatformTime::Seconds();

	// Commandlets do not tick the normal game loop while this smoke helper is on
	// the stack, so pump only the game-thread task queue until the callback arrives.
	while (!SharedState->bCallbackDelivered && (FPlatformTime::Seconds() - StartSeconds) < TimeoutSeconds)
	{
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		if (!SharedState->bCallbackDelivered)
		{
			FPlatformProcess::Sleep(0.01f);
		}
	}

	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);

	if (!SharedState->bCallbackDelivered)
	{
		SharedState->Result.RequestId = Request.RequestId;
		SharedState->Result.DebugName = Request.DebugName;
		SharedState->Result.bSucceeded = false;
		SharedState->Result.ErrorMessage =
			TEXT("SQLUI database async probe timed out waiting for the game-thread callback.");
	}

	return SharedState->Result;
}

void AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
	FSQLUISampleDatabaseAsyncQueueShutdownProbeResult& Result,
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

bool DidSQLUISampleDatabaseAsyncQueueShutdownProbeSucceed(
	const FSQLUISampleDatabaseAsyncQueueShutdownProbeResult& Result)
{
	return Result.bSucceeded
		&& Result.bQueueCreated
		&& Result.bShutdownRequested
		&& Result.bQueueReportedShutdown
		&& Result.bEnqueueAfterShutdownRejected
		&& Result.bPendingWorkSuppressed
		&& Result.bRunningCompletionSuppressed
		&& Result.bNoCallbacksDeliveredAfterShutdown
		&& Result.bNoDeadlock;
}

FString MakeSQLUISampleDatabaseAsyncQueueShutdownProbeFailureMessage(
	const FSQLUISampleDatabaseAsyncQueueShutdownProbeResult& Result)
{
	if (!Result.ErrorMessage.IsEmpty())
	{
		return Result.ErrorMessage;
	}

	return FString::Printf(
		TEXT("SQLUI database async queue shutdown probe failed. QueueCreated=%s ShutdownRequested=%s QueueReportedShutdown=%s EnqueueAfterShutdownRejected=%s PendingWorkSuppressed=%s RunningCompletionSuppressed=%s NoCallbacksDeliveredAfterShutdown=%s NoDeadlock=%s"),
		Result.bQueueCreated ? TEXT("true") : TEXT("false"),
		Result.bShutdownRequested ? TEXT("true") : TEXT("false"),
		Result.bQueueReportedShutdown ? TEXT("true") : TEXT("false"),
		Result.bEnqueueAfterShutdownRejected ? TEXT("true") : TEXT("false"),
		Result.bPendingWorkSuppressed ? TEXT("true") : TEXT("false"),
		Result.bRunningCompletionSuppressed ? TEXT("true") : TEXT("false"),
		Result.bNoCallbacksDeliveredAfterShutdown ? TEXT("true") : TEXT("false"),
		Result.bNoDeadlock ? TEXT("true") : TEXT("false"));
}

struct FSQLUISampleDatabaseAsyncQueueShutdownProbeState
{
	bool bFirstCallbackDelivered = false;
	bool bSecondCallbackDelivered = false;
	bool bPostShutdownCallbackDelivered = false;
};

FSQLUISampleDatabaseAsyncQueueShutdownProbeResult
RunSQLUISampleDatabaseAsyncQueueShutdownProbe()
{
	FSQLUISampleDatabaseAsyncQueueShutdownProbeResult Result;

	TSharedRef<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe> Queue =
		MakeShared<FSQLUIDatabaseAsyncQueue, ESPMode::ThreadSafe>();
	Result.bQueueCreated = true;

	FEvent* FirstWorkStartedEvent = FPlatformProcess::GetSynchEventFromPool(false);
	FEvent* FirstWorkMayFinishEvent = FPlatformProcess::GetSynchEventFromPool(false);
	FEvent* FirstWorkFinishedEvent = FPlatformProcess::GetSynchEventFromPool(false);
	FEvent* SecondWorkStartedEvent = FPlatformProcess::GetSynchEventFromPool(false);
	if (!FirstWorkStartedEvent
		|| !FirstWorkMayFinishEvent
		|| !FirstWorkFinishedEvent
		|| !SecondWorkStartedEvent)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: could not allocate synchronization events."));

		if (FirstWorkStartedEvent)
		{
			FPlatformProcess::ReturnSynchEventToPool(FirstWorkStartedEvent);
		}
		if (FirstWorkMayFinishEvent)
		{
			FPlatformProcess::ReturnSynchEventToPool(FirstWorkMayFinishEvent);
		}
		if (FirstWorkFinishedEvent)
		{
			FPlatformProcess::ReturnSynchEventToPool(FirstWorkFinishedEvent);
		}
		if (SecondWorkStartedEvent)
		{
			FPlatformProcess::ReturnSynchEventToPool(SecondWorkStartedEvent);
		}

		return Result;
	}

	TSharedRef<FSQLUISampleDatabaseAsyncQueueShutdownProbeState, ESPMode::ThreadSafe> SharedState =
		MakeShared<FSQLUISampleDatabaseAsyncQueueShutdownProbeState, ESPMode::ThreadSafe>();

	const bool bFirstEnqueued = Queue->EnqueueResult<int32>(
		[FirstWorkStartedEvent, FirstWorkMayFinishEvent, FirstWorkFinishedEvent]()
		{
			FirstWorkStartedEvent->Trigger();
			FirstWorkMayFinishEvent->Wait(5000);
			FirstWorkFinishedEvent->Trigger();
			return 1;
		},
		[SharedState](const int32&)
		{
			SharedState->bFirstCallbackDelivered = true;
		});
	if (!bFirstEnqueued)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: first enqueue was rejected before shutdown."));
	}

	const bool bFirstWorkStarted = bFirstEnqueued && FirstWorkStartedEvent->Wait(5000);
	if (!bFirstWorkStarted)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: first work did not start before timeout."));
	}

	const bool bSecondEnqueued = Queue->EnqueueResult<int32>(
		[SecondWorkStartedEvent]()
		{
			SecondWorkStartedEvent->Trigger();
			return 2;
		},
		[SharedState](const int32&)
		{
			SharedState->bSecondCallbackDelivered = true;
		});
	if (!bSecondEnqueued)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: second enqueue was rejected before shutdown."));
	}

	Queue->ShutdownAndSuppressCallbacks();
	Result.bShutdownRequested = true;
	Result.bQueueReportedShutdown = Queue->IsShutdown();

	const bool bPostShutdownEnqueued = Queue->EnqueueResult<int32>(
		[]()
		{
			return 3;
		},
		[SharedState](const int32&)
		{
			SharedState->bPostShutdownCallbackDelivered = true;
		});
	Result.bEnqueueAfterShutdownRejected = !bPostShutdownEnqueued;
	if (!Result.bEnqueueAfterShutdownRejected)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: enqueue after shutdown was accepted."));
	}

	FirstWorkMayFinishEvent->Trigger();
	Result.bNoDeadlock = !bFirstEnqueued || FirstWorkFinishedEvent->Wait(5000);
	if (!Result.bNoDeadlock)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: running work did not finish after shutdown."));
	}

	const double PumpStartSeconds = FPlatformTime::Seconds();
	while ((FPlatformTime::Seconds() - PumpStartSeconds) < 0.5)
	{
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FPlatformProcess::Sleep(0.01f);
	}
	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);

	Result.bPendingWorkSuppressed = !SecondWorkStartedEvent->Wait(0);
	Result.bRunningCompletionSuppressed = !SharedState->bFirstCallbackDelivered;
	Result.bNoCallbacksDeliveredAfterShutdown =
		!SharedState->bFirstCallbackDelivered
		&& !SharedState->bSecondCallbackDelivered
		&& !SharedState->bPostShutdownCallbackDelivered;

	if (!Result.bQueueReportedShutdown)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: queue did not report shutdown."));
	}
	if (!Result.bPendingWorkSuppressed)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: pending work ran after shutdown."));
	}
	if (!Result.bRunningCompletionSuppressed)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: running work completion callback was delivered after shutdown."));
	}
	if (!Result.bNoCallbacksDeliveredAfterShutdown)
	{
		AppendSQLUISampleDatabaseAsyncQueueShutdownProbeError(
			Result,
			TEXT("SQLUI database async queue shutdown probe failed: one or more callbacks were delivered after shutdown."));
	}

	Result.bSucceeded =
		Result.bQueueCreated
		&& bFirstEnqueued
		&& bFirstWorkStarted
		&& bSecondEnqueued
		&& Result.bShutdownRequested
		&& Result.bQueueReportedShutdown
		&& Result.bEnqueueAfterShutdownRejected
		&& Result.bPendingWorkSuppressed
		&& Result.bRunningCompletionSuppressed
		&& Result.bNoCallbacksDeliveredAfterShutdown
		&& Result.bNoDeadlock;

	if (Result.bNoDeadlock || !bFirstEnqueued)
	{
		FPlatformProcess::ReturnSynchEventToPool(FirstWorkStartedEvent);
		FPlatformProcess::ReturnSynchEventToPool(FirstWorkMayFinishEvent);
		FPlatformProcess::ReturnSynchEventToPool(FirstWorkFinishedEvent);
		FPlatformProcess::ReturnSynchEventToPool(SecondWorkStartedEvent);
	}

	return Result;
}

FString MakeSQLUISampleLayoutFailureMessage(
	const FSQLUISampleSmokeTestLayoutResult& LayoutResult)
{
	if (LayoutResult.bUsedJsonLayoutFixture && !LayoutResult.bJsonLayoutFixtureParseSucceeded)
	{
		return TEXT("SQLUI sample pipeline smoke test failed: JSON layout fixture parse failed.");
	}

	if (LayoutResult.bUsedJsonLayoutFixture && !LayoutResult.bJsonLayoutFixtureValidationSucceeded)
	{
		return TEXT("SQLUI sample pipeline smoke test failed: JSON layout fixture validation failed.");
	}

	if (LayoutResult.bRepositorySelectionSmokeTested &&
		!LayoutResult.bUnavailableRepositorySelectionSucceeded)
	{
		return TEXT("SQLUI sample pipeline smoke test failed: unavailable layout repository selection smoke failed.");
	}

	if (LayoutResult.bUsedInMemoryLayoutRepository && !LayoutResult.bRepositorySaveSucceeded)
	{
		return TEXT("SQLUI sample pipeline smoke test failed: in-memory layout repository save failed.");
	}

	if (LayoutResult.bUsedInMemoryLayoutRepository && !LayoutResult.bRepositoryLoadSucceeded)
	{
		return TEXT("SQLUI sample pipeline smoke test failed: in-memory layout repository load failed.");
	}

	if (LayoutResult.bUsedInMemoryLayoutRepository &&
		!DidSQLUISampleRepositoryOperationSmokeSucceed(LayoutResult.RepositoryOperationSmoke))
	{
		return TEXT("SQLUI sample pipeline smoke test failed: in-memory layout repository operation smoke failed.");
	}

	if (LayoutResult.bUsedJsonFileLayoutRepository && !LayoutResult.bJsonFileRepositorySaveSucceeded)
	{
		return TEXT("SQLUI sample pipeline smoke test failed: JSON file layout repository save failed.");
	}

	if (LayoutResult.bUsedJsonFileLayoutRepository && !LayoutResult.bJsonFileRepositoryLoadSucceeded)
	{
		return TEXT("SQLUI sample pipeline smoke test failed: JSON file layout repository load failed.");
	}

	if (LayoutResult.bUsedJsonFileLayoutRepository &&
		!DidSQLUISampleRepositoryOperationSmokeSucceed(LayoutResult.JsonFileRepositoryOperationSmoke))
	{
		return TEXT("SQLUI sample pipeline smoke test failed: JSON file layout repository operation smoke failed.");
	}

	return TEXT("SQLUI sample pipeline smoke test failed: layout document could not be resolved.");
}

void SeedSQLUISampleSmokeTestVariables(
	USQLUIVariableStore* VariableStore,
	const FSQLUISampleSmokeTestRequest& Request)
{
	if (!IsValid(VariableStore))
	{
		return;
	}

	FSQLUIVariableKey FilterTextKey;
	FilterTextKey.Name = SQLUISampleSmokeTestFilterVariableName;

	FSQLUIVariableValue FilterTextValue;
	FilterTextValue.Type = ESQLUIVariableValueType::String;
	FilterTextValue.StringValue = Request.SampleFilterVariableValue;
	VariableStore->SetVariable(FilterTextKey, FilterTextValue);
}

FSQLUISampleSmokeTestResult MakeSQLUISampleSmokeTestResult(
	const FSQLUIRuntimeWidgetPipelineResult& PipelineResult,
	const FSQLUISampleSmokeTestLayoutResult& LayoutResult)
{
	FSQLUISampleSmokeTestResult Result;
	ApplySQLUISampleLayoutResult(Result, LayoutResult);
	Result.bSucceeded = PipelineResult.bSucceeded;
	Result.ErrorMessage = PipelineResult.ErrorMessage;
	Result.Errors = PipelineResult.Errors;
	Result.Warnings = LayoutResult.Warnings;
	Result.Warnings.Append(PipelineResult.Warnings);
	Result.StepResults = PipelineResult.StepResults;
	Result.CreatedWidgetCount = PipelineResult.CreatedWidgets.Num();
	Result.RootWidget = PipelineResult.RootWidget;
	Result.bRootWidgetValid = IsValid(Result.RootWidget.Get());
	return Result;
}
}

FSQLUISampleSmokeTestResult USQLUISampleSmokeTestRunner::RunSmokeTest(
	UObject* WorldContextObject,
	const FSQLUISampleSmokeTestRequest& Request)
{
	FSQLUISampleSmokeTestResult Result;
	UObject* Outer = ResolveSQLUISampleSmokeTestOuter(WorldContextObject, Request);

	const FSQLUISampleSmokeTestLayoutResult LayoutResult =
		ResolveSQLUISampleSmokeTestLayoutDocument(Outer, Request);
	ApplySQLUISampleLayoutResult(Result, LayoutResult);
	if (!LayoutResult.bSucceeded)
	{
		AddSQLUISampleSmokeTestError(Result, MakeSQLUISampleLayoutFailureMessage(LayoutResult));
		AddSQLUISampleLayoutValidationMessages(Result, LayoutResult.JsonLayoutFixtureValidation);
		AddSQLUISampleLayoutValidationMessages(Result, LayoutResult.RepositorySaveValidation);
		AddSQLUISampleLayoutValidationMessages(Result, LayoutResult.RepositoryLoadValidation);
		AddSQLUISampleLayoutValidationMessages(Result, LayoutResult.JsonFileRepositorySaveValidation);
		AddSQLUISampleLayoutValidationMessages(Result, LayoutResult.JsonFileRepositoryLoadValidation);
		AddSQLUISampleSmokeTestError(Result, LayoutResult.UnavailableRepositorySelectionErrorMessage);
		AddSQLUISampleSmokeTestError(Result, LayoutResult.RepositorySaveErrorMessage);
		AddSQLUISampleSmokeTestError(Result, LayoutResult.RepositoryLoadErrorMessage);
		AddSQLUISampleSmokeTestError(Result, LayoutResult.JsonFileRepositorySaveErrorMessage);
		AddSQLUISampleSmokeTestError(Result, LayoutResult.JsonFileRepositoryLoadErrorMessage);
		AddSQLUISampleRepositoryOperationMessages(Result, LayoutResult.RepositoryOperationSmoke);
		AddSQLUISampleRepositoryOperationMessages(Result, LayoutResult.JsonFileRepositoryOperationSmoke);
		Result.Warnings.Append(LayoutResult.Warnings);
		return Result;
	}

	USQLUIWidgetCatalog* WidgetCatalog = NewObject<USQLUIWidgetCatalog>(Outer);
	if (!IsValid(WidgetCatalog))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not create widget catalog."));
		return Result;
	}

	if (!USQLUIWidgetCatalogRegistrar::RegisterDefaultSQLUIWidgets(WidgetCatalog))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not register default widget catalog entries."));
		return Result;
	}

	USQLUIVariableStore* VariableStore = NewObject<USQLUIVariableStore>(Outer);
	if (!IsValid(VariableStore))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not create variable store."));
		return Result;
	}

	SeedSQLUISampleSmokeTestVariables(VariableStore, Request);

	USQLUIRuntimeContext* RuntimeContext = NewObject<USQLUIRuntimeContext>(Outer);
	if (!IsValid(RuntimeContext))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not create runtime context."));
		return Result;
	}

	FSQLUIRuntimeContextSettings RuntimeContextSettings;
	RuntimeContextSettings.VariableStore = VariableStore;
	RuntimeContextSettings.WidgetCatalog = WidgetCatalog;
	RuntimeContext->Initialize(RuntimeContextSettings);

	if (!RuntimeContext->IsInitialized())
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: runtime context did not initialize."));
		return Result;
	}

	USQLUIRuntimeWidgetPipeline* Pipeline = NewObject<USQLUIRuntimeWidgetPipeline>(Outer);
	if (!IsValid(Pipeline))
	{
		AddSQLUISampleSmokeTestError(
			Result,
			TEXT("SQLUI sample pipeline smoke test failed: could not create runtime widget pipeline."));
		return Result;
	}

	FSQLUIRuntimeWidgetPipelineRequest PipelineRequest;
	PipelineRequest.Document = LayoutResult.Document;
	PipelineRequest.RuntimeContext = RuntimeContext;
	PipelineRequest.WidgetCatalogOverride = WidgetCatalog;
	PipelineRequest.WorldContextObject = WorldContextObject;
	PipelineRequest.OwningPlayer = Request.OwningPlayer;
	PipelineRequest.bApplyProperties = Request.bApplyProperties;
	PipelineRequest.bApplyBindings = Request.bApplyBindings;
	PipelineRequest.bApplyActions = Request.bApplyActions;
	PipelineRequest.bExecuteActions = Request.bExecuteActions;
	PipelineRequest.bStopOnOptionalStepFailure = Request.bStopOnOptionalStepFailure;

	Result = MakeSQLUISampleSmokeTestResult(Pipeline->RunPipeline(PipelineRequest), LayoutResult);

	if (Request.bUseSQLiteCoreProbe)
	{
		Result.bUsedSQLiteCoreProbe = true;
		Result.SQLiteCoreProbe = FSQLUISQLiteProbe::RunOpenCloseProbe();
		if (!Result.SQLiteCoreProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteCoreProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLiteCore probe failed.")
					: Result.SQLiteCoreProbe.ErrorMessage);
		}
	}

	if (Request.bUseDatabaseAsyncProbe)
	{
		Result.bUsedDatabaseAsyncProbe = true;
		Result.DatabaseAsyncProbe = RunSQLUISampleDatabaseAsyncProbe();
		if (!DidSQLUISampleDatabaseAsyncProbeSucceed(Result.DatabaseAsyncProbe))
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				MakeSQLUISampleDatabaseAsyncProbeFailureMessage(Result.DatabaseAsyncProbe));
		}
	}

	if (Request.bUseDatabaseAsyncQueueShutdownProbe)
	{
		Result.bUsedDatabaseAsyncQueueShutdownProbe = true;
		Result.DatabaseAsyncQueueShutdownProbe =
			RunSQLUISampleDatabaseAsyncQueueShutdownProbe();
		if (!DidSQLUISampleDatabaseAsyncQueueShutdownProbeSucceed(
			Result.DatabaseAsyncQueueShutdownProbe))
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				MakeSQLUISampleDatabaseAsyncQueueShutdownProbeFailureMessage(
					Result.DatabaseAsyncQueueShutdownProbe));
		}
	}

	if (Request.bUseLayoutRepositoryRuntimeConfigProbe)
	{
		Result.bUsedLayoutRepositoryRuntimeConfigProbe = true;
		Result.LayoutRepositoryRuntimeConfigProbe =
			RunSQLUISampleLayoutRepositoryRuntimeConfigProbe(Outer);
		if (!Result.LayoutRepositoryRuntimeConfigProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.LayoutRepositoryRuntimeConfigProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime config probe failed.")
					: Result.LayoutRepositoryRuntimeConfigProbe.ErrorMessage);
		}
	}

	if (Request.bUseLayoutRepositoryRuntimeIntegrationProbe)
	{
		Result.bUsedLayoutRepositoryRuntimeIntegrationProbe = true;
		Result.LayoutRepositoryRuntimeIntegrationProbe =
			RunSQLUISampleLayoutRepositoryRuntimeIntegrationProbe(Outer);
		if (!Result.LayoutRepositoryRuntimeIntegrationProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.LayoutRepositoryRuntimeIntegrationProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime integration probe failed.")
					: Result.LayoutRepositoryRuntimeIntegrationProbe.ErrorMessage);
		}
	}

	if (Request.bUseLayoutRepositoryRuntimeProviderProbe)
	{
		Result.bUsedLayoutRepositoryRuntimeProviderProbe = true;
		Result.LayoutRepositoryRuntimeProviderProbe =
			RunSQLUISampleLayoutRepositoryRuntimeProviderProbe(Outer);
		if (!Result.LayoutRepositoryRuntimeProviderProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.LayoutRepositoryRuntimeProviderProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime provider probe failed.")
					: Result.LayoutRepositoryRuntimeProviderProbe.ErrorMessage);
		}
	}

	if (Request.bUseLayoutRepositoryRuntimeSettingsProbe)
	{
		Result.bUsedLayoutRepositoryRuntimeSettingsProbe = true;
		Result.LayoutRepositoryRuntimeSettingsProbe =
			RunSQLUISampleLayoutRepositoryRuntimeSettingsProbe(Outer);
		if (!Result.LayoutRepositoryRuntimeSettingsProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.LayoutRepositoryRuntimeSettingsProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository runtime settings probe failed.")
					: Result.LayoutRepositoryRuntimeSettingsProbe.ErrorMessage);
		}
	}

	if (Request.bUseLayoutPersistenceWorkflowProbe)
	{
		Result.bUsedLayoutPersistenceWorkflowProbe = true;
		Result.LayoutPersistenceWorkflowProbe =
			RunSQLUISampleLayoutPersistenceWorkflowProbe(Outer);
		if (!Result.LayoutPersistenceWorkflowProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.LayoutPersistenceWorkflowProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout persistence workflow probe failed.")
					: Result.LayoutPersistenceWorkflowProbe.ErrorMessage);
		}
	}

	if (Request.bUseLayoutRepositoryDatabaseManagementProbe)
	{
		Result.bUsedLayoutRepositoryDatabaseManagementProbe = true;
		Result.LayoutRepositoryDatabaseManagementProbe =
			RunSQLUISampleLayoutRepositoryDatabaseManagementProbe(Outer);
		if (!Result.LayoutRepositoryDatabaseManagementProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.LayoutRepositoryDatabaseManagementProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI layout repository database management probe failed.")
					: Result.LayoutRepositoryDatabaseManagementProbe.ErrorMessage);
		}
	}

	if (Request.bUsePersistenceStatusSurfaceProbe)
	{
		Result.bUsedPersistenceStatusSurfaceProbe = true;
		Result.PersistenceStatusSurfaceProbe =
			RunSQLUISamplePersistenceStatusSurfaceProbe(Outer);
		if (!Result.PersistenceStatusSurfaceProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.PersistenceStatusSurfaceProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI persistence status surface probe failed.")
					: Result.PersistenceStatusSurfaceProbe.ErrorMessage);
		}
	}

	if (Request.bUsePersistenceStatusDisplayRowsProbe)
	{
		Result.bUsedPersistenceStatusDisplayRowsProbe = true;
		Result.PersistenceStatusDisplayRowsProbe =
			RunSQLUISamplePersistenceStatusDisplayRowsProbe(Outer);
		if (!Result.PersistenceStatusDisplayRowsProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.PersistenceStatusDisplayRowsProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI persistence status display rows probe failed.")
					: Result.PersistenceStatusDisplayRowsProbe.ErrorMessage);
		}
	}

	if (Request.bUsePersistenceStatusSampleSurfaceProbe)
	{
		Result.bUsedPersistenceStatusSampleSurfaceProbe = true;
		Result.PersistenceStatusSampleSurfaceProbe =
			RunSQLUISamplePersistenceStatusSampleSurfaceProbe(Outer);
		if (!Result.PersistenceStatusSampleSurfaceProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.PersistenceStatusSampleSurfaceProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI persistence status sample surface probe failed.")
					: Result.PersistenceStatusSampleSurfaceProbe.ErrorMessage);
		}
	}

	if (Request.bUsePersistenceSettingsDraftProbe)
	{
		Result.bUsedPersistenceSettingsDraftProbe = true;
		Result.PersistenceSettingsDraftProbe =
			RunSQLUISamplePersistenceSettingsDraftProbe(WorldContextObject);
		if (!Result.PersistenceSettingsDraftProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.PersistenceSettingsDraftProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI persistence settings draft probe failed.")
					: Result.PersistenceSettingsDraftProbe.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteMigrationProbe)
	{
		Result.bUsedSQLiteMigrationProbe = true;
		Result.SQLiteMigrationProbe = FSQLUISQLiteMigrationRunner::RunMigrationProbe();
		if (!DidSQLUISampleSQLiteMigrationProbeSucceed(Result.SQLiteMigrationProbe))
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				MakeSQLUISampleSQLiteMigrationProbeFailureMessage(Result.SQLiteMigrationProbe));
		}
	}

	if (Request.bUseSQLiteLayoutSchemaMigrationProbe)
	{
		Result.bUsedSQLiteLayoutSchemaMigrationProbe = true;
		Result.SQLiteLayoutSchemaMigrationProbe =
			FSQLUISQLiteLayoutSchemaMigration::RunProbe();
		if (!DidSQLUISampleSQLiteLayoutSchemaMigrationProbeSucceed(
			Result.SQLiteLayoutSchemaMigrationProbe))
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				MakeSQLUISampleSQLiteLayoutSchemaMigrationProbeFailureMessage(
					Result.SQLiteLayoutSchemaMigrationProbe));
		}
	}

	if (Request.bUseSQLiteLayoutReadProbe)
	{
		Result.bUsedSQLiteLayoutReadProbe = true;
		Result.SQLiteLayoutReadProbe = FSQLUISQLiteLayoutReadProbe::RunProbe();
		if (!DidSQLUISampleSQLiteLayoutReadProbeSucceed(Result.SQLiteLayoutReadProbe))
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				MakeSQLUISampleSQLiteLayoutReadProbeFailureMessage(
					Result.SQLiteLayoutReadProbe));
		}
	}

	if (Request.bUseSQLiteReadOnlyLayoutRepository)
	{
		Result.bUsedSQLiteReadOnlyLayoutRepository = true;
		Result.SQLiteReadOnlyLayoutRepository =
			RunSQLUISampleSQLiteReadOnlyLayoutRepositorySmoke(Outer);
		if (!Result.SQLiteReadOnlyLayoutRepository.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteReadOnlyLayoutRepository.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite read-only layout repository smoke failed.")
					: Result.SQLiteReadOnlyLayoutRepository.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteSaveLayoutRepository)
	{
		Result.bUsedSQLiteSaveLayoutRepository = true;
		Result.SQLiteSaveLayoutRepository =
			RunSQLUISampleSQLiteSaveLayoutRepositorySmoke(Outer);
		if (!Result.SQLiteSaveLayoutRepository.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteSaveLayoutRepository.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite SaveLayout repository smoke failed.")
					: Result.SQLiteSaveLayoutRepository.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteRemoveLayoutRepository)
	{
		Result.bUsedSQLiteRemoveLayoutRepository = true;
		Result.SQLiteRemoveLayoutRepository =
			RunSQLUISampleSQLiteRemoveLayoutRepositorySmoke(Outer);
		if (!Result.SQLiteRemoveLayoutRepository.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteRemoveLayoutRepository.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite RemoveLayout repository smoke failed.")
					: Result.SQLiteRemoveLayoutRepository.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteClearLayoutsRepository)
	{
		Result.bUsedSQLiteClearLayoutsRepository = true;
		Result.SQLiteClearLayoutsRepository =
			RunSQLUISampleSQLiteClearLayoutsRepositorySmoke(Outer);
		if (!Result.SQLiteClearLayoutsRepository.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteClearLayoutsRepository.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite ClearLayouts repository smoke failed.")
					: Result.SQLiteClearLayoutsRepository.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteFullLifecycleRepository)
	{
		Result.bUsedSQLiteFullLifecycleRepository = true;
		Result.SQLiteFullLifecycleRepository =
			RunSQLUISampleSQLiteFullLifecycleRepositorySmoke(Outer);
		if (!Result.SQLiteFullLifecycleRepository.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteFullLifecycleRepository.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite full lifecycle repository smoke failed.")
					: Result.SQLiteFullLifecycleRepository.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteAsyncCallbackRepository)
	{
		Result.bUsedSQLiteAsyncCallbackRepository = true;
		Result.SQLiteAsyncCallbackRepository =
			RunSQLUISampleSQLiteAsyncCallbackRepositorySmoke(Outer);
		if (!Result.SQLiteAsyncCallbackRepository.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteAsyncCallbackRepository.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite async callback repository smoke failed.")
					: Result.SQLiteAsyncCallbackRepository.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteSerializedAsyncCallbackRepository)
	{
		Result.bUsedSQLiteSerializedAsyncCallbackRepository = true;
		Result.SQLiteSerializedAsyncCallbackRepository =
			RunSQLUISampleSQLiteSerializedAsyncCallbackRepositorySmoke(Outer);
		if (!Result.SQLiteSerializedAsyncCallbackRepository.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteSerializedAsyncCallbackRepository.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite serialized async callback repository smoke failed.")
					: Result.SQLiteSerializedAsyncCallbackRepository.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteFactoryLayoutRepository)
	{
		Result.bUsedSQLiteFactoryLayoutRepository = true;
		Result.SQLiteFactoryLayoutRepository =
			RunSQLUISampleSQLiteFactoryLayoutRepositorySmoke(Outer);
		if (!Result.SQLiteFactoryLayoutRepository.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteFactoryLayoutRepository.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory layout repository smoke failed.")
					: Result.SQLiteFactoryLayoutRepository.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteFactorySchemaInitRepository)
	{
		Result.bUsedSQLiteFactorySchemaInitRepository = true;
		Result.SQLiteFactorySchemaInitRepository =
			RunSQLUISampleSQLiteFactorySchemaInitRepositorySmoke(Outer);
		if (!Result.SQLiteFactorySchemaInitRepository.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteFactorySchemaInitRepository.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite factory schema init repository smoke failed.")
					: Result.SQLiteFactorySchemaInitRepository.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteSchemaInitHardening)
	{
		Result.bUsedSQLiteSchemaInitHardening = true;
		Result.SQLiteSchemaInitHardening =
			RunSQLUISampleSQLiteSchemaInitHardeningSmoke(Outer);
		if (!Result.SQLiteSchemaInitHardening.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteSchemaInitHardening.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite schema init hardening smoke failed.")
					: Result.SQLiteSchemaInitHardening.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteSeedDatabaseCopyPolicyProbe)
	{
		Result.bUsedSQLiteSeedDatabaseCopyPolicyProbe = true;
		Result.SQLiteSeedDatabaseCopyPolicyProbe =
			RunSQLUISampleSQLiteSeedDatabaseCopyPolicyProbe(Outer);
		if (!Result.SQLiteSeedDatabaseCopyPolicyProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteSeedDatabaseCopyPolicyProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite seed database copy policy probe failed.")
					: Result.SQLiteSeedDatabaseCopyPolicyProbe.ErrorMessage);
		}
	}

	if (Request.bUseSQLiteMigrationVersioningPolicyProbe)
	{
		Result.bUsedSQLiteMigrationVersioningPolicyProbe = true;
		Result.SQLiteMigrationVersioningPolicyProbe =
			RunSQLUISampleSQLiteMigrationVersioningPolicyProbe();
		if (!Result.SQLiteMigrationVersioningPolicyProbe.bSucceeded)
		{
			Result.bSucceeded = false;
			AddSQLUISampleSmokeTestError(
				Result,
				Result.SQLiteMigrationVersioningPolicyProbe.ErrorMessage.IsEmpty()
					? TEXT("SQLUI SQLite migration versioning policy probe failed.")
					: Result.SQLiteMigrationVersioningPolicyProbe.ErrorMessage);
		}
	}

	return Result;
}

FSQLUISampleSmokeTestResult USQLUISampleSmokeTestRunner::RunJsonLayoutSmokeTest(
	UObject* WorldContextObject,
	const FSQLUISampleSmokeTestRequest& Request)
{
	FSQLUISampleSmokeTestRequest JsonRequest = Request;
	JsonRequest.bUseJsonLayoutFixture = true;
	return RunSmokeTest(WorldContextObject, JsonRequest);
}
