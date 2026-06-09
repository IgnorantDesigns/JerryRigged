# SQLUI SQLite Backend Evaluation

This document records the SQLite backend evaluation that led SQLUI to use engine `SQLiteCore` as the active runtime candidate. The original backend evaluation was documentation-only; the current code now includes SQLiteCore wiring, repository operations, factory selection, an explicit runtime configuration resolver, an explicit runtime integration helper above resolver/seed-copy/factory creation, a storage-agnostic runtime repository provider, config-backed runtime repository settings with safe defaults, a passive runtime provider `GameInstance` subsystem, a non-mutating persistence settings draft/validation model plus validation display rows, a sample/dev adapter for those rows, an optional C++ UMG widget shell contract for draft validation display, an explicit seed database copy policy helper, opt-in schema initialization, a migration version/status helper for the known layout schema migration set, serialized opt-in async callback execution with queue shutdown/stale-callback suppression for `LoadLayout` and `SaveLayout`, local smoke coverage, a local packaged-build validation scaffold, an opt-in packaged runtime SQLite lifecycle smoke, an opt-in packaged runtime provider startup smoke, and an opt-in packaged runtime provider subsystem smoke. SQLite is still not the default backend, and this work still does not add migrations inside the factory, settings editing UI controls, maps, assets, CI, source-controlled seed databases, or persistent database files.

For the concise phase status and roadmap, see [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md).
For the consolidated current implementation status, see [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md).
For the planned persistence settings editing/reset UX phase, see [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md).
For the local packaged-build validation scaffold, see [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md).

## Purpose

SQLUI needs a durable layout repository that stores the schema documented in [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md) and follows the async boundary documented in [`sqlui_sqlite_async_backend_plan.md`](sqlui_sqlite_async_backend_plan.md).

This evaluation compares backend options and captures why the smallest safe path is engine `SQLiteCore` behind SQLUI-owned repository and worker boundaries while keeping:

- SQL and database details inside `SQLUI.Core`.
- Widgets unaware of SQL, table names, database paths, SQLite connections, worker threads, or concrete storage classes.
- JerryRigged as a thin host.
- Current in-memory and JSON-file repository behavior unchanged.

## Non-Goals

- Do not treat current local smoke coverage as packaged-build or production-service validation.
- Do not treat the local packaged-build pass by itself as packaged runtime SQLite lifecycle validation.
- Do not make SQLite the default repository backend.
- Do not create SQLite databases unless repository schema initialization and database creation are both explicitly enabled.
- Do not add dependencies beyond the engine `SQLiteCore` plugin/module wiring already used by SQLUICore.
- Do not add Marketplace dependencies, third-party Unreal plugins, or vendored third-party source.
- Do not modify widgets, CI, assets, maps, or persistent database files.
- Do not open, create, or write SQLite databases outside the optional smoke-safe probe paths under `Saved/SQLUI/SmokeTests/...`.
- Do not treat one local packaged-build pass as full target-platform packaged readiness.

## Local Verification

The local Unreal Engine install was inspected at:

```text
C:\Program Files\Epic Games\UE_5.7
```

The install includes engine-provided SQLite-related runtime database plugins:

```text
Engine/Plugins/Runtime/Database/SQLiteCore/SQLiteCore.uplugin
Engine/Plugins/Runtime/Database/SQLiteSupport/SQLiteSupport.uplugin
```

Verified local findings:

- `SQLiteCore.uplugin` declares a `SQLiteCore` module with `"Type": "Runtime"` and `"EnabledByDefault": false`.
- `SQLiteCore.uplugin` describes the plugin as a lightweight C++ wrapper for creating and manipulating SQLite databases.
- `SQLiteCore` uses SQLite `3.47.1` in the local UE 5.7 install, with license material referenced under `Engine/Source/ThirdParty/Licenses/SQLite_v3.47.1.license`.
- `SQLiteCore` public headers include `SQLiteDatabase.h`, `SQLitePreparedStatement.h`, `SQLiteTypes.h`, and `IncludeSQLite.h`.
- `FSQLiteDatabase` exposes open modes, `Open`, `Close`, `Execute`, `PrepareStatement`, `GetLastError`, `GetLastInsertRowId`, `PerformQuickIntegrityCheck`, `GetUserVersion`, `SetUserVersion`, `GetApplicationId`, and `SetApplicationId`.
- `FSQLitePreparedStatement` exposes typed binding, typed column reading, `Step`, `Execute`, `ExecuteSingle`, reset, clear-bindings, and read-only checks.
- `SQLiteCore.Build.cs` enables SQLite FTS4, FTS5, deserialize, RTree, and JSON1 support in this engine install.
- `SQLiteCore.Build.cs` notes that when Unreal's custom SQLite platform is used, shared memory and granular file locks are not provided, affecting concurrency so only one `FSQLiteDatabase` can have a file open at the same time in that mode.
- `SQLiteSupport.uplugin` declares a `SQLiteSupport` module with `"Type": "Runtime"` and depends on the engine `DatabaseSupport` and `SQLiteCore` plugins.
- `SQLiteSupport` public headers expose `FSQLiteDatabaseConnection` and `FSQLiteResultSet` through the older `FDataBaseConnection` and `FDataBaseRecordSet` style abstraction.
- At the time of the original evaluation, the JerryRigged project and SQLUI plugin did not declare SQLite plugin/module dependencies. The availability proof below now adds minimal `SQLiteCore` wiring to SQLUI only.

Important limitations from the original local inspection:

- That inspection was a local source/header/descriptor inspection, not a packaged build test.
- A later local packaged-build validation pass now proves UE 5.7 Win64 Development BuildCookRun compatibility for this checkout after installing the preferred MSVC `14.44.x` toolchain.
- Target-platform packaging and packaged runtime SQLite lifecycle behavior beyond the first local Win64 Development proof remain unverified.
- Threading guarantees beyond the public API and build comments still need production hardening around SQLUI's worker boundary.
- No third-party Marketplace plugin was inspected locally for this evaluation.

## SQLiteCore Availability Proof

SQLUICore includes a minimal compile-time availability probe for engine-provided `SQLiteCore`.

The proof:

- Adds the smallest dependency wiring needed for SQLUICore to compile against `SQLiteCore`.
- Adds `FSQLUISQLiteAvailability` in SQLUICore.
- Includes `SQLiteDatabase.h` and compiles a trivial `FSQLiteDatabase` wrapper check.
- Reports a short availability summary without opening a database.

The proof does not:

- Add a SQLite layout repository.
- Add `ESQLUILayoutRepositoryBackend::SQLite`.
- Change `USQLUILayoutRepositoryFactory`.
- Open, create, or write database files.
- Execute SQL.
- Run migrations.
- Add SQLite database workers.
- Modify widgets or smoke-test behavior.

SQLUICore also includes an optional open/close probe for engine-provided `SQLiteCore`.

The open/close proof:

- Resolves a runtime-writable probe database path under `Saved/SQLUI/SmokeTests/SQLiteCoreProbe`.
- Creates the probe directory when needed.
- Opens `SQLiteCoreProbe.db` with `FSQLiteDatabase`.
- Runs a safe SQLiteCore quick integrity check without creating SQLUI schema tables.
- Closes the database.
- Removes the probe database file by default after the check.
- Is available only through the optional smoke-test flag.

The open/close proof does not:

- Add a SQLite layout repository.
- Add `ESQLUILayoutRepositoryBackend::SQLite`.
- Change `USQLUILayoutRepositoryFactory`.
- Run SQLUI migrations.
- Create SQLUI schema tables.
- Add SQLite database workers.
- Modify widgets or default smoke-test behavior.

SQLUICore also includes an optional async-boundary probe.

The async-boundary proof:

- Runs plain database-style request data on a background task.
- Marshals the result back through a game-thread callback.
- Reports whether background work completed and whether the callback was delivered on the game thread.
- Is available only through the optional smoke-test flag.

The async-boundary proof does not:

- Open SQLite databases.
- Execute SQL.
- Create, write, or delete database files.
- Add migrations.
- Add a SQLite layout repository.
- Change `USQLUILayoutRepositoryFactory`.
- Modify widgets or default smoke-test behavior.

SQLUICore also includes an optional smoke-only migration-runner probe.

The migration-runner proof:

- Resolves a runtime-writable probe database path under `Saved/SQLUI/SmokeTests/SQLiteMigrationProbe`.
- Opens `SQLiteMigrationProbe.db` with `FSQLiteDatabase`.
- Creates a minimal smoke-only `sqlui_schema_migrations` tracking table.
- Applies one tiny probe migration that creates `sqlui_migration_probe`.
- Records the probe migration id and verifies the recorded migration row.
- Closes the database.
- Removes the probe database file by default after the check.
- Is available only through the optional smoke-test flag.

The migration-runner proof does not:

- Add a SQLite layout repository.
- Add `ESQLUILayoutRepositoryBackend::SQLite`.
- Change `USQLUILayoutRepositoryFactory`.
- Implement the real layout schema migration.
- Create layout tables.
- Add async SQLite database workers.
- Modify widgets or default smoke-test behavior.

SQLUICore also includes an optional layout schema migration proof.

The layout schema migration proof:

- Resolves a runtime-writable probe database path under `Saved/SQLUI/SmokeTests/LayoutSchemaMigrationProbe`.
- Applies the planned initial layout schema migration id `001_initial_layout_schema`.
- Creates the planned layout tables and recommended indexes.
- Verifies the expected tables and indexes exist.
- Closes the database and removes the probe database file by default after the check.
- Is available only through the optional smoke-test flag.

The layout schema migration proof does not:

- Add a SQLite layout repository.
- Add `ESQLUILayoutRepositoryBackend::SQLite`.
- Change `USQLUILayoutRepositoryFactory`.
- Implement `SaveLayout`, `LoadLayout`, `ListLayouts`, `RemoveLayout`, or `ClearLayouts` against SQLite.
- Add async SQLite database workers.
- Modify widgets or default smoke-test behavior.

SQLUICore also includes an optional layout read proof.

The layout read proof:

- Resolves a runtime-writable probe database path under `Saved/SQLUI/SmokeTests/LayoutReadProbe`.
- Applies the planned initial layout schema through the existing schema migration helper.
- Seeds one valid probe-only layout document into `layouts`, `layout_revisions`, and `layout_tags`.
- Reads list-style metadata for non-deleted layouts.
- Loads the current revision document JSON by layout id.
- Deserializes and validates the loaded SQLUI layout document.
- Closes the database and removes the probe database file by default after the check.
- Is available only through the optional smoke-test flag.

The layout read proof does not:

- Add a SQLite layout repository.
- Add `ESQLUILayoutRepositoryBackend::SQLite`.
- Change `USQLUILayoutRepositoryFactory`.
- Implement repository callbacks for SQLite.
- Implement `SaveLayout`, `RemoveLayout`, or `ClearLayouts` against SQLite.
- Add async SQLite database workers.
- Modify widgets or default smoke-test behavior.

SQLUICore also includes a SQLite layout repository implementation.

The repository implementation:

- Adds `USQLUISQLiteLayoutRepository` in SQLUICore.
- Opens an already-prepared SQLite database with `SQLiteCore`.
- Implements repository-shaped `ListLayouts` and `LoadLayout` reads.
- Implements `SaveLayout`, soft-delete `RemoveLayout`, and destructive scoped `ClearLayouts` when configured with `bReadOnly = false`.
- Reads metadata from `layouts`, tags from `layout_tags`, and current document JSON from `layout_revisions`.
- Saves validated documents by upserting `layouts`, inserting immutable `layout_revisions`, replacing `layout_tags`, and committing a transaction.
- Soft-deletes active layouts by setting `layouts.b_deleted = 1` while preserving revisions and tags.
- Deserializes and validates loaded documents with existing SQLUI layout JSON helpers.
- Keeps `SaveLayout`, `RemoveLayout`, and `ClearLayouts` explicitly rejected when configured read-only.
- Is available through an optional smoke-test flag that prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteReadOnlyRepository`, verifies read-only write rejection, verifies the layout remains readable afterward, and removes the database.
- Is available through a second optional smoke-test flag that prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteSaveLayoutRepository`, verifies `SaveLayout`, verifies `ListLayouts` and `LoadLayout`, saves the same layout id again, verifies the latest revision and updated metadata are read back, and removes the database.
- Is available through a third optional smoke-test flag that prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteRemoveLayoutRepository`, verifies `SaveLayout`, `ListLayouts`, `LoadLayout`, and soft-delete `RemoveLayout`, verifies the removed layout disappears from list/load, verifies revisions remain preserved, and removes the database.
- Is available through a fourth optional smoke-test flag that prepares a temporary database under `Saved/SQLUI/SmokeTests/SQLiteClearLayoutsRepository`, verifies `SaveLayout`, soft-delete `RemoveLayout`, `ListLayouts`, `LoadLayout`, and destructive scoped `ClearLayouts`, verifies active and soft-deleted layout rows plus dependent schema rows are removed, and removes the database.
- Is available through a full lifecycle optional smoke-test flag that verifies save, list, load, revision update, soft-delete remove, clear, revision preservation, and empty schema tables after clear.
- Supports serialized opt-in async callback execution for SQLite `LoadLayout` and `SaveLayout`, including queue shutdown behavior that rejects new work, drops pending work, and suppresses stale completions.
- Is selectable through `USQLUILayoutRepositoryFactory` only when `ESQLUILayoutRepositoryBackend::SQLite` is explicitly requested and `SQLiteSettings.DatabasePath` is configured.
- Has optional smoke coverage for factory-created SQLite repository lifecycle behavior and missing-path unavailable behavior.
- Supports opt-in schema initialization through repository settings, allowing a writable repository to create and initialize an empty configured database only when `bInitializeSchemaIfMissing` and `bCreateDatabaseIfMissing` are both enabled.
- Has optional smoke coverage for factory-created SQLite schema initialization and missing-database-without-init failure behavior.
- Has optional smoke coverage for schema initialization hardening: missing database with creation disabled, empty database with creation enabled, already-initialized database idempotence, complete schema with missing migration record, partial schema failure behavior, and read-only init-blocked protection.
- Has a SQLUICore layout schema version/status helper for the known migration set, currently only `001_initial_layout_schema`.
- Has optional smoke coverage for current schema status, complete-schema/missing-record repair, partial schema failure, ordered smoke-only migrations, pending migration detection, idempotent migration reruns, and failing migration reporting.
- Has a SQLUICore runtime configuration resolver that maps explicit backend, path, and SQLite command-line flags into repository factory settings while keeping `InMemory` as the default and SQLite non-default.
- Has optional smoke coverage for runtime config parsing, SQLite path resolution, missing-path unavailable behavior, invalid-backend fallback, and a factory-created SQLite save through explicit runtime config.
- Has a SQLUICore runtime integration helper that can run explicit seed-copy policy, then create the requested repository through the factory while preserving the default `InMemory` behavior and unavailable behavior for explicit SQLite requests with no database path.
- Has optional smoke coverage for default in-memory runtime integration, explicit SQLite creation/save/list, missing-path unavailable behavior, explicit seed-copy repository creation, fatal missing-seed handling, and cleanup.
- Has a SQLUICore runtime repository provider that stores an explicitly initialized active repository and last integration result while delegating repository creation to the runtime integration helper.
- Has optional smoke coverage for default provider initialization, reset/reinitialization, explicit SQLite save/list/load, command-line SQLite initialization, explicit seed-copy initialization/readback, fatal missing-seed handling, and cleanup.
- Has a SQLUICore seed database copy policy helper that can explicitly copy a closed seed DB into a writable target path, preserve existing targets without overwrite, overwrite only when requested, fail for missing seed or same-path inputs, and map runtime config seed flags into a copy request without copying.
- Has optional smoke coverage for seed copy/no-overwrite/overwrite/missing-seed/same-path behavior and verifies copied targets are readable through the SQLite repository.
- Has optional packaged runtime smoke coverage that launches the packaged executable, creates a SQLite repository through the factory, runs save/list/load/remove/clear under a packaged runtime `Saved/SQLUI/PackagedRuntimeSmoke/...` path, verifies success from the runtime log, and removes the smoke database.
- Has optional packaged runtime provider startup smoke coverage that launches the packaged executable with explicit repository command-line settings, initializes `USQLUILayoutRepositoryRuntimeProvider`, verifies provider-backed SQLite save/list/load behavior, resets the provider, verifies success from the runtime log, and removes the smoke database.
- Has optional packaged runtime provider subsystem smoke coverage that launches the packaged executable with explicit auto-init and repository command-line settings, verifies the passive `USQLUILayoutRepositoryRuntimeSubsystem` can own/access the provider, verifies provider-backed SQLite save/list/load behavior, resets the subsystem/provider, verifies success from the runtime log, and removes the smoke database.

The repository implementation does not:

- Make SQLite the default repository backend.
- Run migrations or create database files inside `USQLUILayoutRepositoryFactory`.
- Modify widgets or default smoke-test behavior.

The local packaged-build validation scaffold now passes for the documented UE 5.7 Win64 Development run after installing the UE 5.7-preferred Visual Studio 2022 MSVC `14.44.x` toolchain. The earlier unresolved `__std_*` linker failure was observed with non-preferred MSVC toolchains, not with SQLUI or SQLiteCore objects in the first linker errors.

Remaining blockers before promoting SQLite to production/default persistence:

- Expanding packaged-build validation across intended target platforms.
- Expanding packaged runtime SQLite lifecycle coverage beyond the first local packaged executable smoke.
- Production SQLite service lifecycle, shutdown draining beyond stale-callback suppression, and cancellation policy.
- Actual future schema migrations, version-specific upgrade transforms, and compatibility policy beyond the first version/status framework.
- Production/user-facing runtime settings UI/UX, settings apply/save behavior, normal startup initialization policy, and database path policy beyond the safe default-off config-backed settings/provider/subsystem path, docs-only settings editing/reset plan, and non-mutating draft validation/display model plus sample/dev display/widget-shell contracts.
- Product seed database asset location, packaging, versioning, and upgrade policy beyond the first explicit copy helper.

## Evaluation Criteria

Each option is compared against:

- UE 5.7 compatibility.
- Runtime support versus editor-only support.
- Packaged build behavior.
- Platform support.
- Threading and connection ownership model.
- Ability to control database paths under `Saved/SQLUI/...`.
- Ability to run migrations or raw SQL safely.
- Transaction support.
- Error reporting quality.
- License suitability.
- Dependency and build footprint.
- Long-term maintainability.
- Smoke-test and commandlet compatibility.
- Risk level.
- Recommendation.

## Options

### Option 1: Engine-Provided `SQLiteCore`

`SQLiteCore` is present in the local UE 5.7 install as an engine runtime database plugin. It exposes direct C++ database and prepared-statement wrappers.

Assessment:

- UE 5.7 compatibility: Strong locally. The plugin and headers are present in the installed UE 5.7 tree.
- Runtime support: Promising. The module type is `Runtime`, and local UE 5.7 Win64 Development BuildCookRun validation now passes.
- Packaged build behavior: Verified locally for UE 5.7 Win64 Development after installing the preferred MSVC `14.44.x` toolchain; the first packaged runtime SQLite lifecycle smoke is available through the packaged validation script, while other target platforms still need validation.
- Platform support: Likely tied to Unreal's engine-supported SQLite configuration. Target platforms must be checked when implementation begins.
- Threading and connection ownership: Suitable if SQLUI owns one worker-side connection per database file and serializes operations. The custom-platform build comment warns against multiple open `FSQLiteDatabase` handles to the same file in that mode.
- Path control: Good. `FSQLiteDatabase::Open` accepts a filename, so SQLUI can resolve paths under `Saved/SQLUI/...` before opening.
- Migrations/raw SQL: Good. `Execute` and prepared statements can run DDL, DML, and `PRAGMA` statements.
- Transactions: Good. Transactions can be executed as raw SQL with `BEGIN`, `COMMIT`, and `ROLLBACK`, though SQLUI should wrap them in a small helper to avoid partial-write mistakes.
- Error reporting: Adequate. `GetLastError` is available, but SQLUI should add operation context in its own result messages.
- License suitability: Promising. SQLite license material ships with the engine; project policy should still confirm notice obligations before shipping.
- Dependency/build footprint: Low relative to third-party or vendored options because the engine already provides the plugin.
- Maintainability: Strongest candidate. Epic maintains the wrapper with the engine version.
- Smoke-test/commandlet compatibility: Promising, but must be verified with SQLUI smoke paths after implementation.
- Risk level: Medium-low, mostly because additional target-platform packaging, broader packaged runtime lifecycle coverage, and production threading behavior still need proof in this project.
- Recommendation: Preferred candidate; local packaged-build, editor smoke, and first packaged runtime lifecycle coverage exist, while broader target coverage still needs proof.

### Option 2: SQLUI-Owned Wrapper Around `SQLiteCore`

This option uses `SQLiteCore` as the backend but hides it behind a small SQLUI Core-owned adapter and async database service. Widgets and samples would still talk only to repository interfaces and factory settings.

Assessment:

- UE 5.7 compatibility: Same as `SQLiteCore`, with a smaller SQLUI-facing surface.
- Runtime support: Same runtime promise as `SQLiteCore`, with SQLUI controlling when the dependency is introduced.
- Packaged build behavior: Inherits the local UE 5.7 Win64 Development packaged-build and first packaged runtime lifecycle smoke from the direct `SQLiteCore` path; other targets still need validation.
- Platform support: Same as `SQLiteCore`.
- Threading and connection ownership: Strong. SQLUI can enforce one worker-side connection context and serialize operations regardless of caller behavior.
- Path control: Strong. SQLUI can centralize `Saved/SQLUI/...` path resolution before opening the database.
- Migrations/raw SQL: Strong. SQLUI can provide migration helpers on top of raw SQL execution.
- Transactions: Strong. SQLUI can provide transaction helpers and consistent rollback/error mapping.
- Error reporting: Stronger than direct use. The wrapper can add repository operation names, layout ids, migration ids, and backend availability context.
- License suitability: Same as `SQLiteCore`.
- Dependency/build footprint: Small. Adds only SQLUI glue when implementation begins.
- Maintainability: Strong. SQLUI owns repository semantics while delegating SQLite maintenance to the engine.
- Smoke-test/commandlet compatibility: Strong. The wrapper can expose deterministic test hooks without leaking SQLite to widgets.
- Risk level: Low-medium. It adds a small internal abstraction but reduces misuse of engine APIs across SQLUI.
- Recommendation: Smallest safe implementation shape if `SQLiteCore` verification passes.

### Option 3: Engine-Provided `SQLiteSupport`

`SQLiteSupport` is also present locally as a runtime database plugin. It depends on `DatabaseSupport` and `SQLiteCore` and exposes `FSQLiteDatabaseConnection` plus `FSQLiteResultSet`.

Assessment:

- UE 5.7 compatibility: Present locally in UE 5.7.
- Runtime support: Module type is `Runtime`, but SQLUI uses `SQLiteCore` directly; this secondary path has not been separately packaged or lifecycle tested.
- Packaged build behavior: Requires validation.
- Platform support: Inherits `SQLiteCore` platform behavior and adds the `DatabaseSupport` abstraction.
- Threading and connection ownership: Needs verification. SQLUI would still need its own worker boundary and serialized connection ownership.
- Path control: Likely acceptable because `FSQLiteDatabaseConnection::Open` accepts a connection string path, but SQLUI would still resolve paths itself.
- Migrations/raw SQL: Acceptable through command execution.
- Transactions: Likely acceptable through raw transaction SQL.
- Error reporting: Adequate through `GetLastError`, but result-set ownership and older database abstractions may add surface area.
- License suitability: Same engine SQLite basis as `SQLiteCore`.
- Dependency/build footprint: Higher than `SQLiteCore` because it also brings `DatabaseSupport`.
- Maintainability: Acceptable, but the abstraction appears broader than SQLUI needs.
- Smoke-test/commandlet compatibility: Requires validation.
- Risk level: Medium because it adds another engine database abstraction without clear benefit for SQLUI's repository-specific needs.
- Recommendation: Secondary engine option. Prefer `SQLiteCore` plus a SQLUI-owned wrapper unless `SQLiteSupport` offers an implementation-time advantage that is not visible from the current docs/header inspection.

### Option 4: Third-Party Unreal SQLite Plugin

This category includes Marketplace or external Unreal plugins that wrap SQLite.

Assessment:

- UE 5.7 compatibility: Unknown until a candidate is selected and tested against UE 5.7.
- Runtime support: Varies by plugin. Some plugins may be editor-oriented or Blueprint-focused; SQLUI needs runtime C++ support.
- Packaged build behavior: Must be proven per plugin and per target platform.
- Platform support: Varies by plugin and by bundled binaries.
- Threading and connection ownership: Must be audited carefully. SQLUI still needs a worker boundary.
- Path control: Must support explicit writable paths under `Saved/SQLUI/...`.
- Migrations/raw SQL: Must support raw SQL and prepared statements or a sufficient equivalent.
- Transactions: Must support explicit transactions and rollback.
- Error reporting: Varies.
- License suitability: Must be reviewed for project and redistribution compatibility.
- Dependency/build footprint: Medium to high depending on plugin size, binaries, and update policy.
- Maintainability: Riskier than engine support because SQLUI would depend on a separate update cadence.
- Smoke-test/commandlet compatibility: Must be proven.
- Risk level: Medium-high until a specific plugin demonstrates clear advantages.
- Recommendation: Do not choose this path unless a specific third-party plugin provides a clearly better runtime, packaging, threading, licensing, and maintenance story than engine-provided support.

### Option 5: Vendor SQLite Directly Into SQLUI

This option adds SQLite amalgamation source or binaries directly to a SQLUI module or plugin.

Assessment:

- UE 5.7 compatibility: Controllable, but SQLUI would own all Unreal build integration.
- Runtime support: Controllable.
- Packaged build behavior: Controllable but costly to validate across platforms.
- Platform support: Requires SQLUI to maintain platform-specific build behavior.
- Threading and connection ownership: Controllable, but SQLUI must configure SQLite correctly.
- Path control: Good.
- Migrations/raw SQL: Good.
- Transactions: Good.
- Error reporting: Good if SQLUI wraps errors carefully.
- License suitability: SQLite itself is permissive, but vendoring still requires project-level license and notice review.
- Dependency/build footprint: Highest. Adds third-party source or binaries to SQLUI.
- Maintainability: Weakest unless engine support is absent or unsuitable. SQLUI would own security, version, build, and platform updates.
- Smoke-test/commandlet compatibility: Controllable but must be built from scratch.
- Risk level: High.
- Recommendation: Avoid unless engine-provided support is absent, unusable for packaged runtime builds, or blocked by platform requirements.

### Option 6: Defer SQLite and Continue JSON-File Persistence

This option keeps using the existing JSON-file repository while schema, async, and backend questions mature.

Assessment:

- UE 5.7 compatibility: Already works in the project.
- Runtime support: Already runtime-safe for current lightweight persistence.
- Packaged build behavior: Existing JSON file behavior still needs normal project validation, but it has no SQLite dependency.
- Platform support: Uses Unreal file APIs and current repository code.
- Threading and connection ownership: No database connection. Current JSON operations are simpler, though not a substitute for the future async SQLite path.
- Path control: Already scoped under `Saved/SQLUI/...`.
- Migrations/raw SQL: Not applicable.
- Transactions: Not applicable.
- Error reporting: Existing repository result semantics are already in place.
- License suitability: No new dependency.
- Dependency/build footprint: None.
- Maintainability: Good for interim sample/runtime persistence, not enough for durable layout history and query features.
- Smoke-test/commandlet compatibility: Already covered by current smoke paths.
- Risk level: Low for deferral, medium for product goals if SQLite is needed soon.
- Recommendation: Safe fallback if engine SQLite packaged runtime lifecycle, target-platform coverage, or threading questions block production use.

## Comparison Matrix

| Option | UE 5.7 Verification | Runtime / Packaging | Threading Fit | Path Control | Migrations / Raw SQL | Transactions | Footprint | Maintainability | Risk | Recommendation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Engine `SQLiteCore` | Verified present locally | Runtime module verified; local UE 5.7 Win64 Development packaging and first runtime smoke pass | Good with one worker-owned connection; concurrency caveat verified in Build.cs | Good | Good | Good through SQL | Low | Strong | Medium-low | Active candidate; broader target/runtime coverage still needed |
| SQLUI wrapper around `SQLiteCore` | Same as `SQLiteCore` | Same as `SQLiteCore`; local packaged-build and first runtime smoke pass exist | Strongest because SQLUI owns the boundary | Strong | Strong | Strong | Low-small | Strong | Low-medium | Smallest safe implementation shape |
| Engine `SQLiteSupport` | Verified present locally | Runtime module verified; not separately used or packaged by SQLUI | Acceptable but less direct | Acceptable | Acceptable | Acceptable | Medium | Acceptable | Medium | Secondary engine option |
| Third-party plugin | Not verified | Unknown until candidate tested | Unknown | Must verify | Must verify | Must verify | Medium-high | Varies | Medium-high | Avoid unless clearly superior |
| Vendored SQLite | Not needed yet | Controllable but costly | Controllable | Good | Good | Good | High | Weak unless necessary | High | Avoid unless engine support fails |
| Defer and keep JSON file | Already implemented | Already in current scope | Simple, no DB thread | Good | Not applicable | Not applicable | None | Good interim | Low for deferral | Fallback if SQLite verification blocks |

## Recommended Path

The conservative recommendation is:

1. Treat engine-provided `SQLiteCore` as the preferred candidate because it is verified in the local UE 5.7 install, exposes direct runtime C++ database and prepared-statement APIs, supports raw SQL, supports explicit file paths, and keeps dependency footprint smaller than third-party or vendored paths.
2. Implement SQLUI's own small wrapper and async database boundary around `SQLiteCore` rather than letting repository or widget code use `FSQLiteDatabase` directly.
3. Keep SQLite explicit and non-default until packaged runtime lifecycle behavior across intended target scenarios, target-platform support beyond the local Win64 Development pass, and production threading/shutdown rules are validated.
4. Keep JSON-file persistence available as the safe lightweight runtime persistence path if any remaining `SQLiteCore` validation item fails.

Do not choose a Marketplace or third-party dependency unless it clearly beats `SQLiteCore` on runtime packaging, threading, licensing, platform coverage, and maintenance. Do not vendor SQLite unless engine-provided support is absent or unsuitable for SQLUI's runtime requirements.

## Implementation-Readiness Checklist

Before promoting SQLite from explicit opt-in backend to broader runtime use, confirm:

- Backend verified in UE 5.7.
- Runtime module dependency identified.
- Required plugin descriptor changes identified.
- `Build.cs` impact understood.
- Packaging impact understood for the first supported target.
- Local packaged-build validation passed for UE 5.7 Win64 Development after installing the preferred MSVC `14.44.x` toolchain.
- First packaged runtime SQLite lifecycle smoke passed for local Win64 Development.
- Local packaged-build and packaged runtime smoke validation expanded for any additional supported targets.
- Threading rules confirmed.
- Single-connection or connection-pool policy confirmed.
- Database path behavior confirmed under `Saved/SQLUI/...`.
- Migration/raw SQL support confirmed.
- Transaction support and rollback behavior confirmed.
- Error reporting confirmed.
- License and notice requirements confirmed.
- Smoke-test strategy confirmed.
- Explicit seed-copy policy confirmed for any source seed DBs.
- Backend-unavailable behavior confirmed for builds without SQLite enabled.

## Decision Record

Current status: engine `SQLiteCore` is the active backend candidate and current implementation basis. SQLUI now has explicit SQLite repository factory selection, opt-in schema initialization with targeted edge-case hardening, a first migration version/status framework for the known layout schema migration set, an explicit seed database copy policy helper, a passing local packaged-build validation run for UE 5.7 Win64 Development, and a first opt-in packaged runtime SQLite lifecycle smoke, but SQLite is not the default backend.

Preferred candidate: engine-provided `SQLiteCore`, wrapped by a small SQLUI Core-owned async database boundary.

Reasons:

- Verified locally in UE 5.7 as an engine runtime database plugin.
- Provides direct C++ APIs for opening files, executing SQL, preparing statements, binding values, reading typed columns, and reporting last errors.
- Keeps dependency footprint smaller than third-party or vendored options.
- Lets SQLUI control repository semantics, migrations, transactions, result mapping, and worker-thread ownership.
- Aligns with the existing plan to keep SQL and storage details out of widgets.

Blocking questions:

- Does adding `SQLiteCore` to SQLUI package and run cleanly for every intended target platform beyond the latest local Win64 Development pass?
- Does the `SQLiteCore.uplugin` descriptor require project/plugin descriptor changes beyond a module dependency?
- Are there platform-specific restrictions from `Target.bCompileCustomSQLitePlatform` that affect SQLUI's desired concurrency model?
- Should SQLUI use only one worker-owned database connection per repository scope, or is a later read/write split safe on the first supported platforms?
- What is the exact graceful behavior when SQLite support is disabled or unavailable in a packaged build?
- What license notice changes, if any, are required for shipping with the engine-provided SQLite module?

Future SQLite hardening PRs may still touch:

- A SQLUI Core async database service or wrapper around the chosen backend.
- Production cancellation and service shutdown behavior beyond the current per-repository queue suppression policy.
- Actual future production migrations and upgrade transforms for later schema changes.
- Product seed database packaging/versioning policy if seed DBs are shipped.
- Packaged-build validation and any target-platform fixes.
- Additional smoke coverage for production migration and async lifecycle behavior.
- Documentation updates reflecting the implemented path.

It should still avoid widgets, maps, Content assets, editor tooling, CI, and unrelated JerryRigged host changes.
