#include "Layout/SQLUILayoutPersistenceWorkflow.h"

#include <atomic>

namespace
{
const TCHAR* SQLUILayoutPersistenceWorkflowNullSubsystemMessage =
	TEXT("SQLUI layout persistence workflow requires a valid runtime subsystem.");
const TCHAR* SQLUILayoutPersistenceWorkflowMissingRepositoryMessage =
	TEXT("SQLUI layout persistence workflow requires an active layout repository.");

struct FSQLUILayoutPersistenceWorkflowSaveState
{
	FSQLUILayoutSaveResult Result;
	std::atomic<bool> bCallbackDelivered { false };
};

struct FSQLUILayoutPersistenceWorkflowLoadState
{
	FSQLUILayoutLoadResult Result;
	std::atomic<bool> bCallbackDelivered { false };
};

FSQLUILayoutSaveResult MakeSQLUILayoutPersistenceWorkflowSaveFailure(
	const FSQLUILayoutDocument& Document,
	const FString& ErrorMessage)
{
	FSQLUILayoutSaveResult Result;
	Result.bSucceeded = false;
	Result.bBackendUnavailable = true;
	Result.SavedLayoutId = Document.Metadata.LayoutId;
	Result.ErrorMessage = ErrorMessage;
	return Result;
}

FSQLUILayoutRepositoryListResult MakeSQLUILayoutPersistenceWorkflowListFailure(
	const FString& ErrorMessage)
{
	FSQLUILayoutRepositoryListResult Result;
	Result.bSucceeded = false;
	Result.ErrorMessage = ErrorMessage;
	return Result;
}

FSQLUILayoutLoadResult MakeSQLUILayoutPersistenceWorkflowLoadFailure(
	const FString& LayoutId,
	const FString& ErrorMessage)
{
	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = false;
	Result.bBackendUnavailable = true;
	Result.Document.Metadata.LayoutId = LayoutId;
	Result.ErrorMessage = ErrorMessage;
	return Result;
}
}

FSQLUILayoutSaveResult FSQLUILayoutPersistenceWorkflow::SaveLayout(
	USQLUILayoutRepositoryRuntimeSubsystem* Subsystem,
	const FSQLUILayoutDocument& Document)
{
	if (!IsValid(Subsystem))
	{
		return MakeSQLUILayoutPersistenceWorkflowSaveFailure(
			Document,
			SQLUILayoutPersistenceWorkflowNullSubsystemMessage);
	}

	USQLUILayoutRepository* Repository = GetRepository(Subsystem);
	if (!IsValid(Repository))
	{
		return MakeSQLUILayoutPersistenceWorkflowSaveFailure(
			Document,
			SQLUILayoutPersistenceWorkflowMissingRepositoryMessage);
	}

	TSharedRef<FSQLUILayoutPersistenceWorkflowSaveState, ESPMode::ThreadSafe> SharedState =
		MakeShared<FSQLUILayoutPersistenceWorkflowSaveState, ESPMode::ThreadSafe>();
	SharedState->Result.SavedLayoutId = Document.Metadata.LayoutId;
	Repository->SaveLayout(
		Document,
		FSQLUILayoutSaveCompleteDelegate::CreateLambda(
			[SharedState](const FSQLUILayoutSaveResult& InResult)
			{
				SharedState->Result = InResult;
				SharedState->bCallbackDelivered.store(true);
			}));

	if (!SharedState->bCallbackDelivered.load())
	{
		SharedState->Result.bSucceeded = false;
		SharedState->Result.bBackendUnavailable = false;
		SharedState->Result.SavedLayoutId = Document.Metadata.LayoutId;
		SharedState->Result.ErrorMessage =
			TEXT("SQLUI layout persistence workflow SaveLayout callback was not delivered synchronously.");
	}

	return SharedState->Result;
}

FSQLUILayoutRepositoryListResult FSQLUILayoutPersistenceWorkflow::ListLayouts(
	USQLUILayoutRepositoryRuntimeSubsystem* Subsystem)
{
	if (!IsValid(Subsystem))
	{
		return MakeSQLUILayoutPersistenceWorkflowListFailure(
			SQLUILayoutPersistenceWorkflowNullSubsystemMessage);
	}

	USQLUILayoutRepository* Repository = GetRepository(Subsystem);
	if (!IsValid(Repository))
	{
		return MakeSQLUILayoutPersistenceWorkflowListFailure(
			SQLUILayoutPersistenceWorkflowMissingRepositoryMessage);
	}

	return Repository->ListLayouts();
}

FSQLUILayoutLoadResult FSQLUILayoutPersistenceWorkflow::LoadLayout(
	USQLUILayoutRepositoryRuntimeSubsystem* Subsystem,
	const FString& LayoutId)
{
	if (!IsValid(Subsystem))
	{
		return MakeSQLUILayoutPersistenceWorkflowLoadFailure(
			LayoutId,
			SQLUILayoutPersistenceWorkflowNullSubsystemMessage);
	}

	USQLUILayoutRepository* Repository = GetRepository(Subsystem);
	if (!IsValid(Repository))
	{
		return MakeSQLUILayoutPersistenceWorkflowLoadFailure(
			LayoutId,
			SQLUILayoutPersistenceWorkflowMissingRepositoryMessage);
	}

	TSharedRef<FSQLUILayoutPersistenceWorkflowLoadState, ESPMode::ThreadSafe> SharedState =
		MakeShared<FSQLUILayoutPersistenceWorkflowLoadState, ESPMode::ThreadSafe>();
	SharedState->Result.Document.Metadata.LayoutId = LayoutId;
	Repository->LoadLayout(
		LayoutId,
		FSQLUILayoutLoadCompleteDelegate::CreateLambda(
			[SharedState](const FSQLUILayoutLoadResult& InResult)
			{
				SharedState->Result = InResult;
				SharedState->bCallbackDelivered.store(true);
			}));

	if (!SharedState->bCallbackDelivered.load())
	{
		SharedState->Result.bSucceeded = false;
		SharedState->Result.bBackendUnavailable = false;
		SharedState->Result.Document.Metadata.LayoutId = LayoutId;
		SharedState->Result.ErrorMessage =
			TEXT("SQLUI layout persistence workflow LoadLayout callback was not delivered synchronously.");
	}

	return SharedState->Result;
}

bool FSQLUILayoutPersistenceWorkflow::HasRepository(
	USQLUILayoutRepositoryRuntimeSubsystem* Subsystem)
{
	return IsValid(GetRepository(Subsystem));
}

USQLUILayoutRepository* FSQLUILayoutPersistenceWorkflow::GetRepository(
	USQLUILayoutRepositoryRuntimeSubsystem* Subsystem)
{
	return IsValid(Subsystem) ? Subsystem->GetRepository() : nullptr;
}
