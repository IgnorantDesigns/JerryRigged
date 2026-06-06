# SQLUI Persistence Settings UX Design

This document defines the intended user-facing settings surface for SQLUI layout persistence before any widget, menu, or settings UI implementation is added.

The backend and policy pieces now exist for a safe first UI, but the UI itself is still future work. This design keeps SQLite explicit, keeps `InMemory` as the safe default, and routes database status/reset behavior through SQLUICore helpers instead of widget-owned storage logic.

Related docs:

- [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md) summarizes the implemented SQLite runtime state.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) describes repository boundaries and backend selection.
- [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md) tracks current phase status and next slices.
- [`sqlui_smoke_test.md`](sqlui_smoke_test.md) lists editor smoke-test commands.
- [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md) documents local packaged validation and packaged runtime smoke paths.

## Purpose

The future persistence settings surface should let an app or sample user understand and configure where SQLUI stores runtime layout data without exposing raw SQL, table names, SQLite connections, or repository implementation details.

The surface should answer three practical questions:

- Is layout persistence enabled for this runtime?
- Which storage backend is active?
- If SQLite is selected, where is the database, is it usable, and what safe maintenance actions are available?

The UI should be a thin product layer above SQLUICore runtime settings, subsystem/provider status, persistence workflow, database management policy, seed-copy policy, and schema status helpers.

## Current Backend Readiness

SQLUICore currently has the runtime pieces needed for a safe settings UI:

- `USQLUILayoutRepositoryRuntimeSettings`
- `FSQLUILayoutRepositoryRuntimeSettingsPolicy`
- `USQLUILayoutRepositoryRuntimeSubsystem`
- `USQLUILayoutRepositoryRuntimeProvider`
- `FSQLUILayoutPersistenceWorkflow`
- `FSQLUILayoutRepositoryDatabaseManagement`
- `FSQLUILayoutRepositoryRuntimeIntegration`
- `FSQLUILayoutRepositoryRuntimeConfigResolver`
- `USQLUILayoutRepositoryFactory`
- `USQLUISQLiteLayoutRepository`
- SQLite schema initialization, hardening, and migration status/versioning helpers
- SQLite seed-copy policy
- Packaged persistence workflow proof across separate executable launches

Those pieces are backend and policy readiness, not product UI. They do not make SQLite the default backend and do not create database files during normal startup.

## UX Goals

- Make the safe default obvious: layout persistence can be off or in-memory without durable database writes.
- Make durable persistence an explicit user or app choice.
- Show the resolved runtime path before any file is created.
- Explain the difference between status, repository row cleanup, soft delete, and database reset.
- Use SQLUICore helpers for path resolution, repository creation, database status, reset, seed copy, and migration/version status.
- Keep widgets and settings panels storage-agnostic where possible.
- Avoid surprising startup side effects.
- Present SQLite as a durable option that requires clear opt-in and a writable runtime path.

## Non-Goals

- No widget, menu, Project Settings, editor tooling, or runtime UI is implemented by this document.
- No startup auto-init behavior changes.
- No default backend changes.
- No database file creation by browsing or selecting a path.
- No repository factory migrations.
- No direct SQL in UI code.
- No source-controlled runtime database files.
- No seed database asset/package policy beyond design guidance.
- No packaged UI validation.

## Storage Modes

### InMemory

`InMemory` is the default and safest backend.

User-facing copy should explain that layouts are stored only for the current runtime session. They are lost when the app exits or the active repository is reset. This is appropriate for tests, temporary sessions, demos, and workflows where the user has not opted into durable layout persistence.

### JsonFile

`JsonFile` is a file-backed non-SQLite backend. It stores one validated layout document per JSON file.

If exposed, it can be presented as a simpler development or fallback option. It is easier to inspect manually than SQLite but is not the primary durable backend direction for the SQLite repository work.

### SQLite

`SQLite` is the durable backend. It is explicit opt-in, persistent across runs, and requires a configured database path.

The UI should make schema initialization and database creation separate explicit choices. Selecting SQLite should not automatically create a database unless the app also enables the schema/create settings through SQLUICore repository settings.

### Unavailable

`Unavailable` is a diagnostic or failure state, not a normal end-user storage choice unless the app intentionally exposes test modes.

The UI can show unavailable when settings are incomplete, a requested backend cannot be configured, or a repository failed to initialize. It should include a clear reason instead of silently falling back to another backend.

## Default Behavior

The committed/default runtime behavior is safe:

- Provider auto-init is off by default.
- `InMemory` is the default backend.
- SQLite is not selected by default.
- SQLite database path is empty by default.
- SQLite schema initialization is disabled by default.
- SQLite database creation is disabled by default.
- SQLite seed copy is disabled by default.
- Async SQLite callback execution is disabled by default.
- No database files are created by default.
- Normal startup remains safe unless settings or command-line options explicitly opt in.

The UI should preserve those defaults unless the product intentionally changes policy in a future PR.

## User-Facing Settings Fields

Recommended labels:

- Enable layout persistence
- Storage backend
- SQLite database path
- Create database if missing
- Initialize schema if missing
- Allow command-line overrides
- Use seed database
- Seed database path
- Copy seed if missing
- Overwrite database from seed
- Show database status
- Reset layout database

Recommended field behavior:

- `Enable layout persistence` maps to whether runtime code initializes a provider/subsystem with durable settings. Disabling it should leave the provider uninitialized or use `InMemory`, depending on product policy.
- `Storage backend` should offer `InMemory`, `JsonFile`, and `SQLite`. `Unavailable` should usually be status-only.
- `SQLite database path` should show both the entered value and resolved path.
- `Create database if missing` and `Initialize schema if missing` should be explicit SQLite-only toggles.
- `Allow command-line overrides` should map to the settings policy that chooses whether command-line repository flags can override config-backed settings.
- Seed database fields should stay hidden or advanced until seed asset packaging/version rules are decided.
- `Overwrite database from seed` should be advanced and dangerous because it can delete the current target database and sidecars before copying.

## Database Path Policy

The recommended app-managed SQLite path is:

```text
Saved/SQLUI/LayoutRepositories/LayoutRepository.sqlite
```

Relative paths should resolve under:

```text
Saved/SQLUI/LayoutRepositories
```

Absolute paths are advanced. If exposed, the UI should label them as advanced and show the normalized resolved path before applying.

The UI should prevent or warn against runtime database paths under:

- `Content/`
- `Config/`
- plugins or plugin content
- `Binaries/`
- `Intermediate/`
- `DerivedDataCache/`
- source-controlled project folders
- source-controlled database files

Browsing or selecting a path should not create files. File creation should happen only through an explicit repository initialization/save path when both database creation and schema initialization are enabled, or through an explicit seed-copy action.

## Database Status Display

Database status should use `FSQLUILayoutRepositoryDatabaseManagement`.

Suggested status fields:

- Backend
- Provider initialized
- Repository active
- Resolved SQLite database path
- Database exists
- Database file size
- SQLite sidecars present
- Last status refresh time
- Warning when backend is not SQLite
- Warning when SQLite path is empty

Status refresh should be read-only. It should not create directories, create databases, open SQLite, initialize schema, copy seed databases, or change the active repository.

## Reset / Clear / Delete Semantics

The UI must not present these actions as equivalent:

- `RemoveLayout` soft-deletes one layout through repository behavior.
- `ClearLayouts` destructively deletes layout rows and dependent schema rows through repository behavior for the selected database scope.
- `Reset layout database` deletes the resolved SQLite database file and SQLite sidecars through `FSQLUILayoutRepositoryDatabaseManagement`.

Reset database is explicit and destructive. It should:

- Require confirmation.
- Show the resolved database path.
- Remove only the resolved SQLite database and sidecars.
- Never delete arbitrary directories.
- Never touch seed databases.
- Never run while a repository is actively using the DB unless the app has first reset or shut down the provider safely.
- Report locked-file or permission failures clearly.

Reset is a file-level maintenance action. `ClearLayouts` is a repository-level data action. `RemoveLayout` is a single-layout lifecycle action.

## Seed Database UX

Seed database UI should be optional and probably delayed beyond the first UI slice.

If exposed later, it should use the SQLUICore seed-copy policy and make these states clear:

- No seed is configured.
- Seed path is configured but missing.
- Runtime target exists and copy-if-missing will leave it untouched.
- Overwrite from seed will delete the target DB and sidecars before copying.
- Seed copy is a closed-file preparation step before repository use.

Seed copy should not run during status refresh and should not copy into source-controlled folders.

## Migration / Upgrade UX

The current known production migration is:

```text
001_initial_layout_schema
```

Future migration UX should show schema/version status before any destructive action. It should surface:

- Latest known migration id.
- Applied migration ids.
- Pending known migration ids.
- Whether the expected initial schema objects are present.
- Whether the database is missing, complete, or partial.
- Any migration or schema validation errors.

Current policy:

- Complete schema with a missing initial migration record can be repaired non-destructively by recording the known row.
- Partial schema should fail clearly and suggest backup, reset, or manual recovery. It should not silently repair by dropping or rewriting user data.
- Unknown future migration rows should be reported carefully instead of treated as permission to modify data blindly.

Upgrade failure should be visible, specific, and separate from ordinary missing-layout errors.

## Error Handling

The settings UI should map backend errors into direct user-facing messages:

- Missing SQLite database path.
- Requested backend unavailable.
- Permission denied.
- Database exists but cannot open.
- Schema is missing and initialization is disabled.
- Partial or corrupt schema.
- Seed database missing.
- Reset failed because the file is locked.
- Command-line overrides disabled by settings.

Toolchain/package validation errors are not user-facing runtime settings errors. They belong in developer documentation and local validation output.

## Startup Behavior

`USQLUILayoutRepositoryRuntimeSubsystem` is passive by default. It can initialize from settings only when `bAutoInitializeProvider` is true.

Command-line auto-init remains available for validation and developer workflows through `-SQLUILayoutRepositoryProviderAutoInit` and explicit repository settings. Command-line overrides can be disabled through config-backed settings.

App and UI code should use:

- `USQLUILayoutRepositoryRuntimeSubsystem`
- `USQLUILayoutRepositoryRuntimeProvider`
- `FSQLUILayoutPersistenceWorkflow`
- `FSQLUILayoutRepositoryDatabaseManagement`

It should not instantiate concrete repositories directly for normal product flows, issue SQL, or delete database files itself.

## Safety Boundaries

- SQLite remains explicit and non-default.
- `InMemory` remains the safe default.
- Provider auto-init remains off unless explicitly enabled.
- Database creation requires both schema initialization and create-if-missing to be enabled on a writable SQLite repository.
- Read-only mode blocks schema-initialization writes.
- Status refresh is read-only.
- Reset deletes only the resolved database file and SQLite sidecars.
- Seed copy is explicit, closed-file, and separate from repository operations.
- Widgets should not know SQLite paths, SQL strings, table names, migrations, or sidecar names.
- JerryRigged remains a thin host.

## Recommended First UI Slice

Start with a read-only status panel:

- Current backend.
- Provider initialized?
- Repository active?
- Resolved SQLite path.
- DB exists?
- DB file size.
- Sidecars present?
- Current schema/migration status if available.

Then add opt-in persistence selection:

- Enable layout persistence.
- Choose SQLite.
- Use the default app-managed path.
- Enable create/init only when the user confirms durable persistence.
- Show an apply/restart note if the product chooses startup-only provider initialization.

Then add reset database:

- Require confirmation.
- Show the resolved path.
- Reset or shut down the provider before deleting.
- Call `FSQLUILayoutRepositoryDatabaseManagement`.

Avoid seed-copy UI, overwrite-from-seed, and arbitrary absolute paths in the first UI unless the product has a clear need.

## Future Enhancements

- Richer schema upgrade previews and migration history.
- Layout history, checkpoints, restore, and preview management.
- Storage usage and retention policy.
- Seed database package/version policy.
- Async status refresh and repository operation progress.
- Cross-platform packaged persistence validation beyond the current local smoke paths.
- Product-specific startup policy once the settings UI is ready.
