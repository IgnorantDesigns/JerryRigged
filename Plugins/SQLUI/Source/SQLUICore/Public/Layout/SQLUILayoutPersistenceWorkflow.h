#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepository.h"
#include "Layout/SQLUILayoutRepositoryRuntimeSubsystem.h"

/**
 * Storage-agnostic app-facing save/list/load workflow.
 *
 * This helper deliberately stays above concrete repositories. It does not
 * initialize providers, create databases, run migrations, copy seed databases,
 * or know SQLite paths. Callers are expected to configure the runtime subsystem
 * explicitly before using this workflow.
 */
class SQLUICORE_API FSQLUILayoutPersistenceWorkflow
{
public:
	static FSQLUILayoutSaveResult SaveLayout(
		USQLUILayoutRepositoryRuntimeSubsystem* Subsystem,
		const FSQLUILayoutDocument& Document);

	static FSQLUILayoutRepositoryListResult ListLayouts(
		USQLUILayoutRepositoryRuntimeSubsystem* Subsystem);

	static FSQLUILayoutLoadResult LoadLayout(
		USQLUILayoutRepositoryRuntimeSubsystem* Subsystem,
		const FString& LayoutId);

	static bool HasRepository(USQLUILayoutRepositoryRuntimeSubsystem* Subsystem);

	static USQLUILayoutRepository* GetRepository(
		USQLUILayoutRepositoryRuntimeSubsystem* Subsystem);
};
