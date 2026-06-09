# SQLUI SQLite Runtime Status

This is the current-state reference for SQLUI's SQLite layout repository work. It consolidates the implementation status that is spread across the repository architecture, schema, async plan, backend evaluation, and smoke-test docs.

Related docs:

- [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md) summarizes the current phase status, validation matrix, safety boundaries, and recommended next slices.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) describes the repository boundary and available backends.
- [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md) describes the SQLite schema and repository-operation mapping.
- [`sqlui_sqlite_async_backend_plan.md`](sqlui_sqlite_async_backend_plan.md) describes the async/threading direction.
- [`sqlui_sqlite_backend_evaluation.md`](sqlui_sqlite_backend_evaluation.md) describes why engine `SQLiteCore` is the active backend candidate.
- [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) describes the future user-facing persistence/settings UX policy.
- [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md) plans the next mutating settings editing, apply/cancel, backend selection, SQLite path, provider auto-init, and reset/delete UX phase.
- [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md) documents the focused read-only UMG binding recipe for the optional widget shell.
- [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) documents the validation-only draft settings UMG binding recipe for the optional widget shell.
- [`sqlui_smoke_test.md`](sqlui_smoke_test.md) is the command reference for local smoke paths.
- [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md) describes the local packaged-build validation scaffold.

## Current SQLite Repository Capabilities

`USQLUISQLiteLayoutRepository` exists in SQLUICore. It uses engine `SQLiteCore` behind SQLUI-owned code and keeps SQLite details out of widgets and the JerryRigged host.

The repository currently supports:

- `SaveLayout` when configured writable.
- `LoadLayout` and `LoadLayoutById`.
- `ListLayouts`.
- `RemoveLayout` as a soft delete using `layouts.b_deleted = 1`.
- `ClearLayouts` as destructive scoped cleanup for the configured database.

SQLite database work is delegated to the non-UObject `FSQLUISQLiteLayoutRepositoryWorker`. The worker accepts plain settings/request data and returns SQLUI repository result structs, which makes the database operation code suitable for worker execution without touching UObjects.

`SaveLayout` validates the layout document, serializes canonical JSON, writes metadata, inserts an immutable revision row, replaces tags, updates the current revision, and commits a transaction. `ListLayouts` reads active layout metadata and tags. `LoadLayout` reads the current revision JSON and validates the loaded document. `RemoveLayout` preserves revisions and tags. `ClearLayouts` removes layouts plus dependent revision, tag, checkpoint, and preview rows for the selected database scope.

`FSQLUILayoutPersistenceWorkflow` provides a storage-agnostic app-facing save/list/load helper above `USQLUILayoutRepositoryRuntimeSubsystem`. It uses only the subsystem's active `USQLUILayoutRepository` and does not cast to SQLite, know database paths, run migrations, copy seed databases, initialize providers, create repositories, attach widgets, touch the viewport, or change normal startup. This gives runtime code a small persistence workflow surface while preserving the repository and subsystem boundaries.

## Repository Selection

SQLite is not the default backend.

SQLite is factory-selectable only when explicitly requested with `ESQLUILayoutRepositoryBackend::SQLite`. `USQLUILayoutRepositoryFactory` creates and configures `USQLUISQLiteLayoutRepository` only when SQLite is selected and a non-empty SQLite database path is provided.

The factory passes settings only. It does not:

- Run migrations.
- Create database files.
- Create directories.
- Seed data.
- Silently fall back to JSON or in-memory when SQLite was requested.

If SQLite is requested without a required database path, the factory returns the existing unavailable repository behavior.

## Runtime Configuration Policy

`FSQLUILayoutRepositoryRuntimeConfigResolver` is the first SQLUICore-owned storage selection policy layer above the repository factory. It maps explicit runtime config values or command-line options into `FSQLUILayoutRepositoryFactorySettings`; it does not create repositories, directories, schema, or database files.

The resolver default is `InMemory`. SQLite is never selected by default, and SQLite schema initialization, database creation, and async callback execution all remain disabled unless explicitly requested.

Supported command-line options include:

- `-SQLUILayoutRepositoryBackend=InMemory`
- `-SQLUILayoutRepositoryBackend=JsonFile`
- `-SQLUILayoutRepositoryBackend=SQLite`
- `-SQLUILayoutRepositoryBackend=Unavailable`
- `-SQLUILayoutRepositoryProviderAutoInit`
- `-SQLUIJsonFileLayoutRepositoryDir="<path>"`
- `-SQLUISQLiteLayoutRepositoryPath="<path>"`
- `-SQLUISQLiteLayoutRepositoryReadOnly`
- `-SQLUISQLiteLayoutRepositoryInitializeSchema`
- `-SQLUISQLiteLayoutRepositoryCreateDatabase`
- `-SQLUISQLiteLayoutRepositoryAsyncCallbacks`
- `-SQLUISQLiteLayoutRepositorySeedPath="<path>"`
- `-SQLUISQLiteLayoutRepositoryCopySeedIfMissing`
- `-SQLUISQLiteLayoutRepositoryOverwriteFromSeed`

When a SQLite database path is relative, the resolver normalizes it under:

```text
Saved/SQLUI/LayoutRepositories
```

Absolute SQLite paths are normalized and preserved. The helper also exposes the explicit `Default` token for callers that intentionally want the safe filename `LayoutRepository.sqlite` under the same `Saved/SQLUI/LayoutRepositories` scope. An empty SQLite path stays empty so explicit SQLite factory selection reports unavailable behavior instead of falling back to another backend.

The resolver can also map explicit seed-copy options into `FSQLUISQLiteSeedDatabaseCopyRequest`. That mapping is configuration-only; it does not copy files, create directories, create repositories, initialize schema, or make SQLite the default backend.

`FSQLUILayoutRepositoryRuntimeIntegration` is the next SQLUICore-owned policy layer above the resolver. It accepts explicit runtime config, runs the closed-file SQLite seed-copy helper only when seed-copy flags are requested, then creates the repository through `USQLUILayoutRepositoryFactory`. It preserves the default `InMemory` backend, reports unavailable behavior for explicit SQLite requests with no database path, and does not make normal startup use SQLite by itself.

`USQLUILayoutRepositoryRuntimeSettings` is the config-backed runtime settings object above the integration request. It is a `Config=Game` UObject, not a Project Settings UI surface. Its committed/default behavior is safe: provider auto-init is off, `RuntimeConfig.Backend` is `InMemory`, SQLite paths are empty, SQLite schema initialization/database creation/async callbacks are disabled, and SQLite seed-copy flags are disabled.

`FSQLUILayoutRepositoryRuntimeSettingsPolicy` converts settings plus command-line text into `FSQLUILayoutRepositoryRuntimeIntegrationRequest`. If `bAllowCommandLineOverrides = true`, command-line repository options override the settings runtime config through the existing resolver. If overrides are disabled, the settings runtime config is preserved even when the command line tries to select SQLite. Provider auto-init is requested when either `bAutoInitializeProvider = true` in settings or `-SQLUILayoutRepositoryProviderAutoInit` is present. When neither is true, the subsystem remains passive.

`USQLUILayoutRepositoryRuntimeProvider` is the first storage-agnostic UObject holder for runtime-selected repositories. It initializes through the runtime integration helper, stores the active repository plus the last integration result, supports explicit reset/reinitialization, and keeps callers on the repository interface instead of concrete storage classes. It does not auto-start, attach widgets, run migrations directly, or make SQLite the default backend.

`USQLUILayoutRepositoryRuntimeSubsystem` is the passive SQLUICore `UGameInstanceSubsystem` holder above the provider. Unreal may construct it with the GameInstance, but it does not initialize a repository, choose SQLite, create databases, copy seed files, attach widgets, or touch the viewport by default. It can be initialized by caller code through runtime config/command-line helpers, or it can opt into startup initialization only when config-backed settings or `-SQLUILayoutRepositoryProviderAutoInit` explicitly request it. That path still uses the settings/resolver default of `InMemory` unless explicit settings or command-line repository options choose another backend.

`FSQLUILayoutPersistenceWorkflow` sits above that subsystem as the first storage-agnostic runtime persistence workflow proof. It assumes the subsystem/provider was already configured by caller code or explicit settings. When a repository is active, it saves, lists, and loads through the repository surface. When the subsystem is null or no repository is active, it returns clear failures and creates no database files.

`USQLUIPersistenceStatusLibrary` is the first read-only SQLUICore status surface intended for future settings/admin UI. It returns `FSQLUIPersistenceStatusSnapshot` with configured backend, active backend, provider initialized state, repository active state, resolved SQLite path, database file status, sidecar status, and migration status when an existing SQLite database can be inspected. It does not initialize providers, create repositories, create directories, create databases, run migrations, copy seed databases, reset databases, delete files, or change startup behavior.

`USQLUIPersistenceStatusDisplayLibrary` is the first UI-consumption adapter above that snapshot. It returns `FSQLUIPersistenceStatusDisplayRow` values with a label, value, state, and detail text for UI binding. It delegates status gathering to `USQLUIPersistenceStatusLibrary` and does not perform direct file checks, initialize providers, create repositories, create databases, run migrations, copy seed databases, reset databases, delete files, or change startup behavior.

`USQLUIPersistenceSettingsDraftLibrary` is the first non-mutating SQLUICore settings draft surface for future persistence UI. It can build current/default `FSQLUIPersistenceSettingsDraft` values, represent pending backend/path/provider-auto-init changes, reset a draft value back to current values, and validate pending values through `FSQLUIPersistenceSettingsValidationResult`. `USQLUIPersistenceSettingsDraftDisplayLibrary` now formats those validation-only results into UI-safe summary/row data for future sample or settings panels. `USQLUISamplePersistenceSettingsDraftPresenter` is an optional SQLUISamples sample/dev adapter that consumes those SQLUICore rows and keeps the latest display result in memory for future sample UI work. `USQLUISamplePersistenceSettingsDraftPanelWidget` is an optional SQLUISamples C++ `UUserWidget` shell over that presenter for future Blueprint binding; it creates no visual layout, adds no widget blueprint asset, does not attach to the viewport, and does not refresh from lifecycle hooks. Validation, display formatting, sample adapter refresh, and widget-shell build/refresh are dry-run only: they do not write config, apply settings, create directories or database files, run migrations, copy seed databases, initialize providers/repositories, delete files, add settings controls, or change startup behavior. The focused usage guide for that shell is [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md).

`USQLUISamplePersistenceStatusPresenter` is the first optional SQLUISamples sample/dev and Blueprint-facing surface over those display rows. Its Blueprint-callable, caller-invoked refresh functions re-query rows from SQLUICore, store stable formatted strings for simple sample UI, Blueprint, or commandlet presentation, and return a small refresh result for sample/dev consumers. It is not a full settings screen, is not wired into default startup, maps, config, timers, tick, or polling, and does not add settings editing, reset/delete actions, provider/repository initialization, database creation, migrations, seed copy, or file deletion.

`USQLUISamplePersistenceStatusPanelAdapter` is the first tiny optional panel-named SQLUISamples adapter for future Blueprint/UMG binding. It delegates refresh to the presenter, stores only the latest rows, formatted lines, summary text, and refresh result in memory, and does not duplicate SQLUICore status logic. It is not a widget, is not wired into default startup, maps, config, timers, tick, or polling, and does not add settings editing, reset/delete actions, provider/repository initialization, database creation, migrations, seed copy, or file deletion.

`USQLUISamplePersistenceStatusPanelWidget` is the first tiny optional C++ UMG shell over that adapter. It lives only in SQLUISamples, creates no visual layout, includes no widget blueprint asset, is not added to the viewport, and is not wired into default startup, maps, config, timers, tick, or polling. It only exposes caller-invoked refresh plus cached rows/result for future Blueprint subclassing or binding, and does not add settings editing, reset/delete actions, provider/repository initialization, database creation, migrations, seed copy, or file deletion.

The read-only persistence status panel contract in [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) and the focused UMG binding recipe in [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md) are the current Blueprint/UMG usage references for that widget-shell/adapter/presenter/display-row path. The validation-only draft settings binding recipe in [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) covers the draft panel shell. A future panel should show row labels and values, optionally use row state/detail text for presentation, and expose refresh/build only as caller-invoked actions that re-query current status rows or validate draft settings rows. The contracts, guides, adapters, and widget shells add no widget blueprint, map, startup wiring, polling, ticking, provider initialization, repository initialization, migrations, seed copy, settings editing, apply/save behavior, or reset/delete behavior.

## Read-Only Persistence Status UMG Foundation

The read-only persistence status UMG foundation is now complete as a sample/dev-facing base for future UI work. The implemented chain is:

- `USQLUIPersistenceStatusLibrary` for read-only persistence snapshots.
- `USQLUIPersistenceStatusDisplayLibrary` for UI-safe display rows.
- `USQLUISamplePersistenceStatusPresenter` for optional SQLUISamples presentation and formatted lines.
- Explicit caller-invoked refresh methods, including the Blueprint-facing presenter hook.
- The documented read-only panel contract and UMG binding recipe.
- `USQLUISamplePersistenceStatusPanelAdapter` for panel-friendly cached rows/results.
- `USQLUISamplePersistenceStatusPanelWidget` as an optional C++ `UUserWidget` shell.
- `-UsePersistenceStatusSampleSurfaceProbe` smoke coverage for the presenter, Blueprint hook, adapter, and widget-shell contract without assets, maps, viewport attachment, startup wiring, polling, ticking, or auto-refresh.

This foundation remains read-only. It does not create databases, run migrations, copy seed databases, initialize providers or repositories, save settings, switch backends, reset databases, delete files, add destructive actions, or change normal startup. SQLUICore continues to own status, display-row, path, repository, schema, seed-copy, and database-management policy; SQLUISamples only provides optional sample/dev-facing presenter, adapter, and UMG-shell infrastructure.

PR #105 planned future settings-editing/reset work in [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md). The first follow-ups add only the non-mutating SQLUICore draft/validation model, UI-safe validation display rows, an optional SQLUISamples sample/dev adapter for those rows, and an optional C++ UMG widget shell contract; they do not add mutating settings behavior. Future implementation should keep widgets ignorant of SQL, schema, migration ids, seed-copy policy, sidecar internals, and direct file deletion. Settings edits should use a pending/apply model instead of mutating live persistence from widget bindings. Reset/delete UX should call SQLUICore database management helper/policy surfaces and must not let widgets delete files directly. SQLite should not become the default, and provider auto-init should not become default, without separate explicitly scoped policy PRs.

The packaged runtime provider startup smoke proves this holder can be intentionally created from packaged startup/runtime code and initialized from command-line repository settings. That proof runs only with `-SQLUIRuntimeProviderStartupSmoke`; normal startup still does not auto-initialize a provider or SQLite.

The packaged runtime provider subsystem smoke proves the app-level subsystem holder path. It runs only with `-SQLUIRuntimeProviderSubsystemSmoke` plus explicit `-SQLUILayoutRepositoryProviderAutoInit` and repository settings. When the flags are absent, the subsystem remains passive and normal startup still does not initialize SQLite.

The packaged runtime persistence workflow smoke proves the first cross-launch packaged workflow path. It runs only with `-SQLUIRuntimePersistenceWorkflowSmoke` and an explicit `Save`, `Verify`, or `Cleanup` phase. The Save phase uses `USQLUILayoutRepositoryRuntimeSubsystem` plus `FSQLUILayoutPersistenceWorkflow` to save/list/load one layout and leave the SQLite database under `Saved/SQLUI/LayoutRepositories/PackagedRuntimeSmoke/PersistenceWorkflow`. The Verify phase starts a separate packaged process and list/loads the same persisted layout without saving first. The Cleanup phase removes the database and SQLite sidecars.

## Runtime Database Management Policy

`FSQLUILayoutRepositoryDatabaseManagement` is a SQLUICore-owned helper for future app/settings/admin flows that need to inspect or reset the configured SQLite layout repository database.

The helper accepts plain `FSQLUILayoutRepositoryRuntimeConfig` request data and uses `FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath` for path policy. For non-SQLite backends, status and reset are safe no-ops. For SQLite, status reports the resolved path, database-file existence, sidecar existence, and database file size. Reset removes only the resolved database file and its SQLite sidecars:

- `<database>.db`
- `<database>.db-journal`
- `<database>.db-wal`
- `<database>.db-shm`

Reset is idempotent: a missing file is treated as already reset. Empty SQLite paths fail reset clearly so callers cannot accidentally delete arbitrary files. The helper does not open SQLite, run SQL, initialize schema, create directories, create databases, copy seed files, seed data, create repositories, touch the factory, initialize providers/subsystems, or change startup behavior.

This is a policy helper only. It does not add a UI surface; product settings screens and admin tools remain future work.

The future product-facing settings surface is documented in [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md). That design keeps `InMemory` as the default, keeps SQLite explicit, recommends an app-managed SQLite path under `Saved/SQLUI/LayoutRepositories/LayoutRepository.sqlite`, and routes status/reset through this helper.

## Seed Database Copy Policy

`FSQLUISQLiteSeedDatabaseCopy` is a SQLUICore-owned helper for explicitly copying a closed SQLite seed database into a writable runtime database path before repository use.

The helper:

- Normalizes seed and target paths.
- Fails clearly when seed path is empty, target path is empty, the seed DB is missing, or seed and target paths are the same.
- Leaves an existing target database untouched unless overwrite is explicitly enabled.
- Copies the seed DB to a missing target only when copy-if-missing is enabled.
- Deletes only the target database file plus target SQLite sidecars before overwrite.
- Creates the target parent directory only when requested.

The helper does not open SQLite, run migrations, create schema, seed data, create repositories, or participate in `USQLUILayoutRepositoryFactory`. It is a pre-repository file preparation policy for callers that already decided to use a seed database.

No seed database assets are included by default, and no source-controlled database files are required for the current runtime path. Any future product seed DB policy still needs explicit asset location, packaging, versioning, and upgrade rules.

## Schema Initialization

Schema initialization is repository-owned and opt-in. Defaults preserve the older prepared-database behavior:

- `bInitializeSchemaIfMissing = false`
- `bCreateDatabaseIfMissing = false`

The repository can create and initialize a missing SQLite database only when all of these are true:

- `bInitializeSchemaIfMissing = true`
- `bCreateDatabaseIfMissing = true`
- `bReadOnly = false`

Read-only mode blocks schema-initialization writes. Writable operations reject read-only repositories before schema initialization can create files.

`FSQLUISQLiteLayoutSchemaMigration::ApplyInitialSchema` applies the planned initial layout schema and records `001_initial_layout_schema` in `sqlui_schema_migrations`. The initialization path is idempotent for an already-initialized database, can record the migration row non-destructively when the complete schema exists but the row is missing, and fails clearly for partial or corrupt schemas instead of dropping or repairing user data.

## Migration Versioning Policy

`FSQLUISQLiteLayoutSchemaVersioning` is the SQLUICore-owned status/version helper for layout schema migrations.

The current production known migration set is still only:

- `001_initial_layout_schema`

No real production `002` schema migration is introduced yet.

The versioning helper can report the latest known migration id, applied migration ids, pending known migration ids, initial-schema record state, and whether the expected layout schema objects are ready. It uses the existing `sqlui_schema_migrations` table and does not make SQLite the default backend, run migrations in the factory, or add startup behavior.

Current policy:

- A missing database fails status checks unless a caller explicitly uses an apply/create path.
- A complete schema with a missing `001_initial_layout_schema` row is detected as schema-ready but pending; applying known migrations records the missing row non-destructively.
- Partial schemas fail clearly and report missing expected schema objects.
- Unknown extra migration rows do not fail status by themselves; known migration state is still reported from the configured known migration list.

Actual future schema migrations and upgrade-specific data transforms remain future work.

## Async Behavior

Default SQLite repository behavior remains synchronous.

The callback-style APIs can opt into async execution:

- `LoadLayout`
- `SaveLayout`

When `bRunCallbackOperationsAsync = true`, those callback methods copy plain request data, enqueue the non-UObject worker helper through a SQLUICore-owned per-repository async queue, run one queued SQLite callback operation at a time off the game thread, then marshal result delivery back to the game thread before invoking the callback. This preserves enqueue order for callback-style `SaveLayout` and `LoadLayout`.

The serialized async queue has an explicit shutdown policy. After shutdown it rejects new work, drops queued work that has not started, lets any already-running worker finish without attempting cancellation, suppresses stale game-thread completion callbacks, and does not dispatch additional queued work. `USQLUISQLiteLayoutRepository` shuts down its queue when reconfigured away from the active async settings and during object destruction.

These methods remain synchronous:

- `ListLayouts`
- `LoadLayoutById`
- `RemoveLayout`
- `ClearLayouts`

A full production async service, cancellation policy, shutdown draining beyond stale-callback suppression, and async coverage for the remaining repository operations are still future work.

## Smoke Coverage

SQLite smoke paths are optional. They write only under `Saved/SQLUI/SmokeTests/...` and remove probe database files plus SQLite sidecar files after each run.

Current SQLite-related smoke flags are:

- `-UseSQLiteCoreProbe`: proves engine `SQLiteCore` open/close behavior under a smoke-safe path.
- `-UseDatabaseAsyncProbe`: proves the generic SQLUI database async boundary without SQLite file I/O.
- `-UseDatabaseAsyncQueueShutdownProbe`: verifies the serialized async queue rejects new work after shutdown, drops pending work, and suppresses stale callbacks.
- `-UseLayoutRepositoryRuntimeConfigProbe`: verifies default/config parsing, SQLite path resolution, missing-path unavailable behavior, and a factory-created SQLite save from explicit runtime config.
- `-UseLayoutRepositoryRuntimeIntegrationProbe`: verifies the runtime integration helper across default in-memory creation, explicit SQLite creation/save/list, missing-path unavailable behavior, explicit seed-copy repository creation, fatal missing-seed handling, and cleanup.
- `-UseLayoutRepositoryRuntimeProviderProbe`: verifies the runtime provider across default in-memory initialization, reset/reinitialization, explicit SQLite save/list/load, command-line SQLite initialization, explicit seed-copy initialization/readback, fatal missing-seed handling, and cleanup.
- `-UseLayoutRepositoryRuntimeSettingsProbe`: verifies config-backed runtime settings safe defaults, settings-driven `InMemory`, settings-driven SQLite, command-line override behavior, disabled overrides, explicit SQLite missing-path unavailable behavior, and cleanup.
- `-UseLayoutPersistenceWorkflowProbe`: verifies the storage-agnostic runtime workflow helper for null/missing repository failures, in-memory save/list/load, explicit SQLite save/list/load, SQLite unavailable behavior, and cleanup.
- `-UseLayoutRepositoryDatabaseManagementProbe`: verifies the SQLUICore database management helper for non-SQLite no-op behavior, SQLite empty-path handling, status before/after repository save, reset/idempotent reset, fake sidecar cleanup, relative path resolution under `Saved/SQLUI/LayoutRepositories`, and cleanup.
- `-UsePersistenceStatusSurfaceProbe`: verifies the read-only SQLUICore persistence status snapshot for default `InMemory` state, provider/repository inactive state without forced initialization, SQLite path/file/sidecar/schema status against pre-created probe files, and cleanup.
- `-UsePersistenceStatusDisplayRowsProbe`: verifies the read-only SQLUICore display-row adapter for default and SQLite snapshots, sidecar reporting, no formatter-side database mutation, and cleanup.
- `-UsePersistenceStatusSampleSurfaceProbe`: documents and validates the optional SQLUISamples presenter, panel adapter, and C++ UMG widget shell binding contract, including Blueprint-callable non-pure refresh hooks, pure cached widget getters, Blueprint-visible cached rows/formatted-lines/result/summary, display-row consumption, explicit repeated refresh without creating files, graceful missing SQLite DB state, smoke-owned sidecar preservation during refresh, no asset/map/viewport requirement, and cleanup.
- `-UsePersistenceSettingsDraftProbe`: verifies the SQLUICore draft/validation model, validation display rows, SQLUISamples sample adapter, and optional C++ UMG widget shell reflection contract for default/current `InMemory`, unknown backend rejection/error display, SQLite draft/path display without DB creation, empty SQLite path rejection/error display, provider auto-init pending validation/display without policy mutation, deterministic validation/display/adapter output, smoke-owned sidecar preservation, and cleanup.
- `-UseSQLiteMigrationProbe`: proves the minimal migration runner with a smoke-only migration.
- `-UseSQLiteLayoutSchemaMigrationProbe`: applies and verifies the planned initial layout schema.
- `-UseSQLiteLayoutReadProbe`: seeds one layout and verifies list/load mapping against the schema.
- `-UseSQLiteReadOnlyLayoutRepository`: verifies read-only `ListLayouts`/`LoadLayout` plus write rejection.
- `-UseSQLiteSaveLayoutRepository`: verifies writable `SaveLayout`, `ListLayouts`, `LoadLayout`, and revision updates.
- `-UseSQLiteRemoveLayoutRepository`: verifies soft-delete `RemoveLayout` and revision preservation.
- `-UseSQLiteClearLayoutsRepository`: verifies destructive scoped `ClearLayouts`.
- `-UseSQLiteFullLifecycleRepository`: exercises save, list, load, update/revision, remove, and clear in one workflow.
- `-UseSQLiteAsyncCallbackRepository`: verifies opt-in async callback `SaveLayout` and `LoadLayout`.
- `-UseSQLiteSerializedAsyncCallbackRepository`: verifies queued async callback saves and load are delivered in enqueue order.
- `-UseSQLiteFactoryLayoutRepository`: verifies explicit factory-created SQLite repository behavior.
- `-UseSQLiteFactorySchemaInitRepository`: verifies opt-in repository-owned schema initialization through factory settings.
- `-UseSQLiteSchemaInitHardening`: verifies schema-init edge cases and read-only init blocking.
- `-UseSQLiteSeedDatabaseCopyPolicyProbe`: verifies explicit seed DB copy/no-overwrite/overwrite/failure behavior and confirms copied targets are repository-readable.
- `-UseSQLiteMigrationVersioningPolicyProbe`: verifies schema version status, missing-record repair, partial-schema failure, ordered smoke-only migrations, pending migration detection, idempotent reruns, and failing migration reporting.

The smoke command reference and expected log lines live in [`sqlui_smoke_test.md`](sqlui_smoke_test.md).

Packaged runtime SQLite smoke is separate from editor commandlet smoke. `RunSQLUIPackagedBuildValidation.ps1 -RunPackagedSQLiteSmoke` builds/packages the project, launches the packaged executable with `-SQLUIPackagedRuntimeSQLiteSmoke`, verifies a repository lifecycle under the packaged runtime `Saved/SQLUI/PackagedRuntimeSmoke/...` path, checks the runtime log for success, and removes the smoke database.

Packaged runtime provider startup smoke is another explicit packaged executable proof. `RunSQLUIPackagedBuildValidation.ps1 -RunPackagedProviderStartupSmoke` launches the packaged executable with `-SQLUIRuntimeProviderStartupSmoke` and explicit SQLite repository command-line settings, verifies provider initialization/save/list/load/reset behavior, checks the runtime log for success, and removes the smoke database under packaged runtime `Saved/SQLUI/LayoutRepositories/PackagedRuntimeSmoke/RuntimeProviderStartup`.

Packaged runtime provider subsystem smoke proves the passive subsystem integration surface. `RunSQLUIPackagedBuildValidation.ps1 -RunPackagedProviderSubsystemSmoke` launches the packaged executable with `-SQLUIRuntimeProviderSubsystemSmoke`, `-SQLUILayoutRepositoryProviderAutoInit`, and explicit SQLite repository command-line settings, waits for the `GameInstance` subsystem, verifies subsystem/provider initialization/save/list/load/reset behavior, checks the runtime log for success, and removes the smoke database under packaged runtime `Saved/SQLUI/LayoutRepositories/PackagedRuntimeSmoke/RuntimeProviderSubsystem`.

Packaged runtime persistence workflow smoke proves persistence across separate packaged launches. `RunSQLUIPackagedBuildValidation.ps1 -RunPackagedPersistenceWorkflowSmoke` launches Save, Verify, and Cleanup packaged processes with `-SQLUIRuntimePersistenceWorkflowSmoke`, explicit subsystem auto-init, and a relative SQLite path of `PackagedRuntimeSmoke/PersistenceWorkflow/PersistenceWorkflow.db`. Save leaves the database in place, Verify reads the persisted layout without saving first, and Cleanup removes the database and sidecars.

## Runtime Boundaries

SQLite details belong in SQLUICore and the SQLUISamples smoke harness only. Widgets should continue to work with repository contracts, layout documents, variable stores, action systems, runtime contexts, and SQLUICore persistence status/display rows.

Runtime-writable databases should live under `Saved/SQLUI/...`. Smoke tests use narrower `Saved/SQLUI/SmokeTests/...` folders. SQLite runtime paths should not write to `Content/`, maps, plugin content, generated folders, or source-controlled database files.

Database reset/status flows should use `USQLUIPersistenceStatusLibrary` for UI-facing read-only snapshots and `FSQLUILayoutRepositoryDatabaseManagement` for explicit database status/reset policy so path resolution and sidecar cleanup stay in SQLUICore. App UI should not delete arbitrary files, infer sidecar names itself, or open SQLite just to answer whether the configured DB exists.

JerryRigged remains a thin host. It should not own SQLite schema, SQL strings, migration logic, worker details, or storage-specific widget behavior.

## Remaining Work

The SQLite path is implemented enough for repository-shaped local smoke coverage, but it is not yet a production-hardened packaged persistence system.

Remaining work includes:

- Expanding packaged-build validation beyond the latest local Win64 Development pass.
- Expanding packaged runtime SQLite lifecycle coverage beyond the first local packaged executable smoke.
- Expanding packaged persistence-across-launches coverage beyond the first local Save/Verify/Cleanup workflow smoke.
- Production async database service design beyond the current per-repository callback queue.
- Cancellation, shutdown draining beyond stale-callback suppression, and async coverage for all repository operations.
- Actual future schema migrations, upgrade-specific data transforms, and version-specific compatibility policy beyond the current version/status framework.
- Implementing the user-facing persistence settings UI described in [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md), [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md), [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md), and [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md). The first read-only SQLUICore status snapshot, display rows, sample presenter, panel adapter, C++ widget shell, panel contract, focused binding guide, docs-only mutating settings/reset plan, non-mutating draft validation model, draft validation display rows, sample/dev-facing draft validation adapter, draft validation C++ widget shell, and focused draft binding guide exist; widget blueprint assets, visual layout, settings apply/save, backend selector UI, SQLite path editor UI, provider auto-init controls, reset UI, and product startup policy remain future work.
- Product seed database asset/package/version policy, if seed DBs are added.
- Optional lifecycle features such as history APIs, checkpoints, previews, restore flows, and richer search.

## Packaged Validation Status

Local smoke tests prove commandlet behavior and temporary database cleanup. The packaged-build validation scaffold in [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md) provides a repeatable local `RunUAT BuildCookRun` command for checking package compatibility with SQLUI and SQLiteCore wiring.

The latest local Win64 Development packaged-build validation passed after installing the UE 5.7-preferred Visual Studio 2022 MSVC `14.44.x` toolchain. That pass proves the local BuildCookRun path can build, cook, stage, package, and archive with SQLUI and SQLiteCore wiring enabled.

The scaffold can also run the packaged executable with `-SQLUIPackagedRuntimeSQLiteSmoke` to prove one packaged runtime SQLite lifecycle against a packaged runtime `Saved/SQLUI/PackagedRuntimeSmoke/...` database path. That runtime smoke covers save, list, load, remove, clear, log verification, and database cleanup.

The scaffold can separately run the packaged executable with `-SQLUIRuntimeProviderStartupSmoke` to prove explicit packaged startup code can create `USQLUILayoutRepositoryRuntimeProvider`, initialize it from command-line repository settings, use the active repository for save/load, verify SQLite list readback in smoke-only code, reset the provider, and clean up its database.

The scaffold can also run the packaged executable with `-SQLUIRuntimeProviderSubsystemSmoke` and `-SQLUILayoutRepositoryProviderAutoInit` to prove the passive `USQLUILayoutRepositoryRuntimeSubsystem` can own/access the provider, initialize it from explicit command-line repository settings, use the active repository for save/load, verify SQLite list readback in smoke-only code, reset the subsystem/provider, and clean up its database. Config-backed provider auto-init is intentionally default-off; packaged subsystem smoke continues to use explicit command-line auto-init.

The scaffold can now run the packaged executable with `-SQLUIRuntimePersistenceWorkflowSmoke` across Save, Verify, and Cleanup launches to prove the subsystem-held repository and storage-agnostic persistence workflow can persist one layout across separate packaged processes and then clean up the database.

It still does not prove platform coverage beyond the requested local target, long-running database service behavior, full async lifecycle handling, production migration upgrades, or product startup policy beyond explicit smoke flags.

Until those items are validated, SQLite should stay explicitly configured, not default.
