# SQLUI Repository Architecture

SQLUI layout persistence sits behind repository interfaces. Runtime widget code should ask for validated layout documents through repositories and related runtime services. It should not issue raw SQL, know database table shapes, or choose storage backends directly.

This boundary keeps `SQLUI.Widgets` focused on building and updating UMG widgets, keeps `SQLUI.Core` responsible for data access contracts, and lets the project swap storage implementations without rewriting widget or runtime pipeline code.

For the consolidated SQLite phase roadmap, see [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md). For the deeper current runtime state, see [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md). For the future user-facing persistence/settings UX policy, see [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md). For the planned mutating settings editing/reset phase, see [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md). For the focused read-only widget-shell binding recipe, see [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md). For the validation-only draft settings widget-shell binding recipe, see [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md). For the dry-run apply-preview widget-shell binding recipe, see [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md).

## Repository Boundary

`ISQLUILayoutRepository` is the current layout repository contract. It exposes:

- `LoadLayout(LayoutId, Callback)`
- `SaveLayout(Document, Callback)`

The `USQLUILayoutRepository` UObject-facing repository surface also exposes `ListLayouts()` for storage-agnostic metadata reads. Concrete repositories can add direct helper methods such as `LoadLayoutById`, `RemoveLayout`, and `ClearLayouts`, but runtime widget code should still stay behind the repository/runtime-service boundary.

The callback shape is important even while the current concrete repositories complete immediately. Future persistent repositories can use the same contract while doing database or file work through appropriate background boundaries.

Widgets, widget factories, and runtime pipeline code should depend on layout documents, repositories, variable stores, action systems, and runtime context objects. They should not depend on raw SQL strings, SQLite connection objects, table names, or direct file paths.

## Repository Selection

Runtime-facing code can choose a repository backend through `FSQLUILayoutRepositoryFactorySettings` and `USQLUILayoutRepositoryFactory`.

`FSQLUILayoutRepositoryRuntimeConfigResolver` is the SQLUICore storage-selection policy helper above the factory. It turns explicit runtime config values or command-line options into `FSQLUILayoutRepositoryFactorySettings` while keeping `InMemory` as the default, keeping SQLite non-default, resolving relative SQLite paths under `Saved/SQLUI/LayoutRepositories`, and leaving all repository creation, schema initialization, and file creation to later factory/repository behavior. It can also map explicit SQLite seed-copy options into `FSQLUISQLiteSeedDatabaseCopyRequest`; that mapping does not copy files or select repositories.

`FSQLUILayoutRepositoryRuntimeIntegration` is the SQLUICore helper above the resolver, seed-copy policy, and factory. Given explicit runtime config, it can run the SQLite seed-copy helper only when copy-if-missing or overwrite is requested, then create the repository through `USQLUILayoutRepositoryFactory`. It preserves the resolver default of `InMemory`, does not make normal startup use SQLite, does not run schema initialization itself, and reports unavailable behavior when SQLite is requested without a database path.

`USQLUILayoutRepositoryRuntimeSettings` and `FSQLUILayoutRepositoryRuntimeSettingsPolicy` sit above the runtime integration request. The settings object is config-backed (`Config=Game`) but does not add a Project Settings UI surface. It defaults to no provider auto-init, `InMemory`, empty SQLite paths, schema initialization/database creation disabled, async callbacks disabled, and seed-copy flags disabled. The policy can build an integration request from settings plus command-line text. Command-line repository options override settings only when `bAllowCommandLineOverrides = true`.

`USQLUILayoutRepositoryRuntimeProvider` is the storage-agnostic UObject holder above the runtime integration helper. It stores the active `ISQLUILayoutRepository` implementation, exposes the last integration result, supports explicit reset/reinitialization, and initializes only when caller code provides a runtime integration request, runtime config, or command-line string. It delegates repository creation to `FSQLUILayoutRepositoryRuntimeIntegration`; it does not construct concrete repositories directly, auto-start, attach widgets, choose SQLite by default, or run migrations by itself.

`USQLUILayoutRepositoryRuntimeSubsystem` is the passive SQLUICore `UGameInstanceSubsystem` holder above the provider. It can own one transient runtime provider for app-level access and exposes provider/repository status plus explicit initialization/reset methods. The subsystem does not initialize a repository, select SQLite, create databases, copy seed files, attach widgets, touch the viewport, or require maps/Content by default. During subsystem initialization it only auto-initializes when config-backed settings or `-SQLUILayoutRepositoryProviderAutoInit` explicitly request it, and that auto-init still uses the settings policy/integration/provider path.

`FSQLUILayoutPersistenceWorkflow` is the storage-agnostic app-facing helper above the runtime subsystem. Once caller code has explicitly initialized the subsystem/provider, the helper uses the active `USQLUILayoutRepository` to `SaveLayout`, `ListLayouts`, and `LoadLayout` without casting to SQLite or knowing database paths, seed-copy policy, migrations, or concrete repository types. It does not initialize providers, create repositories, create databases, copy seed files, run schema initialization, attach widgets, touch the viewport, or change startup behavior. If the subsystem is null or has no active repository, it returns clear failure results. This is the intended shape for future runtime app code that needs persistence while keeping widgets away from storage details.

`FSQLUILayoutRepositoryDatabaseManagement` is the SQLUICore policy helper for future app/settings/admin flows that need to inspect or reset the configured SQLite layout repository database. It uses the runtime config resolver for path policy, treats non-SQLite backends as safe no-ops, reports SQLite database and sidecar file status, and resets only the resolved database plus `.db-journal`, `.db-wal`, and `.db-shm` sidecars. It does not open SQLite, run migrations, create schema, create repositories, initialize providers, touch the factory, or change startup behavior.

`USQLUIPersistenceStatusLibrary` is the first read-only SQLUICore surface intended for future settings UI. It returns a `FSQLUIPersistenceStatusSnapshot` with configured backend, provider initialized state, repository active state, resolved SQLite path, file existence/size, sidecar presence, and migration status when an existing SQLite database can be inspected. It composes existing policy helpers and does not initialize providers, create repositories, create databases, run migrations, copy seeds, reset databases, delete files, or change startup behavior.

`USQLUIPersistenceStatusDisplayLibrary` is the first read-only UI consumption layer above that surface. It converts the status snapshot into `FSQLUIPersistenceStatusDisplayRow` values with label/value/state/detail text for future settings panels or sample displays. It does not probe files directly, create repositories, initialize providers, create databases, run migrations, copy seeds, reset databases, delete files, or change startup behavior.

`USQLUIPersistenceSettingsDraftLibrary` is the first non-mutating SQLUICore settings draft helper for future settings panels. It builds current/default draft data from runtime settings policy, represents pending runtime config and provider auto-init values, validates backend/path/provider-auto-init choices, can reset a draft value back to current values, builds dry-run apply-intent previews through `FSQLUIPersistenceSettingsApplyPreviewResult`, and exposes non-executing apply/cancel contract data through `FSQLUIPersistenceSettingsApplyContractResult` and `FSQLUIPersistenceSettingsCancelPreviewResult`. `USQLUIPersistenceSettingsDraftDisplayLibrary`, `USQLUIPersistenceSettingsApplyPreviewDisplayLibrary`, and `USQLUIPersistenceSettingsApplyContractDisplayLibrary` format validation, apply-preview, and apply/cancel contract results into UI-safe summary/row data for future sample or settings panels. `USQLUISamplePersistenceSettingsDraftPresenter`, `USQLUISamplePersistenceSettingsApplyPreviewPresenter`, and `USQLUISamplePersistenceSettingsApplyContractPresenter` are SQLUISamples-only sample/dev adapters that consume the validation/apply-preview/apply-contract SQLUICore display rows and cache row/summary strings for future sample UI work. `USQLUISamplePersistenceSettingsDraftPanelWidget` and `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget` are optional SQLUISamples C++ `UUserWidget` shells that delegate to the validation/apply-preview presenters and expose cached rows/result/summary/flags for future Blueprint binding. These helpers do not apply settings, write config, initialize providers/repositories, create directories or database files, open databases for writing, run migrations, copy seed databases, delete files, add settings controls, add widget blueprint assets, attach to the viewport, or change startup behavior. Actual Apply execution is explicitly unavailable/not implemented, and cancel/discard remains value-preview only. The safe binding recipes are documented in [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) for draft validation and [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md) for dry-run apply-preview display.

`USQLUISamplePersistenceStatusPresenter` is the first optional SQLUISamples sample/dev and Blueprint-facing surface over those display rows. It stores the rows and stable formatted text lines for simple sample UI, Blueprint, or commandlet consumption, and exposes Blueprint-callable, caller-invoked refresh functions that re-query the same SQLUICore status/display surfaces. It is not a full settings screen, is not wired into default startup, maps, timers, tick, polling, or config, and it does not duplicate SQLUICore probing, edit settings, initialize providers/repositories, create databases, run migrations, copy seeds, reset databases, or delete files.

`USQLUISamplePersistenceStatusPanelAdapter` is the first tiny optional panel-named adapter over that presenter. It is a SQLUISamples `UObject`, not a `UUserWidget`, and it stores only the latest rows, formatted lines, summary text, and refresh result for Blueprint/UMG binding. It delegates refresh to `USQLUISamplePersistenceStatusPresenter` and does not duplicate SQLUICore status logic, add widget blueprint assets, attach to maps/startup/config, poll/tick, edit settings, initialize providers/repositories, create databases, run migrations, copy seeds, reset databases, or delete files.

`USQLUISamplePersistenceStatusPanelWidget` is the first tiny optional C++ UMG shell over that adapter. It lives only in SQLUISamples, creates no visual layout, adds no widget blueprint asset, is not added to the viewport, is not wired into maps/startup/config, does not tick/poll/auto-refresh, and only exposes caller-invoked refresh plus cached rows/result for future Blueprint binding. Its refresh path delegates to the panel adapter instead of owning persistence internals.

The read-only persistence status panel contract is documented in [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md), with the focused UMG binding recipe in [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md). The draft validation widget-shell recipe is documented in [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md), and the dry-run apply-preview widget-shell recipe is documented in [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md). Together they define how future Blueprint/UMG UI should consume the existing widget-shell/adapter/presenter/display-row path: show row labels/values, optionally map row state/detail text to UI affordances, keep refresh/build caller-invoked only, use SQLUICore preview/contract data and display rows only as dry-run report data, and keep widgets away from provider lifecycle, direct file checks, migrations, seed-copy policy, apply/save behavior, and reset/delete behavior. They are usage recipes plus optional sample shells, not finished settings screens.

That read-only persistence status UMG foundation is complete as a sample/dev-facing binding base. The non-mutating persistence settings draft validation UI foundation is also complete through the #105 plan, #106 draft model, #107 validation display rows, #108 sample adapter, #109 C++ UMG shell, #110 usage guide, and #111 final checkpoint. The non-mutating apply-preview UI foundation is complete through #112 dry-run apply-intent preview, #113 UI-safe apply-preview display rows/summary, #114 optional SQLUISamples apply-preview sample adapter, #115 optional SQLUISamples apply-preview C++ UMG widget shell, #116 focused apply-preview UMG usage guide, #117 final checkpoint, the non-mutating apply/cancel contract, UI-safe apply/cancel contract display rows/summary, optional SQLUISamples apply/cancel contract sample adapter, and `-UsePersistenceSettingsDraftProbe` smoke coverage. These foundations do not change repository selection, default startup, default maps/config, provider auto-init policy, SQLite defaults, or packaged runtime behavior. Future settings-editing, backend-selection, SQLite-path editing, actual mutating apply/cancel execution, provider auto-init controls, migration/seed-copy controls, and reset/delete UI must remain above SQLUICore helper/policy surfaces, follow [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md), and use pending/apply semantics rather than widget-owned persistence mutation. Widgets must not write config directly, initialize providers/repositories directly, or delete files directly.

The packaged runtime provider startup smoke proves the intended startup integration shape without changing normal startup. When `-SQLUIRuntimeProviderStartupSmoke` is present, SQLUISamples creates the provider after engine-loop initialization, initializes it from command-line repository settings, uses the active repository through the repository contract for save/load, verifies SQLite list readback in smoke-only code, resets the provider, removes the smoke database, and exits. When the flag is absent, SQLUISamples does not auto-initialize the provider.

The packaged runtime provider subsystem smoke proves the intended normal runtime holder surface without changing normal startup. When `-SQLUIRuntimeProviderSubsystemSmoke` and `-SQLUILayoutRepositoryProviderAutoInit` are present, SQLUISamples waits for the packaged `GameInstance`, finds `USQLUILayoutRepositoryRuntimeSubsystem`, verifies command-line SQLite auto-initialization, uses the provider-held repository through the repository contract for save/load, verifies SQLite list readback in smoke-only code, resets the subsystem/provider, removes the smoke database, and exits. When those flags are absent, the subsystem remains passive.

The packaged runtime persistence workflow smoke proves the app-facing workflow boundary across separate packaged launches without changing normal startup. When `-SQLUIRuntimePersistenceWorkflowSmoke` and an explicit `Save`, `Verify`, or `Cleanup` phase are present, SQLUISamples waits for the packaged `GameInstance` subsystem, uses `FSQLUILayoutPersistenceWorkflow` for save/list/load in the Save phase, uses the same workflow to list/load the persisted layout in a separate Verify process without saving first, and removes the database in a final Cleanup process. When the flag is absent, SQLUISamples does not run the workflow smoke.

`ESQLUILayoutRepositoryBackend` currently supports:

- `Unavailable`: creates `USQLUILayoutRepository`, which reports `bBackendUnavailable` for load and save.
- `InMemory`: creates `USQLUIInMemoryLayoutRepository`.
- `JsonFile`: creates `USQLUIJsonFileLayoutRepository`.
- `SQLite`: creates `USQLUISQLiteLayoutRepository` only when explicitly requested and configured with a non-empty database path.

`FSQLUILayoutRepositoryFactorySettings` also includes an optional `JsonFileBaseDirectory`. When it is set, the JSON file repository is configured with that directory. When it is empty, the JSON file repository keeps its default `Saved/SQLUI/Layouts` path.

`FSQLUILayoutRepositoryFactorySettings` includes `SQLiteSettings` for the SQLite backend. Those settings are passed to `USQLUISQLiteLayoutRepository` without running migrations or creating database files in the factory. If `Backend = SQLite` is requested with an empty `SQLiteSettings.DatabasePath`, the factory returns the existing unavailable repository behavior instead of silently falling back to another backend. Schema initialization remains repository-owned and opt-in through SQLite settings.

`FSQLUISQLiteLayoutSchemaVersioning` is the SQLUICore schema status/versioning helper for known SQLite layout migrations. The known production migration set is currently only `001_initial_layout_schema`; the helper reports applied and pending known ids, can detect complete schemas with a missing migration row, and fails clearly for partial schemas. It does not add a real production v2 schema migration and does not run inside `USQLUILayoutRepositoryFactory`.

The factory falls back to the transient package when no valid outer is supplied. It is the runtime selection boundary for current sample code and later storage implementations. Widgets should stay behind repository/runtime-context APIs and should not select concrete storage classes themselves.

`FSQLUISQLiteSeedDatabaseCopy` is a separate SQLUICore pre-repository file policy helper. It can copy a closed SQLite seed database into a writable runtime database path when a caller explicitly requests copy-if-missing or overwrite behavior. It does not open SQLite, run schema initialization, seed data, create repositories, or run inside `USQLUILayoutRepositoryFactory`. Widgets should never call it directly.

## Current Repositories

### `USQLUILayoutRepository`

`USQLUILayoutRepository` is the base `UObject` implementation of the repository contract. It is intentionally unavailable by default.

Its `LoadLayout` and `SaveLayout` methods return failure results with `bBackendUnavailable = true`, `bSucceeded = false`, and an explanatory `ErrorMessage`. This gives callers a safe default while a concrete backend is not configured or not implemented.

Use this behavior to make unavailable persistence explicit. Do not treat it as a storage implementation.

### `USQLUIInMemoryLayoutRepository`

`USQLUIInMemoryLayoutRepository` is transient test and development storage. It keeps `FSQLUILayoutDocument` values in a transient map keyed by `Metadata.LayoutId`.

It validates documents before save, supports load by layout id, supports load by display name, and has list, remove, and clear helpers. Data lives only for the lifetime of the repository object.

This repository is useful for smoke tests, sample flows, and temporary runtime experiments where persistence is not required. It should not be used as durable user layout storage.

### `USQLUIJsonFileLayoutRepository`

`USQLUIJsonFileLayoutRepository` is runtime-writable JSON persistence. It serializes validated `FSQLUILayoutDocument` values with `FSQLUILayoutJson` and writes one UTF-8 JSON file per layout id.

By default, it resolves its base directory to:

```text
Saved/SQLUI/Layouts
```

The base directory can be configured through `FSQLUIJsonFileLayoutRepositorySettings`. Layout ids must be non-empty and safe to use as file names. The repository creates its target directory when saving, loads documents back through the same JSON validation path, lists `*.json` layouts as metadata, and can remove or clear stored layout files.

This repository is suitable for lightweight runtime persistence and local development workflows. It remains useful even with the SQLite backend available because it is simple, inspectable, and does not require database setup.

### `USQLUISQLiteLayoutRepository`

`USQLUISQLiteLayoutRepository` is the first repository-shaped SQLite implementation in SQLUICore. It supports read operations, writable `SaveLayout`, soft-delete `RemoveLayout`, and destructive scoped `ClearLayouts` when explicitly configured writable.

The repository is configured with `FSQLUISQLiteLayoutRepositorySettings`, including a `DatabasePath`, `bReadOnly`, the opt-in `bRunCallbackOperationsAsync` flag, and opt-in schema initialization flags. It opens the configured database for each operation. By default, it does not create a database, create schema tables, run migrations, or seed data. When `bInitializeSchemaIfMissing = true` and `bCreateDatabaseIfMissing = true` are explicitly set on a writable repository, the repository worker can create an empty database and apply the planned initial layout schema before operations such as `SaveLayout`. It is factory-selectable only when `ESQLUILayoutRepositoryBackend::SQLite` is explicitly requested and a database path is provided.

SQLite database operation logic lives in the non-UObject `FSQLUISQLiteLayoutRepositoryWorker` helper. The helper accepts plain settings/request data and returns plain repository result structs, which keeps SQLite paths, connection handling, SQL statements, transactions, validation, and serialization outside the UObject wrapper. `USQLUISQLiteLayoutRepository` calls the helper synchronously by default. When `bRunCallbackOperationsAsync = true`, only the callback-style `LoadLayout` and `SaveLayout` methods enqueue worker-helper jobs through the SQLUI database async boundary. `FSQLUIDatabaseAsyncQueue` runs those callback operations one at a time in enqueue order and marshals results back to the game thread before invoking callbacks. Direct return-value methods remain synchronous.

Current supported behavior:

- `ListLayouts` reads active rows from `layouts` where `b_deleted = 0`.
- `ListLayouts` reads metadata columns from `layouts` and tags from `layout_tags`.
- `ListLayouts` preserves the planned ordering by `display_name COLLATE NOCASE ASC, layout_id COLLATE NOCASE ASC`.
- `LoadLayout` reads the current revision document JSON by joining `layouts.current_revision` to `layout_revisions.revision`.
- `LoadLayout` deserializes with `FSQLUILayoutJson` and validates after load.
- `SaveLayout` works only when `bReadOnly = false`, `DatabasePath` is configured, and the database already exists with the planned layout schema unless opt-in schema initialization is enabled.
- `SaveLayout` validates the document, serializes canonical JSON, computes the next revision from `layout_revisions`, upserts `layouts`, inserts an immutable `layout_revisions` row, replaces `layout_tags`, and commits the transaction.
- `RemoveLayout` works only when `bReadOnly = false`, `DatabasePath` is configured, and the database already exists with the planned layout schema unless opt-in schema initialization is enabled.
- `RemoveLayout` soft-deletes active rows by setting `layouts.b_deleted = 1`; revisions, tags, checkpoints, and previews remain intact.
- `ClearLayouts` works only when `bReadOnly = false`, `DatabasePath` is configured, and the database already exists with the planned layout schema unless opt-in schema initialization is enabled.
- `ClearLayouts` destructively deletes previews, checkpoints, tags, revisions, and layouts for the configured database scope.
- `ClearLayouts` counts rows in `layouts` before deletion and returns that count as `RemovedCount`, including active and soft-deleted layout rows.
- `bRunCallbackOperationsAsync` defaults to `false` and preserves the existing immediate callback behavior unless explicitly enabled.
- Async callback execution for `SaveLayout` and `LoadLayout` is serialized per repository instance.
- `bInitializeSchemaIfMissing` defaults to `false` and preserves the existing requirement for an already-prepared database.
- `bCreateDatabaseIfMissing` defaults to `false`; the repository creates a missing database only when both schema-init settings are explicitly enabled.
- Async callback execution currently covers only `LoadLayout` and `SaveLayout`; `LoadLayoutById`, `ListLayouts`, `RemoveLayout`, and `ClearLayouts` remain synchronous.

Unsupported behavior remains explicit. `SaveLayout`, `RemoveLayout`, and `ClearLayouts` return clear read-only failures when `bReadOnly = true`; read-only mode does not create or initialize schemas. This repository is selected by `USQLUILayoutRepositoryFactory` only when SQLite is explicitly requested. It has local smoke coverage for the current lifecycle and a first migration status/versioning helper, but broader packaged validation, full production async service behavior, and actual future schema upgrades remain future work.

## Result Types

Load and save result types live in `Plugins/SQLUI/Source/SQLUICore/Public/Layout/SQLUILayoutTypes.h`.

`FSQLUILayoutLoadResult` includes:

- `bSucceeded`
- `bBackendUnavailable`
- `ErrorMessage`
- `Document`
- `Validation`

`FSQLUILayoutSaveResult` includes:

- `bSucceeded`
- `bBackendUnavailable`
- `ErrorMessage`
- `SavedLayoutId`
- `Validation`

List, remove, and clear result types live in `Plugins/SQLUI/Source/SQLUICore/Public/Layout/SQLUILayoutRepository.h`.

`FSQLUILayoutRepositoryListResult` includes `bSucceeded`, `ErrorMessage`, and a `Layouts` metadata array.

`FSQLUILayoutRepositoryRemoveResult` includes `bSucceeded`, `ErrorMessage`, `RemovedLayoutId`, and `bRemoved`. `bRemoved` distinguishes a successful no-op from a successful delete.

`FSQLUILayoutRepositoryClearResult` includes `bSucceeded`, `ErrorMessage`, and `RemovedCount`.

Repository implementations should preserve these result semantics:

- `bSucceeded` tells the caller whether the requested operation completed successfully.
- `bBackendUnavailable` should mean the storage backend itself is unavailable, not merely that a layout id was missing or validation failed.
- `ErrorMessage` should explain failures and useful no-op warnings.
- Validation payloads should travel with load and save results whenever validation was attempted.
- Metadata arrays should be enough for layout pickers and search screens to show available layouts without loading widget trees directly.
- Removal counts should report how much state a clear operation actually deleted.

## Runtime Persistence Paths

Runtime-writable layout state belongs under `Saved/SQLUI/...`.

The JSON file layout repository defaults to:

```text
Saved/SQLUI/Layouts
```

The SQLUISamples JSON file repository smoke path configures:

```text
Saved/SQLUI/SmokeTests/Layouts
```

`Content/` and maps or levels are not runtime persistence stores. They should not be used for user-edited layouts, smoke-test writes, generated layout saves, or future SQLite database writes.

Future seed data may live in source-controlled project or plugin locations, but writable runtime copies should still be made under `Saved` before mutation.

Future user-facing database status/reset controls should follow [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) and [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md), and call the SQLUICore database management helper instead of deleting files directly. That keeps path resolution and SQLite sidecar naming in SQLUICore while preserving the repository boundary for widgets and runtime UI.

## Current Smoke-Test Paths

The local smoke-test workflow is documented in `docs/sqlui_smoke_test.md`.

Current paths are:

- Default in-memory C++ document: the sample runner creates a minimal `FSQLUILayoutDocument` in code and runs the widget pipeline.
- JSON fixture: SQLUISamples parses and validates a built-in JSON layout fixture, then runs the same widget pipeline.
- In-memory repository round trip: the factory selects `InMemory`, the JSON fixture is saved into `USQLUIInMemoryLayoutRepository`, loaded back by layout id, and passed into the widget pipeline.
- JSON file repository round trip: the factory selects `JsonFile`, the JSON fixture is saved into `USQLUIJsonFileLayoutRepository`, loaded back by layout id, removed from `Saved/SQLUI/SmokeTests/Layouts`, and passed into the widget pipeline.
- Unavailable repository selection: repository smoke paths also select `Unavailable` and verify load/save report `bBackendUnavailable` cleanly.
- Runtime repository config probe: SQLUISamples exercises `FSQLUILayoutRepositoryRuntimeConfigResolver` directly, verifies default `InMemory` behavior, JSON/SQLite command-line parsing, SQLite path resolution, missing-path unavailable behavior, invalid-backend fallback, and a factory-created SQLite save through explicit settings under `Saved/SQLUI/SmokeTests/LayoutRepositoryRuntimeConfig`.
- Runtime repository integration probe: SQLUISamples exercises `FSQLUILayoutRepositoryRuntimeIntegration`, verifies default non-SQLite `InMemory` creation, explicit SQLite repository creation/save/list, missing-path unavailable behavior without DB creation, explicit seed-copy integration, fatal missing-seed behavior before repository creation, and cleanup under `Saved/SQLUI/SmokeTests/LayoutRepositoryRuntimeIntegration`.
- Runtime repository provider probe: SQLUISamples exercises `USQLUILayoutRepositoryRuntimeProvider`, verifies default `InMemory` initialization, reset/reinitialization, explicit SQLite save/list/load, command-line SQLite initialization, seed-copy initialization/readback, fatal missing-seed behavior before repository storage, and cleanup under `Saved/SQLUI/SmokeTests/LayoutRepositoryRuntimeProvider`.
- Runtime layout persistence workflow probe: SQLUISamples exercises `FSQLUILayoutPersistenceWorkflow` above `USQLUILayoutRepositoryRuntimeSubsystem`, verifies null/missing repository failure, in-memory save/list/load, explicit SQLite save/list/load, SQLite unavailable behavior, and cleanup under `Saved/SQLUI/SmokeTests/LayoutPersistenceWorkflow`.
- Runtime database management policy probe: SQLUISamples exercises `FSQLUILayoutRepositoryDatabaseManagement`, verifies non-SQLite status/reset are safe no-ops, verifies SQLite empty path handling, checks status before and after repository save, verifies reset and idempotent reset remove the DB, verifies fake sidecar cleanup, verifies relative path resolution under `Saved/SQLUI/LayoutRepositories`, and cleans up under `Saved/SQLUI/SmokeTests/LayoutRepositoryDatabaseManagement`.
- Persistence status surface probe: SQLUISamples exercises `USQLUIPersistenceStatusLibrary`, verifies default `InMemory` status does not initialize a provider or repository, verifies pre-created SQLite database path/file/schema status is reported read-only, verifies sidecar presence is reported without file deletion, and cleans up under `Saved/SQLUI/SmokeTests/PersistenceStatusSurface`.
- Persistence status display rows probe: SQLUISamples exercises `USQLUIPersistenceStatusDisplayLibrary`, verifies default `InMemory` rows and SQLite rows are generated from the read-only status snapshot, verifies sidecar reporting, confirms row formatting does not mutate the observed database, and cleans up under `Saved/SQLUI/SmokeTests/PersistenceStatusDisplayRows`.
- Persistence status sample surface probe: SQLUISamples exercises `USQLUISamplePersistenceStatusPresenter` and `USQLUISamplePersistenceStatusPanelAdapter`, documents and validates the existing presenter/adapter refresh functions and result as reflected for Blueprint use, validates the optional `USQLUISamplePersistenceStatusPanelWidget` shell by reflection without creating a widget blueprint, map, or viewport instance, verifies explicit and repeated refresh of default `InMemory` rows and formatted lines without initializing a provider or repository, verifies the adapter rows match the presenter path, verifies repeated refresh is deterministic and does not create a DB, verifies an explicit missing SQLite path is shown gracefully without creating a DB, verifies a smoke-owned sidecar is not deleted by refresh, and cleans up under `Saved/SQLUI/SmokeTests/PersistenceStatusSampleSurface`.
- Persistence settings draft probe: SQLUISamples exercises `USQLUIPersistenceSettingsDraftLibrary`, `USQLUIPersistenceSettingsDraftDisplayLibrary`, `USQLUIPersistenceSettingsApplyPreviewDisplayLibrary`, `USQLUIPersistenceSettingsApplyContractDisplayLibrary`, `USQLUISamplePersistenceSettingsDraftPresenter`, `USQLUISamplePersistenceSettingsApplyPreviewPresenter`, `USQLUISamplePersistenceSettingsApplyContractPresenter`, `USQLUISamplePersistenceSettingsDraftPanelWidget`, and `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget`, verifies default/current draft validation and display rows, dry-run apply-preview no-change behavior, non-mutating apply/cancel contract reporting and display rows, validation/apply-preview/apply-contract sample adapter consumption, validation/apply-preview C++ UMG widget shell reflection contracts, unknown backend rejection/error display/blocked preview/blocked apply contract/contract display, SQLite draft/path display and preview without DB creation, empty SQLite path rejection/error display/blocked preview/blocked apply contract/contract display, provider auto-init pending validation/display/preview/contract/contract display without policy mutation, deterministic validation/display/preview/preview-display/contract-display/contract/adapter output, sidecar preservation, and cleanup under `Saved/SQLUI/SmokeTests/PersistenceSettingsDraft`.
- SQLite seed database copy policy probe: SQLUISamples prepares a closed seed database under `Saved/SQLUI/SmokeTests/SQLiteSeedDatabaseCopyPolicy/Seed`, exercises the SQLUICore seed copy helper for missing target, existing target without overwrite, existing target with overwrite, missing seed, same-path failure, and runtime config mapping, verifies copied targets are readable through `USQLUISQLiteLayoutRepository`, removes all probe database files, and passes the default layout through the widget pipeline.
- SQLite read-only repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteReadOnlyRepository`, instantiates `USQLUISQLiteLayoutRepository` directly, verifies `ListLayouts` metadata and tags, verifies `LoadLayout` deserializes and validates the document, verifies unsupported `SaveLayout`, `RemoveLayout`, and `ClearLayouts` calls are rejected without mutating the prepared database, removes the database, and passes the default layout through the widget pipeline.
- SQLite SaveLayout repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteSaveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, `ListLayouts`, and `LoadLayout`, saves the same layout id a second time, verifies the latest revision and updated metadata are read back, removes the database, and passes the default layout through the widget pipeline.
- SQLite RemoveLayout repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteRemoveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, `ListLayouts`, `LoadLayout`, and soft-delete `RemoveLayout`, verifies the removed layout disappears from list/load while revisions remain preserved, removes the database, and passes the default layout through the widget pipeline.
- SQLite ClearLayouts repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteClearLayoutsRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, soft-delete `RemoveLayout`, `ListLayouts`, `LoadLayout`, and destructive scoped `ClearLayouts`, verifies active and soft-deleted layout rows plus dependent schema rows are removed, removes the database, and passes the default layout through the widget pipeline.
- SQLite full lifecycle repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteFullLifecycleRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, `ListLayouts`, `LoadLayout`, revision 2 update behavior, soft-delete `RemoveLayout`, revision preservation, destructive scoped `ClearLayouts`, empty schema tables after clear, removes the database, and passes the default layout through the widget pipeline.
- SQLite async callback repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteAsyncCallbackRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false` and `bRunCallbackOperationsAsync = true`, verifies callback-style `SaveLayout` and `LoadLayout` complete through the async boundary with callbacks delivered on the game thread, verifies synchronous `ListLayouts` metadata and tags afterward, removes the database, and passes the default layout through the widget pipeline.
- SQLite serialized async callback repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteSerializedAsyncCallbackRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false` and `bRunCallbackOperationsAsync = true`, enqueues two callback-style saves and one callback-style load without waiting between calls, verifies callback order, latest revision load, updated metadata/tags, database cleanup, and passes the default layout through the widget pipeline.
- SQLite factory layout repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteFactoryRepository`, requests `ESQLUILayoutRepositoryBackend::SQLite` through `USQLUILayoutRepositoryFactory`, verifies a SQLite repository is created only with a configured database path, exercises `SaveLayout`, `ListLayouts`, `LoadLayout`, `RemoveLayout`, and `ClearLayouts`, verifies missing path selection reports unavailable behavior, removes the database, and passes the default layout through the widget pipeline.
- SQLite factory schema-init repository smoke path: SQLUISamples starts with no database under `Saved/SQLUI/SmokeTests/SQLiteFactorySchemaInitRepository`, requests `ESQLUILayoutRepositoryBackend::SQLite` through `USQLUILayoutRepositoryFactory`, enables `bInitializeSchemaIfMissing` and `bCreateDatabaseIfMissing` in `SQLiteSettings`, verifies repository `SaveLayout` creates and initializes the schema, exercises `ListLayouts`, `LoadLayout`, `RemoveLayout`, and `ClearLayouts`, verifies a missing database without schema-init settings fails without creating a file, removes database files, and passes the default layout through the widget pipeline.
- SQLite schema-init hardening proof: SQLUISamples writes only under `Saved/SQLUI/SmokeTests/SQLiteSchemaInitHardening` and verifies schema initialization refuses missing databases when creation is disabled, initializes empty databases only when creation is enabled, treats already-initialized databases idempotently without duplicate migration rows, records a missing initial migration row for a complete schema non-destructively, fails clearly for partial schemas with missing expected objects, blocks read-only repository initialization before file creation, removes database files, and passes the default layout through the widget pipeline.
- SQLite migration versioning policy proof: SQLUISamples writes only under `Saved/SQLUI/SmokeTests/SQLiteMigrationVersioningPolicy` and verifies current status for `001_initial_layout_schema`, complete-schema/missing-record detection and repair, partial-schema failure, ordered and idempotent smoke-only migrations, pending migration detection, failing migration reporting, database cleanup, and the default widget pipeline.

The default, JSON fixture, in-memory, JSON file, and unavailable paths do not use SQLite. SQLite smoke paths are optional and write only under their `Saved/SQLUI/SmokeTests/...` directories. No smoke path uses Content, maps, viewport attachment, or durable project assets.

## SQLite Runtime Status

The current SQLite runtime status is consolidated in [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md). In short, SQLite is explicitly selectable through the repository factory, supports the current layout repository lifecycle, can opt into schema initialization, has a first schema version/status helper for known migrations, and can opt into serialized async execution for callback-style `LoadLayout` and `SaveLayout`. SQLite is still not the default backend, the factory still passes settings only, and full production async service behavior plus actual future schema migration upgrades remain open.

Local packaged-build validation is documented in [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md). That script is a local `RunUAT BuildCookRun` scaffold for checking project packaging compatibility with SQLUI and SQLiteCore wiring, and it can optionally launch explicit packaged runtime smoke paths for the SQLite lifecycle, direct runtime provider startup, passive runtime provider subsystem startup, and persistence workflow Save/Verify/Cleanup across separate packaged launches. It does not add CI or change normal startup.

The SQLite schema is documented in [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md). That document defines the current planned tables, keys, indexes, revision/history behavior, soft-delete semantics for normal remove operations, destructive clear behavior for scoped cleanup, migration/versioning expectations, validation boundaries, threading expectations, and repository-operation mapping.

The async/backend implementation plan is documented in [`sqlui_sqlite_async_backend_plan.md`](sqlui_sqlite_async_backend_plan.md). That document defines the intended game-thread and worker-thread responsibilities, callback delivery rules, UObject lifetime and shutdown safety, database connection and transaction ownership, migration and seed-copy timing, result mapping, unavailable-backend behavior, logging expectations, smoke-test implications, repository factory fit, and remaining production async work.

The SQLite backend evaluation is documented in [`sqlui_sqlite_backend_evaluation.md`](sqlui_sqlite_backend_evaluation.md). That document compares engine-provided SQLite support, a SQLUI-owned wrapper around engine SQLite APIs, third-party plugins, vendored SQLite, and continued JSON-file persistence. Engine `SQLiteCore` is the active candidate and current implementation basis; local Win64 Development packaged-build and packaged runtime smoke validation exist, while broader target-platform validation and production hardening remain open.

The SQLite implementation should:

- Use async or background database boundaries. Database work must not run on the game thread.
- Keep SQL and table details inside `SQLUI.Core`, not in widgets or sample callers.
- Keep status widgets/sample presenters consuming `USQLUIPersistenceStatusDisplayLibrary` rows instead of duplicating persistence probing.
- Return the same result types and honor the same `bSucceeded`, `bBackendUnavailable`, `ErrorMessage`, and validation semantics.
- Fit the planned `Layouts.db` persistence path and gracefully report unavailable backend state when SQLite is disabled or missing.
- Support later layout lifecycle work such as revisions, history, checkpoints, tags, search metadata, and preview sessions.
- Preserve the current document validation boundary before saving and after loading.
- Use `Saved/SQLUI/...` for writable runtime database state, with explicit seed-copy behavior handled before repository mutation.

SQLite repository smoke coverage is broad enough for the current local lifecycle, and the first migration version/status framework plus explicit editor and packaged runtime provider/subsystem proofs exist. A config-backed runtime settings policy exists with safe defaults, the future user-facing persistence settings UX and read-only panel contract are documented, tiny optional read-only panel adapter/widget-shell surfaces exist for future Blueprint/UMG binding, and SQLUICore now has a non-mutating draft validation model with dry-run apply-intent preview, a non-mutating apply/cancel contract, plus UI-safe validation/apply-preview/apply-contract display rows, with validation/apply-preview/apply-contract rows consumed by SQLUISamples sample/dev presenters and validation/apply-preview widget-shell adapters plus focused safe binding guides, but actual settings apply/save execution, UI controls, reset/delete UX, future production migrations, full async database execution, broader shutdown behavior, product UI implementation, and broader packaged validation should happen in later implementation work.

## Suggested Next Steps

Near-term implementation work can stay small and repository-focused:

1. Run and expand packaged-build validation for the `SQLiteCore` path on target platforms.
2. Extend the async database boundary beyond callback-style `LoadLayout` and `SaveLayout` before using SQLite for normal runtime persistence.
3. Keep SQLite factory selection explicitly configured and unavailable when required settings are missing.
4. Implement the next persistence settings slice from [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) and [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md), building on the non-mutating draft validation model, apply-intent preview, apply/cancel contract, display rows, sample presenters, and widget shells before adding actual Apply execution or editing controls.
5. Define product seed database asset/package/version rules before shipping source-controlled seed DBs.
6. Extend lifecycle features through repository contracts instead of exposing storage details to widgets.
