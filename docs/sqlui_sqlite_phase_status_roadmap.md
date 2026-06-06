# SQLUI SQLite Phase Status And Roadmap

This document is the concise phase-level status for SQLUI's SQLite durable backend work. It summarizes what is implemented, what has local validation, which boundaries remain explicit, and which follow-up slices should come next.

For deeper reference, see:

- [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md) for the full current runtime status.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) for repository boundaries and backend selection.
- [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md) for schema details.
- [`sqlui_sqlite_async_backend_plan.md`](sqlui_sqlite_async_backend_plan.md) for async and shutdown policy.
- [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) for the future user-facing persistence/settings UX policy.
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
- A SQLUICore read-only persistence status snapshot surface now exposes backend/provider/repository/path/file/sidecar/schema status for future UI without initializing providers or mutating files.
- A SQLUICore read-only persistence status display-row adapter now converts that snapshot into label/value/state/detail rows for future UI binding without adding a widget or settings editing.
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
| Read-only persistence status surface | Implemented | Blueprint-callable SQLUICore snapshot exposes backend/provider/repository/SQLite file status without settings edits or destructive actions. |
| Read-only persistence status display rows | Implemented | Blueprint-callable SQLUICore adapter formats the status snapshot into UI-friendly rows without probing files directly or mutating state. |
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

1. Implement the production/user-facing runtime settings UI described in [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md), starting from the read-only status display rows.
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

1. First actual persistence settings widget/panel slice from [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md).
   Bind to the read-only status display rows, keep SQLite opt-in, keep `InMemory` as the safe default, and keep reset/settings editing out until confirmation and provider-shutdown policy are defined.
2. Production async service design doc or small scaffold.
   Decide whether the current per-repository callback queue is enough or whether SQLUI needs a longer-lived DB service for production runtime use.
3. SQLite history/checkpoint/previews API planning.
   Keep this docs/design first, then add repository contracts only when product workflows need them.
4. Broader packaged validation target matrix docs.
   Record intended platforms and local validation commands without adding CI yet.
5. First real v2 migration.
   Add this only when there is an actual schema change and a matching data-transform/compatibility policy.
