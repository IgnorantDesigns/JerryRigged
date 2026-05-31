# SQLUI Repository Architecture

SQLUI layout persistence sits behind repository interfaces. Runtime widget code should ask for validated layout documents through repositories and related runtime services. It should not issue raw SQL, know database table shapes, or choose storage backends directly.

This boundary keeps `SQLUI.Widgets` focused on building and updating UMG widgets, keeps `SQLUI.Core` responsible for data access contracts, and lets the project swap storage implementations without rewriting widget or runtime pipeline code.

For the consolidated current SQLite runtime state, see [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md).

## Repository Boundary

`ISQLUILayoutRepository` is the current layout repository contract. It exposes:

- `LoadLayout(LayoutId, Callback)`
- `SaveLayout(Document, Callback)`

The callback shape is important even while the current concrete repositories complete immediately. Future persistent repositories can use the same contract while doing database or file work through appropriate background boundaries.

Widgets, widget factories, and runtime pipeline code should depend on layout documents, repositories, variable stores, action systems, and runtime context objects. They should not depend on raw SQL strings, SQLite connection objects, table names, or direct file paths.

## Repository Selection

Runtime-facing code can choose a repository backend through `FSQLUILayoutRepositoryFactorySettings` and `USQLUILayoutRepositoryFactory`.

`ESQLUILayoutRepositoryBackend` currently supports:

- `Unavailable`: creates `USQLUILayoutRepository`, which reports `bBackendUnavailable` for load and save.
- `InMemory`: creates `USQLUIInMemoryLayoutRepository`.
- `JsonFile`: creates `USQLUIJsonFileLayoutRepository`.
- `SQLite`: creates `USQLUISQLiteLayoutRepository` only when explicitly requested and configured with a non-empty database path.

`FSQLUILayoutRepositoryFactorySettings` also includes an optional `JsonFileBaseDirectory`. When it is set, the JSON file repository is configured with that directory. When it is empty, the JSON file repository keeps its default `Saved/SQLUI/Layouts` path.

`FSQLUILayoutRepositoryFactorySettings` includes `SQLiteSettings` for the SQLite backend. Those settings are passed to `USQLUISQLiteLayoutRepository` without running migrations or creating database files in the factory. If `Backend = SQLite` is requested with an empty `SQLiteSettings.DatabasePath`, the factory returns the existing unavailable repository behavior instead of silently falling back to another backend. Schema initialization remains repository-owned and opt-in through SQLite settings.

The factory falls back to the transient package when no valid outer is supplied. It is the runtime selection boundary for current sample code and later storage implementations. Widgets should stay behind repository/runtime-context APIs and should not select concrete storage classes themselves.

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

SQLite database operation logic lives in the non-UObject `FSQLUISQLiteLayoutRepositoryWorker` helper. The helper accepts plain settings/request data and returns plain repository result structs, which keeps SQLite paths, connection handling, SQL statements, transactions, validation, and serialization outside the UObject wrapper. `USQLUISQLiteLayoutRepository` calls the helper synchronously by default. When `bRunCallbackOperationsAsync = true`, only the callback-style `LoadLayout` and `SaveLayout` methods run the worker helper through the SQLUI database async boundary and marshal results back to the game thread before invoking callbacks. Direct return-value methods remain synchronous.

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
- `bInitializeSchemaIfMissing` defaults to `false` and preserves the existing requirement for an already-prepared database.
- `bCreateDatabaseIfMissing` defaults to `false`; the repository creates a missing database only when both schema-init settings are explicitly enabled.
- Async callback execution currently covers only `LoadLayout` and `SaveLayout`; `LoadLayoutById`, `ListLayouts`, `RemoveLayout`, and `ClearLayouts` remain synchronous.

Unsupported behavior remains explicit. `SaveLayout`, `RemoveLayout`, and `ClearLayouts` return clear read-only failures when `bReadOnly = true`; read-only mode does not create or initialize schemas. This repository is selected by `USQLUILayoutRepositoryFactory` only when SQLite is explicitly requested. It has local smoke coverage for the current lifecycle, but packaged-build validation, full production async service behavior, and migration upgrades beyond the initial schema remain future work.

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

## Current Smoke-Test Paths

The local smoke-test workflow is documented in `docs/sqlui_smoke_test.md`.

Current paths are:

- Default in-memory C++ document: the sample runner creates a minimal `FSQLUILayoutDocument` in code and runs the widget pipeline.
- JSON fixture: SQLUISamples parses and validates a built-in JSON layout fixture, then runs the same widget pipeline.
- In-memory repository round trip: the factory selects `InMemory`, the JSON fixture is saved into `USQLUIInMemoryLayoutRepository`, loaded back by layout id, and passed into the widget pipeline.
- JSON file repository round trip: the factory selects `JsonFile`, the JSON fixture is saved into `USQLUIJsonFileLayoutRepository`, loaded back by layout id, removed from `Saved/SQLUI/SmokeTests/Layouts`, and passed into the widget pipeline.
- Unavailable repository selection: repository smoke paths also select `Unavailable` and verify load/save report `bBackendUnavailable` cleanly.
- SQLite read-only repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteReadOnlyRepository`, instantiates `USQLUISQLiteLayoutRepository` directly, verifies `ListLayouts` metadata and tags, verifies `LoadLayout` deserializes and validates the document, verifies unsupported `SaveLayout`, `RemoveLayout`, and `ClearLayouts` calls are rejected without mutating the prepared database, removes the database, and passes the default layout through the widget pipeline.
- SQLite SaveLayout repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteSaveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, `ListLayouts`, and `LoadLayout`, saves the same layout id a second time, verifies the latest revision and updated metadata are read back, removes the database, and passes the default layout through the widget pipeline.
- SQLite RemoveLayout repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteRemoveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, `ListLayouts`, `LoadLayout`, and soft-delete `RemoveLayout`, verifies the removed layout disappears from list/load while revisions remain preserved, removes the database, and passes the default layout through the widget pipeline.
- SQLite ClearLayouts repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteClearLayoutsRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, soft-delete `RemoveLayout`, `ListLayouts`, `LoadLayout`, and destructive scoped `ClearLayouts`, verifies active and soft-deleted layout rows plus dependent schema rows are removed, removes the database, and passes the default layout through the widget pipeline.
- SQLite full lifecycle repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteFullLifecycleRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, `ListLayouts`, `LoadLayout`, revision 2 update behavior, soft-delete `RemoveLayout`, revision preservation, destructive scoped `ClearLayouts`, empty schema tables after clear, removes the database, and passes the default layout through the widget pipeline.
- SQLite async callback repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteAsyncCallbackRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false` and `bRunCallbackOperationsAsync = true`, verifies callback-style `SaveLayout` and `LoadLayout` complete through the async boundary with callbacks delivered on the game thread, verifies synchronous `ListLayouts` metadata and tags afterward, removes the database, and passes the default layout through the widget pipeline.
- SQLite factory layout repository smoke path: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteFactoryRepository`, requests `ESQLUILayoutRepositoryBackend::SQLite` through `USQLUILayoutRepositoryFactory`, verifies a SQLite repository is created only with a configured database path, exercises `SaveLayout`, `ListLayouts`, `LoadLayout`, `RemoveLayout`, and `ClearLayouts`, verifies missing path selection reports unavailable behavior, removes the database, and passes the default layout through the widget pipeline.
- SQLite factory schema-init repository smoke path: SQLUISamples starts with no database under `Saved/SQLUI/SmokeTests/SQLiteFactorySchemaInitRepository`, requests `ESQLUILayoutRepositoryBackend::SQLite` through `USQLUILayoutRepositoryFactory`, enables `bInitializeSchemaIfMissing` and `bCreateDatabaseIfMissing` in `SQLiteSettings`, verifies repository `SaveLayout` creates and initializes the schema, exercises `ListLayouts`, `LoadLayout`, `RemoveLayout`, and `ClearLayouts`, verifies a missing database without schema-init settings fails without creating a file, removes database files, and passes the default layout through the widget pipeline.
- SQLite schema-init hardening proof: SQLUISamples writes only under `Saved/SQLUI/SmokeTests/SQLiteSchemaInitHardening` and verifies schema initialization refuses missing databases when creation is disabled, initializes empty databases only when creation is enabled, treats already-initialized databases idempotently without duplicate migration rows, records a missing initial migration row for a complete schema non-destructively, fails clearly for partial schemas with missing expected objects, blocks read-only repository initialization before file creation, removes database files, and passes the default layout through the widget pipeline.

The default, JSON fixture, in-memory, JSON file, and unavailable paths do not use SQLite. SQLite smoke paths are optional and write only under their `Saved/SQLUI/SmokeTests/...` directories. No smoke path uses Content, maps, viewport attachment, or durable project assets.

## SQLite Runtime Status

The current SQLite runtime status is consolidated in [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md). In short, SQLite is explicitly selectable through the repository factory, supports the current layout repository lifecycle, can opt into schema initialization, and can opt into async execution for callback-style `LoadLayout` and `SaveLayout`. SQLite is still not the default backend, the factory still passes settings only, and production packaging, full async service behavior, and migration upgrades beyond the initial schema remain open.

The SQLite schema is documented in [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md). That document defines the current planned tables, keys, indexes, revision/history behavior, soft-delete semantics for normal remove operations, destructive clear behavior for scoped cleanup, migration/versioning expectations, validation boundaries, threading expectations, and repository-operation mapping.

The async/backend implementation plan is documented in [`sqlui_sqlite_async_backend_plan.md`](sqlui_sqlite_async_backend_plan.md). That document defines the intended game-thread and worker-thread responsibilities, callback delivery rules, UObject lifetime and shutdown safety, database connection and transaction ownership, migration and seed-copy timing, result mapping, unavailable-backend behavior, logging expectations, smoke-test implications, repository factory fit, and remaining production async work.

The SQLite backend evaluation is documented in [`sqlui_sqlite_backend_evaluation.md`](sqlui_sqlite_backend_evaluation.md). That document compares engine-provided SQLite support, a SQLUI-owned wrapper around engine SQLite APIs, third-party plugins, vendored SQLite, and continued JSON-file persistence. Engine `SQLiteCore` is the active candidate and current implementation basis; packaged-build validation and production hardening remain open.

The SQLite implementation should:

- Use async or background database boundaries. Database work must not run on the game thread.
- Keep SQL and table details inside `SQLUI.Core`, not in widgets or sample callers.
- Return the same result types and honor the same `bSucceeded`, `bBackendUnavailable`, `ErrorMessage`, and validation semantics.
- Fit the planned `Layouts.db` persistence path and gracefully report unavailable backend state when SQLite is disabled or missing.
- Support later layout lifecycle work such as revisions, history, checkpoints, tags, search metadata, and preview sessions.
- Preserve the current document validation boundary before saving and after loading.
- Use `Saved/SQLUI/...` for writable runtime database state, with any seed-copy behavior handled before mutation.

SQLite repository smoke coverage is broad enough for the current local lifecycle, but production migration versioning and upgrade paths, full async database execution, shutdown behavior, and packaged-build validation should happen in later implementation work.

## Suggested Next Steps

Near-term implementation work can stay small and repository-focused:

1. Validate the `SQLiteCore` path in packaged builds for the target platforms.
2. Extend the async database boundary beyond callback-style `LoadLayout` and `SaveLayout` before using SQLite for normal runtime persistence.
3. Keep SQLite factory selection explicitly configured and unavailable when required settings are missing.
4. Harden migration versioning, upgrade behavior, and database file handling in SQLUICore, not in widgets.
5. Extend lifecycle features through repository contracts instead of exposing storage details to widgets.
