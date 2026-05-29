# SQLUI SQLite Backend Evaluation

This document evaluates realistic backend options for a future SQLite-backed SQLUI layout repository. The original backend evaluation was documentation-only. The current SQLiteCore proof work adds minimal engine `SQLiteCore` plugin/module wiring, a compile/link probe, and an optional smoke-safe open/close probe under `Saved/SQLUI/SmokeTests/SQLiteCoreProbe`; it does not add SQLite layout persistence, migrations, async database work, repository factory changes, widgets, maps, assets, CI, or persistent database files.

## Purpose

SQLUI needs a future durable layout repository that can store the schema drafted in [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md) and follow the async boundary drafted in [`sqlui_sqlite_async_backend_plan.md`](sqlui_sqlite_async_backend_plan.md).

This evaluation compares backend options before implementation work starts. The goal is to identify the smallest safe path for runtime SQLite persistence while keeping:

- SQL and database details inside `SQLUI.Core`.
- Widgets unaware of SQL, table names, database paths, SQLite connections, worker threads, or concrete storage classes.
- JerryRigged as a thin host.
- Current in-memory and JSON-file repository behavior unchanged.

## Non-Goals

- Do not implement SQLite layout persistence in this step.
- Do not add `ESQLUILayoutRepositoryBackend::SQLite` or select SQLite in repository factory behavior.
- Do not add dependencies beyond the minimal engine `SQLiteCore` plugin/module wiring used for the compile proof.
- Do not add Marketplace dependencies, third-party Unreal plugins, or vendored third-party source.
- Do not modify repository C++ code, widgets, CI, assets, maps, or persistent database files.
- Do not open, create, or write SQLite databases outside the optional smoke-safe probe path under `Saved/SQLUI/SmokeTests/SQLiteCoreProbe`.
- Do not treat this evaluation as proof that packaged builds work. Packaging still needs implementation-time validation.

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

Important limitations:

- This was a local source/header/descriptor inspection, not a packaged build test.
- Runtime module type is verified, but target-platform packaging behavior for JerryRigged remains unverified.
- Threading guarantees beyond the public API and build comments need an implementation proof with SQLUI's planned worker boundary.
- No third-party Marketplace plugin was inspected locally for this evaluation.

## SQLiteCore Availability Proof

SQLUICore now includes a minimal compile-time availability probe for engine-provided `SQLiteCore`.

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
- Add async database workers.
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
- Add async database workers.
- Modify widgets or default smoke-test behavior.

Remaining blockers before SQLite layout persistence:

- Packaged-build validation.
- Async worker boundary.
- Migration runner.
- Repository implementation.
- SQLite repository smoke coverage.

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
- Runtime support: Promising. The module type is `Runtime`, but project integration and packaging still need validation.
- Packaged build behavior: Requires verification in a real implementation PR. The local installed tree contains runtime build artifacts, but SQLUI still needs to prove packaging after adding the dependency.
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
- Risk level: Medium-low, mostly because packaging, platform, and threading behavior still need proof in this project.
- Recommendation: Preferred candidate, pending packaged-build and smoke-test verification.

### Option 2: SQLUI-Owned Wrapper Around `SQLiteCore`

This option uses `SQLiteCore` as the backend but hides it behind a small SQLUI Core-owned adapter and async database service. Widgets and samples would still talk only to repository interfaces and factory settings.

Assessment:

- UE 5.7 compatibility: Same as `SQLiteCore`, with a smaller SQLUI-facing surface.
- Runtime support: Same runtime promise as `SQLiteCore`, with SQLUI controlling when the dependency is introduced.
- Packaged build behavior: Requires the same packaged-build validation as direct `SQLiteCore`.
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
- Runtime support: Module type is `Runtime`, but integration and packaging still need verification.
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
- Recommendation: Safe fallback if engine SQLite packaging or threading questions cannot be answered in the next implementation PR.

## Comparison Matrix

| Option | UE 5.7 Verification | Runtime / Packaging | Threading Fit | Path Control | Migrations / Raw SQL | Transactions | Footprint | Maintainability | Risk | Recommendation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Engine `SQLiteCore` | Verified present locally | Runtime module verified; packaging requires proof | Good with one worker-owned connection; concurrency caveat verified in Build.cs | Good | Good | Good through SQL | Low | Strong | Medium-low | Preferred candidate pending implementation proof |
| SQLUI wrapper around `SQLiteCore` | Same as `SQLiteCore` | Same as `SQLiteCore` | Strongest because SQLUI owns the boundary | Strong | Strong | Strong | Low-small | Strong | Low-medium | Smallest safe implementation shape |
| Engine `SQLiteSupport` | Verified present locally | Runtime module verified; packaging requires proof | Acceptable but less direct | Acceptable | Acceptable | Acceptable | Medium | Acceptable | Medium | Secondary engine option |
| Third-party plugin | Not verified | Unknown until candidate tested | Unknown | Must verify | Must verify | Must verify | Medium-high | Varies | Medium-high | Avoid unless clearly superior |
| Vendored SQLite | Not needed yet | Controllable but costly | Controllable | Good | Good | Good | High | Weak unless necessary | High | Avoid unless engine support fails |
| Defer and keep JSON file | Already implemented | Already in current scope | Simple, no DB thread | Good | Not applicable | Not applicable | None | Good interim | Low for deferral | Fallback if SQLite verification blocks |

## Recommended Path

The conservative recommendation is:

1. Treat engine-provided `SQLiteCore` as the preferred candidate because it is verified in the local UE 5.7 install, exposes direct runtime C++ database and prepared-statement APIs, supports raw SQL, supports explicit file paths, and keeps dependency footprint smaller than third-party or vendored paths.
2. Implement SQLUI's own small wrapper and async database boundary around `SQLiteCore` rather than letting repository or widget code use `FSQLiteDatabase` directly.
3. Defer a final backend selection until an implementation PR proves packaged build behavior, target-platform support, threading rules, and smoke-test compatibility.
4. Keep JSON-file persistence as the safe interim runtime persistence path if any `SQLiteCore` verification item fails.

Do not choose a Marketplace or third-party dependency unless it clearly beats `SQLiteCore` on runtime packaging, threading, licensing, platform coverage, and maintenance. Do not vendor SQLite unless engine-provided support is absent or unsuitable for SQLUI's runtime requirements.

## Implementation-Readiness Checklist

Before any SQLite implementation PR changes project code, confirm:

- Backend verified in UE 5.7.
- Runtime module dependency identified.
- Required plugin descriptor changes identified.
- `Build.cs` impact understood.
- Packaging impact understood for the first supported targets.
- Threading rules confirmed.
- Single-connection or connection-pool policy confirmed.
- Database path behavior confirmed under `Saved/SQLUI/...`.
- Migration/raw SQL support confirmed.
- Transaction support and rollback behavior confirmed.
- Error reporting confirmed.
- License and notice requirements confirmed.
- Smoke-test strategy confirmed.
- Backend-unavailable behavior confirmed for builds without SQLite enabled.

## Decision Record

Current status: compile availability proven only; not selected as a layout repository backend.

Preferred candidate: engine-provided `SQLiteCore`, wrapped by a small SQLUI Core-owned async database boundary.

Reasons:

- Verified locally in UE 5.7 as an engine runtime database plugin.
- Provides direct C++ APIs for opening files, executing SQL, preparing statements, binding values, reading typed columns, and reporting last errors.
- Keeps dependency footprint smaller than third-party or vendored options.
- Lets SQLUI control repository semantics, migrations, transactions, result mapping, and worker-thread ownership.
- Aligns with the existing plan to keep SQL and storage details out of widgets.

Blocking questions:

- Does adding `SQLiteCore` to SQLUI package cleanly for JerryRigged's target platforms?
- Does the `SQLiteCore.uplugin` descriptor require project/plugin descriptor changes beyond a module dependency?
- Are there platform-specific restrictions from `Target.bCompileCustomSQLitePlatform` that affect SQLUI's desired concurrency model?
- Should SQLUI use only one worker-owned database connection per repository scope, or is a later read/write split safe on the first supported platforms?
- What is the exact graceful behavior when SQLite support is disabled or unavailable in a packaged build?
- What license notice changes, if any, are required for shipping with the engine-provided SQLite module?

Once the decision is made, the next implementation PR may touch:

- SQLUI plugin descriptor dependency wiring if needed.
- `SQLUICore.Build.cs` module dependencies.
- `ESQLUILayoutRepositoryBackend` and factory settings for a SQLite backend value.
- A SQLUI Core async database service or wrapper around the chosen backend.
- A SQLite layout repository implementation.
- Executable migration code for the approved schema.
- Smoke coverage for SQLite backend selection and repository lifecycle behavior.
- Documentation updates reflecting the implemented path.

It should still avoid widgets, maps, Content assets, editor tooling, CI, and unrelated JerryRigged host changes.
