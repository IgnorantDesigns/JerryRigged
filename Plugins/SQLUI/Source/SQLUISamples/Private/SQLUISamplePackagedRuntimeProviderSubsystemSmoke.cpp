#include "SQLUISamplePackagedRuntimeProviderSubsystemSmoke.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"
#include "Layout/SQLUILayoutRepositoryRuntimeSubsystem.h"
#include "Layout/SQLUISQLiteLayoutRepository.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "SQLUISamplesModule.h"

namespace
{
constexpr double SQLUIPackagedRuntimeProviderSubsystemSmokeTimeoutSeconds = 5.0;

struct FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult
{
	bool bSucceeded = false;
	bool bSubsystemFound = false;
	bool bAutoInitRequested = false;
	bool bAutoInitSucceeded = false;
	bool bProviderAvailable = false;
	bool bRepositoryAvailable = false;
	bool bBackendSQLite = false;
	bool bRepositoryIsSQLite = false;
	bool bSaveSucceeded = false;
	bool bDatabaseCreated = false;
	bool bListSucceeded = false;
	bool bListedMetadataFound = false;
	bool bLoadSucceeded = false;
	bool bLoadedDocumentValid = false;
	bool bLoadedLayoutIdMatched = false;
	bool bResetClearedRepository = false;
	bool bDatabaseRemoved = false;
	FString DatabasePath;
	FString ErrorMessage;
};

void AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
	FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult& Result,
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

bool DeleteSQLUIPackagedRuntimeProviderSubsystemSmokeFiles(
	const FString& DatabasePath,
	FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult& Result)
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
			AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
				Result,
				FString::Printf(
					TEXT("SQLUI packaged runtime provider subsystem smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DoSQLUIPackagedRuntimeProviderSubsystemSmokeTagsMatch(
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

bool DoesSQLUIPackagedRuntimeProviderSubsystemSmokeMetadataMatch(
	const FSQLUILayoutMetadata& Candidate,
	const FSQLUILayoutMetadata& Expected)
{
	return Candidate.LayoutId == Expected.LayoutId
		&& Candidate.DisplayName == Expected.DisplayName
		&& Candidate.Description == Expected.Description
		&& Candidate.CreatedBy == Expected.CreatedBy
		&& DoSQLUIPackagedRuntimeProviderSubsystemSmokeTagsMatch(Candidate.Tags, Expected.Tags);
}

bool DoesSQLUIPackagedRuntimeProviderSubsystemSmokeListContainMetadata(
	const TArray<FSQLUILayoutMetadata>& Layouts,
	const FSQLUILayoutMetadata& Expected)
{
	for (const FSQLUILayoutMetadata& Layout : Layouts)
	{
		if (DoesSQLUIPackagedRuntimeProviderSubsystemSmokeMetadataMatch(Layout, Expected))
		{
			return true;
		}
	}

	return false;
}

FSQLUILayoutDocument MakeSQLUIPackagedRuntimeProviderSubsystemSmokeDocument()
{
	FSQLUILayoutDocument Document;
	Document.Version.SchemaVersion = 1;
	Document.Version.Revision = 1;
	Document.Version.Label = TEXT("SQLUI Packaged Runtime Provider Subsystem Smoke");
	Document.Metadata.LayoutId = TEXT("sqlui.smoke.packaged-runtime.provider-subsystem");
	Document.Metadata.DisplayName = TEXT("SQLUI Packaged Runtime Provider Subsystem Smoke");
	Document.Metadata.Description =
		TEXT("Probe-only layout for packaged runtime provider subsystem validation.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-06-05T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = TEXT("2026-06-05T00:00:00Z");
	Document.Metadata.Tags = {
		TEXT("smoke"),
		TEXT("packaged-runtime"),
		TEXT("runtime-provider"),
		TEXT("subsystem"),
		TEXT("sqlite")
	};
	Document.RootWidgetId = TEXT("SQLUI.PackagedRuntime.ProviderSubsystemSmoke.Root");

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = Document.RootWidgetId;
	RootNode.WidgetTypeKey = TEXT("SQLUI.FilterBox");
	RootNode.Properties.Add(TEXT("FilterText"), TEXT("SQLUI packaged runtime provider subsystem smoke"));
	RootNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));
	RootNode.Tags = {
		TEXT("smoke-root")
	};

	Document.Nodes.Add(RootNode);
	return Document;
}

FSQLUILayoutSaveResult SaveSQLUIPackagedRuntimeProviderSubsystemSmokeLayout(
	USQLUILayoutRepository* Repository,
	const FSQLUILayoutDocument& Document)
{
	FSQLUILayoutSaveResult SaveResult;
	SaveResult.SavedLayoutId = Document.Metadata.LayoutId;

	if (!IsValid(Repository))
	{
		SaveResult.ErrorMessage =
			TEXT("SQLUI packaged runtime provider subsystem smoke failed: repository was not valid.");
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
			TEXT("SQLUI packaged runtime provider subsystem smoke failed: SaveLayout callback did not complete synchronously.");
	}

	return SaveResult;
}

FSQLUILayoutLoadResult LoadSQLUIPackagedRuntimeProviderSubsystemSmokeLayout(
	USQLUILayoutRepository* Repository,
	const FString& LayoutId)
{
	FSQLUILayoutLoadResult LoadResult;
	LoadResult.Document.Metadata.LayoutId = LayoutId;

	if (!IsValid(Repository))
	{
		LoadResult.ErrorMessage =
			TEXT("SQLUI packaged runtime provider subsystem smoke failed: repository was not valid.");
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
			TEXT("SQLUI packaged runtime provider subsystem smoke failed: LoadLayout callback did not complete synchronously.");
	}

	return LoadResult;
}

void ReadSQLUIPackagedRuntimeProviderSubsystemSmokeCommandLine(
	FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult& Result)
{
	const TCHAR* CommandLine = FCommandLine::Get();

	FString SQLitePath;
	const bool bParsedPath =
		FParse::Value(CommandLine, TEXT("SQLUISQLiteLayoutRepositoryPath="), SQLitePath)
		|| FParse::Value(CommandLine, TEXT("-SQLUISQLiteLayoutRepositoryPath="), SQLitePath);

	if (bParsedPath)
	{
		Result.DatabasePath =
			FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(SQLitePath);
	}
}

USQLUILayoutRepositoryRuntimeSubsystem* FindSQLUIPackagedRuntimeProviderSubsystem()
{
	if (!GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		UWorld* World = WorldContext.World();
		if (!IsValid(World))
		{
			continue;
		}

		UGameInstance* GameInstance = World->GetGameInstance();
		if (!IsValid(GameInstance))
		{
			continue;
		}

		USQLUILayoutRepositoryRuntimeSubsystem* Subsystem =
			GameInstance->GetSubsystem<USQLUILayoutRepositoryRuntimeSubsystem>();
		if (IsValid(Subsystem))
		{
			return Subsystem;
		}
	}

	return nullptr;
}

void LogSQLUIPackagedRuntimeProviderSubsystemSmokeResult(
	const FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult& Result)
{
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke selected: true"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke database path: '%s'"), *Result.DatabasePath);
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke subsystem found: %s"), Result.bSubsystemFound ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke auto init requested: %s"), Result.bAutoInitRequested ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke auto init succeeded: %s"), Result.bAutoInitSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke provider available: %s"), Result.bProviderAvailable ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke repository available: %s"), Result.bRepositoryAvailable ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke backend SQLite: %s"), Result.bBackendSQLite ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke repository is SQLite: %s"), Result.bRepositoryIsSQLite ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke save succeeded: %s"), Result.bSaveSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke database created: %s"), Result.bDatabaseCreated ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke list succeeded: %s"), Result.bListSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke listed metadata found: %s"), Result.bListedMetadataFound ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke load succeeded: %s"), Result.bLoadSucceeded ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke loaded document valid: %s"), Result.bLoadedDocumentValid ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke loaded layout id matched: %s"), Result.bLoadedLayoutIdMatched ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke reset cleared repository: %s"), Result.bResetClearedRepository ? TEXT("true") : TEXT("false"));
	UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke database removed: %s"), Result.bDatabaseRemoved ? TEXT("true") : TEXT("false"));

	if (Result.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime provider subsystem smoke succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI packaged runtime provider subsystem smoke failed: %s"),
			Result.ErrorMessage.IsEmpty() ? TEXT("unknown failure") : *Result.ErrorMessage);
	}
}

FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult
RunSQLUIPackagedRuntimeProviderSubsystemSmoke(
	USQLUILayoutRepositoryRuntimeSubsystem* Subsystem)
{
	FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult Result;
	ReadSQLUIPackagedRuntimeProviderSubsystemSmokeCommandLine(Result);

	Result.bSubsystemFound = IsValid(Subsystem);
	if (!Result.bSubsystemFound)
	{
		AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
			Result,
			TEXT("SQLUI packaged runtime provider subsystem smoke failed: subsystem was not found."));
		Result.bDatabaseRemoved =
			DeleteSQLUIPackagedRuntimeProviderSubsystemSmokeFiles(Result.DatabasePath, Result);
		LogSQLUIPackagedRuntimeProviderSubsystemSmokeResult(Result);
		return Result;
	}

	Result.bAutoInitRequested = Subsystem->WasAutoInitializationRequested();
	Result.bAutoInitSucceeded = Subsystem->WasAutoInitializationSuccessful();
	Result.bProviderAvailable = IsValid(Subsystem->GetProvider());
	Result.bRepositoryAvailable = Subsystem->HasRepository();
	const FSQLUILayoutRepositoryRuntimeIntegrationResult IntegrationResult =
		Subsystem->GetLastIntegrationResult();
	if (Result.DatabasePath.IsEmpty())
	{
		Result.DatabasePath = IntegrationResult.SQLiteDatabasePath;
	}

	Result.bBackendSQLite =
		Subsystem->GetActiveBackend() == ESQLUILayoutRepositoryBackend::SQLite
		&& IntegrationResult.Backend == ESQLUILayoutRepositoryBackend::SQLite;
	USQLUILayoutRepository* Repository = Subsystem->GetRepository();
	USQLUISQLiteLayoutRepository* SQLiteRepository =
		Cast<USQLUISQLiteLayoutRepository>(Repository);
	Result.bRepositoryIsSQLite = IsValid(SQLiteRepository);

	if (!Result.bAutoInitRequested
		|| !Result.bAutoInitSucceeded
		|| !Result.bProviderAvailable
		|| !Result.bRepositoryAvailable
		|| !Result.bBackendSQLite
		|| !Result.bRepositoryIsSQLite)
	{
		AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
			Result,
			IntegrationResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI packaged runtime provider subsystem smoke failed: subsystem did not auto-initialize a SQLite repository.")
				: IntegrationResult.ErrorMessage);
	}

	const FSQLUILayoutDocument Document =
		MakeSQLUIPackagedRuntimeProviderSubsystemSmokeDocument();
	const FString LayoutId = Document.Metadata.LayoutId;

	if (Result.bRepositoryAvailable)
	{
		const FSQLUILayoutSaveResult SaveResult =
			SaveSQLUIPackagedRuntimeProviderSubsystemSmokeLayout(Repository, Document);
		Result.bSaveSucceeded =
			SaveResult.bSucceeded
			&& SaveResult.SavedLayoutId == LayoutId
			&& SaveResult.Validation.bIsValid;
		Result.bDatabaseCreated = FPaths::FileExists(Result.DatabasePath);
		if (!Result.bSaveSucceeded)
		{
			AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
				Result,
				SaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime provider subsystem smoke failed: SaveLayout did not succeed.")
					: SaveResult.ErrorMessage);
		}
		if (!Result.bDatabaseCreated)
		{
			AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
				Result,
				TEXT("SQLUI packaged runtime provider subsystem smoke failed: database was not created after SaveLayout."));
		}
	}

	if (Result.bRepositoryIsSQLite)
	{
		const FSQLUILayoutRepositoryListResult ListResult = SQLiteRepository->ListLayouts();
		Result.bListSucceeded = ListResult.bSucceeded;
		Result.bListedMetadataFound =
			Result.bListSucceeded
			&& DoesSQLUIPackagedRuntimeProviderSubsystemSmokeListContainMetadata(
				ListResult.Layouts,
				Document.Metadata);
		if (!Result.bListSucceeded)
		{
			AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
				Result,
				ListResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime provider subsystem smoke failed: ListLayouts did not succeed.")
					: ListResult.ErrorMessage);
		}
		else if (!Result.bListedMetadataFound)
		{
			AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
				Result,
				TEXT("SQLUI packaged runtime provider subsystem smoke failed: ListLayouts did not include saved metadata and tags."));
		}
	}

	if (Result.bRepositoryAvailable)
	{
		const FSQLUILayoutLoadResult LoadResult =
			LoadSQLUIPackagedRuntimeProviderSubsystemSmokeLayout(Repository, LayoutId);
		Result.bLoadSucceeded = LoadResult.bSucceeded;
		Result.bLoadedDocumentValid =
			Result.bLoadSucceeded && LoadResult.Validation.bIsValid;
		Result.bLoadedLayoutIdMatched =
			Result.bLoadedDocumentValid
			&& LoadResult.Document.Metadata.LayoutId == LayoutId;
		if (!Result.bLoadSucceeded)
		{
			AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
				Result,
				LoadResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime provider subsystem smoke failed: LoadLayout did not succeed.")
					: LoadResult.ErrorMessage);
		}
		else if (!Result.bLoadedDocumentValid || !Result.bLoadedLayoutIdMatched)
		{
			AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
				Result,
				TEXT("SQLUI packaged runtime provider subsystem smoke failed: loaded document was invalid or had the wrong layout id."));
		}
	}

	Subsystem->ResetRepository();
	Result.bResetClearedRepository =
		!Subsystem->HasRepository()
		&& Subsystem->GetActiveBackend() == ESQLUILayoutRepositoryBackend::Unavailable;
	if (!Result.bResetClearedRepository)
	{
		AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
			Result,
			TEXT("SQLUI packaged runtime provider subsystem smoke failed: reset did not clear the repository."));
	}

	Result.bDatabaseRemoved =
		DeleteSQLUIPackagedRuntimeProviderSubsystemSmokeFiles(Result.DatabasePath, Result)
		&& !FPaths::FileExists(Result.DatabasePath)
		&& !FPaths::FileExists(Result.DatabasePath + TEXT("-journal"))
		&& !FPaths::FileExists(Result.DatabasePath + TEXT("-wal"))
		&& !FPaths::FileExists(Result.DatabasePath + TEXT("-shm"));

	Result.bSucceeded =
		Result.bSubsystemFound
		&& Result.bAutoInitRequested
		&& Result.bAutoInitSucceeded
		&& Result.bProviderAvailable
		&& Result.bRepositoryAvailable
		&& Result.bBackendSQLite
		&& Result.bRepositoryIsSQLite
		&& Result.bSaveSucceeded
		&& Result.bDatabaseCreated
		&& Result.bListSucceeded
		&& Result.bListedMetadataFound
		&& Result.bLoadSucceeded
		&& Result.bLoadedDocumentValid
		&& Result.bLoadedLayoutIdMatched
		&& Result.bResetClearedRepository
		&& Result.bDatabaseRemoved;

	LogSQLUIPackagedRuntimeProviderSubsystemSmokeResult(Result);
	return Result;
}

void RequestExitForSQLUIPackagedRuntimeProviderSubsystemSmoke(
	const FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult& Result)
{
	FPlatformMisc::RequestExitWithStatus(
		false,
		Result.bSucceeded ? 0 : 1,
		TEXT("SQLUI packaged runtime provider subsystem smoke"));
}

struct FSQLUIPackagedRuntimeProviderSubsystemSmokeTickerState
{
	double StartTimeSeconds = FPlatformTime::Seconds();
	FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult Result;
};
}

void FSQLUISamplePackagedRuntimeProviderSubsystemSmoke::RunAndRequestExit()
{
	if (USQLUILayoutRepositoryRuntimeSubsystem* Subsystem =
		FindSQLUIPackagedRuntimeProviderSubsystem())
	{
		const FSQLUISamplePackagedRuntimeProviderSubsystemSmokeResult Result =
			RunSQLUIPackagedRuntimeProviderSubsystemSmoke(Subsystem);
		RequestExitForSQLUIPackagedRuntimeProviderSubsystemSmoke(Result);
		return;
	}

	TSharedRef<FSQLUIPackagedRuntimeProviderSubsystemSmokeTickerState, ESPMode::ThreadSafe>
		SharedState =
			MakeShared<FSQLUIPackagedRuntimeProviderSubsystemSmokeTickerState, ESPMode::ThreadSafe>();
	ReadSQLUIPackagedRuntimeProviderSubsystemSmokeCommandLine(SharedState->Result);

	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda(
			[SharedState](float)
			{
				if (USQLUILayoutRepositoryRuntimeSubsystem* Subsystem =
					FindSQLUIPackagedRuntimeProviderSubsystem())
				{
					SharedState->Result =
						RunSQLUIPackagedRuntimeProviderSubsystemSmoke(Subsystem);
					RequestExitForSQLUIPackagedRuntimeProviderSubsystemSmoke(SharedState->Result);
					return false;
				}

				const double ElapsedSeconds =
					FPlatformTime::Seconds() - SharedState->StartTimeSeconds;
				if (ElapsedSeconds >= SQLUIPackagedRuntimeProviderSubsystemSmokeTimeoutSeconds)
				{
					AppendSQLUIPackagedRuntimeProviderSubsystemSmokeError(
						SharedState->Result,
						TEXT("SQLUI packaged runtime provider subsystem smoke failed: timed out waiting for GameInstance subsystem."));
					SharedState->Result.bDatabaseRemoved =
						DeleteSQLUIPackagedRuntimeProviderSubsystemSmokeFiles(
							SharedState->Result.DatabasePath,
							SharedState->Result);
					LogSQLUIPackagedRuntimeProviderSubsystemSmokeResult(SharedState->Result);
					RequestExitForSQLUIPackagedRuntimeProviderSubsystemSmoke(SharedState->Result);
					return false;
				}

				return true;
			}),
		0.1f);
}
