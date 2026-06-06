#include "SQLUISamplePackagedRuntimePersistenceWorkflowSmoke.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Layout/SQLUILayoutPersistenceWorkflow.h"
#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"
#include "Layout/SQLUILayoutRepositoryRuntimeSubsystem.h"
#include "Layout/SQLUILayoutTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "SQLUISamplesModule.h"

namespace
{
constexpr double SQLUIPackagedRuntimePersistenceWorkflowSmokeTimeoutSeconds = 5.0;

enum class ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase
{
	Unknown,
	Save,
	Verify,
	Cleanup
};

struct FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult
{
	bool bSucceeded = false;
	bool bSubsystemFound = false;
	bool bAutoInitRequested = false;
	bool bAutoInitSucceeded = false;
	bool bProviderAvailable = false;
	bool bRepositoryAvailable = false;
	bool bBackendSQLite = false;
	bool bWorkflowSaveSucceeded = false;
	bool bWorkflowListSucceeded = false;
	bool bWorkflowListedMetadataFound = false;
	bool bWorkflowLoadSucceeded = false;
	bool bWorkflowLoadedDocumentValid = false;
	bool bWorkflowLoadedLayoutIdMatched = false;
	bool bProviderReset = false;
	bool bDatabaseExistsAfterPhase = false;
	bool bDatabaseRemoved = false;
	ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase Phase =
		ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Unknown;
	FString PhaseText = TEXT("Unknown");
	FString DatabasePath;
	FString ErrorMessage;
};

FString ToSQLUIPackagedRuntimePersistenceWorkflowSmokePhaseText(
	const ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase Phase)
{
	switch (Phase)
	{
	case ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Save:
		return TEXT("Save");
	case ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Verify:
		return TEXT("Verify");
	case ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Cleanup:
		return TEXT("Cleanup");
	default:
		return TEXT("Unknown");
	}
}

ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase
ParseSQLUIPackagedRuntimePersistenceWorkflowSmokePhase(const FString& PhaseText)
{
	if (PhaseText.Equals(TEXT("Save"), ESearchCase::IgnoreCase))
	{
		return ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Save;
	}

	if (PhaseText.Equals(TEXT("Verify"), ESearchCase::IgnoreCase))
	{
		return ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Verify;
	}

	if (PhaseText.Equals(TEXT("Cleanup"), ESearchCase::IgnoreCase))
	{
		return ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Cleanup;
	}

	return ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Unknown;
}

void AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
	FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult& Result,
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

TArray<FString> MakeSQLUIPackagedRuntimePersistenceWorkflowSmokeFiles(
	const FString& DatabasePath)
{
	return {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};
}

bool DoSQLUIPackagedRuntimePersistenceWorkflowSmokeFilesExist(
	const FString& DatabasePath)
{
	if (DatabasePath.IsEmpty())
	{
		return false;
	}

	for (const FString& PathToCheck :
		MakeSQLUIPackagedRuntimePersistenceWorkflowSmokeFiles(DatabasePath))
	{
		if (FPaths::FileExists(PathToCheck))
		{
			return true;
		}
	}

	return false;
}

bool DeleteSQLUIPackagedRuntimePersistenceWorkflowSmokeFiles(
	const FString& DatabasePath,
	FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult& Result)
{
	if (DatabasePath.IsEmpty())
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: SQLite database path was not resolved."));
		return false;
	}

	bool bRemoved = true;
	for (const FString& PathToRemove :
		MakeSQLUIPackagedRuntimePersistenceWorkflowSmokeFiles(DatabasePath))
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
				Result,
				FString::Printf(
					TEXT("SQLUI packaged runtime persistence workflow smoke failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool DoSQLUIPackagedRuntimePersistenceWorkflowSmokeTagsMatch(
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

bool DoesSQLUIPackagedRuntimePersistenceWorkflowSmokeMetadataMatch(
	const FSQLUILayoutMetadata& Candidate,
	const FSQLUILayoutMetadata& Expected)
{
	return Candidate.LayoutId == Expected.LayoutId
		&& Candidate.DisplayName == Expected.DisplayName
		&& Candidate.Description == Expected.Description
		&& Candidate.CreatedBy == Expected.CreatedBy
		&& DoSQLUIPackagedRuntimePersistenceWorkflowSmokeTagsMatch(
			Candidate.Tags,
			Expected.Tags);
}

bool DoesSQLUIPackagedRuntimePersistenceWorkflowSmokeListContainMetadata(
	const TArray<FSQLUILayoutMetadata>& Layouts,
	const FSQLUILayoutMetadata& Expected)
{
	for (const FSQLUILayoutMetadata& Layout : Layouts)
	{
		if (DoesSQLUIPackagedRuntimePersistenceWorkflowSmokeMetadataMatch(
			Layout,
			Expected))
		{
			return true;
		}
	}

	return false;
}

FSQLUILayoutDocument MakeSQLUIPackagedRuntimePersistenceWorkflowSmokeDocument()
{
	FSQLUILayoutDocument Document;
	Document.Version.SchemaVersion = 1;
	Document.Version.Revision = 1;
	Document.Version.Label = TEXT("SQLUI Packaged Runtime Persistence Workflow Smoke");
	Document.Metadata.LayoutId =
		TEXT("sqlui.smoke.packaged-runtime.persistence-workflow");
	Document.Metadata.DisplayName =
		TEXT("SQLUI Packaged Runtime Persistence Workflow Smoke");
	Document.Metadata.Description =
		TEXT("Probe-only layout for packaged runtime persistence workflow validation.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.Metadata.CreatedAtUtc = TEXT("2026-06-05T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = TEXT("2026-06-05T00:00:00Z");
	Document.Metadata.Tags = {
		TEXT("smoke"),
		TEXT("packaged-runtime"),
		TEXT("persistence-workflow"),
		TEXT("sqlite")
	};
	Document.RootWidgetId = TEXT("SQLUI.PackagedRuntime.PersistenceWorkflowSmoke.Root");

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = Document.RootWidgetId;
	RootNode.WidgetTypeKey = TEXT("SQLUI.FilterBox");
	RootNode.Properties.Add(
		TEXT("FilterText"),
		TEXT("SQLUI packaged runtime persistence workflow smoke"));
	RootNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));
	RootNode.Tags = {
		TEXT("smoke-root")
	};

	Document.Nodes.Add(RootNode);
	return Document;
}

void ReadSQLUIPackagedRuntimePersistenceWorkflowSmokeCommandLine(
	FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult& Result)
{
	const TCHAR* CommandLine = FCommandLine::Get();

	FString ParsedPhaseText;
	const bool bParsedPhase =
		FParse::Value(
			CommandLine,
			TEXT("SQLUIRuntimePersistenceWorkflowSmokePhase="),
			ParsedPhaseText)
		|| FParse::Value(
			CommandLine,
			TEXT("-SQLUIRuntimePersistenceWorkflowSmokePhase="),
			ParsedPhaseText);

	if (bParsedPhase)
	{
		Result.Phase = ParseSQLUIPackagedRuntimePersistenceWorkflowSmokePhase(
			ParsedPhaseText);
		Result.PhaseText =
			ToSQLUIPackagedRuntimePersistenceWorkflowSmokePhaseText(Result.Phase);
	}
	else
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: phase was not provided."));
	}

	if (Result.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Unknown)
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: phase must be Save, Verify, or Cleanup."));
	}

	const FSQLUILayoutRepositoryRuntimeConfig RuntimeConfig =
		FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
			CommandLine,
			FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault());
	Result.DatabasePath =
		FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(
			RuntimeConfig.SQLiteDatabasePath);
	if (Result.DatabasePath.IsEmpty())
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: SQLite database path command-line setting was missing."));
	}
}

USQLUILayoutRepositoryRuntimeSubsystem*
FindSQLUIPackagedRuntimePersistenceWorkflowSubsystem()
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

void ReadSQLUIPackagedRuntimePersistenceWorkflowSubsystemState(
	USQLUILayoutRepositoryRuntimeSubsystem* Subsystem,
	FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult& Result)
{
	Result.bSubsystemFound = IsValid(Subsystem);
	if (!Result.bSubsystemFound)
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: subsystem was not found."));
		return;
	}

	Result.bAutoInitRequested = Subsystem->WasAutoInitializationRequested();
	Result.bAutoInitSucceeded = Subsystem->WasAutoInitializationSuccessful();
	Result.bProviderAvailable = IsValid(Subsystem->GetProvider());
	Result.bRepositoryAvailable = FSQLUILayoutPersistenceWorkflow::HasRepository(Subsystem);

	const FSQLUILayoutRepositoryRuntimeIntegrationResult IntegrationResult =
		Subsystem->GetLastIntegrationResult();
	if (Result.DatabasePath.IsEmpty())
	{
		Result.DatabasePath = IntegrationResult.SQLiteDatabasePath;
	}

	Result.bBackendSQLite =
		Subsystem->GetActiveBackend() == ESQLUILayoutRepositoryBackend::SQLite
		&& IntegrationResult.Backend == ESQLUILayoutRepositoryBackend::SQLite;

	if (!Result.bAutoInitRequested
		|| !Result.bAutoInitSucceeded
		|| !Result.bProviderAvailable
		|| !Result.bRepositoryAvailable
		|| !Result.bBackendSQLite)
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			IntegrationResult.ErrorMessage.IsEmpty()
				? TEXT("SQLUI packaged runtime persistence workflow smoke failed: subsystem did not auto-initialize a SQLite repository.")
				: IntegrationResult.ErrorMessage);
	}
}

void ResetSQLUIPackagedRuntimePersistenceWorkflowSubsystem(
	USQLUILayoutRepositoryRuntimeSubsystem* Subsystem,
	FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult& Result)
{
	if (!IsValid(Subsystem))
	{
		return;
	}

	Subsystem->ResetRepository();
	Result.bProviderReset =
		!Subsystem->HasRepository()
		&& Subsystem->GetActiveBackend() == ESQLUILayoutRepositoryBackend::Unavailable;
	if (!Result.bProviderReset)
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: provider reset did not clear the repository."));
	}
}

void LogSQLUIPackagedRuntimePersistenceWorkflowSmokeResult(
	const FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult& Result)
{
	UE_LOG(
		LogSQLUISamples,
		Display,
		TEXT("SQLUI packaged runtime persistence workflow smoke selected phase: %s"),
		*Result.PhaseText);
	UE_LOG(
		LogSQLUISamples,
		Display,
		TEXT("SQLUI packaged runtime persistence workflow smoke database path: '%s'"),
		*Result.DatabasePath);

	if (Result.Phase != ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Cleanup)
	{
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke auto init requested: %s"),
			Result.bAutoInitRequested ? TEXT("true") : TEXT("false"));
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke auto init succeeded: %s"),
			Result.bAutoInitSucceeded ? TEXT("true") : TEXT("false"));
	}

	if (Result.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Save)
	{
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke workflow save succeeded: %s"),
			Result.bWorkflowSaveSucceeded ? TEXT("true") : TEXT("false"));
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke workflow list succeeded: %s"),
			Result.bWorkflowListSucceeded ? TEXT("true") : TEXT("false"));
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke workflow listed metadata found: %s"),
			Result.bWorkflowListedMetadataFound ? TEXT("true") : TEXT("false"));
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke workflow load succeeded: %s"),
			Result.bWorkflowLoadSucceeded ? TEXT("true") : TEXT("false"));
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke workflow loaded document valid: %s"),
			Result.bWorkflowLoadedDocumentValid ? TEXT("true") : TEXT("false"));
	}
	else if (Result.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Verify)
	{
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke workflow list succeeded: %s"),
			Result.bWorkflowListSucceeded ? TEXT("true") : TEXT("false"));
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke workflow listed persisted metadata found: %s"),
			Result.bWorkflowListedMetadataFound ? TEXT("true") : TEXT("false"));
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke workflow load succeeded: %s"),
			Result.bWorkflowLoadSucceeded ? TEXT("true") : TEXT("false"));
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke workflow loaded persisted document valid: %s"),
			Result.bWorkflowLoadedDocumentValid ? TEXT("true") : TEXT("false"));
	}

	if (Result.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Save
		|| Result.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Verify)
	{
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke provider reset: %s"),
			Result.bProviderReset ? TEXT("true") : TEXT("false"));
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke database exists after phase: %s"),
			Result.bDatabaseExistsAfterPhase ? TEXT("true") : TEXT("false"));
	}

	if (Result.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Cleanup)
	{
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke database removed: %s"),
			Result.bDatabaseRemoved ? TEXT("true") : TEXT("false"));
	}

	if (Result.bSucceeded)
	{
		UE_LOG(
			LogSQLUISamples,
			Display,
			TEXT("SQLUI packaged runtime persistence workflow smoke %s phase succeeded."),
			*Result.PhaseText);
		UE_LOG(LogSQLUISamples, Display, TEXT("SQLUI packaged runtime persistence workflow smoke succeeded."));
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: %s"),
			Result.ErrorMessage.IsEmpty() ? TEXT("unknown failure") : *Result.ErrorMessage);
	}
}

FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult
RunSQLUIPackagedRuntimePersistenceWorkflowCleanupPhase()
{
	FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult Result;
	ReadSQLUIPackagedRuntimePersistenceWorkflowSmokeCommandLine(Result);

	Result.bDatabaseRemoved =
		Result.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Cleanup
		&& DeleteSQLUIPackagedRuntimePersistenceWorkflowSmokeFiles(
			Result.DatabasePath,
			Result)
		&& !DoSQLUIPackagedRuntimePersistenceWorkflowSmokeFilesExist(
			Result.DatabasePath);
	if (!Result.bDatabaseRemoved)
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: cleanup did not remove all database files."));
	}

	Result.bSucceeded =
		Result.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Cleanup
		&& Result.bDatabaseRemoved;
	LogSQLUIPackagedRuntimePersistenceWorkflowSmokeResult(Result);
	return Result;
}

FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult
RunSQLUIPackagedRuntimePersistenceWorkflowSaveOrVerifyPhase(
	USQLUILayoutRepositoryRuntimeSubsystem* Subsystem,
	const ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase ExpectedPhase)
{
	FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult Result;
	ReadSQLUIPackagedRuntimePersistenceWorkflowSmokeCommandLine(Result);
	if (Result.Phase != ExpectedPhase)
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: runtime phase changed before execution."));
		LogSQLUIPackagedRuntimePersistenceWorkflowSmokeResult(Result);
		return Result;
	}

	ReadSQLUIPackagedRuntimePersistenceWorkflowSubsystemState(Subsystem, Result);

	const FSQLUILayoutDocument Document =
		MakeSQLUIPackagedRuntimePersistenceWorkflowSmokeDocument();
	const FString LayoutId = Document.Metadata.LayoutId;

	if (Result.bRepositoryAvailable
		&& Result.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Save)
	{
		const FSQLUILayoutSaveResult SaveResult =
			FSQLUILayoutPersistenceWorkflow::SaveLayout(Subsystem, Document);
		Result.bWorkflowSaveSucceeded =
			SaveResult.bSucceeded
			&& SaveResult.SavedLayoutId == LayoutId
			&& SaveResult.Validation.bIsValid;
		if (!Result.bWorkflowSaveSucceeded)
		{
			AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
				Result,
				SaveResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime persistence workflow smoke failed: workflow SaveLayout did not succeed.")
					: SaveResult.ErrorMessage);
		}
	}

	if (Result.bRepositoryAvailable)
	{
		const FSQLUILayoutRepositoryListResult ListResult =
			FSQLUILayoutPersistenceWorkflow::ListLayouts(Subsystem);
		Result.bWorkflowListSucceeded = ListResult.bSucceeded;
		Result.bWorkflowListedMetadataFound =
			Result.bWorkflowListSucceeded
			&& DoesSQLUIPackagedRuntimePersistenceWorkflowSmokeListContainMetadata(
				ListResult.Layouts,
				Document.Metadata);
		if (!Result.bWorkflowListSucceeded)
		{
			AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
				Result,
				ListResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime persistence workflow smoke failed: workflow ListLayouts did not succeed.")
					: ListResult.ErrorMessage);
		}
		else if (!Result.bWorkflowListedMetadataFound)
		{
			AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
				Result,
				TEXT("SQLUI packaged runtime persistence workflow smoke failed: workflow ListLayouts did not include expected metadata and tags."));
		}
	}

	if (Result.bRepositoryAvailable)
	{
		const FSQLUILayoutLoadResult LoadResult =
			FSQLUILayoutPersistenceWorkflow::LoadLayout(Subsystem, LayoutId);
		Result.bWorkflowLoadSucceeded = LoadResult.bSucceeded;
		Result.bWorkflowLoadedDocumentValid =
			Result.bWorkflowLoadSucceeded
			&& LoadResult.Validation.bIsValid;
		Result.bWorkflowLoadedLayoutIdMatched =
			Result.bWorkflowLoadedDocumentValid
			&& LoadResult.Document.Metadata.LayoutId == LayoutId;
		if (!Result.bWorkflowLoadSucceeded)
		{
			AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
				Result,
				LoadResult.ErrorMessage.IsEmpty()
					? TEXT("SQLUI packaged runtime persistence workflow smoke failed: workflow LoadLayout did not succeed.")
					: LoadResult.ErrorMessage);
		}
		else if (!Result.bWorkflowLoadedDocumentValid
			|| !Result.bWorkflowLoadedLayoutIdMatched)
		{
			AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
				Result,
				TEXT("SQLUI packaged runtime persistence workflow smoke failed: loaded document was invalid or had the wrong layout id."));
		}
	}

	ResetSQLUIPackagedRuntimePersistenceWorkflowSubsystem(Subsystem, Result);
	Result.bDatabaseExistsAfterPhase = FPaths::FileExists(Result.DatabasePath);
	if (!Result.bDatabaseExistsAfterPhase)
	{
		AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
			Result,
			TEXT("SQLUI packaged runtime persistence workflow smoke failed: database did not remain on disk after the phase."));
	}

	Result.bSucceeded =
		Result.bSubsystemFound
		&& Result.bAutoInitRequested
		&& Result.bAutoInitSucceeded
		&& Result.bProviderAvailable
		&& Result.bRepositoryAvailable
		&& Result.bBackendSQLite
		&& (Result.Phase != ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Save
			|| Result.bWorkflowSaveSucceeded)
		&& Result.bWorkflowListSucceeded
		&& Result.bWorkflowListedMetadataFound
		&& Result.bWorkflowLoadSucceeded
		&& Result.bWorkflowLoadedDocumentValid
		&& Result.bWorkflowLoadedLayoutIdMatched
		&& Result.bProviderReset
		&& Result.bDatabaseExistsAfterPhase;

	LogSQLUIPackagedRuntimePersistenceWorkflowSmokeResult(Result);
	return Result;
}

void RequestExitForSQLUIPackagedRuntimePersistenceWorkflowSmoke(
	const FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult& Result)
{
	FPlatformMisc::RequestExitWithStatus(
		false,
		Result.bSucceeded ? 0 : 1,
		TEXT("SQLUI packaged runtime persistence workflow smoke"));
}

struct FSQLUIPackagedRuntimePersistenceWorkflowSmokeTickerState
{
	double StartTimeSeconds = FPlatformTime::Seconds();
	FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult Result;
};
}

void FSQLUISamplePackagedRuntimePersistenceWorkflowSmoke::RunAndRequestExit()
{
	FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult InitialResult;
	ReadSQLUIPackagedRuntimePersistenceWorkflowSmokeCommandLine(InitialResult);

	if (InitialResult.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Unknown)
	{
		LogSQLUIPackagedRuntimePersistenceWorkflowSmokeResult(InitialResult);
		RequestExitForSQLUIPackagedRuntimePersistenceWorkflowSmoke(InitialResult);
		return;
	}

	if (InitialResult.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Cleanup)
	{
		const FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult Result =
			RunSQLUIPackagedRuntimePersistenceWorkflowCleanupPhase();
		RequestExitForSQLUIPackagedRuntimePersistenceWorkflowSmoke(Result);
		return;
	}

	if (InitialResult.Phase == ESQLUIPackagedRuntimePersistenceWorkflowSmokePhase::Save
		&& !DeleteSQLUIPackagedRuntimePersistenceWorkflowSmokeFiles(
			InitialResult.DatabasePath,
			InitialResult))
	{
		LogSQLUIPackagedRuntimePersistenceWorkflowSmokeResult(InitialResult);
		RequestExitForSQLUIPackagedRuntimePersistenceWorkflowSmoke(InitialResult);
		return;
	}

	if (USQLUILayoutRepositoryRuntimeSubsystem* Subsystem =
		FindSQLUIPackagedRuntimePersistenceWorkflowSubsystem())
	{
		const FSQLUISamplePackagedRuntimePersistenceWorkflowSmokeResult Result =
			RunSQLUIPackagedRuntimePersistenceWorkflowSaveOrVerifyPhase(
				Subsystem,
				InitialResult.Phase);
		RequestExitForSQLUIPackagedRuntimePersistenceWorkflowSmoke(Result);
		return;
	}

	TSharedRef<FSQLUIPackagedRuntimePersistenceWorkflowSmokeTickerState, ESPMode::ThreadSafe>
		SharedState =
			MakeShared<FSQLUIPackagedRuntimePersistenceWorkflowSmokeTickerState, ESPMode::ThreadSafe>();
	SharedState->Result = InitialResult;

	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda(
			[SharedState](float)
			{
				if (USQLUILayoutRepositoryRuntimeSubsystem* Subsystem =
					FindSQLUIPackagedRuntimePersistenceWorkflowSubsystem())
				{
					SharedState->Result =
						RunSQLUIPackagedRuntimePersistenceWorkflowSaveOrVerifyPhase(
							Subsystem,
							SharedState->Result.Phase);
					RequestExitForSQLUIPackagedRuntimePersistenceWorkflowSmoke(
						SharedState->Result);
					return false;
				}

				const double ElapsedSeconds =
					FPlatformTime::Seconds() - SharedState->StartTimeSeconds;
				if (ElapsedSeconds >= SQLUIPackagedRuntimePersistenceWorkflowSmokeTimeoutSeconds)
				{
					AppendSQLUIPackagedRuntimePersistenceWorkflowSmokeError(
						SharedState->Result,
						TEXT("SQLUI packaged runtime persistence workflow smoke failed: timed out waiting for GameInstance subsystem."));
					LogSQLUIPackagedRuntimePersistenceWorkflowSmokeResult(
						SharedState->Result);
					RequestExitForSQLUIPackagedRuntimePersistenceWorkflowSmoke(
						SharedState->Result);
					return false;
				}

				return true;
			}),
		0.1f);
}
