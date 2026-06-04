#include "SQLUISampleSmokeTestRunner.h"

#include "Database/SQLUIDatabaseAsyncRunner.h"
#include "Database/SQLUISQLiteLayoutReadProbe.h"
#include "Database/SQLUISQLiteLayoutSchemaMigration.h"
#include "Database/SQLUISQLiteMigrationRunner.h"
#include "Database/SQLUISQLiteProbe.h"
#include "Async/TaskGraphInterfaces.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Layout/ISQLUILayoutRepository.h"
#include "Layout/SQLUIInMemoryLayoutRepository.h"
#include "Layout/SQLUIJsonFileLayoutRepository.h"
#include "Layout/SQLUILayoutJson.h"
#include "Layout/SQLUILayoutRepositoryFactory.h"
#include "Layout/SQLUISQLiteLayoutRepository.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Misc/Paths.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "Variables/SQLUIVariableStore.h"
#include "Variables/SQLUIVariableTypes.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"
#include "WidgetCatalog/SQLUIWidgetCatalogRegistrar.h"
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
