# SQLUI Smoke Test

The SQLUI sample smoke test commandlet runs the current sample runtime widget pipeline in a transient commandlet world. It creates the sample widget catalog, variable store, runtime context, and layout request, then reports whether the root widget and pipeline steps succeeded.

For the concise SQLite phase status and roadmap, see [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md). For the consolidated current SQLite runtime state, see [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md). For the future persistence/settings UX policy, see [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md). For the planned mutating settings editing/reset phase, see [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md). For the focused read-only UMG binding recipe, see [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md). For the validation-only draft settings UMG binding recipe, see [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md). For the dry-run apply-preview UMG binding recipe, see [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md). For the apply/cancel contract UMG binding recipe, see [`sqlui_persistence_settings_apply_contract_umg_usage.md`](sqlui_persistence_settings_apply_contract_umg_usage.md). For local packaged-build validation, see [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md). This document remains the command reference for local editor commandlet smoke paths and expected log lines.

By default, the smoke test uses the existing in-memory C++ layout document. It can also run the built-in SQLUISamples JSON layout fixture, which deserializes a tiny `SQLUI.FilterBox` layout through SQLUICore's layout JSON helpers before running the same runtime widget pipeline.

The in-memory repository smoke path reuses that same JSON fixture, selects the in-memory backend through the SQLUICore layout repository factory, saves the deserialized layout into `USQLUIInMemoryLayoutRepository`, verifies `ListLayouts` includes the saved metadata, loads it back by layout id, verifies `RemoveLayout` removes it, verifies `ListLayouts` no longer includes it, exercises `ClearLayouts`, and then runs the runtime widget pipeline with the loaded document.

The JSON file repository smoke path also reuses the JSON fixture, selects the JSON-file backend through the SQLUICore layout repository factory, saves the deserialized layout into `USQLUIJsonFileLayoutRepository`, verifies `ListLayouts` includes the saved metadata, loads it back by layout id, verifies `RemoveLayout` removes it, verifies `ListLayouts` no longer includes it, exercises `ClearLayouts`, and then runs the runtime widget pipeline with the loaded document.

Both repository smoke paths also select the unavailable backend through the factory and verify load/save report `bBackendUnavailable` cleanly. This keeps unavailable persistence explicit without relying on SQLite.

The optional SQLiteCore probe opens and closes a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteCoreProbe`, then removes the probe database file. This proves engine `SQLiteCore` can use a runtime-writable SQLUI smoke-test path without adding a layout repository, migrations, schema tables, async database workers, or repository factory behavior.

The optional database async probe runs a tiny SQLUICore-owned database-boundary request on a background thread and delivers the result back through the game-thread callback path. It does not open SQLite, run SQL, write database files, add migrations, or change repository selection.

The optional database async queue shutdown probe exercises `FSQLUIDatabaseAsyncQueue` directly without SQLite file I/O. It starts one queued work item, leaves a second item pending, shuts the queue down, verifies new work is rejected, verifies the pending item does not run, verifies the running item's stale completion callback is suppressed, and verifies the probe does not deadlock.

The optional layout repository runtime config probe exercises the SQLUICore config resolver directly. It verifies default `InMemory` policy, JSON and SQLite command-line parsing, relative SQLite path resolution under `Saved\SQLUI\LayoutRepositories`, absolute SQLite path preservation, SQLite flag mapping, missing-path unavailable behavior, invalid-backend fallback, and a factory-created SQLite save through explicit settings. The temporary SQLite database is scoped under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeConfig` and removed afterward.

The optional layout repository runtime integration probe exercises the SQLUICore helper above runtime config, seed-copy policy, and repository factory creation. It verifies the default config still creates a non-SQLite in-memory repository, explicit SQLite config creates a writable SQLite repository only when configured, missing SQLite path selection remains unavailable without creating DB files, explicit seed-copy policy prepares a copied runtime target that is readable through the SQLite repository, seed-copy failure is fatal before repository creation when configured that way, and all temporary DB files are removed.

The optional layout repository runtime provider probe exercises the SQLUICore UObject holder above the runtime integration helper. It verifies default `InMemory` initialization, reset/reinitialization, explicit SQLite save/list/load, missing-path unavailable behavior without DB creation, command-line SQLite initialization, explicit seed-copy initialization/readback, fatal missing-seed handling before repository storage, and cleanup under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeProvider`.

The optional layout repository runtime settings probe exercises the config-backed SQLUICore runtime settings policy above the provider/subsystem path. It verifies safe defaults, settings-driven `InMemory`, settings-driven SQLite, command-line override behavior, disabled command-line overrides, explicit SQLite missing-path unavailable behavior, and cleanup under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeSettings`.

The optional layout persistence workflow probe exercises the storage-agnostic SQLUICore helper above `USQLUILayoutRepositoryRuntimeSubsystem`. It verifies null and missing-repository failures, initializes an in-memory repository through the subsystem and saves/lists/loads through the helper, initializes an explicit SQLite repository through the same subsystem path and saves/lists/loads through the helper, verifies SQLite missing-path unavailable behavior, and cleans up under `Saved\SQLUI\SmokeTests\LayoutPersistenceWorkflow`.

The optional layout repository database management probe exercises the SQLUICore status/reset policy helper for the configured SQLite layout repository database. It verifies non-SQLite status/reset are safe no-ops, empty SQLite paths fail reset clearly, SQLite status detects a repository-created DB, reset removes the DB and sidecars idempotently, fake sidecars are removed, relative paths resolve under `Saved\SQLUI\LayoutRepositories`, and cleanup stays under `Saved\SQLUI\SmokeTests\LayoutRepositoryDatabaseManagement`.

The optional persistence status surface probe exercises the read-only SQLUICore status snapshot intended for future settings UI. It verifies default `InMemory` status does not initialize a provider or repository and does not create a DB, verifies a pre-created SQLite DB path/file/schema status is reported read-only, verifies sidecar presence is reported without deleting files, and cleans up under `Saved\SQLUI\SmokeTests\PersistenceStatusSurface`.

The optional persistence status display rows probe exercises the first UI-friendly adapter above that status snapshot. It verifies default and SQLite snapshots are converted into label/value/state/detail rows, verifies sidecar reporting, verifies the row adapter does not mutate the observed database, and cleans up under `Saved\SQLUI\SmokeTests\PersistenceStatusDisplayRows`.

The optional persistence status sample surface probe exercises the first SQLUISamples sample/dev presenter, tiny read-only panel adapter, and optional C++ UMG widget shell above those display rows. It documents and validates the existing presenter, adapter, and widget-shell Blueprint-callable refresh hooks plus the reflected refresh result, verifies refresh-style functions are not `BlueprintPure`, verifies cached widget-shell getters are `BlueprintPure`, verifies the widget shell derives from `UUserWidget` and exposes cached rows/formatted-lines/result/summary for Blueprint binding, verifies the shell contract by reflection without widget blueprint assets, maps, viewport attachment, or startup wiring, verifies the presenter and adapter can explicitly refresh rows and expose formatted lines under default `InMemory` config without initializing a provider or repository, verifies the adapter rows match the presenter path, verifies repeated refresh is deterministic and does not create a database, verifies a missing SQLite database path is presented gracefully without creating files, verifies refresh does not delete a smoke-owned sidecar, and cleans up under `Saved\SQLUI\SmokeTests\PersistenceStatusSampleSurface`.

That existing sample-surface probe is also the validation path for the read-only persistence status display rows, sample presenter, explicit refresh path, Blueprint-facing hook, panel adapter, C++ UMG widget shell contract, panel contract, and UMG binding recipe described in `docs/sqlui_persistence_settings_ux_design.md` and `docs/sqlui_persistence_status_umg_usage.md`. The probe does not require widget blueprint assets or maps, does not attach anything to the viewport, does not alter startup/config behavior, and keeps cleanup limited to smoke-owned files. Cleanup confirms no persistence status sample surface probe database or SQLite sidecar files remain. This path adds no new smoke flag, widget blueprint asset, map, startup behavior, polling, ticking, provider auto-init, settings editing, reset/delete action, migration, seed copy, database creation, or file deletion; it only validates how future Blueprint/UMG UI can consume the already-validated widget-shell/adapter/presenter/display rows safely.

The mutating settings editing and reset phase is planned in `docs/sqlui_persistence_settings_editing_reset_plan.md`. The non-mutating draft validation UI foundation is complete through the #105-#111 sequence and `-UsePersistenceSettingsDraftProbe`: validation-only draft model, UI-safe validation display rows/summary, SQLUISamples draft validation adapter, and C++ draft validation UMG widget shell contract for future Blueprint binding. The same draft probe also covers the SQLUICore dry-run apply-intent preview, UI-safe apply-preview display rows/summary, SQLUISamples apply-preview adapter, C++ apply-preview UMG widget shell contract, SQLUICore non-mutating apply/cancel contract or apply-readiness preview, the unavailable/non-mutating actual apply request skeleton, UI-safe apply result display rows/summary, UI-safe apply/cancel contract display rows/summary, SQLUISamples apply/cancel contract adapter, C++ apply/cancel contract UMG widget shell contract, and the #124 final docs-only foundation checkpoint. It does this without widget Blueprint assets, maps, viewport attachment, startup/config behavior changes, provider/repository initialization, settings mutation, config writes, DB creation, directory creation, migrations, seed copy, or file deletion outside smoke-owned cleanup. The safe binding recipes for those shells are documented in `docs/sqlui_persistence_settings_draft_umg_usage.md` for draft validation, `docs/sqlui_persistence_settings_apply_preview_umg_usage.md` for dry-run apply-preview display, and `docs/sqlui_persistence_settings_apply_contract_umg_usage.md` for apply/cancel contract display. Future implementation PRs that add mutating actual apply behavior, backend selector UI, SQLite path editor UI, provider auto-init controls, or reset/delete actions should add focused smoke coverage for those behaviors while preserving the existing read-only status, draft-validation, apply-preview, apply-request-refusal, apply-result display, and apply/cancel contract semantics.

The actual apply implementation gate in `docs/sqlui_persistence_settings_editing_reset_plan.md` requires any first mutating apply/config-write smoke path to stay SQLUICore-first and smoke-owned. The current apply entrypoint skeleton is covered by `-UsePersistenceSettingsDraftProbe` and deliberately returns unavailable/not implemented without mutation. Future mutating smoke coverage should prove valid apply in an isolated config context, invalid draft refusal with no mutation, no-change/no-op behavior, smoke-owned config cleanup/restore, default config safety, and no side-effect DB creation, migrations, seed copy, provider initialization, repository initialization, reset, or file deletion. No separate actual apply/config-write smoke flag exists for this non-mutating skeleton.

The optional SQLite migration probe opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe`, creates the smoke-only migration tracking table, applies and records one probe migration, verifies the migration row, closes the database, and removes the probe database file. This is not the planned SQLUI layout schema migration and does not add a SQLite layout repository.

The optional SQLite layout schema migration probe opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\LayoutSchemaMigrationProbe`, applies the planned initial layout schema through the SQLUICore migration runner, verifies the expected layout tables and indexes exist, closes the database, and removes the probe database file. This proves the schema DDL can apply locally without exercising repository operations or repository factory selection.

The optional SQLite layout read probe opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\LayoutReadProbe`, applies the planned initial layout schema, seeds one probe-only layout document into `layouts`, `layout_revisions`, and `layout_tags`, verifies list-style metadata and current-revision document load queries, deserializes and validates the loaded JSON, closes the database, and removes the probe database file. This proves read/list/load mapping against the schema without exercising the SQLite repository wrapper or repository factory selection.

The optional SQLite read-only layout repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteReadOnlyRepository`, prepares it with the planned schema and one probe-only layout, then reads it through `USQLUISQLiteLayoutRepository`. It verifies `ListLayouts` metadata and tags, verifies `LoadLayout` deserializes and validates the current document, verifies `SaveLayout`, `RemoveLayout`, and `ClearLayouts` are rejected while the repository is read-only, closes all database handles, and removes the probe database file.

The optional SQLite SaveLayout repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteSaveLayoutRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false`, saves one probe-only layout, verifies `ListLayouts` metadata and tags, verifies `LoadLayout`, saves the same layout id again with updated metadata, verifies the latest revision is loaded, and removes the probe database file. This proves the SaveLayout SQLite repository operation without using factory selection, async SQLite workers, or persistent database files.

The optional SQLite RemoveLayout repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteRemoveLayoutRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false`, saves one probe-only layout, verifies it is listed and loadable, soft-deletes it through `RemoveLayout`, verifies `ListLayouts` and `LoadLayout` no longer expose the layout, verifies revision history remains present, and removes the probe database file. This proves SQLite soft-delete semantics without using factory selection, async SQLite workers, or persistent database files.

The optional SQLite ClearLayouts repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteClearLayoutsRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false`, saves two probe-only layouts, soft-deletes one through `RemoveLayout`, destructively clears the selected repository scope through `ClearLayouts`, verifies active and soft-deleted layout rows plus dependent schema rows are gone, and removes the probe database file. This proves scoped cleanup semantics without using factory selection, async SQLite workers, or persistent database files.

The optional SQLite full lifecycle repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteFullLifecycleRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false`, saves one layout, lists and loads it, saves it again as revision 2 with updated metadata and tags, saves a second layout, soft-deletes the first layout, verifies revision history remains, destructively clears the selected repository scope, verifies schema rows are empty, and removes the probe database file. This combines the currently supported SQLite repository operations without using factory selection, async SQLite workers, or persistent database files.

The optional SQLite async callback repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteAsyncCallbackRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false` and `bRunCallbackOperationsAsync = true`, saves and loads one probe-only layout through the callback-style APIs, verifies the callbacks are delivered on the game thread, verifies synchronous `ListLayouts` metadata and tags afterward, and removes the probe database file. This proves opt-in async callback execution for `LoadLayout` and `SaveLayout` without changing default synchronous behavior or making `ListLayouts`, `RemoveLayout`, or `ClearLayouts` async.

The optional SQLite serialized async callback repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteSerializedAsyncCallbackRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false` and `bRunCallbackOperationsAsync = true`, enqueues two callback-style `SaveLayout` calls followed immediately by a callback-style `LoadLayout`, verifies callbacks are delivered on the game thread in enqueue order, verifies the loaded document is the second revision with updated metadata and tags, verifies synchronous `ListLayouts` metadata afterward, and removes the probe database file. This proves the opt-in async callback path is serialized for `SaveLayout` and `LoadLayout` without changing default synchronous behavior or making direct return-value methods async.

The optional SQLite factory layout repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteFactoryRepository`, prepares it with the planned schema, requests `ESQLUILayoutRepositoryBackend::SQLite` through `USQLUILayoutRepositoryFactory`, verifies the factory creates `USQLUISQLiteLayoutRepository`, exercises save/list/load/remove/clear behavior through that factory-created repository, verifies missing SQLite database path selection reports unavailable behavior, and removes the probe database file. This proves explicit SQLite factory selection without making SQLite the default backend or running migrations inside the factory.

The optional SQLite factory schema-init repository smoke path starts with no database under `Saved\SQLUI\SmokeTests\SQLiteFactorySchemaInitRepository`, requests `ESQLUILayoutRepositoryBackend::SQLite` through `USQLUILayoutRepositoryFactory`, configures `USQLUISQLiteLayoutRepository` with `bInitializeSchemaIfMissing = true` and `bCreateDatabaseIfMissing = true`, verifies `SaveLayout` creates and initializes the database through repository behavior, verifies list/load/remove/clear behavior, verifies a missing database without schema-init settings fails without creating a file, and removes the probe database files. This proves schema initialization remains opt-in and outside the factory.

The optional SQLite schema-init hardening smoke path writes temporary databases under `Saved\SQLUI\SmokeTests\SQLiteSchemaInitHardening`, verifies missing-database creation-disabled failure behavior, verifies create-enabled initialization, verifies already-initialized idempotence, verifies a complete schema with a missing migration row is recorded non-destructively, verifies a partial schema with a recorded migration fails clearly, verifies read-only repositories block schema initialization before creating files, and removes all probe database files.

The optional SQLite seed database copy policy probe writes temporary seed and target databases under `Saved\SQLUI\SmokeTests\SQLiteSeedDatabaseCopyPolicy`, verifies the SQLUICore seed-copy helper for missing target, existing target without overwrite, existing target with overwrite, missing seed, same-path failure, and runtime config mapping, verifies copied targets are readable through `USQLUISQLiteLayoutRepository`, and removes all probe database files.

The optional SQLite migration versioning policy probe writes temporary databases under `Saved\SQLUI\SmokeTests\SQLiteMigrationVersioningPolicy`, verifies the current known production migration status for `001_initial_layout_schema`, verifies complete-schema/missing-record detection and repair, verifies partial schemas fail clearly, verifies ordered/idempotent smoke-only migrations, verifies pending migration detection, verifies a failing smoke-only migration is not recorded, and removes all probe database files.

This is a local developer workflow only. It is not CI yet, and it does not assume Unreal Engine is installed on GitHub Actions or any build agent.

Packaged-build and packaged runtime validation are separate from these editor commandlet smoke paths. Use [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md) to run the local `RunUAT BuildCookRun` scaffold. Pass `-RunPackagedSQLiteSmoke` when you need the packaged executable to run the SQLUI SQLite lifecycle smoke, pass `-RunPackagedProviderStartupSmoke` when you need the packaged executable to prove direct runtime provider startup from command-line repository settings, pass `-RunPackagedProviderSubsystemSmoke` when you need the packaged executable to prove the passive runtime provider subsystem opt-in startup path, and pass `-RunPackagedPersistenceWorkflowSmoke` when you need the packaged executable to prove one workflow-saved SQLite layout persists across separate Save and Verify launches before Cleanup removes the database.

The smoke test does not edit maps, levels, Content, persistent database files, or the viewport. It attaches no widgets to the viewport. The JSON file repository smoke path writes only under `Saved\SQLUI\SmokeTests\Layouts`, removes its saved layout after loading it, and clears remaining layouts in that smoke-test repository directory. The SQLiteCore probe writes only under `Saved\SQLUI\SmokeTests\SQLiteCoreProbe` and removes `SQLiteCoreProbe.db` after the check. The layout repository runtime config probe writes only under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeConfig` and removes `LayoutRepositoryRuntimeConfig.db` after the check. The layout repository runtime integration probe writes only under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeIntegration` and removes `RuntimeIntegration.db`, `SeedRuntimeIntegration.db`, `SeedCopiedRuntimeIntegration.db`, `MissingSeedTargetRuntimeIntegration.db`, and SQLite sidecar files after the check. The layout repository runtime provider probe writes only under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeProvider` and removes `RuntimeProvider.db`, `CommandLineRuntimeProvider.db`, `SeedRuntimeProvider.db`, `SeedCopiedRuntimeProvider.db`, `MissingSeedTargetRuntimeProvider.db`, and SQLite sidecar files after the check. The layout repository runtime settings probe writes only under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeSettings` and removes `RuntimeSettings.db`, `CommandLineOverrideRuntimeSettings.db`, `MissingPathShouldNotExist.db`, and SQLite sidecar files after the check. The layout persistence workflow probe writes only under `Saved\SQLUI\SmokeTests\LayoutPersistenceWorkflow` and removes `LayoutPersistenceWorkflow.db`, `MissingRepositoryShouldNotExist.db`, and SQLite sidecar files after the check. The layout repository database management probe writes only under `Saved\SQLUI\SmokeTests\LayoutRepositoryDatabaseManagement` and removes `LayoutRepositoryDatabaseManagement.db`, `LayoutRepositoryDatabaseManagementSidecars.db`, and SQLite sidecar files after the check. The persistence status surface probe writes only under `Saved\SQLUI\SmokeTests\PersistenceStatusSurface` and removes `PersistenceStatusSurface.db`, `PersistenceStatusSidecarOnly.db`, and SQLite sidecar files after the check. The persistence status display rows probe writes only under `Saved\SQLUI\SmokeTests\PersistenceStatusDisplayRows` and removes `PersistenceStatusDisplayRows.db`, `PersistenceStatusDisplaySidecarOnly.db`, and SQLite sidecar files after the check. The persistence status sample surface probe writes only under `Saved\SQLUI\SmokeTests\PersistenceStatusSampleSurface` and removes `PersistenceStatusSampleSurface.db`, `PersistenceStatusSampleSurfaceSidecarOnly.db`, and SQLite sidecar files after the check. The persistence settings draft probe writes only under `Saved\SQLUI\SmokeTests\PersistenceSettingsDraft` for the explicit smoke-owned sidecar fixture and removes `PersistenceSettingsDraft.db`, `PersistenceSettingsDraftSidecarOnly.db`, and SQLite sidecar files after the check. The SQLite migration probe writes only under `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe` and removes `SQLiteMigrationProbe.db` after the check. The SQLite layout schema migration probe writes only under `Saved\SQLUI\SmokeTests\LayoutSchemaMigrationProbe` and removes `LayoutSchemaMigrationProbe.db` after the check. The SQLite layout read probe writes only under `Saved\SQLUI\SmokeTests\LayoutReadProbe` and removes `LayoutReadProbe.db` after the check. The SQLite read-only layout repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteReadOnlyRepository` and removes `SQLiteReadOnlyRepository.db` after the check. The SQLite SaveLayout repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteSaveLayoutRepository` and removes `SQLiteSaveLayoutRepository.db` after the check. The SQLite RemoveLayout repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteRemoveLayoutRepository` and removes `SQLiteRemoveLayoutRepository.db` after the check. The SQLite ClearLayouts repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteClearLayoutsRepository` and removes `SQLiteClearLayoutsRepository.db` after the check. The SQLite full lifecycle repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteFullLifecycleRepository` and removes `SQLiteFullLifecycleRepository.db` after the check. The SQLite async callback repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteAsyncCallbackRepository` and removes `SQLiteAsyncCallbackRepository.db` after the check. The SQLite serialized async callback repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteSerializedAsyncCallbackRepository` and removes `SQLiteSerializedAsyncCallbackRepository.db` after the check. The SQLite factory layout repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteFactoryRepository` and removes `SQLiteFactoryRepository.db` after the check. The SQLite factory schema-init repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteFactorySchemaInitRepository` and removes `SQLiteFactorySchemaInitRepository.db`, `SQLiteFactorySchemaInitRepositoryMissing.db`, and SQLite sidecar files after the check. The SQLite schema-init hardening smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteSchemaInitHardening` and removes `MissingCreateDisabled.db`, `EmptyCreateEnabled.db`, `AlreadyInitialized.db`, `CompleteSchemaMissingMigration.db`, `PartialSchema.db`, `ReadOnlyInitBlocked.db`, and SQLite sidecar files after the check. The SQLite seed database copy policy probe writes only under `Saved\SQLUI\SmokeTests\SQLiteSeedDatabaseCopyPolicy` and removes its seed, runtime target, missing-seed target, runtime-config target, and SQLite sidecar files after the check. The SQLite migration versioning policy probe writes only under `Saved\SQLUI\SmokeTests\SQLiteMigrationVersioningPolicy` and removes `LayoutSchemaCurrent.db`, `LayoutSchemaMissingRecord.db`, `LayoutSchemaPartial.db`, `SmokeOrderedMigrations.db`, `SmokePendingMigration.db`, `SmokeFailingMigration.db`, and SQLite sidecar files after the check. The database async probe and database async queue shutdown probe do not perform file I/O.

## Build JerryRiggedEditor

Build the editor target before running the commandlet so the SQLUISamples module and commandlet are available.

From the repo root:

```powershell
$EngineRoot = "C:\Program Files\Epic Games\UE_5.7"
$ProjectPath = (Resolve-Path .\JerryRigged.uproject).Path
& "$EngineRoot\Engine\Build\BatchFiles\Build.bat" JerryRiggedEditor Win64 Development "-Project=$ProjectPath" -WaitMutex -NoHotReloadFromIDE
```

If your engine is installed somewhere else, change `$EngineRoot`.

## Run The Default Smoke Test

From the repo root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"
```

The script derives `UnrealEditor-Cmd.exe` from `-EngineRoot`:

```text
<EngineRoot>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
```

If you need to point at a project file outside the default repo layout, pass `-ProjectPath`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -ProjectPath "D:\Projects\JerryRigged\JerryRigged.uproject"
```

If you already know the exact editor commandlet executable path, pass `-UnrealEditorCmdPath` instead:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -UnrealEditorCmdPath "C:\UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
```

## Run The JSON Fixture Smoke Test

The JSON fixture path keeps the same transient commandlet flow, but enables the built-in SQLUISamples JSON layout fixture with `-UseJsonLayoutFixture`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseJsonLayoutFixture
```

The commandlet also accepts `-JsonLayoutFixture` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

## Run The In-Memory Repository Smoke Test

The in-memory repository path keeps the same transient commandlet flow, deserializes the built-in SQLUISamples JSON layout fixture, selects `InMemory` through the repository factory, saves it into `USQLUIInMemoryLayoutRepository`, verifies list-after-save, loads it back by `LayoutId`, checks remove/list-after-remove/clear repository operations, and runs the same runtime widget pipeline with the loaded layout document:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseInMemoryLayoutRepository
```

The commandlet also accepts `-InMemoryLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is still sample scaffolding only. It does not use SQLite, disk persistence, file I/O, maps, Content, or viewport attachment.

## Run The JSON File Repository Smoke Test

The JSON file repository path keeps the same transient commandlet flow, deserializes the built-in SQLUISamples JSON layout fixture, selects `JsonFile` through the repository factory, saves it into `USQLUIJsonFileLayoutRepository`, verifies list-after-save, loads it back by `LayoutId`, checks remove/list-after-remove/clear repository operations, and runs the same runtime widget pipeline with the loaded layout document:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseJsonFileLayoutRepository
```

The commandlet also accepts `-JsonFileLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is still sample scaffolding only. It does not use SQLite, Content, maps, or viewport attachment. It writes temporary layout files under `Saved\SQLUI\SmokeTests\Layouts`, removes the saved fixture layout after the load step, and verifies `ClearLayouts` removes the same number of remaining layouts reported by the post-remove list.

## Run The SQLiteCore Probe

The SQLiteCore probe keeps the same transient commandlet flow, opens a temporary database under `Saved\SQLUI\SmokeTests\SQLiteCoreProbe`, runs a safe SQLiteCore integrity check, closes the database, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteCoreProbe
```

The commandlet also accepts `-SQLiteCoreProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a backend availability probe only. It does not exercise the SQLite layout repository, repository factory selection, schema tables, migrations, async database work, Content, maps, or persistent database files.

## Run The Database Async Probe

The database async probe keeps the same transient commandlet flow, enqueues a plain SQLUICore database-boundary request onto a background task, marshals the result back to the game thread, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseDatabaseAsyncProbe
```

The commandlet also accepts `-DatabaseAsyncProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is an async-boundary probe only. It does not open SQLite, create database files, run SQL, run migrations, exercise the SQLite layout repository, change repository factory selection, modify Content, or edit maps.

## Run The Database Async Queue Shutdown Probe

The database async queue shutdown probe keeps the same transient commandlet flow, exercises the SQLUICore serialized async queue directly, verifies shutdown rejects new work, drops pending work, suppresses stale completions from running work, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseDatabaseAsyncQueueShutdownProbe
```

The commandlet also accepts `-DatabaseAsyncQueueShutdownProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a queue shutdown-policy probe only. It does not open SQLite, create database files, run SQL, run migrations, exercise the SQLite layout repository, change repository factory selection, modify Content, or edit maps.

## Run The Layout Repository Runtime Config Probe

The layout repository runtime config probe keeps the same transient commandlet flow, exercises the SQLUICore runtime config resolver, verifies explicit backend and SQLite flag parsing, verifies SQLite path resolution policy, verifies missing SQLite path selection remains unavailable, verifies invalid backend text falls back to defaults, creates one factory-selected SQLite repository only from explicit settings, saves one probe-only layout with schema init/create enabled, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryRuntimeConfigProbe
```

The commandlet also accepts `-LayoutRepositoryRuntimeConfigProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves storage-selection policy only. It does not make SQLite the default backend, make normal startup use SQLite, add a settings UI, modify widgets, edit Content or maps, or leave persistent database files behind.

## Run The Layout Repository Runtime Integration Probe

The layout repository runtime integration probe keeps the same transient commandlet flow, exercises the SQLUICore helper that combines resolved runtime config, optional SQLite seed-copy policy, and repository factory creation, verifies default config still creates a non-SQLite in-memory repository, verifies explicit SQLite config can create a repository and save/list one layout, verifies missing SQLite path selection remains unavailable without creating database files, verifies explicit seed-copy creates a readable runtime target while leaving the seed DB intact, verifies seed-copy failure is fatal before repository creation, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryRuntimeIntegrationProbe
```

The commandlet also accepts `-LayoutRepositoryRuntimeIntegrationProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves runtime integration policy only. It does not make SQLite the default backend, make normal startup use SQLite, add a settings UI, modify widgets, edit Content or maps, add source-controlled seed DBs, or leave persistent database files behind.

## Run The Layout Repository Runtime Provider Probe

The layout repository runtime provider probe keeps the same transient commandlet flow, creates a `USQLUILayoutRepositoryRuntimeProvider`, verifies default `InMemory` initialization through the runtime integration helper, verifies reset/reinitialization, verifies explicit SQLite save/list/load through the provider-held repository, verifies missing SQLite path selection stays unavailable without creating database files, verifies command-line SQLite initialization, verifies explicit seed-copy initialization/readback, verifies fatal missing-seed behavior before repository storage, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryRuntimeProviderProbe
```

The commandlet also accepts `-LayoutRepositoryRuntimeProviderProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves runtime repository provider policy only. It does not make SQLite the default backend, make normal startup use SQLite, auto-initialize the passive runtime provider subsystem, add a settings UI, modify widgets, edit Content or maps, add source-controlled seed DBs, or leave persistent database files behind.

## Run The Layout Repository Runtime Settings Probe

The layout repository runtime settings probe keeps the same transient commandlet flow, verifies `USQLUILayoutRepositoryRuntimeSettings` safe defaults, verifies settings-driven `InMemory` auto-init resolution, verifies settings-driven SQLite save/list/load, verifies command-line repository settings override config only when allowed, verifies disabled overrides preserve `InMemory` settings without creating a database, verifies explicit SQLite with a missing path stays unavailable, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryRuntimeSettingsProbe
```

The commandlet also accepts `-LayoutRepositoryRuntimeSettingsProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves config-backed runtime settings policy only. It does not make SQLite the default backend, make normal startup use SQLite, add a settings UI, modify widgets, edit Content or maps, add source-controlled seed DBs, or leave persistent database files behind.

## Run The Layout Persistence Workflow Probe

The layout persistence workflow probe keeps the same transient commandlet flow, exercises the storage-agnostic `FSQLUILayoutPersistenceWorkflow` helper above `USQLUILayoutRepositoryRuntimeSubsystem`, verifies null and missing-repository failures, initializes in-memory and explicit SQLite repositories through the subsystem path, saves/lists/loads one probe layout through the helper for each configured backend, verifies SQLite missing-path unavailable behavior, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutPersistenceWorkflowProbe
```

The commandlet also accepts `-LayoutPersistenceWorkflowProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves the app-facing persistence workflow shape only. It does not make SQLite the default backend, make normal startup use SQLite, add a settings UI, modify widgets, edit Content or maps, copy seed databases, run migrations in the factory, or leave persistent database files behind.

## Run The Layout Repository Database Management Probe

The layout repository database management probe keeps the same transient commandlet flow, exercises `FSQLUILayoutRepositoryDatabaseManagement`, verifies status/reset behavior for non-SQLite and SQLite configs, saves one layout through an explicitly configured SQLite repository so status can detect a real database file, resets the database and sidecars, verifies reset is idempotent, verifies relative path resolution under `Saved\SQLUI\LayoutRepositories`, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryDatabaseManagementProbe
```

The commandlet also accepts `-LayoutRepositoryDatabaseManagementProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves database status/reset policy only. It does not add a settings UI, make SQLite the default backend, make normal startup use SQLite, open SQLite inside the management helper, run migrations in the helper, change factory behavior, modify widgets, edit Content or maps, or leave persistent database files behind.

## Run The Persistence Status Surface Probe

The persistence status surface probe keeps the same transient commandlet flow, exercises `USQLUIPersistenceStatusLibrary`, verifies the default `InMemory` status snapshot does not initialize a provider or repository and does not create database files, verifies a pre-created SQLite database path/file/schema status is reported read-only, verifies sidecar presence is reported without deleting files, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceStatusSurfaceProbe
```

The commandlet also accepts `-PersistenceStatusSurfaceProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves read-only status surface behavior only. It does not add settings editing, add reset actions, make SQLite the default backend, auto-initialize the provider, create database files from default config, run migrations from the status call, copy seed databases, modify widgets, edit Content or maps, or leave persistent database files behind.

## Run The Persistence Status Display Rows Probe

The persistence status display rows probe keeps the same transient commandlet flow, exercises `USQLUIPersistenceStatusDisplayLibrary`, verifies the default `InMemory` status rows are generated without initializing a provider or repository, verifies SQLite path/file/schema/sidecar rows are generated from read-only status snapshots, verifies row formatting does not mutate the observed database file, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceStatusDisplayRowsProbe
```

The commandlet also accepts `-PersistenceStatusDisplayRowsProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves the first UI-row adapter only. It does not add a widget, add settings editing, add reset actions, make SQLite the default backend, auto-initialize the provider, create database files from default config, run migrations from the display-row call, copy seed databases, modify widgets, edit Content or maps, or leave persistent database files behind.

## Run The Persistence Status Sample Surface Probe

The persistence status sample surface probe keeps the same transient commandlet flow, exercises `USQLUISamplePersistenceStatusPresenter` and `USQLUISamplePersistenceStatusPanelAdapter`, validates `USQLUISamplePersistenceStatusPanelWidget` by reflection without a widget blueprint, map, viewport instance, or startup wiring, verifies the existing presenter/adapter/widget-shell refresh functions and refresh result are reflected for Blueprint use, verifies refresh-style functions are callable but not pure, verifies cached widget-shell getters are pure, verifies cached widget-shell row/formatted-lines/result/summary binding surfaces are Blueprint-visible, verifies explicit caller-invoked refresh returns the default `InMemory` status rows and formatted lines without initializing a provider or repository, verifies repeated refresh is deterministic and does not create a database, verifies an explicit missing SQLite path is displayed without creating a database, verifies refresh does not delete a smoke-owned SQLite sidecar, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceStatusSampleSurfaceProbe
```

The commandlet also accepts `-PersistenceStatusSampleSurfaceProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves the first optional SQLUISamples sample/dev presenter, panel adapter, and C++ UMG widget shell. Refresh means re-querying the existing SQLUICore status/display surfaces on explicit caller request. It is not a full settings screen. It does not add widget blueprint assets, visual layout, viewport attachment, polling, ticking, startup refresh, settings editing, reset/delete controls, startup/maps/config wiring, SQLite as the default backend, provider auto-init, database creation from default config, migrations from presenter/adapter/widget code, seed database copy, file deletion from presenter/adapter/widget code, Content or map edits, or persistent database files.

## Run The Persistence Settings Draft Probe

The persistence settings draft probe keeps the same transient commandlet flow, exercises `USQLUIPersistenceSettingsDraftLibrary`, `USQLUIPersistenceSettingsDraftDisplayLibrary`, `USQLUIPersistenceSettingsApplyPreviewDisplayLibrary`, `USQLUIPersistenceSettingsApplyContractDisplayLibrary`, `USQLUIPersistenceSettingsApplyResultDisplayLibrary`, the SQLUISamples draft validation sample adapter, the SQLUISamples apply-preview sample adapter, the SQLUISamples apply/cancel contract sample adapter, and the optional validation/apply-preview/apply-contract C++ UMG widget shell contracts. It verifies the default/current `InMemory` draft validates safely, produces safe validation display rows, and reports no changes in the dry-run apply-intent preview, non-mutating apply/cancel contract, actual apply request skeleton, apply-result display rows, apply-preview display rows/adapters, and apply/cancel contract display rows/adapter; verifies an unknown backend is rejected with validation messages, a user-readable validation error row, a blocked apply preview, a blocked apply contract, blocked apply request, blocked apply-result display, user-readable apply-preview error rows, and user-readable apply/cancel contract display rows/adapter rows; verifies a pending SQLite draft/path can be represented and previewed without applying it or creating a database; verifies the apply request skeleton and apply-result display report SQLite as preview-only and do not create DB files or directories; verifies an empty SQLite path is rejected safely and previewed/displayed as blocked; verifies a pending provider auto-init value validates, displays, and previews/displays as pending without changing provider auto-init policy; verifies the apply request skeleton and apply-result display do not change provider auto-init policy or config files; verifies cancel preview reports pending changes as discardable value state only; verifies the widget shells derive from `UUserWidget`; verifies their caller-invoked refresh/build hooks are Blueprint-callable but not pure; verifies cached getters/properties are Blueprint-visible as intended; verifies the shell contracts by reflection without a widget blueprint asset, map, viewport instance, or startup wiring; verifies repeated validation/display/preview/apply-preview display/apply-contract display/apply-result display/apply-contract/apply-request/cancel-preview and adapter output is deterministic; verifies validation, preview, preview-display, apply-preview adapter, apply-contract display, apply-result display, apply-contract adapter, apply-contract, apply-request, and cancel-preview generation do not delete a smoke-owned SQLite sidecar; removes all probe files; and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceSettingsDraftProbe
```

The commandlet also accepts `-PersistenceSettingsDraftProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This existing probe is the validation path for the completed non-mutating persistence settings apply/cancel contract UI foundation checkpoint. No new smoke flag was added for the checkpoint; the same probe covers the #105-#124 chain while preserving the no-asset, no-map, no-startup, no-viewport, no-settings-mutation boundaries.

This same probe covers:

- Validation-only draft model.
- Draft validation display rows/summary.
- SQLUISamples draft validation adapter.
- C++ draft validation UMG widget shell contract.
- Dry-run apply-intent preview.
- Non-mutating apply contract/readiness result.
- Non-mutating actual apply request skeleton/refusal result.
- Apply result display rows/summary.
- Non-mutating cancel/discard preview result.
- Apply-preview display rows/summary.
- Apply/cancel contract display rows/summary.
- SQLUISamples apply-preview adapter.
- SQLUISamples apply/cancel contract adapter.
- C++ apply-preview UMG widget shell contract.
- C++ apply/cancel contract UMG widget shell contract.

The probe does not require widget blueprint assets or maps, does not attach widgets to the viewport, does not alter startup/config behavior, and does not add settings editing or actual apply/save behavior. Actual Apply execution is explicitly unavailable/not implemented, and the apply request skeleton plus apply-result display keep all config/settings/provider/repository/database/directory/write-open/migration/seed-copy/delete side-effect flags false. Cancel/discard is represented only as a pure value preview. Cleanup removes only smoke-owned files and the explicit checkpoint cleanup check should confirm no draft/status probe database or SQLite sidecar files remain.

Expected log lines include:

```text
SQLUI persistence settings draft probe selected: true
SQLUI persistence settings draft probe default draft created: true
SQLUI persistence settings draft probe default draft validated: true
SQLUI persistence settings draft probe default InMemory safe: true
SQLUI persistence settings draft probe current draft validated: true
SQLUI persistence settings draft probe unknown backend rejected: true
SQLUI persistence settings draft probe SQLite draft represented: true
SQLUI persistence settings draft probe SQLite draft did not create DB: true
SQLUI persistence settings draft probe SQLite empty path rejected: true
SQLUI persistence settings draft probe provider auto-init pending validated: true
SQLUI persistence settings draft probe provider auto-init policy unchanged: true
SQLUI persistence settings draft probe default display generated: true
SQLUI persistence settings draft probe default display safe: true
SQLUI persistence settings draft probe unknown backend display shows error: true
SQLUI persistence settings draft probe SQLite draft display generated: true
SQLUI persistence settings draft probe SQLite display did not create DB: true
SQLUI persistence settings draft probe SQLite empty path display shows error: true
SQLUI persistence settings draft probe provider auto-init display pending: true
SQLUI persistence settings draft probe default apply preview safe: true
SQLUI persistence settings draft probe current apply preview no changes: true
SQLUI persistence settings draft probe backend change apply preview detected: true
SQLUI persistence settings draft probe SQLite apply preview safe: true
SQLUI persistence settings draft probe unknown backend apply preview rejected: true
SQLUI persistence settings draft probe SQLite empty path apply preview rejected: true
SQLUI persistence settings draft probe provider auto-init apply preview detected: true
SQLUI persistence settings draft probe default apply contract safe: true
SQLUI persistence settings draft probe current apply contract no changes: true
SQLUI persistence settings draft probe apply execution unavailable: true
SQLUI persistence settings draft probe default apply request unavailable: true
SQLUI persistence settings draft probe default apply request did not mutate: true
SQLUI persistence settings draft probe unknown backend apply request blocked: true
SQLUI persistence settings draft probe SQLite apply request preview only: true
SQLUI persistence settings draft probe SQLite apply request did not create DB: true
SQLUI persistence settings draft probe provider auto-init apply request did not change policy: true
SQLUI persistence settings draft probe repeated apply request deterministic: true
SQLUI persistence settings draft probe apply request preserved config files: true
SQLUI persistence settings draft probe apply request did not create directory: true
SQLUI persistence settings draft probe default apply result display safe: true
SQLUI persistence settings draft probe unknown backend apply result display blocked: true
SQLUI persistence settings draft probe SQLite apply result display preview only: true
SQLUI persistence settings draft probe provider auto-init apply result display pending: true
SQLUI persistence settings draft probe repeated apply result display deterministic: true
SQLUI persistence settings draft probe apply result display preserved config files: true
SQLUI persistence settings draft probe apply result display did not create directory: true
SQLUI persistence settings draft probe backend change apply contract detected: true
SQLUI persistence settings draft probe SQLite apply contract safe: true
SQLUI persistence settings draft probe unknown backend apply contract blocked: true
SQLUI persistence settings draft probe SQLite empty path apply contract blocked: true
SQLUI persistence settings draft probe provider auto-init apply contract detected: true
SQLUI persistence settings draft probe cancel preview safe: true
SQLUI persistence settings draft probe cancel preview would discard changes: true
SQLUI persistence settings draft probe default apply preview display safe: true
SQLUI persistence settings draft probe current apply preview display no changes: true
SQLUI persistence settings draft probe backend change apply preview display detected: true
SQLUI persistence settings draft probe SQLite apply preview display safe: true
SQLUI persistence settings draft probe unknown backend apply preview display shows error: true
SQLUI persistence settings draft probe SQLite empty path apply preview display shows error: true
SQLUI persistence settings draft probe provider auto-init apply preview display pending: true
SQLUI persistence settings draft probe default apply contract display safe: true
SQLUI persistence settings draft probe current apply contract display no changes: true
SQLUI persistence settings draft probe apply contract display execution unavailable: true
SQLUI persistence settings draft probe backend change apply contract display detected: true
SQLUI persistence settings draft probe SQLite apply contract display safe: true
SQLUI persistence settings draft probe unknown backend apply contract display shows error: true
SQLUI persistence settings draft probe SQLite empty path apply contract display shows error: true
SQLUI persistence settings draft probe provider auto-init apply contract display pending: true
SQLUI persistence settings draft probe cancel preview display would discard changes: true
SQLUI persistence settings draft probe sample adapter default display generated: true
SQLUI persistence settings draft probe sample adapter default display safe: true
SQLUI persistence settings draft probe sample adapter unknown backend display shows error: true
SQLUI persistence settings draft probe sample adapter SQLite draft display generated: true
SQLUI persistence settings draft probe sample adapter SQLite display did not create DB: true
SQLUI persistence settings draft probe sample adapter SQLite empty path display shows error: true
SQLUI persistence settings draft probe sample adapter provider auto-init display pending: true
SQLUI persistence settings draft probe sample adapter repeated display deterministic: true
SQLUI persistence settings draft probe apply preview adapter default display generated: true
SQLUI persistence settings draft probe apply preview adapter default display safe: true
SQLUI persistence settings draft probe apply preview adapter current display no changes: true
SQLUI persistence settings draft probe apply preview adapter backend change detected: true
SQLUI persistence settings draft probe apply preview adapter SQLite display generated: true
SQLUI persistence settings draft probe apply preview adapter SQLite display did not create DB: true
SQLUI persistence settings draft probe apply preview adapter unknown backend shows error: true
SQLUI persistence settings draft probe apply preview adapter SQLite empty path shows error: true
SQLUI persistence settings draft probe apply preview adapter provider auto-init pending: true
SQLUI persistence settings draft probe apply preview adapter repeated display deterministic: true
SQLUI persistence settings draft probe apply contract adapter default display generated: true
SQLUI persistence settings draft probe apply contract adapter default display safe: true
SQLUI persistence settings draft probe apply contract adapter current display no changes: true
SQLUI persistence settings draft probe apply contract adapter execution unavailable: true
SQLUI persistence settings draft probe apply contract adapter backend change detected: true
SQLUI persistence settings draft probe apply contract adapter SQLite display generated: true
SQLUI persistence settings draft probe apply contract adapter SQLite display did not create DB: true
SQLUI persistence settings draft probe apply contract adapter unknown backend shows error: true
SQLUI persistence settings draft probe apply contract adapter SQLite empty path shows error: true
SQLUI persistence settings draft probe apply contract adapter provider auto-init pending: true
SQLUI persistence settings draft probe apply contract adapter cancel preview would discard changes: true
SQLUI persistence settings draft probe apply contract adapter repeated display deterministic: true
SQLUI persistence settings draft probe apply contract panel widget class derived from UUserWidget: true
SQLUI persistence settings draft probe apply contract panel widget Blueprint default refresh function callable: true
SQLUI persistence settings draft probe apply contract panel widget Blueprint current refresh function callable: true
SQLUI persistence settings draft probe apply contract panel widget Blueprint build function callable: true
SQLUI persistence settings draft probe apply contract panel widget refresh functions not BlueprintPure: true
SQLUI persistence settings draft probe apply contract panel widget cached getter functions BlueprintPure: true
SQLUI persistence settings draft probe apply contract panel widget rows property Blueprint visible: true
SQLUI persistence settings draft probe apply contract panel widget formatted lines property Blueprint visible: true
SQLUI persistence settings draft probe apply contract panel widget refresh result property Blueprint visible: true
SQLUI persistence settings draft probe apply contract panel widget summary text property Blueprint visible: true
SQLUI persistence settings draft probe apply contract panel widget contract flags Blueprint visible: true
SQLUI persistence settings draft probe apply contract panel widget contract validated without asset or viewport: true
SQLUI persistence settings draft probe panel widget class derived from UUserWidget: true
SQLUI persistence settings draft probe panel widget Blueprint default refresh function callable: true
SQLUI persistence settings draft probe panel widget Blueprint current refresh function callable: true
SQLUI persistence settings draft probe panel widget Blueprint build function callable: true
SQLUI persistence settings draft probe panel widget refresh functions not BlueprintPure: true
SQLUI persistence settings draft probe panel widget cached getter functions BlueprintPure: true
SQLUI persistence settings draft probe panel widget rows property Blueprint visible: true
SQLUI persistence settings draft probe panel widget formatted lines property Blueprint visible: true
SQLUI persistence settings draft probe panel widget refresh result property Blueprint visible: true
SQLUI persistence settings draft probe panel widget summary text property Blueprint visible: true
SQLUI persistence settings draft probe panel widget validation flags Blueprint visible: true
SQLUI persistence settings draft probe panel widget contract validated without asset or viewport: true
SQLUI persistence settings draft probe repeated validation deterministic: true
SQLUI persistence settings draft probe repeated display deterministic: true
SQLUI persistence settings draft probe repeated apply preview deterministic: true
SQLUI persistence settings draft probe repeated apply preview display deterministic: true
SQLUI persistence settings draft probe repeated apply contract display deterministic: true
SQLUI persistence settings draft probe repeated apply contract deterministic: true
SQLUI persistence settings draft probe repeated cancel preview deterministic: true
SQLUI persistence settings draft probe sidecar preserved during validation: true
SQLUI persistence settings draft probe sidecar preserved during apply preview: true
SQLUI persistence settings draft probe sidecar preserved during apply preview display: true
SQLUI persistence settings draft probe sidecar preserved during apply preview adapter: true
SQLUI persistence settings draft probe sidecar preserved during apply contract adapter: true
SQLUI persistence settings draft probe sidecar preserved during apply contract display: true
SQLUI persistence settings draft probe sidecar preserved during apply contract: true
SQLUI persistence settings draft probe sidecar preserved during apply request: true
SQLUI persistence settings draft probe sidecar preserved during apply result display: true
SQLUI persistence settings draft probe sidecar preserved during cancel preview: true
SQLUI persistence settings draft probe database files removed: true
SQLUI persistence settings draft probe succeeded.
```

This path proves validation/preview-only draft behavior, non-mutating apply/cancel contract reporting, the unavailable/non-mutating actual apply request skeleton, UI-safe validation display-row formatting and summary generation, UI-safe apply-preview display-row formatting and summary generation, UI-safe apply/cancel contract display-row formatting and summary generation, UI-safe apply-result display-row formatting and summary generation, SQLUISamples sample adapter consumption of validation, apply-preview, and apply/cancel contract rows, and the optional validation/apply-preview/apply-contract C++ UMG widget shell binding contracts. The future Blueprint/UMG binding recipes for these shells live in [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md), [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md), and [`sqlui_persistence_settings_apply_contract_umg_usage.md`](sqlui_persistence_settings_apply_contract_umg_usage.md). It does not require widget blueprint assets or maps, does not attach anything to the viewport, does not alter startup/config behavior, does not apply settings, write config, add settings editing UI controls, add backend selector controls, add SQLite path editor controls, add provider auto-init toggle controls, add reset/delete behavior, add widget blueprint assets, add visual layout, initialize providers or repositories, create database files, create directories from adapter/display/widget/preview/contract/apply-request/apply-result generation, open databases for writing, run migrations, copy seed databases, delete files outside smoke-owned cleanup, change startup behavior, edit Content or maps, or leave persistent database files behind. The only directory/file creation in this smoke path is the explicit smoke-owned sidecar fixture used to verify cleanup and non-deletion behavior.

After the probe succeeds, database files under `Saved\SQLUI\SmokeTests\PersistenceSettingsDraft` should not exist. A full checkpoint cleanup check should also confirm that the related read-only status paths used by this phase, such as `Saved\SQLUI\SmokeTests\PersistenceStatusSampleSurface` and `Saved\SQLUI\SmokeTests\PersistenceStatusDisplayRows`, contain no smoke-owned `.db`, `.db-journal`, `.db-wal`, or `.db-shm` files.

## Run The SQLite Migration Probe

The SQLite migration probe keeps the same transient commandlet flow, opens a temporary database under `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe`, creates a smoke-only `sqlui_schema_migrations` table, applies a tiny probe migration, records and verifies that migration, closes the database, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteMigrationProbe
```

The commandlet also accepts `-SQLiteMigrationProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a migration-runner probe only. The probe migration is clearly separate from the planned layout schema. It does not exercise SQLite layout persistence, run the real layout schema migration, change repository factory selection, modify Content, edit maps, or leave persistent database files behind.

## Run The SQLite Layout Schema Migration Probe

The SQLite layout schema migration probe keeps the same transient commandlet flow, applies the planned initial layout schema to a temporary database under `Saved\SQLUI\SmokeTests\LayoutSchemaMigrationProbe`, verifies expected tables and indexes, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteLayoutSchemaMigrationProbe
```

The commandlet also accepts `-SQLiteLayoutSchemaMigrationProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a layout-schema migration probe only. It does not exercise repository operations, repository factory selection, Content changes, map edits, or persistent database files.

## Run The SQLite Layout Read Probe

The SQLite layout read probe keeps the same transient commandlet flow, applies the planned initial layout schema to a temporary database under `Saved\SQLUI\SmokeTests\LayoutReadProbe`, seeds one probe-only layout document, verifies list-style metadata and current-revision load queries, deserializes and validates the loaded document JSON, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteLayoutReadProbe
```

The commandlet also accepts `-SQLiteLayoutReadProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a read/list/load mapping probe only. It does not exercise the SQLite repository wrapper, repository callbacks, repository factory selection, `SaveLayout`, `RemoveLayout`, `ClearLayouts`, Content changes, map edits, or persistent database files.

## Run The SQLite Read-Only Layout Repository Smoke Test

The SQLite read-only layout repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteReadOnlyRepository`, instantiates `USQLUISQLiteLayoutRepository` directly against that database, verifies `ListLayouts` metadata and tags, verifies `LoadLayout` deserializes and validates the current document JSON, verifies `SaveLayout`, `RemoveLayout`, and `ClearLayouts` are rejected as read-only operations, verifies the seeded layout remains readable afterward, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteReadOnlyLayoutRepository
```

The commandlet also accepts `-SQLiteReadOnlyLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a read-only repository smoke path. It instantiates the SQLite repository directly and does not exercise repository factory selection, writable SQLite operations, Content changes, map edits, or persistent database files. Writable `SaveLayout`, `RemoveLayout`, and `ClearLayouts` behavior are covered by separate optional smoke paths.

## Run The SQLite SaveLayout Repository Smoke Test

The SQLite SaveLayout repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteSaveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, saves one probe-only layout, verifies `ListLayouts` metadata and tags, verifies `LoadLayout` deserializes and validates the saved document JSON, saves the same layout id again with updated metadata, verifies the latest revision is loaded, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteSaveLayoutRepository
```

The commandlet also accepts `-SQLiteSaveLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a SaveLayout repository smoke path. It instantiates the SQLite repository directly and does not exercise repository factory selection, async callback execution, Content changes, map edits, or persistent database files. Soft-delete `RemoveLayout` and destructive `ClearLayouts` behavior are covered by separate optional smoke paths.

## Run The SQLite RemoveLayout Repository Smoke Test

The SQLite RemoveLayout repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteRemoveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, saves one probe-only layout, verifies it is listed and loadable, soft-deletes it, verifies the removed layout is absent from list/load results while revision history remains preserved, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteRemoveLayoutRepository
```

The commandlet also accepts `-SQLiteRemoveLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a RemoveLayout repository smoke path. It instantiates the SQLite repository directly and does not exercise repository factory selection, async callback execution, Content changes, map edits, or persistent database files. Destructive `ClearLayouts` behavior is covered by a separate optional smoke path.

## Run The SQLite ClearLayouts Repository Smoke Test

The SQLite ClearLayouts repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteClearLayoutsRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, saves two probe-only layouts, verifies both are listed, soft-deletes one layout, verifies only the active layout remains listed, clears the repository scope, verifies active and soft-deleted layout rows plus dependent schema rows are gone, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteClearLayoutsRepository
```

The commandlet also accepts `-SQLiteClearLayoutsRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a ClearLayouts repository smoke path. It instantiates the SQLite repository directly and does not exercise repository factory selection, async callback execution, Content changes, map edits, or persistent database files.

## Run The SQLite Full Lifecycle Repository Smoke Test

The SQLite full lifecycle repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteFullLifecycleRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, saves one probe-only layout, verifies list/load behavior, saves that layout again as revision 2 with updated metadata and tags, saves a second layout, soft-deletes the first layout, verifies the second layout remains readable, verifies the first layout's revision history remains preserved, clears the repository scope, verifies schema rows are empty, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteFullLifecycleRepository
```

The commandlet also accepts `-SQLiteFullLifecycleRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a combined currently supported lifecycle smoke path. It instantiates the SQLite repository directly and does not exercise repository factory selection, async callback execution, Content changes, map edits, or persistent database files.

## Run The SQLite Async Callback Repository Smoke Test

The SQLite async callback repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteAsyncCallbackRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false` and `bRunCallbackOperationsAsync = true`, saves and loads one probe-only layout through the callback-style APIs, verifies the callbacks are delivered on the game thread, verifies `ListLayouts` metadata and tags afterward, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteAsyncCallbackRepository
```

The commandlet also accepts `-SQLiteAsyncCallbackRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves opt-in async callback execution only. It instantiates the SQLite repository directly and does not exercise repository factory selection, async `ListLayouts`, async `RemoveLayout`, async `ClearLayouts`, Content changes, map edits, or persistent database files.

## Run The SQLite Serialized Async Callback Repository Smoke Test

The SQLite serialized async callback repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteSerializedAsyncCallbackRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false` and `bRunCallbackOperationsAsync = true`, enqueues two `SaveLayout` callbacks and one `LoadLayout` callback without waiting between calls, verifies callback order and game-thread delivery, verifies the loaded document is revision 2 with updated metadata and tags, verifies `ListLayouts` metadata afterward, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteSerializedAsyncCallbackRepository
```

The commandlet also accepts `-SQLiteSerializedAsyncCallbackRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves serialized opt-in async callback execution only. It instantiates the SQLite repository directly and does not exercise repository factory selection, async `ListLayouts`, async `RemoveLayout`, async `ClearLayouts`, Content changes, map edits, or persistent database files.

## Run The SQLite Factory Layout Repository Smoke Test

The SQLite factory layout repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteFactoryRepository`, requests `ESQLUILayoutRepositoryBackend::SQLite` through `USQLUILayoutRepositoryFactory`, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false` and `bRunCallbackOperationsAsync = true`, verifies save/list/load/remove/clear behavior through the factory-created repository, verifies missing SQLite database path selection reports unavailable behavior, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteFactoryLayoutRepository
```

The commandlet also accepts `-SQLiteFactoryLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves explicit factory selection only. It does not make SQLite the default backend, run migrations inside the factory, add packaged-build validation, add CI, modify widgets, edit Content or maps, or add persistent database files.

## Run The SQLite Factory Schema-Init Repository Smoke Test

The SQLite factory schema-init repository path keeps the same transient commandlet flow, ensures the temporary database under `Saved\SQLUI\SmokeTests\SQLiteFactorySchemaInitRepository` is absent, requests `ESQLUILayoutRepositoryBackend::SQLite` through `USQLUILayoutRepositoryFactory`, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false`, `bRunCallbackOperationsAsync = true`, `bInitializeSchemaIfMissing = true`, and `bCreateDatabaseIfMissing = true`, verifies `SaveLayout` creates and initializes the schema through repository behavior, verifies list/load/remove/clear behavior, verifies a missing database without schema-init settings fails without creating a file, removes probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteFactorySchemaInitRepository
```

The commandlet also accepts `-SQLiteFactorySchemaInitRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves opt-in schema initialization only. It does not make SQLite the default backend, run migrations inside the factory, create databases unless both schema-init settings are enabled, add packaged-build validation, add CI, modify widgets, edit Content or maps, or add persistent database files.

## Run The SQLite Schema-Init Hardening Smoke Test

The SQLite schema-init hardening path keeps the same transient commandlet flow, uses only temporary databases under `Saved\SQLUI\SmokeTests\SQLiteSchemaInitHardening`, and verifies the edge cases around `FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema` and read-only repository protection:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteSchemaInitHardening
```

The commandlet also accepts `-SQLiteSchemaInitHardening` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves schema initialization reliability only. It does not make SQLite the default backend, run migrations inside the factory, add packaged-build validation, add CI, modify widgets, edit Content or maps, or add persistent database files.

## Run The SQLite Seed Database Copy Policy Probe

The SQLite seed database copy policy probe keeps the same transient commandlet flow, creates a temporary seed database under `Saved\SQLUI\SmokeTests\SQLiteSeedDatabaseCopyPolicy\Seed`, copies it into runtime target paths under `Saved\SQLUI\SmokeTests\SQLiteSeedDatabaseCopyPolicy\Runtime`, verifies copied targets are readable through `USQLUISQLiteLayoutRepository`, verifies existing targets are preserved unless overwrite is explicit, verifies missing seed and same-path failures, verifies runtime config seed flags map to a copy request without copying, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteSeedDatabaseCopyPolicyProbe
```

The commandlet also accepts `-SQLiteSeedDatabaseCopyPolicyProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves explicit seed database file-copy policy only. It does not include source-controlled seed database assets, make SQLite the default backend, run seed copy inside the repository factory, open SQLite inside the copy helper, add startup behavior, add packaged-build validation, add CI, modify widgets, edit Content or maps, or add persistent database files.

## Run The SQLite Migration Versioning Policy Probe

The SQLite migration versioning policy probe keeps the same transient commandlet flow, creates temporary databases under `Saved\SQLUI\SmokeTests\SQLiteMigrationVersioningPolicy`, verifies the production known migration set is current at `001_initial_layout_schema`, verifies a complete schema with a missing migration row is detected and repaired non-destructively, verifies partial schemas fail clearly, applies two smoke-only migrations in order, verifies rerunning them is idempotent, verifies pending migration detection, verifies an intentionally failing smoke migration is not recorded, removes all probe database files, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteMigrationVersioningPolicyProbe
```

The commandlet also accepts `-SQLiteMigrationVersioningPolicyProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path proves migration versioning/status policy only. It does not add a real production `002` schema migration, alter the current initial schema, make SQLite the default backend, run migrations inside the repository factory, add startup behavior, add packaged-build validation, add CI, modify widgets, edit Content or maps, or add persistent database files.

## Expected Results

The script prints the exact command it runs, then returns the same exit code as the commandlet. It passes `-DDC-AllowNoActiveStores` so this transient smoke-test commandlet does not require a writable local Derived Data Cache.

On success, the commandlet exits with code `0`. Look for log lines like:

```text
SQLUI sample smoke test commandlet started.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
SQLUI sample smoke test step '<StepName>': Succeeded
```

For the JSON fixture smoke test, also look for:

```text
SQLUI sample smoke test JSON fixture selected: true
SQLUI sample smoke test JSON fixture parse succeeded.
SQLUI sample smoke test JSON fixture validation succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

For the in-memory repository smoke test, also look for:

```text
SQLUI sample smoke test JSON fixture selected: true
SQLUI sample smoke test JSON fixture parse succeeded.
SQLUI sample smoke test JSON fixture validation succeeded.
SQLUI sample smoke test layout repository factory unavailable backend succeeded. SaveBackendUnavailable=true LoadBackendUnavailable=true
SQLUI sample smoke test in-memory layout repository selected.
SQLUI sample smoke test in-memory layout repository save succeeded.
SQLUI sample smoke test in-memory layout repository load succeeded.
SQLUI sample smoke test in-memory layout repository list after save succeeded. LayoutCount=1 MetadataFound=true
SQLUI sample smoke test in-memory layout repository remove succeeded. Removed=true
SQLUI sample smoke test in-memory layout repository list after remove succeeded. LayoutCount=0 MetadataAbsent=true
SQLUI sample smoke test in-memory layout repository clear succeeded. RemovedCount=0 ExpectedRemovedCount=0
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

For the JSON file repository smoke test, also look for:

```text
SQLUI sample smoke test JSON fixture selected: true
SQLUI sample smoke test JSON fixture parse succeeded.
SQLUI sample smoke test JSON fixture validation succeeded.
SQLUI sample smoke test layout repository factory unavailable backend succeeded. SaveBackendUnavailable=true LoadBackendUnavailable=true
SQLUI sample smoke test JSON file layout repository selected.
SQLUI sample smoke test JSON file layout repository save succeeded.
SQLUI sample smoke test JSON file layout repository load succeeded.
SQLUI sample smoke test JSON file layout repository list after save succeeded. LayoutCount=1 MetadataFound=true
SQLUI sample smoke test JSON file layout repository remove succeeded. Removed=true
SQLUI sample smoke test JSON file layout repository list after remove succeeded. LayoutCount=0 MetadataAbsent=true
SQLUI sample smoke test JSON file layout repository clear succeeded. RemovedCount=0 ExpectedRemovedCount=0
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

If the JSON file repository smoke directory already contains valid layout files, the listed counts can be higher; the clear step still expects `RemovedCount` to match the post-remove list count.

For the SQLiteCore probe, also look for:

```text
SQLUI SQLiteCore probe selected: true
SQLUI SQLiteCore probe database opened: true
SQLUI SQLiteCore probe database closed: true
SQLUI SQLiteCore probe database removed: true
SQLUI SQLiteCore probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, `Saved\SQLUI\SmokeTests\SQLiteCoreProbe\SQLiteCoreProbe.db` should not exist.

For the database async probe, also look for:

```text
SQLUI database async probe selected: true
SQLUI database async probe background work completed: true
SQLUI database async probe callback delivered: true
SQLUI database async probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

For the database async queue shutdown probe, also look for:

```text
SQLUI database async queue shutdown probe selected: true
SQLUI database async queue shutdown probe queue created: true
SQLUI database async queue shutdown probe shutdown requested: true
SQLUI database async queue shutdown probe queue reported shutdown: true
SQLUI database async queue shutdown probe enqueue after shutdown rejected: true
SQLUI database async queue shutdown probe pending work suppressed: true
SQLUI database async queue shutdown probe running completion suppressed: true
SQLUI database async queue shutdown probe no callbacks delivered after shutdown: true
SQLUI database async queue shutdown probe no deadlock: true
SQLUI database async queue shutdown probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

For the layout repository runtime config probe, also look for:

```text
SQLUI layout repository runtime config probe selected: true
SQLUI layout repository runtime config probe default backend in-memory: true
SQLUI layout repository runtime config probe JSON file backend parsed: true
SQLUI layout repository runtime config probe SQLite backend parsed: true
SQLUI layout repository runtime config probe relative SQLite path resolved under Saved: true
SQLUI layout repository runtime config probe absolute SQLite path preserved: true
SQLUI layout repository runtime config probe SQLite flags parsed: true
SQLUI layout repository runtime config probe SQLite seed flags parsed: true
SQLUI layout repository runtime config probe SQLite seed copy request mapped: true
SQLUI layout repository runtime config probe SQLite missing path unavailable: true
SQLUI layout repository runtime config probe invalid backend falls back to default: true
SQLUI layout repository runtime config probe factory created SQLite repository: true
SQLUI layout repository runtime config probe factory SQLite save succeeded: true
SQLUI layout repository runtime config probe database removed: true
SQLUI layout repository runtime config probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeConfig\LayoutRepositoryRuntimeConfig.db` should not exist.

For the layout repository runtime integration probe, also look for:

```text
SQLUI layout repository runtime integration probe selected: true
SQLUI layout repository runtime integration probe default created repository: true
SQLUI layout repository runtime integration probe default backend in-memory: true
SQLUI layout repository runtime integration probe default not SQLite: true
SQLUI layout repository runtime integration probe SQLite created repository: true
SQLUI layout repository runtime integration probe SQLite repository created: true
SQLUI layout repository runtime integration probe SQLite save succeeded: true
SQLUI layout repository runtime integration probe SQLite database created: true
SQLUI layout repository runtime integration probe SQLite list succeeded: true
SQLUI layout repository runtime integration probe SQLite listed metadata found: true
SQLUI layout repository runtime integration probe SQLite missing path unavailable: true
SQLUI layout repository runtime integration probe SQLite missing path did not create DB: true
SQLUI layout repository runtime integration probe seed database prepared: true
SQLUI layout repository runtime integration probe seed copy requested: true
SQLUI layout repository runtime integration probe seed copy succeeded: true
SQLUI layout repository runtime integration probe seed copied target readable: true
SQLUI layout repository runtime integration probe seed copied target loaded layout: true
SQLUI layout repository runtime integration probe seed database left intact: true
SQLUI layout repository runtime integration probe seed copy failure fatal: true
SQLUI layout repository runtime integration probe seed copy failure did not create repository: true
SQLUI layout repository runtime integration probe seed copy failure did not create target: true
SQLUI layout repository runtime integration probe database files removed: true
SQLUI layout repository runtime integration probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, database files under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeIntegration` should not exist.

For the layout repository runtime provider probe, also look for:

```text
SQLUI layout repository runtime provider probe selected: true
SQLUI layout repository runtime provider probe provider created: true
SQLUI layout repository runtime provider probe default initialization succeeded: true
SQLUI layout repository runtime provider probe default backend in-memory: true
SQLUI layout repository runtime provider probe default repository available: true
SQLUI layout repository runtime provider probe default repository not SQLite: true
SQLUI layout repository runtime provider probe reset cleared repository: true
SQLUI layout repository runtime provider probe reinitialize after reset succeeded: true
SQLUI layout repository runtime provider probe SQLite initialization succeeded: true
SQLUI layout repository runtime provider probe SQLite backend selected: true
SQLUI layout repository runtime provider probe SQLite repository available: true
SQLUI layout repository runtime provider probe SQLite save succeeded: true
SQLUI layout repository runtime provider probe SQLite list succeeded: true
SQLUI layout repository runtime provider probe SQLite load succeeded: true
SQLUI layout repository runtime provider probe SQLite missing path handled: true
SQLUI layout repository runtime provider probe SQLite missing path did not create DB: true
SQLUI layout repository runtime provider probe command-line initialization succeeded: true
SQLUI layout repository runtime provider probe command-line SQLite save succeeded: true
SQLUI layout repository runtime provider probe seed database prepared: true
SQLUI layout repository runtime provider probe seed copy initialization succeeded: true
SQLUI layout repository runtime provider probe seed copy requested: true
SQLUI layout repository runtime provider probe seed copy succeeded: true
SQLUI layout repository runtime provider probe seed copied target readable: true
SQLUI layout repository runtime provider probe seed database left intact: true
SQLUI layout repository runtime provider probe seed copy failure fatal: true
SQLUI layout repository runtime provider probe seed copy failure did not create repository: true
SQLUI layout repository runtime provider probe seed copy failure did not create target: true
SQLUI layout repository runtime provider probe database files removed: true
SQLUI layout repository runtime provider probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, database files under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeProvider` should not exist.

For the layout repository runtime settings probe, also look for:

```text
SQLUI layout repository runtime settings probe selected: true
SQLUI layout repository runtime settings probe default settings safe: true
SQLUI layout repository runtime settings probe default does not auto initialize: true
SQLUI layout repository runtime settings probe settings in-memory auto init resolved: true
SQLUI layout repository runtime settings probe settings in-memory repository created: true
SQLUI layout repository runtime settings probe settings in-memory repository not SQLite: true
SQLUI layout repository runtime settings probe settings SQLite resolved: true
SQLUI layout repository runtime settings probe settings SQLite repository created: true
SQLUI layout repository runtime settings probe settings SQLite save succeeded: true
SQLUI layout repository runtime settings probe settings SQLite list succeeded: true
SQLUI layout repository runtime settings probe settings SQLite load succeeded: true
SQLUI layout repository runtime settings probe command-line override resolved SQLite: true
SQLUI layout repository runtime settings probe command-line override save succeeded: true
SQLUI layout repository runtime settings probe command-line override disabled preserved settings: true
SQLUI layout repository runtime settings probe command-line override disabled did not create DB: true
SQLUI layout repository runtime settings probe SQLite missing path unavailable: true
SQLUI layout repository runtime settings probe SQLite missing path did not create DB: true
SQLUI layout repository runtime settings probe database files removed: true
SQLUI layout repository runtime settings probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, database files under `Saved\SQLUI\SmokeTests\LayoutRepositoryRuntimeSettings` should not exist.

For the layout persistence workflow probe, also look for:

```text
SQLUI layout persistence workflow probe selected: true
SQLUI layout persistence workflow probe null subsystem save failed: true
SQLUI layout persistence workflow probe null subsystem list failed: true
SQLUI layout persistence workflow probe null subsystem load failed: true
SQLUI layout persistence workflow probe missing repository save failed: true
SQLUI layout persistence workflow probe missing repository list failed: true
SQLUI layout persistence workflow probe missing repository load failed: true
SQLUI layout persistence workflow probe missing repository did not create DB: true
SQLUI layout persistence workflow probe in-memory initialized: true
SQLUI layout persistence workflow probe in-memory save succeeded: true
SQLUI layout persistence workflow probe in-memory list succeeded: true
SQLUI layout persistence workflow probe in-memory listed metadata found: true
SQLUI layout persistence workflow probe in-memory load succeeded: true
SQLUI layout persistence workflow probe in-memory loaded document valid: true
SQLUI layout persistence workflow probe in-memory loaded layout id matched: true
SQLUI layout persistence workflow probe SQLite initialized: true
SQLUI layout persistence workflow probe SQLite save succeeded: true
SQLUI layout persistence workflow probe SQLite database created: true
SQLUI layout persistence workflow probe SQLite list succeeded: true
SQLUI layout persistence workflow probe SQLite listed metadata found: true
SQLUI layout persistence workflow probe SQLite load succeeded: true
SQLUI layout persistence workflow probe SQLite loaded document valid: true
SQLUI layout persistence workflow probe SQLite loaded layout id matched: true
SQLUI layout persistence workflow probe SQLite unavailable handled: true
SQLUI layout persistence workflow probe SQLite unavailable workflow failed clearly: true
SQLUI layout persistence workflow probe SQLite unavailable did not create DB: true
SQLUI layout persistence workflow probe database files removed: true
SQLUI layout persistence workflow probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, database files under `Saved\SQLUI\SmokeTests\LayoutPersistenceWorkflow` should not exist.

For the layout repository database management probe, also look for:

```text
SQLUI layout repository database management probe selected: true
SQLUI layout repository database management probe non-SQLite status safe: true
SQLUI layout repository database management probe non-SQLite reset safe: true
SQLUI layout repository database management probe SQLite empty path status handled: true
SQLUI layout repository database management probe SQLite empty path reset failed clearly: true
SQLUI layout repository database management probe SQLite status before create succeeded: true
SQLUI layout repository database management probe SQLite status before create absent: true
SQLUI layout repository database management probe SQLite status after save detected database: true
SQLUI layout repository database management probe SQLite status after save size positive: true
SQLUI layout repository database management probe SQLite reset succeeded: true
SQLUI layout repository database management probe SQLite reset removed database: true
SQLUI layout repository database management probe SQLite reset idempotent: true
SQLUI layout repository database management probe SQLite status after reset absent: true
SQLUI layout repository database management probe SQLite sidecar removal succeeded: true
SQLUI layout repository database management probe relative path resolved under Saved: true
SQLUI layout repository database management probe database files removed: true
SQLUI layout repository database management probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, database files under `Saved\SQLUI\SmokeTests\LayoutRepositoryDatabaseManagement` should not exist.

For the persistence status surface probe, also look for:

```text
SQLUI persistence status surface probe selected: true
SQLUI persistence status surface probe default status succeeded: true
SQLUI persistence status surface probe default backend InMemory: true
SQLUI persistence status surface probe default provider not initialized: true
SQLUI persistence status surface probe default repository not active: true
SQLUI persistence status surface probe default status did not create DB: true
SQLUI persistence status surface probe SQLite database prepared: true
SQLUI persistence status surface probe SQLite status succeeded: true
SQLUI persistence status surface probe SQLite path resolved: true
SQLUI persistence status surface probe SQLite database detected: true
SQLUI persistence status surface probe SQLite database size positive: true
SQLUI persistence status surface probe SQLite sidecars detected: true
SQLUI persistence status surface probe SQLite migration status checked: true
SQLUI persistence status surface probe SQLite migration status succeeded: true
SQLUI persistence status surface probe SQLite schema ready: true
SQLUI persistence status surface probe SQLite status read-only: true
SQLUI persistence status surface probe database files removed: true
SQLUI persistence status surface probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, database files under `Saved\SQLUI\SmokeTests\PersistenceStatusSurface` should not exist.

For the persistence status display rows probe, also look for:

```text
SQLUI persistence status display rows probe selected: true
SQLUI persistence status display rows probe default rows generated: true
SQLUI persistence status display rows probe default backend row found: true
SQLUI persistence status display rows probe default provider row found: true
SQLUI persistence status display rows probe default repository row found: true
SQLUI persistence status display rows probe default SQLite rows graceful: true
SQLUI persistence status display rows probe default rows did not create DB: true
SQLUI persistence status display rows probe SQLite database prepared: true
SQLUI persistence status display rows probe SQLite rows generated: true
SQLUI persistence status display rows probe SQLite path row found: true
SQLUI persistence status display rows probe SQLite database exists row found: true
SQLUI persistence status display rows probe SQLite database size row found: true
SQLUI persistence status display rows probe SQLite sidecars row found: true
SQLUI persistence status display rows probe SQLite schema row found: true
SQLUI persistence status display rows probe SQLite rows read-only: true
SQLUI persistence status display rows probe database files removed: true
SQLUI persistence status display rows probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, database files under `Saved\SQLUI\SmokeTests\PersistenceStatusDisplayRows` should not exist.

For the persistence status sample surface probe, also look for:

```text
SQLUI persistence status sample surface probe selected: true
SQLUI persistence status sample surface probe presenter created: true
SQLUI persistence status sample surface probe panel adapter created: true
SQLUI persistence status sample surface probe panel widget class derived from UUserWidget: true
SQLUI persistence status sample surface probe Blueprint refresh function callable: true
SQLUI persistence status sample surface probe Blueprint runtime config refresh function callable: true
SQLUI persistence status sample surface probe panel adapter Blueprint refresh function callable: true
SQLUI persistence status sample surface probe panel adapter Blueprint runtime config refresh function callable: true
SQLUI persistence status sample surface probe panel widget Blueprint refresh function callable: true
SQLUI persistence status sample surface probe panel widget Blueprint runtime config refresh function callable: true
SQLUI persistence status sample surface probe presenter refresh functions not BlueprintPure: true
SQLUI persistence status sample surface probe panel adapter refresh functions not BlueprintPure: true
SQLUI persistence status sample surface probe panel widget refresh functions not BlueprintPure: true
SQLUI persistence status sample surface probe panel widget cached getter functions BlueprintPure: true
SQLUI persistence status sample surface probe panel widget rows property Blueprint visible: true
SQLUI persistence status sample surface probe panel widget formatted lines property Blueprint visible: true
SQLUI persistence status sample surface probe panel widget refresh result property Blueprint visible: true
SQLUI persistence status sample surface probe panel widget summary text property Blueprint visible: true
SQLUI persistence status sample surface probe panel widget contract validated without asset or viewport: true
SQLUI persistence status sample surface probe Blueprint refresh result reflected: true
SQLUI persistence status sample surface probe default rows presented: true
SQLUI persistence status sample surface probe explicit refresh result succeeded: true
SQLUI persistence status sample surface probe panel adapter refresh succeeded: true
SQLUI persistence status sample surface probe panel adapter rows matched presenter: true
SQLUI persistence status sample surface probe panel adapter did not create DB: true
SQLUI persistence status sample surface probe default formatted lines generated: true
SQLUI persistence status sample surface probe default backend line found: true
SQLUI persistence status sample surface probe default provider line found: true
SQLUI persistence status sample surface probe default repository line found: true
SQLUI persistence status sample surface probe default SQLite rows graceful: true
SQLUI persistence status sample surface probe default surface did not create DB: true
SQLUI persistence status sample surface probe repeated refresh succeeded: true
SQLUI persistence status sample surface probe repeated refresh deterministic: true
SQLUI persistence status sample surface probe repeated refresh did not create DB: true
SQLUI persistence status sample surface probe panel adapter repeated refresh succeeded: true
SQLUI persistence status sample surface probe panel adapter repeated refresh deterministic: true
SQLUI persistence status sample surface probe missing SQLite rows presented: true
SQLUI persistence status sample surface probe missing SQLite path line found: true
SQLUI persistence status sample surface probe missing SQLite database absent line found: true
SQLUI persistence status sample surface probe missing SQLite surface did not create DB: true
SQLUI persistence status sample surface probe sidecar still present after refresh: true
SQLUI persistence status sample surface probe database files removed: true
SQLUI persistence status sample surface probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, database files under `Saved\SQLUI\SmokeTests\PersistenceStatusSampleSurface` should not exist.

For the SQLite migration probe, also look for:

```text
SQLUI SQLite migration probe selected: true
SQLUI SQLite migration probe database opened: true
SQLUI SQLite migration probe migration table created: true
SQLUI SQLite migration probe migration applied: true
SQLUI SQLite migration probe migration recorded: true
SQLUI SQLite migration probe database closed: true
SQLUI SQLite migration probe database removed: true
SQLUI SQLite migration probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe\SQLiteMigrationProbe.db` should not exist.

For the SQLite layout schema migration probe, also look for:

```text
SQLUI SQLite layout schema migration probe selected: true
SQLUI SQLite layout schema migration probe migration succeeded: true
SQLUI SQLite layout schema migration probe layouts table exists: true
SQLUI SQLite layout schema migration probe layout revisions table exists: true
SQLUI SQLite layout schema migration probe layout tags table exists: true
SQLUI SQLite layout schema migration probe layout checkpoints table exists: true
SQLUI SQLite layout schema migration probe layout previews table exists: true
SQLUI SQLite layout schema migration probe expected indexes exist: true
SQLUI SQLite layout schema migration probe database removed: true
SQLUI SQLite layout schema migration probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, `Saved\SQLUI\SmokeTests\LayoutSchemaMigrationProbe\LayoutSchemaMigrationProbe.db` should not exist.

For the SQLite layout read probe, also look for:

```text
SQLUI SQLite layout read probe selected: true
SQLUI SQLite layout read probe schema migration succeeded: true
SQLUI SQLite layout read probe seed inserted: true
SQLUI SQLite layout read probe list succeeded: true
SQLUI SQLite layout read probe listed metadata found: true
SQLUI SQLite layout read probe load succeeded: true
SQLUI SQLite layout read probe loaded document valid: true
SQLUI SQLite layout read probe database removed: true
SQLUI SQLite layout read probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, `Saved\SQLUI\SmokeTests\LayoutReadProbe\LayoutReadProbe.db` should not exist.

For the SQLite read-only layout repository smoke test, also look for:

```text
SQLUI SQLite read-only layout repository selected: true
SQLUI SQLite read-only layout repository prepared database: true
SQLUI SQLite read-only layout repository list succeeded: true
SQLUI SQLite read-only layout repository listed metadata found: true
SQLUI SQLite read-only layout repository load succeeded: true
SQLUI SQLite read-only layout repository loaded document valid: true
SQLUI SQLite read-only layout repository save rejected: true
SQLUI SQLite read-only layout repository remove rejected: true
SQLUI SQLite read-only layout repository clear rejected: true
SQLUI SQLite read-only layout repository list after rejected writes succeeded: true
SQLUI SQLite read-only layout repository load after rejected writes succeeded: true
SQLUI SQLite read-only layout repository database removed: true
SQLUI SQLite read-only layout repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteReadOnlyRepository\SQLiteReadOnlyRepository.db` should not exist.

For the SQLite SaveLayout repository smoke test, also look for:

```text
SQLUI SQLite SaveLayout repository selected: true
SQLUI SQLite SaveLayout repository database prepared: true
SQLUI SQLite SaveLayout repository first save succeeded: true
SQLUI SQLite SaveLayout repository list after save succeeded: true
SQLUI SQLite SaveLayout repository listed saved metadata found: true
SQLUI SQLite SaveLayout repository load after save succeeded: true
SQLUI SQLite SaveLayout repository loaded document valid: true
SQLUI SQLite SaveLayout repository second save succeeded: true
SQLUI SQLite SaveLayout repository list after second save succeeded: true
SQLUI SQLite SaveLayout repository load after second save succeeded: true
SQLUI SQLite SaveLayout repository latest revision loaded: true
SQLUI SQLite SaveLayout repository database removed: true
SQLUI SQLite SaveLayout repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteSaveLayoutRepository\SQLiteSaveLayoutRepository.db` should not exist.

For the SQLite RemoveLayout repository smoke test, also look for:

```text
SQLUI SQLite RemoveLayout repository selected: true
SQLUI SQLite RemoveLayout repository database prepared: true
SQLUI SQLite RemoveLayout repository save succeeded: true
SQLUI SQLite RemoveLayout repository list before remove succeeded: true
SQLUI SQLite RemoveLayout repository listed metadata found before remove: true
SQLUI SQLite RemoveLayout repository load before remove succeeded: true
SQLUI SQLite RemoveLayout repository remove succeeded: true
SQLUI SQLite RemoveLayout repository removed: true
SQLUI SQLite RemoveLayout repository list after remove succeeded: true
SQLUI SQLite RemoveLayout repository metadata absent after remove: true
SQLUI SQLite RemoveLayout repository load after remove failed as expected: true
SQLUI SQLite RemoveLayout repository revisions preserved: true
SQLUI SQLite RemoveLayout repository database removed: true
SQLUI SQLite RemoveLayout repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteRemoveLayoutRepository\SQLiteRemoveLayoutRepository.db` should not exist.

For the SQLite ClearLayouts repository smoke test, also look for:

```text
SQLUI SQLite ClearLayouts repository selected: true
SQLUI SQLite ClearLayouts repository database prepared: true
SQLUI SQLite ClearLayouts repository first save succeeded: true
SQLUI SQLite ClearLayouts repository second save succeeded: true
SQLUI SQLite ClearLayouts repository list before remove succeeded: true
SQLUI SQLite ClearLayouts repository both metadata entries found before remove: true
SQLUI SQLite ClearLayouts repository remove succeeded: true
SQLUI SQLite ClearLayouts repository removed: true
SQLUI SQLite ClearLayouts repository list before clear succeeded: true
SQLUI SQLite ClearLayouts repository active metadata preserved before clear: true
SQLUI SQLite ClearLayouts repository removed metadata absent before clear: true
SQLUI SQLite ClearLayouts repository clear succeeded: true
SQLUI SQLite ClearLayouts repository removed count matched expected: true
SQLUI SQLite ClearLayouts repository list after clear succeeded: true
SQLUI SQLite ClearLayouts repository empty after clear: true
SQLUI SQLite ClearLayouts repository loads after clear failed as expected: true
SQLUI SQLite ClearLayouts repository tables empty after clear: true
SQLUI SQLite ClearLayouts repository database removed: true
SQLUI SQLite ClearLayouts repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteClearLayoutsRepository\SQLiteClearLayoutsRepository.db` should not exist.

For the SQLite full lifecycle repository smoke test, also look for:

```text
SQLUI SQLite full lifecycle repository selected: true
SQLUI SQLite full lifecycle repository database prepared: true
SQLUI SQLite full lifecycle repository first save succeeded: true
SQLUI SQLite full lifecycle repository list after first save succeeded: true
SQLUI SQLite full lifecycle repository first metadata found after first save: true
SQLUI SQLite full lifecycle repository load after first save succeeded: true
SQLUI SQLite full lifecycle repository first revision loaded: true
SQLUI SQLite full lifecycle repository second revision save succeeded: true
SQLUI SQLite full lifecycle repository latest revision loaded: true
SQLUI SQLite full lifecycle repository second layout save succeeded: true
SQLUI SQLite full lifecycle repository list before remove succeeded: true
SQLUI SQLite full lifecycle repository both metadata entries found before remove: true
SQLUI SQLite full lifecycle repository remove succeeded: true
SQLUI SQLite full lifecycle repository removed: true
SQLUI SQLite full lifecycle repository list after remove succeeded: true
SQLUI SQLite full lifecycle repository removed metadata absent after remove: true
SQLUI SQLite full lifecycle repository second metadata preserved after remove: true
SQLUI SQLite full lifecycle repository removed load failed as expected: true
SQLUI SQLite full lifecycle repository second load after remove succeeded: true
SQLUI SQLite full lifecycle repository revision history preserved after remove: true
SQLUI SQLite full lifecycle repository clear succeeded: true
SQLUI SQLite full lifecycle repository removed count matched expected: true
SQLUI SQLite full lifecycle repository list after clear succeeded: true
SQLUI SQLite full lifecycle repository empty after clear: true
SQLUI SQLite full lifecycle repository loads after clear failed as expected: true
SQLUI SQLite full lifecycle repository tables empty after clear: true
SQLUI SQLite full lifecycle repository database removed: true
SQLUI SQLite full lifecycle repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteFullLifecycleRepository\SQLiteFullLifecycleRepository.db` should not exist.

For the SQLite async callback repository smoke test, also look for:

```text
SQLUI SQLite async callback repository selected: true
SQLUI SQLite async callback repository database prepared: true
SQLUI SQLite async callback repository save callback delivered: true
SQLUI SQLite async callback repository save succeeded: true
SQLUI SQLite async callback repository load callback delivered: true
SQLUI SQLite async callback repository load succeeded: true
SQLUI SQLite async callback repository loaded document valid: true
SQLUI SQLite async callback repository list after callbacks succeeded: true
SQLUI SQLite async callback repository listed metadata found: true
SQLUI SQLite async callback repository callbacks delivered on game thread: true
SQLUI SQLite async callback repository database removed: true
SQLUI SQLite async callback repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteAsyncCallbackRepository\SQLiteAsyncCallbackRepository.db` should not exist.

For the SQLite serialized async callback repository smoke test, also look for:

```text
SQLUI SQLite serialized async callback repository selected: true
SQLUI SQLite serialized async callback repository database prepared: true
SQLUI SQLite serialized async callback repository first save callback delivered: true
SQLUI SQLite serialized async callback repository first save succeeded: true
SQLUI SQLite serialized async callback repository second save callback delivered: true
SQLUI SQLite serialized async callback repository second save succeeded: true
SQLUI SQLite serialized async callback repository load callback delivered: true
SQLUI SQLite serialized async callback repository load succeeded: true
SQLUI SQLite serialized async callback repository callbacks delivered in order: true
SQLUI SQLite serialized async callback repository callbacks delivered on game thread: true
SQLUI SQLite serialized async callback repository latest revision loaded: true
SQLUI SQLite serialized async callback repository list after serialized callbacks succeeded: true
SQLUI SQLite serialized async callback repository listed updated metadata found: true
SQLUI SQLite serialized async callback repository database removed: true
SQLUI SQLite serialized async callback repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteSerializedAsyncCallbackRepository\SQLiteSerializedAsyncCallbackRepository.db` should not exist.

For the SQLite factory layout repository smoke test, also look for:

```text
SQLUI SQLite factory layout repository selected: true
SQLUI SQLite factory layout repository database prepared: true
SQLUI SQLite factory layout repository created repository: true
SQLUI SQLite factory layout repository created SQLite repository: true
SQLUI SQLite factory layout repository save succeeded: true
SQLUI SQLite factory layout repository list succeeded: true
SQLUI SQLite factory layout repository listed metadata found: true
SQLUI SQLite factory layout repository load succeeded: true
SQLUI SQLite factory layout repository loaded document valid: true
SQLUI SQLite factory layout repository remove succeeded: true
SQLUI SQLite factory layout repository removed: true
SQLUI SQLite factory layout repository metadata absent after remove: true
SQLUI SQLite factory layout repository clear succeeded: true
SQLUI SQLite factory layout repository missing path unavailable: true
SQLUI SQLite factory layout repository database removed: true
SQLUI SQLite factory layout repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteFactoryRepository\SQLiteFactoryRepository.db` should not exist.

For the SQLite factory schema-init repository smoke test, also look for:

```text
SQLUI SQLite factory schema init repository selected: true
SQLUI SQLite factory schema init repository database absent before start: true
SQLUI SQLite factory schema init repository created repository: true
SQLUI SQLite factory schema init repository created SQLite repository: true
SQLUI SQLite factory schema init repository save initialized schema: true
SQLUI SQLite factory schema init repository database created: true
SQLUI SQLite factory schema init repository save succeeded: true
SQLUI SQLite factory schema init repository list succeeded: true
SQLUI SQLite factory schema init repository listed metadata found: true
SQLUI SQLite factory schema init repository load succeeded: true
SQLUI SQLite factory schema init repository loaded document valid: true
SQLUI SQLite factory schema init repository remove succeeded: true
SQLUI SQLite factory schema init repository clear succeeded: true
SQLUI SQLite factory schema init repository missing db without init failed: true
SQLUI SQLite factory schema init repository missing db without init not created: true
SQLUI SQLite factory schema init repository database removed: true
SQLUI SQLite factory schema init repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteFactorySchemaInitRepository\SQLiteFactorySchemaInitRepository.db` and `Saved\SQLUI\SmokeTests\SQLiteFactorySchemaInitRepository\SQLiteFactorySchemaInitRepositoryMissing.db` should not exist.

For the SQLite schema-init hardening smoke test, also look for:

```text
SQLUI SQLite schema init hardening selected: true
SQLUI SQLite schema init hardening missing DB create disabled failed: true
SQLUI SQLite schema init hardening missing DB create disabled not created: true
SQLUI SQLite schema init hardening empty DB create enabled succeeded: true
SQLUI SQLite schema init hardening empty DB schema ready: true
SQLUI SQLite schema init hardening already initialized succeeded: true
SQLUI SQLite schema init hardening already initialized detected: true
SQLUI SQLite schema init hardening migration row not duplicated: true
SQLUI SQLite schema init hardening complete schema missing migration succeeded: true
SQLUI SQLite schema init hardening complete schema missing migration recorded: true
SQLUI SQLite schema init hardening partial schema failed clearly: true
SQLUI SQLite schema init hardening partial schema reported missing objects: true
SQLUI SQLite schema init hardening read-only init blocked: true
SQLUI SQLite schema init hardening read-only init did not create DB: true
SQLUI SQLite schema init hardening database files removed: true
SQLUI SQLite schema init hardening succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, the probe database files under `Saved\SQLUI\SmokeTests\SQLiteSchemaInitHardening` should not exist.

For the SQLite seed database copy policy probe, also look for:

```text
SQLUI SQLite seed database copy policy probe selected: true
SQLUI SQLite seed database copy policy probe seed database created: true
SQLUI SQLite seed database copy policy probe missing target copied: true
SQLUI SQLite seed database copy policy probe copied target readable: true
SQLUI SQLite seed database copy policy probe copied target loaded seed layout: true
SQLUI SQLite seed database copy policy probe existing target preserved without overwrite: true
SQLUI SQLite seed database copy policy probe overwrite target copied seed: true
SQLUI SQLite seed database copy policy probe overwrite removed existing layout: true
SQLUI SQLite seed database copy policy probe missing seed failed: true
SQLUI SQLite seed database copy policy probe missing seed did not create target: true
SQLUI SQLite seed database copy policy probe same path failed: true
SQLUI SQLite seed database copy policy probe same path left seed intact: true
SQLUI SQLite seed database copy policy probe runtime config seed flags parsed: true
SQLUI SQLite seed database copy policy probe database files removed: true
SQLUI SQLite seed database copy policy probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, the probe database files under `Saved\SQLUI\SmokeTests\SQLiteSeedDatabaseCopyPolicy` should not exist.

For the SQLite migration versioning policy probe, also look for:

```text
SQLUI SQLite migration versioning policy probe selected: true
SQLUI SQLite migration versioning policy probe current initial schema status succeeded: true
SQLUI SQLite migration versioning policy probe latest known migration matched: true
SQLUI SQLite migration versioning policy probe no pending known migrations: true
SQLUI SQLite migration versioning policy probe complete schema missing record detected: true
SQLUI SQLite migration versioning policy probe missing record repaired non-destructively: true
SQLUI SQLite migration versioning policy probe partial schema failed clearly: true
SQLUI SQLite migration versioning policy probe smoke migrations applied in order: true
SQLUI SQLite migration versioning policy probe smoke migrations idempotent: true
SQLUI SQLite migration versioning policy probe smoke pending migration detected: true
SQLUI SQLite migration versioning policy probe smoke pending migration applied: true
SQLUI SQLite migration versioning policy probe failing migration failed clearly: true
SQLUI SQLite migration versioning policy probe failing migration not recorded: true
SQLUI SQLite migration versioning policy probe database files removed: true
SQLUI SQLite migration versioning policy probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

The failing-migration case intentionally prepares invalid SQL, so Unreal may also report a SQLite prepared-statement warning in this smoke path. The expected pass condition is that the commandlet exits `0`, the failed smoke migration is not recorded, and the probe database files under `Saved\SQLUI\SmokeTests\SQLiteMigrationVersioningPolicy` do not exist afterward.

Some optional pipeline steps may log `Skipped` depending on the current sample request. Failures are logged with `SQLUI sample smoke test commandlet failed.` and the script returns a non-zero exit code.

Useful logs appear in the command window. Unreal may also write local generated logs under `Saved\Logs`, such as `Saved\Logs\JerryRigged.log`.

## Packaged Persistence Workflow Smoke

The packaged persistence workflow smoke is run through the packaged-build validation script, not the editor commandlet:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput -RunPackagedPersistenceWorkflowSmoke
```

That script builds/packages once, then launches the packaged executable three times with `-SQLUIRuntimePersistenceWorkflowSmoke` and `-SQLUIRuntimePersistenceWorkflowSmokePhase=Save|Verify|Cleanup`. The relative SQLite path is `PackagedRuntimeSmoke/PersistenceWorkflow/PersistenceWorkflow.db`, which resolves under packaged runtime `Saved/SQLUI/LayoutRepositories`.

The Save phase log should include:

```text
SQLUI packaged runtime persistence workflow smoke selected phase: Save
SQLUI packaged runtime persistence workflow smoke auto init requested: true
SQLUI packaged runtime persistence workflow smoke auto init succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow save succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow list succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow listed metadata found: true
SQLUI packaged runtime persistence workflow smoke workflow load succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow loaded document valid: true
SQLUI packaged runtime persistence workflow smoke provider reset: true
SQLUI packaged runtime persistence workflow smoke database exists after phase: true
SQLUI packaged runtime persistence workflow smoke Save phase succeeded.
```

The Verify phase log should include:

```text
SQLUI packaged runtime persistence workflow smoke selected phase: Verify
SQLUI packaged runtime persistence workflow smoke auto init requested: true
SQLUI packaged runtime persistence workflow smoke auto init succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow list succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow listed persisted metadata found: true
SQLUI packaged runtime persistence workflow smoke workflow load succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow loaded persisted document valid: true
SQLUI packaged runtime persistence workflow smoke provider reset: true
SQLUI packaged runtime persistence workflow smoke database exists after phase: true
SQLUI packaged runtime persistence workflow smoke Verify phase succeeded.
```

The Cleanup phase log should include:

```text
SQLUI packaged runtime persistence workflow smoke selected phase: Cleanup
SQLUI packaged runtime persistence workflow smoke database removed: true
SQLUI packaged runtime persistence workflow smoke Cleanup phase succeeded.
```

The validation script also checks that the database and SQLite sidecar files are gone after Cleanup. Cleanup runs even when Verify fails, but the script still returns a non-zero exit code for the failing phase.

## Common Local Issues

- Quote paths that contain spaces, especially `C:\Program Files\Epic Games\...`.
- If PowerShell blocks the script, run it with `-ExecutionPolicy Bypass` as shown above.
- If `UnrealEditor-Cmd.exe` is not found, check `-EngineRoot` or pass `-UnrealEditorCmdPath`.
- If the commandlet cannot be found, build `JerryRiggedEditor` first.
- Visual Studio, MSVC, Windows SDK, or Unreal toolchain prompts can appear on first build. Install the C++ desktop workload and the engine-required Windows SDK if prompted.
- Windows firewall prompts can appear the first time Unreal tools run locally. The smoke test itself does not require network access.
