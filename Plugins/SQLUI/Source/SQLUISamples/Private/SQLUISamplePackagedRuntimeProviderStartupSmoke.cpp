#include "SQLUISamplePackagedRuntimeProviderStartupSmoke.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Layout/SQLUILayoutRepositoryRuntimeProvider.h"
#include "Layout/SQLUISQLiteLayoutRepository.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "SQLUISamplesModule.h"
#include "UObject/Package.h"

namespace
{
struct FSQLUISamplePackagedRuntimeProviderStartupSmokeResult
{
	bool bSucceeded = false;
	bool bProviderCreated = false;
	bool bInitializedFromCommandLine = false;
	bool bInitializationSucceeded = false;
	bool bBackendSQLite = false;
	bool bRepositoryAvailable = false;
	bool bRepositoryIsSQLite = false;
	bool bSaveSucceeded = false;
	bool bDatabaseCreated = false;
	bool bListSucceeded = false;
	bool bListedMetadataFound = false;
	bool bLoadSucceeded = false;
	bool bLoadedDocumentValid = false;
	bool bLoadedLayoutIdMatched = false;
	bool bProviderResetClearedRepository = false;
	bool bDatabaseRemoved = false;
	FString DatabasePath;
	FString ErrorMessage;
};

void AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
	FSQLUISamplePackagedRuntimeProviderStartupSmokeResult& Result,
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

bool DeleteSQLUIPackagedRuntimeProviderStartupSmokeFiles(
	const FString& DatabasePath,
	FSQLUISamplePackagedRuntimeProviderStartupSmokeResult& Result)
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
			AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
				Result,
				FString::Printf(
					TEXT("SQLUI packaged runtime provider startup smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DoSQLUIPackagedRuntimeProviderStartupSmokeTagsMatch(
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

bool DoesSQLUIPackagedRuntimeProviderStartupSmokeMetadataMatch(
	const FSQLUILayoutMetadata& Candidate,
	const FSQLUILayoutMetadata& Expected)
{
	return Candidate.LayoutId == Expected.LayoutId
		&& Candidate.DisplayName == Expected.DisplayName
		&& Candidate.Description == Expected.Description
		&& Candidate.CreatedBy == Expected.CreatedBy
		&& DoSQLUIPackagedRuntimeProviderStartupSmokeTagsMatch(Candidate.Tags, Expected.Tags);
}

bool DoesSQLUIPackagedRuntimeProviderStartupSmokeListContainMetadata(
	const TArray<FSQLUILayoutMetadata>& Layouts,
	const FSQLUILayoutMetadata& Expected)
{
	for (const FSQLUILayoutMetadata& Layout : Layouts)
	{
		if (DoesSQLUIPackagedRuntimeProviderStartupSmokeMetadataMatch(Layout, Expected))
		{
			return true;
		}
	}

	return false;
}

FSQLUILayoutDocument MakeSQLUIPackagedRuntimeProviderStartupSmokeDocument()
{
	FSQLUILayoutDocument Document;
	Document.Version.SchemaVersion = 1;
	Document.Version.Revision = 1;
	Document.Version.Label = TEXT("SQLUI Packaged Runtime Provider Startup Smoke");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.packaged-runtime.provider-startup");
	Document.Metadata.DisplayName = TEXT("SQLUI Packaged Runtime Provider Startup Smoke");
	Document.Metadata.Description =
		TEXT("Probe-only layout for packaged runtime provider startup validation.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-06-05T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = TEXT("2026-06-05T00:00:00Z");
	Document.Metadata.Tags = {
		TEXT("smoke"),
		TEXT("packaged-runtime"),
		TEXT("runtime-provider"),
		TEXT("sqlite")
	};
	Document.RootWidgetId = TEXT("SQLUI.PackagedRuntime.ProviderStartupSmoke.Root");

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = Document.RootWidgetId;
	RootNode.WidgetTypeKey = TEXT("SQLUI.FilterBox");
	RootNode.Properties.Add(TEXT("FilterText"), TEXT("SQLUI packaged runtime provider startup smoke"));
	RootNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));
	RootNode.Tags = {
		TEXT("smoke-root")
	};

	Document.Nodes.Add(RootNode);
	return Document;
}

FSQLUILayoutSaveResult SaveSQLUIPackagedRuntimeProviderStartupSmokeLayout(
	USQLUILayoutRepository* Repository,
	const FSQLUILayoutDocument& Document)
{
	FSQLUILayoutSaveResult SaveResult;
	SaveResult.SavedLayoutId = Document.Metadata.LayoutId;

	if (!IsValid(Repository))
	{
		SaveResult.ErrorMessage =
			TEXT("SQLUI packaged runtime provider startup smoke failed: repository was not valid.");
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
			TEXT("SQLUI packaged runtime provider startup smoke failed: SaveLayout callback did not complete synchronously.");
	}

	return SaveResult;
}

FSQLUILayoutLoadResult LoadSQLUIPackagedRuntimeProviderStartupSmokeLayout(
	USQLUILayoutRepository* Repository,
	const FString& LayoutId)
{
	FSQLUILayoutLoadResult LoadResult;
	LoadResult.Document.Metadata.LayoutId = LayoutId;

	if (!IsValid(Repository))
	{
		LoadResult.ErrorMessage =
			TEXT("SQLUI packaged runtime provider startup smoke failed: repository was not valid.");
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
			TEXT("SQLUI packaged runtime provider startup smoke failed: LoadLayout callback did not complete synchronously.");
	}

	return LoadResult;
}

bool ReadSQLUIPackagedRuntimeProviderStartupSmokeCommandLine(
	FSQLUISamplePackagedRuntimeProviderStartupSmokeResult& Result)
{
	const TCHAR* CommandLine = FCommandLine::Get();

	FString BackendText;
	FString SQLitePath;
	const bool bParsedBackend =
		FParse::Value(CommandLine, TEXT("SQLUILayoutRepositoryBackend="), BackendText)
		|| FParse::Value(CommandLine, TEXT("-SQLUILayoutRepositoryBackend="), BackendText);
	const bool bParsedPath =
		FParse::Value(CommandLine, TEXT("SQLUISQLiteLayoutRepositoryPath="), SQLitePath)
		|| FParse::Value(CommandLine, TEXT("-SQLUISQLiteLayoutRepositoryPath="), SQLitePath);
	const bool bParsedInitializeSchema =
		FParse::Param(CommandLine, TEXT("SQLUISQLiteLayoutRepositoryInitializeSchema"));
	const bool bParsedCreateDatabase =
		FParse::Param(CommandLine, TEXT("SQLUISQLiteLayoutRepositoryCreateDatabase"));

	Result.DatabasePath =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(SQLitePath);

	Result.bInitializedFromCommandLine =
		bParsedBackend
		&& BackendText.Equals(TEXT("SQLite"), ESearchCase::IgnoreCase)
		&& bParsedPath
		&& !Result.DatabasePath.IsEmpty()
		&& bParsedInitializeSchema
		&& bParsedCreateDatabase;

	if (!Result.bInitializedFromCommandLine)
	{
		AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
			Result,
			TEXT("SQLUI packaged runtime provider startup smoke failed: required command-line repository settings were not present."));
	}

	return Result.bInitializedFromCommandLine;
}

void LogSQLUIPackagedRuntimeProviderStartupSmokeResult(
	const FSQLUISamplePackagedRuntimeProviderStartupSmokeResult& Result)
{
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke selected: true"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke database path: '%s'"), *Result.DatabasePath);
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke provider created: %s"), Result.bProviderCreated ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke initialized from command line: %s"), Result.bInitializedFromCommandLine ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke initialization succeeded: %s"), Result.bInitializationSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke backend SQLite: %s"), Result.bBackendSQLite ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke repository available: %s"), Result.bRepositoryAvailable ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke repository is SQLite: %s"), Result.bRepositoryIsSQLite ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke save succeeded: %s"), Result.bSaveSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke database created: %s"), Result.bDatabaseCreated ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke list succeeded: %s"), Result.bListSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke listed metadata found: %s"), Result.bListedMetadataFound ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke load succeeded: %s"), Result.bLoadSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke loaded document valid: %s"), Result.bLoadedDocumentValid ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke loaded layout id matched: %s"), Result.bLoadedLayoutIdMatched ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke provider reset cleared repository: %s"), Result.bProviderResetClearedRepository ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke database removed: %s"), Result.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (Result.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider startup smoke succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI packaged runtime provider startup smoke failed: %s"),
			Result.ErrorMessage.IsEmpty() ? TEXT("unknown failure") : *Result.ErrorMessage);
	}
}

FSQLUISamplePackagedRuntimeProviderStartupSmokeResult RunSQLUIPackagedRuntimeProviderStartupSmoke()
{
	FSQLUISamplePackagedRuntimeProviderStartupSmokeResult Result;
	ReadSQLUIPackagedRuntimeProviderStartupSmokeCommandLine(Result);
	DeleteSQLUIPackagedRuntimeProviderStartupSmokeFiles(Result.DatabasePath, Result);

	USQLUILayoutRepositoryRuntimeProvider* Provider =
		NewObject<USQLUILayoutRepositoryRuntimeProvider>(GetTransientPackage());
	Result.bProviderCreated = IsValid(Provider);
	if (!Result.bProviderCreated)
	{
		AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
			Result,
			TEXT("SQLUI packaged runtime provider startup smoke failed: provider was not created."));
		LogSQLUIPackagedRuntimeProviderStartupSmokeResult(Result);
		return Result;
	}

	Result.bInitializationSucceeded =
		Provider->InitializeRepositoryFromCommandLine(
			FCommandLine::Get(),
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault());

	const FSQLUILayoutRepositoryRuntimeIntegrationResult IntegrationResult =
		Provider->GetLastIntegrationResult();
	if (Result.DatabasePath.IsEmpty())
	{
		Result.DatabasePath = IntegrationResult.SQLiteDatabasePath;
	}

	Result.bBackendSQLite =
		Provider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::SQLite
		&& IntegrationResult.Backend == ESQLUILayoutRepositoryBackend::SQLite;
	Result.bRepositoryAvailable = Provider->HasRepository();
	USQLUILayoutRepository* Repository = Provider->GetRepository();
	USQLUISQLiteLayoutRepository* SQLiteRepository =
		Cast<USQLUISQLiteLayoutRepository>(Repository);
	Result.bRepositoryIsSQLite = IsValid(SQLiteRepository);

	if (!Result.bInitializationSucceeded
		|| !Result.bBackendSQLite
		|| !Result.bRepositoryAvailable
		|| !Result.bRepositoryIsSQLite)
	{
		AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
			Result,
			IntegrationResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI packaged runtime provider startup smoke failed: provider did not initialize a SQLite repository from command-line settings.")
				: IntegrationResult.ErrorMessage);
	}

	const FSQLUILayoutDocument Document =
		MakeSQLUIPackagedRuntimeProviderStartupSmokeDocument();
	const FString LayoutId = Document.Metadata.LayoutId;

	if (Result.bRepositoryAvailable)
	{
		const FSQLUILayoutSaveResult SaveResult =
			SaveSQLUIPackagedRuntimeProviderStartupSmokeLayout(Repository, Document);
		Result.bSaveSucceeded =
			SaveResult.bSucceeded
			&& SaveResult.SavedLayoutId == LayoutId
			&& SaveResult.Validation.bIsValid;
		Result.bDatabaseCreated = FPaths::FileExists(Result.DatabasePath);
		if (!Result.bSaveSucceeded)
		{
			AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
				Result,
				SaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime provider startup smoke failed: SaveLayout did not succeed.")
					: SaveResult.ErrorMessage);
		}
		if (!Result.bDatabaseCreated)
		{
			AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
				Result,
				TEXT("SQLUI packaged runtime provider startup smoke failed: database was not created after SaveLayout."));
		}
	}

	if (Result.bRepositoryIsSQLite)
	{
		const FSQLUILayoutRepositoryListResult ListResult = SQLiteRepository->ListLayouts();
		Result.bListSucceeded = ListResult.bSucceeded;
		Result.bListedMetadataFound =
			Result.bListSucceeded
			&& DoesSQLUIPackagedRuntimeProviderStartupSmokeListContainMetadata(
				ListResult.Layouts,
				Document.Metadata);
		if (!Result.bListSucceeded)
		{
			AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
				Result,
				ListResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime provider startup smoke failed: ListLayouts did not succeed.")
					: ListResult.ErrorMessage);
		}
		else if (!Result.bListedMetadataFound)
		{
			AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
				Result,
				TEXT("SQLUI packaged runtime provider startup smoke failed: ListLayouts did not include saved metadata and tags."));
		}
	}

	if (Result.bRepositoryAvailable)
	{
		const FSQLUILayoutLoadResult LoadResult =
			LoadSQLUIPackagedRuntimeProviderStartupSmokeLayout(Repository, LayoutId);
		Result.bLoadSucceeded = LoadResult.bSucceeded;
		Result.bLoadedDocumentValid =
			Result.bLoadSucceeded && LoadResult.Validation.bIsValid;
		Result.bLoadedLayoutIdMatched =
			Result.bLoadedDocumentValid
			&& LoadResult.Document.Metadata.LayoutId == LayoutId;
		if (!Result.bLoadSucceeded)
		{
			AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
				Result,
				LoadResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime provider startup smoke failed: LoadLayout did not succeed.")
					: LoadResult.ErrorMessage);
		}
		else if (!Result.bLoadedDocumentValid || !Result.bLoadedLayoutIdMatched)
		{
			AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
				Result,
				TEXT("SQLUI packaged runtime provider startup smoke failed: loaded document was invalid or had the wrong layout id."));
		}
	}

	Provider->ResetRepository();
	Result.bProviderResetClearedRepository =
		!Provider->HasRepository()
		&& !Provider->WasLastInitializationSuccessful()
		&& Provider->GetActiveBackend() == ESQLUILayoutRepositoryBackend::Unavailable;
	if (!Result.bProviderResetClearedRepository)
	{
		AppendSQLUIPackagedRuntimeProviderStartupSmokeError(
			Result,
			TEXT("SQLUI packaged runtime provider startup smoke failed: provider reset did not clear the repository."));
	}

	Result.bDatabaseRemoved =
		DeleteSQLUIPackagedRuntimeProviderStartupSmokeFiles(Result.DatabasePath, Result)
		&& !FPaths::FileExists(Result.DatabasePath)
		&& !FPaths::FileExists(Result.DatabasePath + TEXT("-journal"))
		&& !FPaths::FileExists(Result.DatabasePath + TEXT("-wal"))
		&& !FPaths::FileExists(Result.DatabasePath + TEXT("-shm"));

	Result.bSucceeded =
		Result.bProviderCreated
		&& Result.bInitializedFromCommandLine
		&& Result.bInitializationSucceeded
		&& Result.bBackendSQLite
		&& Result.bRepositoryAvailable
		&& Result.bRepositoryIsSQLite
		&& Result.bSaveSucceeded
		&& Result.bDatabaseCreated
		&& Result.bListSucceeded
		&& Result.bListedMetadataFound
		&& Result.bLoadSucceeded
		&& Result.bLoadedDocumentValid
		&& Result.bLoadedLayoutIdMatched
		&& Result.bProviderResetClearedRepository
		&& Result.bDatabaseRemoved;

	LogSQLUIPackagedRuntimeProviderStartupSmokeResult(Result);
	return Result;
}
}

void FSQLUISamplePackagedRuntimeProviderStartupSmoke::RunAndRequestExit()
{
	const FSQLUISamplePackagedRuntimeProviderStartupSmokeResult Result =
		RunSQLUIPackagedRuntimeProviderStartupSmoke();

	FPlatformMisc::RequestExitWithStatus(
		false,
		Result.bSucceeded ? 0 : 1,
		TEXT("SQLUI packaged runtime provider startup smoke"));
}
