#include "SQLUISampleSmokeTestRunner.h"

#include "Layout/ISQLUILayoutRepository.h"
#include "Layout/SQLUIInMemoryLayoutRepository.h"
#include "Layout/SQLUIJsonFileLayoutRepository.h"
#include "Layout/SQLUILayoutJson.h"
#include "Layout/SQLUILayoutRepositoryFactory.h"
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

	return MakeSQLUISampleSmokeTestResult(Pipeline->RunPipeline(PipelineRequest), LayoutResult);
}

FSQLUISampleSmokeTestResult USQLUISampleSmokeTestRunner::RunJsonLayoutSmokeTest(
	UObject* WorldContextObject,
	const FSQLUISampleSmokeTestRequest& Request)
{
	FSQLUISampleSmokeTestRequest JsonRequest = Request;
	JsonRequest.bUseJsonLayoutFixture = true;
	return RunSmokeTest(WorldContextObject, JsonRequest);
}
