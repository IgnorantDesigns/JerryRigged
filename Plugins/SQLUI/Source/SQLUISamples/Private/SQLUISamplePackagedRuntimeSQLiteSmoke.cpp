#include "SQLUISamplePackagedRuntimeSQLiteSmoke.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Layout/ISQLUILayoutRepository.h"
#include "Layout/SQLUILayoutRepositoryFactory.h"
#include "Layout/SQLUISQLiteLayoutRepository.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Misc/Paths.h"
#include "SQLUISamplesModule.h"
#include "UObject/Package.h"

namespace
{
struct FSQLUISamplePackagedRuntimeSQLiteSmokeResult
{
	bool bSucceeded = false;
	bool bCreatedRepository = false;
	bool bCreatedSQLiteRepository = false;
	bool bSaveSucceeded = false;
	bool bDatabaseCreated = false;
	bool bListSucceeded = false;
	bool bListedMetadataFound = false;
	bool bLoadSucceeded = false;
	bool bLoadedDocumentValid = false;
	bool bRemoveSucceeded = false;
	bool bRemoved = false;
	bool bListAfterRemoveSucceeded = false;
	bool bMetadataAbsentAfterRemove = false;
	bool bClearSucceeded = false;
	bool bDatabaseRemoved = false;
	FString DatabasePath;
	FString ErrorMessage;
};

void AppendSQLUIPackagedRuntimeSQLiteSmokeError(
	FSQLUISamplePackagedRuntimeSQLiteSmokeResult& Result,
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

FString MakeSQLUIPackagedRuntimeSQLiteSmokeDatabasePath()
{
	FString DatabasePath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("PackagedRuntimeSmoke"),
		TEXT("SQLiteLifecycle"),
		TEXT("SQLUIPackagedRuntimeSQLiteLifecycle.db"));
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUIPackagedRuntimeSQLiteSmokeFiles(
	const FString& DatabasePath,
	FSQLUISamplePackagedRuntimeSQLiteSmokeResult& Result)
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
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				FString::Printf(
					TEXT("SQLUI packaged runtime SQLite smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

FSQLUILayoutDocument MakeSQLUIPackagedRuntimeSQLiteSmokeDocument()
{
	FSQLUILayoutDocument Document;
	Document.Version.SchemaVersion = 1;
	Document.Version.Revision = 1;
	Document.Version.Label = TEXT("SQLUI Packaged Runtime SQLite Smoke");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.packaged-runtime.sqlite-lifecycle");
	Document.Metadata.DisplayName = TEXT("SQLUI Packaged Runtime SQLite Smoke");
	Document.Metadata.Description = TEXT("Probe-only layout for packaged runtime SQLite lifecycle validation.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-06-04T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = TEXT("2026-06-04T00:00:00Z");
	Document.Metadata.Tags = {
		TEXT("smoke"),
		TEXT("packaged-runtime"),
		TEXT("sqlite")
	};
	Document.RootWidgetId = TEXT("SQLUI.PackagedRuntime.SQLiteSmoke.Root");

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = Document.RootWidgetId;
	RootNode.WidgetTypeKey = TEXT("SQLUI.FilterBox");
	RootNode.Properties.Add(TEXT("FilterText"), TEXT("SQLUI packaged runtime SQLite smoke"));
	RootNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));
	RootNode.Tags = {
		TEXT("smoke-root")
	};

	Document.Nodes.Add(RootNode);
	return Document;
}

bool DoSQLUIPackagedRuntimeSQLiteSmokeTagsMatch(
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

bool DoesSQLUIPackagedRuntimeSQLiteSmokeMetadataMatch(
	const FSQLUILayoutMetadata& Candidate,
	const FSQLUILayoutMetadata& Expected)
{
	return Candidate.LayoutId == Expected.LayoutId
		&& Candidate.DisplayName == Expected.DisplayName
		&& Candidate.Description == Expected.Description
		&& Candidate.CreatedBy == Expected.CreatedBy
		&& DoSQLUIPackagedRuntimeSQLiteSmokeTagsMatch(Candidate.Tags, Expected.Tags);
}

bool DoesSQLUIPackagedRuntimeSQLiteSmokeListContainMetadata(
	const TArray<FSQLUILayoutMetadata>& Layouts,
	const FSQLUILayoutMetadata& Expected)
{
	for (const FSQLUILayoutMetadata& Layout : Layouts)
	{
		if (DoesSQLUIPackagedRuntimeSQLiteSmokeMetadataMatch(Layout, Expected))
		{
			return true;
		}
	}

	return false;
}

FSQLUILayoutSaveResult SaveSQLUIPackagedRuntimeSQLiteSmokeLayout(
	USQLUISQLiteLayoutRepository* Repository,
	const FSQLUILayoutDocument& Document)
{
	FSQLUILayoutSaveResult SaveResult;
	SaveResult.SavedLayoutId = Document.Metadata.LayoutId;

	if (!IsValid(Repository))
	{
		SaveResult.ErrorMessage =
			TEXT("SQLUI packaged runtime SQLite smoke failed: SQLite repository was not valid.");
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
		SaveResult.ErrorMessage =
			TEXT("SQLUI packaged runtime SQLite smoke failed: SaveLayout callback did not complete synchronously.");
	}

	return SaveResult;
}

FSQLUILayoutLoadResult LoadSQLUIPackagedRuntimeSQLiteSmokeLayout(
	USQLUISQLiteLayoutRepository* Repository,
	const FString& LayoutId)
{
	FSQLUILayoutLoadResult LoadResult;
	LoadResult.Document.Metadata.LayoutId = LayoutId;

	if (!IsValid(Repository))
	{
		LoadResult.ErrorMessage =
			TEXT("SQLUI packaged runtime SQLite smoke failed: SQLite repository was not valid.");
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
		LoadResult.ErrorMessage =
			TEXT("SQLUI packaged runtime SQLite smoke failed: LoadLayout callback did not complete synchronously.");
	}

	return LoadResult;
}

void LogSQLUIPackagedRuntimeSQLiteSmokeResult(
	const FSQLUISamplePackagedRuntimeSQLiteSmokeResult& Result)
{
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke selected: true"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke database path: '%s'"), *Result.DatabasePath);
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke created repository: %s"), Result.bCreatedRepository ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke created SQLite repository: %s"), Result.bCreatedSQLiteRepository ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke save succeeded: %s"), Result.bSaveSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke database created: %s"), Result.bDatabaseCreated ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke list succeeded: %s"), Result.bListSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke listed metadata found: %s"), Result.bListedMetadataFound ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke load succeeded: %s"), Result.bLoadSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke loaded document valid: %s"), Result.bLoadedDocumentValid ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke remove succeeded: %s"), Result.bRemoveSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke removed: %s"), Result.bRemoved ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke metadata absent after remove: %s"), Result.bMetadataAbsentAfterRemove ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke clear succeeded: %s"), Result.bClearSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke database removed: %s"), Result.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (Result.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime SQLite smoke succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI packaged runtime SQLite smoke failed: %s"),
			Result.ErrorMessage.IsEmpty() ? TEXT("unknown failure") : *Result.ErrorMessage);
	}
}

FSQLUISamplePackagedRuntimeSQLiteSmokeResult RunSQLUIPackagedRuntimeSQLiteSmoke()
{
	FSQLUISamplePackagedRuntimeSQLiteSmokeResult Result;
	Result.DatabasePath = MakeSQLUIPackagedRuntimeSQLiteSmokeDatabasePath();

	if (!DeleteSQLUIPackagedRuntimeSQLiteSmokeFiles(Result.DatabasePath, Result))
	{
		Result.bDatabaseRemoved = false;
		LogSQLUIPackagedRuntimeSQLiteSmokeResult(Result);
		return Result;
	}

	FSQLUILayoutRepositoryFactorySettings FactorySettings;
	FactorySettings.Backend = ESQLUILayoutRepositoryBackend::SQLite;
	FactorySettings.SQLiteSettings.DatabasePath = Result.DatabasePath;
	FactorySettings.SQLiteSettings.bReadOnly = false;
	FactorySettings.SQLiteSettings.bRunCallbackOperationsAsync = false;
	FactorySettings.SQLiteSettings.bInitializeSchemaIfMissing = true;
	FactorySettings.SQLiteSettings.bCreateDatabaseIfMissing = true;

	USQLUILayoutRepository* Repository =
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(GetTransientPackage(), FactorySettings);
	Result.bCreatedRepository = IsValid(Repository);

	USQLUISQLiteLayoutRepository* SQLiteRepository = Cast<USQLUISQLiteLayoutRepository>(Repository);
	Result.bCreatedSQLiteRepository = IsValid(SQLiteRepository);
	if (!Result.bCreatedSQLiteRepository)
	{
		AppendSQLUIPackagedRuntimeSQLiteSmokeError(
			Result,
			TEXT("SQLUI packaged runtime SQLite smoke failed: factory did not create a SQLite repository."));
		Result.bDatabaseRemoved = DeleteSQLUIPackagedRuntimeSQLiteSmokeFiles(Result.DatabasePath, Result);
		LogSQLUIPackagedRuntimeSQLiteSmokeResult(Result);
		return Result;
	}

	const FSQLUILayoutDocument Document = MakeSQLUIPackagedRuntimeSQLiteSmokeDocument();
	const FString LayoutId = Document.Metadata.LayoutId;

	const FSQLUILayoutSaveResult SaveResult =
		SaveSQLUIPackagedRuntimeSQLiteSmokeLayout(SQLiteRepository, Document);
	Result.bSaveSucceeded = SaveResult.bSucceeded && SaveResult.SavedLayoutId == LayoutId;
	Result.bDatabaseCreated = FPaths::FileExists(Result.DatabasePath);
	if (!Result.bSaveSucceeded)
	{
		AppendSQLUIPackagedRuntimeSQLiteSmokeError(
			Result,
			SaveResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI packaged runtime SQLite smoke failed: SaveLayout did not succeed.")
				: SaveResult.ErrorMessage);
	}
	if (!Result.bDatabaseCreated)
	{
		AppendSQLUIPackagedRuntimeSQLiteSmokeError(
			Result,
			TEXT("SQLUI packaged runtime SQLite smoke failed: database file was not created after SaveLayout."));
	}

	if (Result.bSaveSucceeded)
	{
		const FSQLUILayoutRepositoryListResult ListResult = SQLiteRepository->ListLayouts();
		Result.bListSucceeded = ListResult.bSucceeded;
		Result.bListedMetadataFound =
			Result.bListSucceeded
			&& DoesSQLUIPackagedRuntimeSQLiteSmokeListContainMetadata(
				ListResult.Layouts,
				Document.Metadata);
		if (!Result.bListSucceeded)
		{
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				ListResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime SQLite smoke failed: ListLayouts did not succeed.")
					: ListResult.ErrorMessage);
		}
		else if (!Result.bListedMetadataFound)
		{
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				TEXT("SQLUI packaged runtime SQLite smoke failed: ListLayouts did not include saved metadata and tags."));
		}

		const FSQLUILayoutLoadResult LoadResult =
			LoadSQLUIPackagedRuntimeSQLiteSmokeLayout(SQLiteRepository, LayoutId);
		Result.bLoadSucceeded = LoadResult.bSucceeded;
		Result.bLoadedDocumentValid =
			Result.bLoadSucceeded
			&& LoadResult.Validation.bIsValid
			&& LoadResult.Document.Metadata.LayoutId == LayoutId;
		if (!Result.bLoadSucceeded)
		{
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				LoadResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime SQLite smoke failed: LoadLayout did not succeed.")
					: LoadResult.ErrorMessage);
		}
		else if (!Result.bLoadedDocumentValid)
		{
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				TEXT("SQLUI packaged runtime SQLite smoke failed: loaded document was invalid or had the wrong layout id."));
		}

		const FSQLUILayoutRepositoryRemoveResult RemoveResult =
			SQLiteRepository->RemoveLayout(LayoutId);
		Result.bRemoveSucceeded = RemoveResult.bSucceeded;
		Result.bRemoved = RemoveResult.bRemoved && RemoveResult.RemovedLayoutId == LayoutId;
		if (!Result.bRemoveSucceeded)
		{
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				RemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime SQLite smoke failed: RemoveLayout did not succeed.")
					: RemoveResult.ErrorMessage);
		}
		else if (!Result.bRemoved)
		{
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				TEXT("SQLUI packaged runtime SQLite smoke failed: RemoveLayout did not remove the saved layout."));
		}

		const FSQLUILayoutRepositoryListResult ListAfterRemoveResult =
			SQLiteRepository->ListLayouts();
		Result.bListAfterRemoveSucceeded = ListAfterRemoveResult.bSucceeded;
		Result.bMetadataAbsentAfterRemove =
			Result.bListAfterRemoveSucceeded
			&& !DoesSQLUIPackagedRuntimeSQLiteSmokeListContainMetadata(
				ListAfterRemoveResult.Layouts,
				Document.Metadata);
		if (!Result.bListAfterRemoveSucceeded)
		{
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				ListAfterRemoveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime SQLite smoke failed: ListLayouts after remove did not succeed.")
					: ListAfterRemoveResult.ErrorMessage);
		}
		else if (!Result.bMetadataAbsentAfterRemove)
		{
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				TEXT("SQLUI packaged runtime SQLite smoke failed: removed metadata was still listed after RemoveLayout."));
		}

		const FSQLUILayoutRepositoryClearResult ClearResult = SQLiteRepository->ClearLayouts();
		Result.bClearSucceeded = ClearResult.bSucceeded;
		if (!Result.bClearSucceeded)
		{
			AppendSQLUIPackagedRuntimeSQLiteSmokeError(
				Result,
				ClearResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime SQLite smoke failed: ClearLayouts did not succeed.")
					: ClearResult.ErrorMessage);
		}
	}

	Result.bDatabaseRemoved = DeleteSQLUIPackagedRuntimeSQLiteSmokeFiles(Result.DatabasePath, Result);
	Result.bSucceeded =
		Result.bCreatedRepository
		&& Result.bCreatedSQLiteRepository
		&& Result.bSaveSucceeded
		&& Result.bDatabaseCreated
		&& Result.bListSucceeded
		&& Result.bListedMetadataFound
		&& Result.bLoadSucceeded
		&& Result.bLoadedDocumentValid
		&& Result.bRemoveSucceeded
		&& Result.bRemoved
		&& Result.bListAfterRemoveSucceeded
		&& Result.bMetadataAbsentAfterRemove
		&& Result.bClearSucceeded
		&& Result.bDatabaseRemoved;

	LogSQLUIPackagedRuntimeSQLiteSmokeResult(Result);
	return Result;
}
}

void FSQLUISamplePackagedRuntimeSQLiteSmoke::RunAndRequestExit()
{
	const FSQLUISamplePackagedRuntimeSQLiteSmokeResult Result =
		RunSQLUIPackagedRuntimeSQLiteSmoke();

	FPlatformMisc::RequestExitWithStatus(
		false,
		Result.bSucceeded ? 0 : 1,
		TEXT("SQLUI packaged runtime SQLite smoke"));
}
