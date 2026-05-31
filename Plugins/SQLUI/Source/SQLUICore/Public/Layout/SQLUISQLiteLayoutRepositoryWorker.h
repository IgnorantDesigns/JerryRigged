#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepository.h"

struct SQLUICORE_API FSQLUISQLiteLayoutRepositoryWorkerSettings
{
	FString DatabasePath;
};

/**
 * Non-UObject boundary for SQLite layout repository database work.
 *
 * The UObject repository calls this helper synchronously for now. Because this
 * helper only accepts plain data and returns plain result structs, a later async
 * PR can run these operations behind the SQLUI database worker boundary and
 * marshal results back to the game thread before invoking repository callbacks.
 */
class SQLUICORE_API FSQLUISQLiteLayoutRepositoryWorker
{
public:
	static FString ResolveDatabasePath(const FString& DatabasePath);

	static FSQLUILayoutLoadResult LoadLayoutById(
		const FSQLUISQLiteLayoutRepositoryWorkerSettings& Settings,
		const FString& LayoutId);

	static FSQLUILayoutRepositoryListResult ListLayouts(
		const FSQLUISQLiteLayoutRepositoryWorkerSettings& Settings);

	static FSQLUILayoutSaveResult SaveLayout(
		const FSQLUISQLiteLayoutRepositoryWorkerSettings& Settings,
		const FSQLUILayoutDocument& Document);

	static FSQLUILayoutRepositoryRemoveResult RemoveLayout(
		const FSQLUISQLiteLayoutRepositoryWorkerSettings& Settings,
		const FString& LayoutId);

	static FSQLUILayoutRepositoryClearResult ClearLayouts(
		const FSQLUISQLiteLayoutRepositoryWorkerSettings& Settings);

	static FSQLUILayoutSaveResult MakeReadOnlySaveFailure(const FString& LayoutId);
	static FSQLUILayoutRepositoryRemoveResult MakeReadOnlyRemoveFailure(const FString& LayoutId);
	static FSQLUILayoutRepositoryClearResult MakeReadOnlyClearFailure();
};
