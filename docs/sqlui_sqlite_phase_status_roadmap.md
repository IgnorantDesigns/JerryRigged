# SQLUI SQLite Phase Status And Roadmap

This document is the concise phase-level status for SQLUI's SQLite durable backend work. It summarizes what is implemented, what has local validation, which boundaries remain explicit, and which follow-up slices should come next.

For deeper reference, see:

- [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md) for the full current runtime status.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) for repository boundaries and backend selection.
- [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md) for schema details.
- [`sqlui_sqlite_async_backend_plan.md`](sqlui_sqlite_async_backend_plan.md) for async and shutdown policy.
- [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) for the future user-facing persistence/settings UX policy.
- [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md) for the planned settings editing, apply/cancel, backend selection, SQLite path, provider auto-init, and reset/delete UX phase.
- [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md) for the focused read-only UMG binding recipe.
- [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) for the validation/preview-only draft settings UMG binding recipe.
- [`sqlui_smoke_test.md`](sqlui_smoke_test.md) for local editor smoke commands.
- [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md) for packaged build and packaged runtime smoke validation.

## Current Status

The SQLUI SQLite phase has moved past proof-only work into an explicit, opt-in repository backend:

- `USQLUISQLiteLayoutRepository` exists in SQLUICore.
- SQLite supports `SaveLayout`, `LoadLayout`, `LoadLayoutById`, `ListLayouts`, soft-delete `RemoveLayout`, and destructive scoped `ClearLayouts`.
- SQLite is selectable through `USQLUILayoutRepositoryFactory` only when `ESQLUILayoutRepositoryBackend::SQLite` is explicitly requested and a database path is configured.
- SQLite remains non-default. The runtime config resolver defaults to `InMemory`.
- A SQLUICore runtime integration helper can combine explicit runtime config, optional seed-copy policy, and repository factory creation without changing normal startup.
- A SQLUICore runtime repository provider can hold the explicitly initialized active repository and last integration result without changing normal startup.
- Config-backed SQLUICore runtime repository settings can request provider auto-init when explicitly configured, while default settings remain passive and `InMemory`.
- A passive SQLUICore runtime repository `GameInstance` subsystem can own/access the provider and auto-initializes only when config-backed settings or `-SQLUILayoutRepositoryProviderAutoInit` explicitly request it.
- A SQLUICore runtime layout persistence workflow helper can save, list, and load through the subsystem's active repository without knowing SQLite details.
- A SQLUICore runtime database management policy helper can inspect and reset the explicitly configured SQLite database path plus sidecars without opening SQLite, creating schema, selecting repositories, or changing startup.
- A docs-only persistence settings UX design now defines the intended user-facing storage modes, safe defaults, database path policy, status/reset behavior, migration/seed/error UX, and recommended first UI slice.
- PR #105 adds a docs-only persistence settings editing/reset plan that defines the next mutating UX phase boundaries: pending/apply/cancel semantics, backend selection, SQLite path editing, provider auto-init policy, reset/delete UX, and validation sequencing. It does not implement mutating settings behavior.
- A SQLUICore read-only persistence status snapshot surface now exposes backend/provider/repository/path/file/sidecar/schema status for future UI without initializing providers or mutating files.
- A SQLUICore read-only persistence status display-row adapter now converts that snapshot into label/value/state/detail rows for future UI binding without adding a widget or settings editing.
- An optional SQLUISamples sample/dev presenter now consumes those display rows, exposes stable formatted lines, and supports explicit caller-invoked refresh without wiring into startup or adding settings editing/reset behavior.
- An optional SQLUISamples read-only persistence status panel adapter now delegates to the existing presenter/display-row path and stores latest rows/results for future Blueprint/UMG binding without owning persistence internals.
- An optional SQLUISamples C++ UMG widget shell now delegates to the panel adapter and exposes cached rows/results for future Blueprint subclassing or binding without adding widget blueprint assets, maps, viewport attachment, startup wiring, or settings controls.
- A docs-only read-only persistence status panel contract records how future Blueprint/UMG UI should consume the adapter/presenter/display-row path safely.
- A focused read-only UMG usage guide now records the safe future widget blueprint subclass/binding recipe, refresh boundaries, display semantics, and manual local checklist.
- A non-mutating SQLUICore persistence settings draft model now represents current/pending backend/path/provider-auto-init choices and validates them without applying settings, creating DB files, running migrations, copying seeds, deleting files, or initializing providers/repositories.
- A non-mutating SQLUICore dry-run apply-intent preview now reports what a future Apply would do for a validated settings draft without applying settings, saving config, creating directories or DB files, opening DBs for writing, running migrations, copying seeds, deleting files, or initializing providers/repositories.
- Optional SQLUISamples persistence settings draft presenter and C++ UMG widget shell surfaces now consume the SQLUICore validation display rows for sample/dev-facing display and future Blueprint binding without adding settings controls, apply/save behavior, DB creation, directory creation from display generation, migrations, seed copy, provider/repository initialization, widget blueprint assets, maps, startup wiring, viewport attachment, or destructive actions.
- A focused validation/preview-only draft UMG usage guide now records the safe future widget blueprint subclass/binding recipe, display semantics, preview semantics, refresh/build boundaries, and manual local checklist for that draft widget shell.
- The draft validation sequence is: #106 validation-only draft model, #107 validation display rows/summary, #108 SQLUISamples draft presenter/adapter, #109 optional C++ draft validation UMG shell, and #110 docs-only safe usage/binding guide.
- Schema initialization and database creation are repository-owned and opt-in.
- The current known production migration set is only `001_initial_layout_schema`.
- `LoadLayout` and `SaveLayout` callback APIs can opt into serialized async execution with shutdown/stale-callback suppression.
- `ListLayouts`, `LoadLayoutById`, `RemoveLayout`, and `ClearLayouts` remain synchronous.
- Local Win64 Development packaged BuildCookRun validation has passed.
- An explicit packaged runtime SQLite lifecycle smoke exists and runs only with `-RunPackagedSQLiteSmoke` / `-SQLUIPackagedRuntimeSQLiteSmoke`.
- An explicit packaged runtime provider startup smoke exists and runs only with `-RunPackagedProviderStartupSmoke` / `-SQLUIRuntimeProviderStartupSmoke`.
- An explicit packaged runtime provider subsystem smoke exists and runs only with `-RunPackagedProviderSubsystemSmoke` / `-SQLUIRuntimeProviderSubsystemSmoke`.
- An explicit packaged runtime persistence workflow smoke exists and runs only with `-RunPackagedPersistenceWorkflowSmoke` / `-SQLUIRuntimePersistenceWorkflowSmoke`; it saves in one packaged launch, verifies the persisted layout in a second launch, and cleans up in a third launch.

This is still not a default production persistence policy. Implementing the user-facing settings UI, choosing product startup policy, broader platform validation, production async service hardening, and actual future schema upgrades remain future work.

## Read-Only Persistence Status UMG Foundation Checkpoint

The read-only SQLUI persistence status UMG foundation is complete as a documented and smoke-validated base for future settings UI work.

This #104 checkpoint records the completed read-only foundation sequence:

- #94: read-only persistence status snapshot/surface.
- #95: UI-safe display rows/view model.
- #96: optional SQLUISamples presenter/sample surface.
- #97: explicit caller-invoked refresh path.
- #98: documented and validated Blueprint-facing presenter hook.
- #99: read-only panel contract and Blueprint usage recipe.
- #100: optional SQLUISamples panel adapter.
- #101: optional C++ UMG widget shell.
- #102: read-only UMG usage and binding guide.
- #103: non-asset smoke hardening for presenter, Blueprint hook, panel adapter, and UMG shell contract.
- #104: final read-only foundation checkpoint.

That foundation now includes:

- A read-only SQLUICore persistence status snapshot/surface.
- UI-safe SQLUICore display rows and view-model formatting.
- An optional SQLUISamples sample/dev presenter.
- An explicit caller-invoked refresh path.
- A Blueprint-facing presenter hook that is documented and smoke-validated.
- A read-only panel contract and usage recipe.
- An optional SQLUISamples panel adapter.
- An optional SQLUISamples C++ UMG widget shell.
- A focused UMG usage and binding guide.
- Non-asset smoke coverage for the presenter, Blueprint hook, panel adapter, and C++ UMG widget shell contract.

This checkpoint is still not a settings screen. It adds no widget blueprint asset, map, default startup wiring, viewport attachment, polling, ticking, auto-refresh, settings editing, backend selector, SQLite path editor, provider auto-init control, reset/delete behavior, migration control, seed-copy behavior, or provider/repository lifecycle behavior.

The safety boundaries remain unchanged:

- SQLite is still opt-in and non-default.
- `InMemory` remains the safe default backend.
- Provider auto-init remains off by default.
- No default config creates SQLite database files.
- No startup/default behavior changed.
- SQLUICore remains free of UMG, Slate, SlateCore, editor-only, and widget dependencies.
- SQLUISamples contains the optional UMG shell only as sample/dev-facing infrastructure.
- No widget blueprint assets or maps are committed.
- No startup wiring, viewport attachment, timers, tick, polling, or auto-refresh exist.
- Refresh remains caller-invoked only.
- Status/display/refresh/widget-shell paths remain read-only.

Future settings-editing or reset work should build on this foundation and the dedicated plan in [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md). That work must keep widgets ignorant of SQL, schema, migrations, seed-copy policy, SQLite sidecar internals, direct file deletion, and concrete repository lifecycle details. Writable runtime database state should remain under `Saved/SQLUI/...`. User-facing reset/delete behavior should route through SQLUICore database management helper/policy surfaces, not widget-owned deletion. Editable settings should use pending/apply semantics instead of directly mutating live persistence from widgets. Any future PR that adds settings editing, reset/delete behavior, startup/default map/config wiring, or packaged runtime lifecycle changes should include matching smoke coverage and packaged validation where appropriate.

## Persistence Settings Draft Validation UI Foundation Checkpoint

The non-mutating SQLUI persistence settings draft validation UI foundation is complete as a documented and smoke-validated base for future apply/cancel work.

This checkpoint records the completed validation-only sequence:

- #105: docs-only settings editing/reset UX plan.
- #106: `USQLUIPersistenceSettingsDraftLibrary` and `FSQLUIPersistenceSettingsDraft` for validation-only pending settings.
- #107: `USQLUIPersistenceSettingsDraftDisplayLibrary` and UI-safe validation display rows/summary.
- #108: optional SQLUISamples sample/dev-facing draft validation presenter/adapter.
- #109: optional SQLUISamples C++ UMG widget shell for draft validation display.
- #110: safe UMG subclassing and binding guide.
- #111: final non-mutating draft validation foundation checkpoint.
- Dry-run SQLUICore apply-intent preview.
- `-UsePersistenceSettingsDraftProbe`: smoke coverage for the draft model, apply preview, validation display rows, sample adapter, and C++ UMG widget shell contract.

This checkpoint is still not settings editing. It adds no actual apply/save/config-write behavior, backend selector UI, SQLite path editor UI, provider auto-init control, reset/delete UX, widget blueprint assets, maps, startup wiring, viewport attachment, timers, tick, polling, auto-refresh, provider/repository initialization, migrations, seed-copy behavior, default config changes, or default startup behavior changes. Refresh/build/validation/preview remains caller-invoked only.

The dry-run apply-intent preview reports whether a future Apply would have changes, whether backend/SQLite/provider-auto-init policy would change, and whether restart/reopen/reinitialize messaging should be shown. It uses future-oriented "would change" and "not applied" semantics and does not write config or mutate runtime persistence.

The safety boundaries remain unchanged:

- `InMemory` remains the safe default backend.
- SQLite remains opt-in and is not enabled by default.
- Provider auto-init remains off by default.
- No default config creates SQLite database files.
- SQLUICore remains free of UMG, Slate, SlateCore, editor-only, and widget dependencies.
- SQLUISamples is sample/dev-facing consumption only and does not own persistence policy.
- Draft validation, validation display generation, SQLUISamples draft adapter refresh, and the draft validation widget shell remain non-mutating: they do not create databases or directories, open databases for writing, run migrations, copy seeds, initialize providers/repositories, or delete files outside smoke-owned cleanup.

The next apply/cancel phase must build on SQLUICore helper/policy surfaces. Future work should keep widgets ignorant of SQL, schema, migrations, seed-copy policy, sidecar internals, deletion behavior, and provider/repository lifecycle. Widgets should not write config directly, initialize providers/repositories, or delete files directly. Apply/cancel work should use explicit pending/apply/cancel semantics, keep validation failures user-readable and non-destructive, avoid silently initializing providers/repositories during apply, and show restart/reopen/reinitialize-required messaging instead of forcing lifecycle changes from widget code. Reset/delete behavior must route through SQLUICore database management helper/policy surfaces. Any new apply/cancel path needs focused smoke coverage, and any startup, default map, config, viewport, or packaged lifecycle change needs packaged validation.

## Implemented Capabilities

| Capability | Current status | Notes |
| --- | --- | --- |
| SQLiteCore availability proof | Implemented | SQLUICore compiles/links against engine `SQLiteCore`. |
| SQLite open/close probe | Implemented | Smoke-safe path under `Saved/SQLUI/SmokeTests/SQLiteCoreProbe`. |
| Initial schema migration | Implemented | `001_initial_layout_schema` creates planned layout tables/indexes. |
| Schema init and hardening | Implemented | Opt-in repository-owned initialization with edge-case smoke coverage. |
| Read/list/load mapping | Implemented | Metadata, tags, current revision JSON, deserialization, and validation. |
| Save/update revisions | Implemented | `SaveLayout` creates immutable revisions and updates current metadata/tags. |
| Remove soft-delete | Implemented | `RemoveLayout` sets `layouts.b_deleted = 1` and preserves revisions. |
| Clear destructive scoped cleanup | Implemented | Deletes layouts plus dependent revision/tag/checkpoint/preview rows for the configured DB scope. |
| Full lifecycle smoke | Implemented | Exercises save/list/load/update/remove/clear in one workflow. |
| Read-only rejection | Implemented | Save/remove/clear reject clearly when `bReadOnly = true`. |
| Async callback SaveLayout/LoadLayout | Implemented | Opt-in only through SQLite settings. |
| Serialized async callback queue | Implemented | Per-repository FIFO queue for callback-style SQLite save/load operations. |
| Queue shutdown/stale callback suppression | Implemented | Rejects new work after shutdown, drops pending work, suppresses stale completion callbacks. |
| Factory selection | Implemented | Explicit SQLite backend selection; SQLite is not default. |
| Runtime config resolver | Implemented | Parses explicit backend/path/schema/async/seed flags; defaults to `InMemory`. |
| Runtime integration helper | Implemented | Combines explicit runtime config, optional seed-copy policy, and factory repository creation. |
| Runtime repository provider | Implemented | Storage-agnostic UObject holder for an explicitly initialized active repository and last integration result. |
| Runtime settings policy | Implemented | `Config=Game` UObject settings default to no auto-init/`InMemory`; command-line overrides can be allowed or disabled. |
| Runtime repository subsystem | Implemented | Passive `GameInstance` subsystem can own/access the provider; startup auto-init requires explicit settings or `-SQLUILayoutRepositoryProviderAutoInit`. |
| Runtime layout persistence workflow | Implemented | Storage-agnostic save/list/load helper uses the subsystem's active repository and keeps SQLite details out of callers. |
| Runtime database management policy | Implemented | Storage-agnostic status/reset helper inspects or removes only the resolved SQLite DB and sidecars. |
| Persistence settings UX design | Documented | Future UI policy covers storage modes, defaults, DB path/status/reset, seed, migration, error, and startup behavior. |
| Persistence settings editing/reset plan | Documented | Future mutating UX policy covers pending/apply/cancel, backend selection, SQLite path editing, provider auto-init, reset/delete safety, validation, and sequencing. |
| Persistence settings draft validation | Implemented | SQLUICore draft/pending model validates future backend/path/provider-auto-init edits without apply/save/config writes, DB creation, migrations, seed copy, provider init, repository init, or UI controls. |
| Persistence settings apply-intent preview | Implemented | SQLUICore dry-run result reports whether a future Apply would have changes and require restart/reopen/reinitialize messaging; it does not apply/save settings, write config, create directories/DBs, open DBs for writing, run migrations, copy seeds, initialize providers/repositories, delete files, or add UI controls. |
| Persistence settings draft validation display rows | Implemented | SQLUICore formats validation-only draft results into UI-safe summary/row data without apply/save/config writes, lifecycle behavior, DB creation, migrations, seed copy, provider/repository init, destructive actions, or UI controls. |
| Persistence settings draft validation sample adapter | Implemented | SQLUISamples consumes SQLUICore draft validation display rows through a sample/dev presenter with cached rows/summary strings, without settings controls, apply/save/config writes, DB creation, migrations, seed copy, provider/repository init, destructive actions, widget assets, maps, or startup wiring. |
| Persistence settings draft validation UMG shell | Implemented | Optional SQLUISamples C++ `UUserWidget` shell delegates to the draft presenter and exposes cached validation rows/result/summary/flags for future Blueprint binding; no widget blueprint asset, map, viewport attachment, startup wiring, settings controls, apply/save behavior, reset/delete actions, DB creation, migrations, seed copy, provider/repository init, or destructive actions. |
| Persistence settings draft validation UMG usage guide | Documented | Focused binding recipe and local/manual checklist for future widget blueprint subclasses; no asset, map, startup wiring, polling, editing controls, apply/save behavior, reset/delete actions, or runtime behavior. |
| Read-only persistence status surface | Implemented | Blueprint-callable SQLUICore snapshot exposes backend/provider/repository/SQLite file status without settings edits or destructive actions. |
| Read-only persistence status display rows | Implemented | Blueprint-callable SQLUICore adapter formats the status snapshot into UI-friendly rows without probing files directly or mutating state. |
| Persistence status sample surface / Blueprint hook | Implemented | Optional SQLUISamples presenter already provides Blueprint-callable display-row refresh/formatted lines without startup wiring, settings editing, or destructive actions. |
| Persistence status panel adapter | Implemented | Optional SQLUISamples UObject delegates to the presenter and stores latest rows/result for future Blueprint/UMG binding; no widget asset, startup wiring, editing controls, reset/delete actions, or runtime behavior. |
| Persistence status UMG widget shell | Implemented | Optional SQLUISamples C++ UUserWidget shell delegates to the panel adapter and exposes cached rows/result; no widget blueprint asset, map, viewport attachment, startup wiring, editing controls, reset/delete actions, or runtime behavior. |
| Persistence status panel contract | Documented | Blueprint/UMG usage recipe for the read-only widget-shell/adapter/presenter path; no startup wiring, editing controls, reset/delete actions, or runtime behavior. |
| Persistence status UMG usage guide | Documented | Focused binding recipe and local/manual checklist for future widget blueprint subclasses; no asset, map, startup wiring, polling, editing controls, reset/delete actions, or runtime behavior. |
| Seed database copy policy | Implemented | Explicit pre-repository closed-file copy helper; not factory-owned. |
| Migration version/status framework | Implemented | Reports known/applied/pending status for current known migration set. |
| Packaged build validation | Implemented locally | Local Win64 Development BuildCookRun validation passed with UE 5.7 preferred MSVC toolchain. |
| Packaged runtime SQLite smoke | Implemented locally | Optional packaged executable lifecycle smoke passes when explicitly requested. |
| Packaged runtime provider startup smoke | Implemented locally | Optional packaged executable startup proof initializes `USQLUILayoutRepositoryRuntimeProvider` from command-line repository settings. |

## Validation Matrix

| Area | Validation path / script flag | Current status | Notes |
| --- | --- | --- | --- |
| Default editor smoke | `RunSQLUISmokeTest.ps1` | Covered | Uses in-memory C++ document, not SQLite. |
| In-memory repository | `-UseInMemoryLayoutRepository` | Covered | Proves non-SQLite repository behavior remains available. |
| JSON-file repository | `-UseJsonFileLayoutRepository` | Covered | Proves file repository remains available. |
| SQLiteCore open/close | `-UseSQLiteCoreProbe` | Covered | Creates and removes a temporary probe DB. |
| Generic async boundary | `-UseDatabaseAsyncProbe` | Covered | No SQLite file I/O. |
| Async queue shutdown | `-UseDatabaseAsyncQueueShutdownProbe` | Covered | Proves reject/drop/stale-callback suppression behavior. |
| Runtime config resolver | `-UseLayoutRepositoryRuntimeConfigProbe` | Covered | Verifies explicit config parsing and default `InMemory` policy. |
| Runtime integration helper | `-UseLayoutRepositoryRuntimeIntegrationProbe` | Covered | Verifies default in-memory creation, explicit SQLite creation, seed-copy integration, missing-path unavailable behavior, and cleanup. |
| Runtime repository provider | `-UseLayoutRepositoryRuntimeProviderProbe` | Covered | Verifies provider initialization, reset/reinit, explicit SQLite save/list/load, command-line config, seed-copy integration, fatal missing-seed behavior, and cleanup. |
| Runtime settings policy | `-UseLayoutRepositoryRuntimeSettingsProbe` | Covered | Verifies safe defaults, settings-driven `InMemory`, settings-driven SQLite, command-line override behavior, disabled overrides, missing-path unavailable behavior, and cleanup. |
| Runtime layout persistence workflow | `-UseLayoutPersistenceWorkflowProbe` | Covered | Verifies null/missing repository failure, in-memory save/list/load, explicit SQLite save/list/load, unavailable SQLite behavior, and cleanup. |
| Runtime database management policy | `-UseLayoutRepositoryDatabaseManagementProbe` | Covered | Verifies non-SQLite no-op status/reset, SQLite empty-path behavior, status before/after save, reset/idempotent reset, sidecar removal, relative path resolution, and cleanup. |
| Persistence status surface | `-UsePersistenceStatusSurfaceProbe` | Covered | Verifies default `InMemory` status, inactive provider/repository state without forced init, read-only SQLite DB/schema status, sidecar detection, and cleanup. |
| Persistence status display rows | `-UsePersistenceStatusDisplayRowsProbe` | Covered | Verifies UI-friendly rows for default and SQLite snapshots, sidecar reporting, read-only formatting, and cleanup. |
| Persistence status sample surface / Blueprint hook | `-UsePersistenceStatusSampleSurfaceProbe` | Covered | Documents and validates the optional SQLUISamples presenter, panel adapter, and C++ UMG widget shell binding contract, including non-pure refresh hooks, pure cached widget getters, Blueprint-visible cached rows/formatted-lines/result/summary, display-row consumption, explicit deterministic refresh, graceful missing SQLite DB state, sidecar preservation during refresh, no asset/map/viewport requirement, and cleanup. |
| Persistence settings draft validation/display rows, apply preview, sample adapter, and UMG shell | `-UsePersistenceSettingsDraftProbe` | Covered | Verifies default/current draft validation, display rows, dry-run apply-preview no-change behavior, sample adapter consumption, C++ UMG widget shell reflection contract, unknown backend rejection/error display/blocked preview, SQLite draft/path display and preview without DB creation, empty SQLite path rejection/error display/blocked preview, provider auto-init pending validation/display/preview without policy mutation, deterministic validation/display/preview/adapter output, sidecar preservation, and cleanup. |
| SQLite migration runner | `-UseSQLiteMigrationProbe` | Covered | Smoke-only migration runner proof. |
| Layout schema migration | `-UseSQLiteLayoutSchemaMigrationProbe` | Covered | Applies and verifies `001_initial_layout_schema`. |
| SQLite layout read probe | `-UseSQLiteLayoutReadProbe` | Covered | Seeds one layout and verifies list/load mapping. |
| Read-only SQLite repository | `-UseSQLiteReadOnlyLayoutRepository` | Covered | Verifies read behavior and write rejection. |
| SQLite SaveLayout | `-UseSQLiteSaveLayoutRepository` | Covered | Verifies saves, revision update, list, and load. |
| SQLite RemoveLayout | `-UseSQLiteRemoveLayoutRepository` | Covered | Verifies soft-delete and revision preservation. |
| SQLite ClearLayouts | `-UseSQLiteClearLayoutsRepository` | Covered | Verifies active and soft-deleted rows are cleared with dependents. |
| SQLite full lifecycle | `-UseSQLiteFullLifecycleRepository` | Covered | Combined repository lifecycle smoke. |
| Async callback repository | `-UseSQLiteAsyncCallbackRepository` | Covered | Proves opt-in async callback save/load. |
| Serialized async callbacks | `-UseSQLiteSerializedAsyncCallbackRepository` | Covered | Proves callback order and latest revision readback. |
| Factory-created SQLite repository | `-UseSQLiteFactoryLayoutRepository` | Covered | Proves explicit factory selection and missing-path unavailable behavior. |
| Factory schema initialization | `-UseSQLiteFactorySchemaInitRepository` | Covered | Proves opt-in DB creation/schema initialization via repository settings. |
| Schema-init hardening | `-UseSQLiteSchemaInitHardening` | Covered | Missing DB, empty DB, initialized DB, missing row, partial schema, read-only protection. |
| Seed-copy policy | `-UseSQLiteSeedDatabaseCopyPolicyProbe` | Covered | Verifies copy-if-missing, preserve, overwrite, missing seed, same-path failure. |
| Migration versioning policy | `-UseSQLiteMigrationVersioningPolicyProbe` | Covered | Verifies current status, missing-row repair, partial failure, pending/failing smoke migrations. |
| Packaged build validation | `RunSQLUIPackagedBuildValidation.ps1 -CleanPackageOutput` | Local Win64 Development covered | Passed after installing UE 5.7 preferred MSVC `14.44.x`. |
| Packaged runtime SQLite smoke | `RunSQLUIPackagedBuildValidation.ps1 -CleanPackageOutput -RunPackagedSQLiteSmoke` | Local Win64 Development covered | Launches packaged executable with `-SQLUIPackagedRuntimeSQLiteSmoke`. |
| Packaged runtime provider startup smoke | `RunSQLUIPackagedBuildValidation.ps1 -CleanPackageOutput -RunPackagedProviderStartupSmoke` | Local Win64 Development covered | Launches packaged executable with `-SQLUIRuntimeProviderStartupSmoke` and explicit SQLite repository command-line settings. |
| Packaged runtime provider subsystem smoke | `RunSQLUIPackagedBuildValidation.ps1 -CleanPackageOutput -RunPackagedProviderSubsystemSmoke` | Local Win64 Development covered | Launches packaged executable with `-SQLUIRuntimeProviderSubsystemSmoke`, `-SQLUILayoutRepositoryProviderAutoInit`, and explicit SQLite repository command-line settings. |
| Packaged runtime persistence workflow smoke | `RunSQLUIPackagedBuildValidation.ps1 -CleanPackageOutput -RunPackagedPersistenceWorkflowSmoke` | Local Win64 Development covered | Launches Save, Verify, and Cleanup packaged processes with `-SQLUIRuntimePersistenceWorkflowSmoke` and proves one workflow-saved layout persists across separate runs. |

## Runtime And Packaging Status

The local packaged validation path now proves that this checkout can build, cook, stage, package, and archive with SQLUI and engine `SQLiteCore` wiring enabled for the documented local Win64 Development scenario.

The packaged runtime SQLite smoke goes one step further: it launches the packaged executable with an explicit smoke flag, creates a factory-configured SQLite repository, runs save/list/load/remove/clear under the packaged runtime saved directory, verifies the success log, and removes the smoke database.

The packaged runtime persistence workflow smoke proves a narrower but important runtime behavior: `USQLUILayoutRepositoryRuntimeSubsystem` plus `FSQLUILayoutPersistenceWorkflow` can save one layout in a packaged process, a later packaged process can list/load the same persisted layout without saving first, and a final packaged process can remove the database and sidecars.

This does not yet prove:

- Target-platform coverage beyond the local validation target.
- Long-running packaged gameplay persistence behavior beyond the explicit three-launch smoke.
- Production async service lifecycle, cancellation, or shutdown draining.
- Implemented user-facing settings UI and product startup policy beyond the docs-only design.
- Future migration upgrade transforms beyond `001_initial_layout_schema`.

## Safety Boundaries

The current SQLite path keeps these boundaries:

- SQLite is not the default backend.
- Factory selection is explicit through `ESQLUILayoutRepositoryBackend::SQLite`.
- `FSQLUILayoutRepositoryRuntimeConfigResolver` defaults to `InMemory`.
- `USQLUILayoutRepositoryRuntimeSettings` defaults to no provider auto-init, `InMemory`, empty SQLite paths, schema/create disabled, async callbacks disabled, and seed-copy flags disabled.
- `FSQLUILayoutRepositoryRuntimeIntegration` runs only when explicitly invoked by caller code or smoke tests.
- `USQLUILayoutRepositoryRuntimeProvider` initializes only when caller code or smoke tests explicitly invoke it.
- `USQLUILayoutRepositoryRuntimeSubsystem` is passive by default; startup auto-init requires explicit runtime settings or `-SQLUILayoutRepositoryProviderAutoInit`.
- `FSQLUILayoutPersistenceWorkflow` uses only the subsystem's active repository; it does not initialize providers, create repositories, know SQLite paths, run migrations, copy seed databases, attach widgets, or change startup behavior.
- `FSQLUILayoutRepositoryDatabaseManagement` only resolves configured SQLite paths, reports file status, and removes the resolved database plus `.db-journal`, `.db-wal`, and `.db-shm` sidecars when explicitly asked.
- Database management reset is idempotent and does not open SQLite, run migrations, create directories, create databases, seed data, select repositories, or remove arbitrary directories.
- `USQLUIPersistenceSettingsDraftLibrary` validates pending settings only, and `USQLUIPersistenceSettingsDraftDisplayLibrary` formats those validation results for UI consumption only. `USQLUISamplePersistenceSettingsDraftPresenter` and `USQLUISamplePersistenceSettingsDraftPanelWidget` consume those rows for optional sample/dev display and binding contracts only. They do not write config, apply changes, initialize providers/repositories, create SQLite database files, create directories from display generation, run migrations, copy seeds, delete files, add UI controls, add widget blueprint assets, attach to the viewport, or change startup behavior.
- The factory passes settings only.
- The factory does not run migrations.
- The factory does not copy seed databases.
- The factory does not create database files directly.
- Schema initialization is repository-owned and opt-in.
- A missing database can be created only when `bInitializeSchemaIfMissing = true`, `bCreateDatabaseIfMissing = true`, and `bReadOnly = false`.
- Read-only mode blocks schema-initialization writes.
- Seed copying is explicit and outside the factory.
- No source-controlled database files are required.
- Widgets should not know SQL, table names, SQLite connections, DB file paths, worker queues, or concrete storage details.
- Widgets and sample presenters should consume SQLUICore status/display rows instead of owning persistence probing or deletion behavior.
- JerryRigged remains a thin host.
- Normal startup does not use SQLite unless future runtime code explicitly opts in.
- Packaged runtime smoke runs only with the explicit `-SQLUIPackagedRuntimeSQLiteSmoke` command-line flag.
- Packaged runtime provider startup smoke runs only with the explicit `-SQLUIRuntimeProviderStartupSmoke` command-line flag.
- Packaged runtime provider subsystem smoke runs only with the explicit `-SQLUIRuntimeProviderSubsystemSmoke` command-line flag.
- Packaged runtime persistence workflow smoke runs only with the explicit `-SQLUIRuntimePersistenceWorkflowSmoke` command-line flag and a `Save`, `Verify`, or `Cleanup` phase argument.
- Editor smoke DB writes stay under `Saved/SQLUI/SmokeTests/...` and remove DB files afterward.
- Packaged runtime smoke DB writes stay under packaged runtime `Saved/SQLUI/PackagedRuntimeSmoke/...` and remove DB files afterward.
- Packaged runtime provider startup/subsystem smoke DB writes resolve under packaged runtime `Saved/SQLUI/LayoutRepositories/PackagedRuntimeSmoke/...` and remove DB files afterward.
- Packaged runtime persistence workflow smoke DB writes resolve under packaged runtime `Saved/SQLUI/LayoutRepositories/PackagedRuntimeSmoke/PersistenceWorkflow`, intentionally remain on disk between Save and Verify, and are removed by the Cleanup phase.

## What Is Still Explicit / Non-Default

SQLite requires a deliberate selection path:

1. Choose SQLite through factory settings, runtime config, or direct repository construction in tests/smoke paths.
2. Provide a database path.
3. Enable writable behavior with `bReadOnly = false` when writes are intended.
4. Enable schema initialization only when the caller wants the repository to prepare a missing/empty DB.
5. Enable async callback execution only when the caller wants callback-style `SaveLayout`/`LoadLayout` to run through the serialized queue.
6. Run seed-copy preparation explicitly before repository mutation if a seed DB is used.

The safe default remains non-SQLite.

## Remaining Work

Prioritized remaining work:

1. Implement the production/user-facing runtime settings UI described in [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md), [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md), [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md), and [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md), following the read-only panel contract and validation/preview-only draft binding recipe while keeping SQLite opt-in and `InMemory` as the safe default. The current draft model, apply preview, and display rows are non-mutating; actual apply/cancel behavior, actual UI controls, and reset/delete UX remain future work.
2. Product startup policy that intentionally configures the passive runtime provider subsystem outside packaged smoke flags.
3. Actual future schema migrations and data transforms beyond `001_initial_layout_schema`.
4. Production async database service design beyond the current per-repository callback queue.
5. Cancellation and shutdown draining beyond stale-callback suppression.
6. Async APIs or async wrappers for `ListLayouts`, `LoadLayoutById`, `RemoveLayout`, and `ClearLayouts` if runtime workflows require them.
7. Seed database asset packaging, versioning, and upgrade policy if seed DBs are shipped.
8. Broader packaged target-platform validation and persistence-across-launches scenarios beyond local Win64 Development.
9. CI only if Unreal-capable build agents become available.
10. Optional lifecycle features: history APIs, checkpoints, previews, restore flows, richer search, and retention policy.

## Recommended Next Slices

Suggested next PRs, in priority order:

1. Add the next settings draft helper slice: explicit Apply/Cancel request/result shapes that still avoid widget controls until the lifecycle policy is reviewed.
   Build on the completed validation-only draft model, dry-run apply-intent preview, and display rows, keep SQLite opt-in, keep `InMemory` as the safe default, and do not write config or reinitialize providers until an explicitly scoped apply PR does so.
2. First actual persistence settings widget/panel slice from [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) and [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md).
   Build on the completed read-only foundation and validation/preview-only draft/display model, bind to the read-only status display rows or optional SQLUISamples widget-shell/adapter/presenter shape, keep refresh caller-invoked, and add mutating settings/reset behavior only after pending/apply semantics, confirmation, provider-shutdown policy, and SQLUICore-mediated reset/delete boundaries are implemented and smoke-tested.
3. Production async service design doc or small scaffold.
   Decide whether the current per-repository callback queue is enough or whether SQLUI needs a longer-lived DB service for production runtime use.
4. SQLite history/checkpoint/previews API planning.
   Keep this docs/design first, then add repository contracts only when product workflows need them.
5. Broader packaged validation target matrix docs.
   Record intended platforms and local validation commands without adding CI yet.
6. First real v2 migration.
   Add this only when there is an actual schema change and a matching data-transform/compatibility policy.
