# SQLUI Repository Architecture

SQLUI layout persistence sits behind repository interfaces. Runtime widget code should ask for validated layout documents through repositories and related runtime services. It should not issue raw SQL, know database table shapes, or choose storage backends directly.

This boundary keeps `SQLUI.Widgets` focused on building and updating UMG widgets, keeps `SQLUI.Core` responsible for data access contracts, and lets the project swap storage implementations without rewriting widget or runtime pipeline code.

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

`FSQLUILayoutRepositoryFactorySettings` also includes an optional `JsonFileBaseDirectory`. When it is set, the JSON file repository is configured with that directory. When it is empty, the JSON file repository keeps its default `Saved/SQLUI/Layouts` path.

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

This repository is suitable for lightweight runtime persistence and local development workflows. It is not a replacement for the future SQLite-backed persistence path.

### `USQLUISQLiteLayoutRepository`

`USQLUISQLiteLayoutRepository` is the first repository-shaped SQLite implementation in SQLUICore. It supports read operations, writable `SaveLayout`, and soft-delete `RemoveLayout` when explicitly configured writable.

The repository is configured with `FSQLUISQLiteLayoutRepositorySettings`, including a `DatabasePath` and `bReadOnly`. It opens the configured database for each operation. It does not create a database, create schema tables, run migrations, seed data, or select itself through the repository factory.

Current supported behavior:

- `ListLayouts` reads active rows from `layouts` where `b_deleted = 0`.
- `ListLayouts` reads metadata columns from `layouts` and tags from `layout_tags`.
- `ListLayouts` preserves the planned ordering by `display_name COLLATE NOCASE ASC, layout_id COLLATE NOCASE ASC`.
- `LoadLayout` reads the current revision document JSON by joining `layouts.current_revision` to `layout_revisions.revision`.
- `LoadLayout` deserializes with `FSQLUILayoutJson` and validates after load.
- `SaveLayout` works only when `bReadOnly = false`, `DatabasePath` is configured, and the database already exists with the planned layout schema.
- `SaveLayout` validates the document, serializes canonical JSON, computes the next revision from `layout_revisions`, upserts `layouts`, inserts an immutable `layout_revisions` row, replaces `layout_tags`, and commits the transaction.
- `RemoveLayout` works only when `bReadOnly = false`, `DatabasePath` is configured, and the database already exists with the planned layout schema.
- `RemoveLayout` soft-deletes active rows by setting `layouts.b_deleted = 1`; revisions, tags, checkpoints, and previews remain intact.

Unsupported behavior remains explicit. `SaveLayout` and `RemoveLayout` return clear read-only failures when `bReadOnly = true`; `ClearLayouts` still returns a clear unsupported/read-only failure. This repository is not selected by `USQLUILayoutRepositoryFactory` yet and should not be treated as complete durable SQLite layout persistence.

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
- SQLite read-only repository proof: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteReadOnlyRepository`, instantiates `USQLUISQLiteLayoutRepository` directly, verifies `ListLayouts` metadata and tags, verifies `LoadLayout` deserializes and validates the document, verifies unsupported `SaveLayout`, `RemoveLayout`, and `ClearLayouts` calls are rejected without mutating the prepared database, removes the database, and passes the default layout through the widget pipeline.
- SQLite SaveLayout repository proof: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteSaveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, `ListLayouts`, and `LoadLayout`, saves the same layout id a second time, verifies the latest revision and updated metadata are read back, removes the database, and passes the default layout through the widget pipeline.
- SQLite RemoveLayout repository proof: SQLUISamples prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteRemoveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, verifies `SaveLayout`, `ListLayouts`, `LoadLayout`, and soft-delete `RemoveLayout`, verifies the removed layout disappears from list/load while revisions remain preserved, removes the database, and passes the default layout through the widget pipeline.

The default, JSON fixture, in-memory, JSON file, and unavailable paths do not use SQLite. SQLite smoke paths are optional and write only under their `Saved/SQLUI/SmokeTests/...` directories. No smoke path uses Content, maps, viewport attachment, or durable project assets.

## Future SQLite Repository Direction

The SQLite repository proof now sits behind the same repository shape for `ListLayouts`, `LoadLayout`, writable `SaveLayout`, and soft-delete `RemoveLayout`, but full SQLite persistence remains future work. Callers should eventually be able to request, save, list, remove, and clear layouts without knowing whether the backing store is in memory, JSON files, or SQLite.

The proposed SQLite schema is drafted in [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md). That document defines the planned tables, keys, indexes, revision/history behavior, soft-delete semantics for normal remove operations, destructive clear behavior for scoped cleanup, migration/versioning expectations, validation boundaries, threading expectations, and repository-operation mapping.

The future async/backend implementation plan is drafted in [`sqlui_sqlite_async_backend_plan.md`](sqlui_sqlite_async_backend_plan.md). That document defines the intended game-thread and worker-thread responsibilities, callback delivery rules, UObject lifetime and shutdown safety, database connection and transaction ownership, migration and seed-copy timing, result mapping, unavailable-backend behavior, logging expectations, smoke-test implications, repository factory fit, and criteria for choosing a concrete SQLite backend later.

The SQLite backend evaluation is drafted in [`sqlui_sqlite_backend_evaluation.md`](sqlui_sqlite_backend_evaluation.md). That document compares engine-provided SQLite support, a SQLUI-owned wrapper around engine SQLite APIs, third-party plugins, vendored SQLite, and continued JSON-file persistence. The current docs-only recommendation is to prefer the locally verified engine `SQLiteCore` runtime plugin behind a small SQLUI-owned async wrapper if packaged-build and smoke-test verification pass, and otherwise defer SQLite while continuing JSON-file persistence.

The SQLite implementation should:

- Use async or background database boundaries. Database work must not run on the game thread.
- Keep SQL and table details inside `SQLUI.Core`, not in widgets or sample callers.
- Return the same result types and honor the same `bSucceeded`, `bBackendUnavailable`, `ErrorMessage`, and validation semantics.
- Fit the planned `Layouts.db` persistence path and gracefully report unavailable backend state when SQLite is disabled or missing.
- Support later layout lifecycle work such as revisions, history, checkpoints, tags, search metadata, and preview sessions.
- Preserve the current document validation boundary before saving and after loading.
- Use `Saved/SQLUI/...` for writable runtime database state, with any seed-copy behavior handled before mutation.

SQLite persistence is still incomplete. Factory selection, destructive scoped `ClearLayouts`, production migration integration, async database execution, and packaged-build validation should happen in later implementation work.

## Suggested Next Steps

Near-term implementation work can stay small and repository-focused:

1. Choose a SQLite backend only after the schema, async boundaries, backend selection criteria, and backend evaluation blockers are resolved.
2. Move the SQLite repository behind the planned async database boundary before using it for runtime persistence.
3. Extend repository selection with a SQLite backend setting only when the repository is ready for normal runtime use.
4. Add executable migrations and database file handling in SQLUICore, not in widgets.
5. Extend lifecycle features through repository contracts instead of exposing storage details to widgets.
