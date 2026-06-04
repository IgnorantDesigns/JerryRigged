# SQLUI SQLite Runtime Status

This is the current-state reference for SQLUI's SQLite layout repository work. It consolidates the implementation status that is spread across the repository architecture, schema, async plan, backend evaluation, and smoke-test docs.

Related docs:

- [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md) summarizes the current phase status, validation matrix, safety boundaries, and recommended next slices.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) describes the repository boundary and available backends.
- [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md) describes the SQLite schema and repository-operation mapping.
- [`sqlui_sqlite_async_backend_plan.md`](sqlui_sqlite_async_backend_plan.md) describes the async/threading direction.
- [`sqlui_sqlite_backend_evaluation.md`](sqlui_sqlite_backend_evaluation.md) describes why engine `SQLiteCore` is the active backend candidate.
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

## Runtime Boundaries

SQLite details belong in SQLUICore and the SQLUISamples smoke harness only. Widgets should continue to work with repository contracts, layout documents, variable stores, action systems, and runtime contexts.

Runtime-writable databases should live under `Saved/SQLUI/...`. Smoke tests use narrower `Saved/SQLUI/SmokeTests/...` folders. SQLite runtime paths should not write to `Content/`, maps, plugin content, generated folders, or source-controlled database files.

JerryRigged remains a thin host. It should not own SQLite schema, SQL strings, migration logic, worker details, or storage-specific widget behavior.

## Remaining Work

The SQLite path is implemented enough for repository-shaped local smoke coverage, but it is not yet a production-hardened packaged persistence system.

Remaining work includes:

- Expanding packaged-build validation beyond the latest local Win64 Development pass.
- Expanding packaged runtime SQLite lifecycle coverage beyond the first local packaged executable smoke.
- Production async database service design beyond the current per-repository callback queue.
- Cancellation, shutdown draining beyond stale-callback suppression, and async coverage for all repository operations.
- Actual future schema migrations, upgrade-specific data transforms, and version-specific compatibility policy beyond the current version/status framework.
- User-facing runtime configuration surfaces, production database path policy, and product startup code that intentionally invokes the runtime integration helper.
- Product seed database asset/package/version policy, if seed DBs are added.
- Optional lifecycle features such as history APIs, checkpoints, previews, restore flows, and richer search.

## Packaged Validation Status

Local smoke tests prove commandlet behavior and temporary database cleanup. The packaged-build validation scaffold in [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md) provides a repeatable local `RunUAT BuildCookRun` command for checking package compatibility with SQLUI and SQLiteCore wiring.

The latest local Win64 Development packaged-build validation passed after installing the UE 5.7-preferred Visual Studio 2022 MSVC `14.44.x` toolchain. That pass proves the local BuildCookRun path can build, cook, stage, package, and archive with SQLUI and SQLiteCore wiring enabled.

The scaffold can also run the packaged executable with `-SQLUIPackagedRuntimeSQLiteSmoke` to prove one packaged runtime SQLite lifecycle against a packaged runtime `Saved/SQLUI/PackagedRuntimeSmoke/...` database path. That runtime smoke covers save, list, load, remove, clear, log verification, and database cleanup.

It still does not prove platform coverage beyond the requested local target, long-running database service behavior, full async lifecycle handling, or production migration upgrades.

Until those items are validated, SQLite should stay explicitly configured, not default.
