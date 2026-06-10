# SQLUI Persistence Settings UX Design

This document defines the intended user-facing settings surface for SQLUI layout persistence before any product widget, menu, or settings UI implementation is added.

The backend and policy pieces now exist for a safe first UI. Tiny optional SQLUISamples sample/dev presenter, panel-adapter, and C++ UMG widget shell objects already provide Blueprint-callable, explicitly refreshed read-only display-row hooks for validation and Blueprint/sample consumption, and SQLUICore now has a dry-run apply-intent preview for drafts, but the full product settings UI is still future work. This design keeps SQLite explicit, keeps `InMemory` as the safe default, and routes database status/reset behavior through SQLUICore helpers instead of widget-owned storage logic.

Related docs:

- [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md) summarizes the implemented SQLite runtime state.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) describes repository boundaries and backend selection.
- [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md) tracks current phase status and next slices.
- [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md) plans the next mutating settings editing, apply/cancel, backend selection, SQLite path, provider auto-init, and reset/delete UX phase.
- [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md) documents the focused read-only UMG binding recipe for the optional widget shell.
- [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) documents the validation/preview-only draft settings UMG binding recipe for the optional widget shell.
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

The first implementation slice for this design now exists as `USQLUIPersistenceStatusLibrary` and `FSQLUIPersistenceStatusSnapshot`. It is a read-only SQLUICore status surface that future UI can call to inspect configured backend, provider/repository state, resolved SQLite path, database file status, sidecar status, and migration status when a database already exists. It does not add a settings widget, settings editing, reset action, startup behavior, or default backend change.

The first UI-consumption slice also exists as `USQLUIPersistenceStatusDisplayLibrary` and `FSQLUIPersistenceStatusDisplayRow`. It converts the read-only status snapshot into label/value/state/detail rows that a future settings panel can bind to directly. The display library does not perform its own file checks, initialize providers, create repositories, create databases, run migrations, copy seeds, reset databases, or delete files.

The first settings-editing foundation slice now exists as `FSQLUIPersistenceSettingsDraft`, `FSQLUIPersistenceSettingsValidationResult`, `FSQLUIPersistenceSettingsApplyPreviewResult`, and `USQLUIPersistenceSettingsDraftLibrary`. It is still non-mutating: callers can represent current and pending runtime settings, validate backend/path/provider-auto-init choices, reset the in-memory draft value back to current values, and build a dry-run apply-intent preview, but no settings are applied or saved. Draft validation and apply-preview generation do not create database files or directories, open databases for writing, run migrations, copy seed databases, initialize providers/repositories, delete files, add UI controls, or change startup behavior. `USQLUIPersistenceSettingsDraftDisplayLibrary` now adds a UI-safe summary/row formatter for validation results so a future sample or settings panel can show pending backend/path/provider-auto-init status without owning apply, save, or lifecycle behavior.

`USQLUISamplePersistenceSettingsDraftPresenter` and `USQLUISamplePersistenceSettingsDraftPanelWidget` are optional SQLUISamples sample/dev surfaces for those draft validation rows. The widget shell is a C++ `UUserWidget` subclass only: it creates no visual layout, adds no widget blueprint asset, is not added to the viewport, is not wired into maps/startup/config, and does not refresh from lifecycle hooks. Its caller-invoked refresh/build functions delegate to the presenter, cache rows/result/summary in memory, and remain validation/display-only. The focused usage recipe in [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) records how a future Blueprint subclass can bind to those rows without adding apply/save/reset behavior or persistence lifecycle side effects.

`USQLUISamplePersistenceStatusPresenter` is the first optional SQLUISamples sample/dev surface over those rows. It stores the display rows plus stable formatted strings for simple sample UI, Blueprint, or commandlet presentation and exposes Blueprint-callable, caller-invoked refresh functions for re-querying those same rows. It is not a full settings screen, is not wired into startup, maps, or default config, and it does not add settings editing, reset/delete actions, provider initialization, repository initialization, migrations, seed copy, database creation, or file deletion.

`USQLUISamplePersistenceStatusPanelAdapter` is the first tiny panel-named adapter for future Blueprint/UMG binding. It is a plain optional SQLUISamples `UObject`, not a `UUserWidget`, and it delegates refresh to `USQLUISamplePersistenceStatusPresenter` instead of duplicating persistence status logic. It stores the latest rows, formatted lines, summary text, and refresh result in memory only. It adds no widget blueprint asset, map, startup binding, polling/tick/timer behavior, settings editing, backend selector, SQLite path editor, reset/delete action, provider/repository initialization, migration, seed copy, database creation, or file deletion.

`USQLUISamplePersistenceStatusPanelWidget` is the first tiny optional C++ UMG shell over that adapter. It creates no visual layout, includes no widget blueprint asset, is not added to the viewport, and is not wired into maps, startup, or config. Its refresh functions remain caller-invoked only and delegate to the panel adapter. It is still not a full settings screen and does not add polling/tick/timer behavior, settings editing, backend selector, SQLite path editor, reset/delete action, provider/repository initialization, migration, seed copy, database creation, or file deletion.

## Read-Only Foundation Checkpoint

The read-only persistence status foundation is complete enough for future settings UI to build on without inventing new storage probing in widgets. The completed chain is:

- SQLUICore read-only status snapshot/surface.
- SQLUICore UI-safe display rows/view model.
- Optional SQLUISamples presenter.
- Explicit caller-invoked refresh path.
- Documented and validated Blueprint-facing presenter hook.
- Read-only panel contract and UMG binding recipe.
- Optional SQLUISamples panel adapter.
- Optional SQLUISamples C++ UMG widget shell.
- Non-asset smoke coverage for presenter, Blueprint hook, adapter, and widget-shell reflection/binding contract.

This checkpoint is not a settings screen and not a startup path. It adds no widget blueprint asset, visual layout, map wiring, viewport attachment, polling, ticking, timer, auto-refresh, backend selector, SQLite path editor, provider auto-init toggle, settings save/apply flow, reset/delete behavior, migration control, seed-copy behavior, provider initialization, repository initialization, or database creation.

The dedicated next-phase plan for mutating settings and reset UX lives in [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md). PR #105 recorded that plan only; the first follow-ups implement only the non-mutating draft/validation model, dry-run apply-intent preview, validation display rows, apply-preview display rows, a SQLUISamples sample/dev adapter for validation rows, and an optional C++ UMG widget shell contract. Backend selector UI, SQLite path editor UI, provider auto-init controls, actual settings apply/save, reset/delete behavior, and product startup policy remain future work.

## Draft Validation Foundation Checkpoint

The non-mutating persistence settings draft validation UI foundation is complete as a safe base for future Apply/Cancel work. It includes the #105 editing/reset UX plan, #106 validation-only draft model, #107 validation display rows/summary, #108 optional SQLUISamples draft validation presenter/adapter, #109 optional C++ UMG widget shell, #110 safe UMG usage guide, #111 final checkpoint, the SQLUICore dry-run apply-intent preview, UI-safe apply-preview display rows/summary, and `-UsePersistenceSettingsDraftProbe` smoke coverage.

This checkpoint still does not implement a product settings screen. It adds no settings controls, backend selector UI, SQLite path editor UI, provider auto-init controls, settings apply/save/config-write behavior, reset/delete actions, widget blueprint assets, maps, startup wiring, viewport attachment, polling, ticking, auto-refresh, provider/repository initialization, migrations, seed-copy behavior, or default config changes. The draft validation, apply-preview, and display paths remain caller-invoked, display/report-only, and non-mutating.

The dry-run apply-intent preview and apply-preview display rows report what a future Apply would do, including whether a draft is valid enough to apply, whether it has changes, whether backend/SQLite/provider-auto-init policy would change, and whether restart/reopen/reinitialize messaging should be shown. They use "would change" and "not applied" semantics and do not write config, create directories or database files, open databases for writing, run migrations, copy seed databases, initialize providers/repositories, or delete files except smoke-owned cleanup in tests.

Future actual Apply/Cancel work should keep widgets ignorant of SQL, schema, migrations, seed-copy policy, sidecar internals, deletion behavior, and provider/repository lifecycle details. Widgets should not write config directly, initialize providers/repositories, or delete files. Apply should route through SQLUICore helper/policy surfaces, keep validation failures user-readable and non-destructive, avoid silently initializing providers/repositories, and show restart/reopen/reinitialize-required messaging when lifecycle work is needed.

Future settings-editing and reset work must preserve these prerequisites:

- Use SQLUICore helper/policy surfaces for repository selection, status, path resolution, schema/migration status, seed-copy policy, and database management.
- Keep widgets ignorant of SQL, schema tables, migration ids, seed-copy policy, SQLite sidecar internals, and direct file deletion.
- Keep runtime database writes under `Saved/SQLUI/...`.
- Route user-facing reset/delete behavior through SQLUICore database management helper/policy surfaces, not widget-owned file deletion.
- Keep SQLite non-default unless a separate explicitly scoped policy PR changes that default.
- Keep provider auto-init default-off unless a separate explicitly scoped policy PR changes that policy.
- Add pending/apply semantics for editable settings instead of mutating live persistence from widget bindings.
- Include smoke coverage for any settings-editing, apply, reset, or destructive action path.
- Include packaged validation when startup behavior, default maps, config wiring, or packaged runtime lifecycle behavior changes.

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

- No product settings widget, menu, Project Settings, editor tooling, or runtime UI is implemented by this document.
- No settings editing or reset/delete controls are added by the optional SQLUISamples status presenter, status panel adapter, status UMG widget shell, draft validation presenter, or draft validation UMG widget shell.
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

Database status should use `FSQLUILayoutRepositoryDatabaseManagement` and the read-only status/display helpers.

The current read-only status snapshot surface is `USQLUIPersistenceStatusLibrary`. It composes runtime settings/config policy, provider state, `FSQLUILayoutRepositoryDatabaseManagement`, and SQLite schema version status. It should be the first UI-facing API for a status panel.

The current UI-facing row adapter is `USQLUIPersistenceStatusDisplayLibrary`. It formats `FSQLUIPersistenceStatusSnapshot` into `FSQLUIPersistenceStatusDisplayRow` values with a label, value, state, and detail text. Widgets or view models can use those rows instead of duplicating storage status wording. The row adapter is still read-only and delegates status gathering to the snapshot helper.

The current sample/dev-facing presenter is `USQLUISamplePersistenceStatusPresenter` in SQLUISamples. It consumes the row adapter and turns rows into formatted strings for optional Blueprint/sample surfaces or commandlet logs. It is a passive presenter only: Blueprint-callable, caller-invoked refresh re-queries the current SQLUICore status/display surfaces and regenerates rows, but does not poll, tick, auto-refresh, create DBs, run migrations, copy seeds, initialize providers/repositories, delete files, attach widgets, or change startup behavior.

The current panel-named adapter is `USQLUISamplePersistenceStatusPanelAdapter` in SQLUISamples. It exists for future Blueprint/UMG panels that want a binding-friendly object name and cached latest rows/result. It delegates to the presenter, remains caller-invoked only, and is not an implemented widget or settings screen.

The current C++ widget shell is `USQLUISamplePersistenceStatusPanelWidget` in SQLUISamples. It is an optional `UUserWidget` subclass for future Blueprint/widget work to subclass or bind to, but it creates no visual layout, adds no widget blueprint asset, does not add itself to the viewport, and does not refresh automatically from widget lifecycle hooks. It only delegates caller-invoked refresh to the panel adapter and caches the latest rows/result in memory.

## Read-Only Panel Contract / Blueprint Usage Recipe

This contract is the intended first Blueprint/UMG consumption shape for a future persistence settings/status panel. The current C++ shell is only a binding scaffold; it does not add a widget blueprint, visual layout, map, startup binding, settings editor, reset button, or backend selector.

Use the existing SQLUISamples panel adapter, which delegates to the presenter hook validated by the sample-surface smoke probe:

- Create or reference a `USQLUISamplePersistenceStatusPanelAdapter` from the future panel/view model.
- Call `RefreshPersistenceStatusPanel` for current runtime settings, or `RefreshPersistenceStatusPanelFromRuntimeConfig` when the panel is previewing an explicit runtime config.
- Read `FSQLUISamplePersistenceStatusRefreshResult.Rows`, call `GetRows()`, or read the adapter's cached last refresh result for structured display.
- Use `GetFormattedLines()` only for simple sample text/log presentation. Product UI should prefer rows.

Future Blueprint/UMG work may alternatively subclass or bind to `USQLUISamplePersistenceStatusPanelWidget`. Its refresh methods call the same adapter path, and its row/result/summary properties are only cached in-memory values. The shell should not be treated as a completed settings screen.

For the focused read-only status widget-shell binding recipe and manual local checklist, see [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md). For the validation-only draft settings widget-shell binding recipe, see [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md).

Display each `FSQLUIPersistenceStatusDisplayRow` without reinterpreting storage details:

- Show `Label`.
- Show `Value`.
- Optionally map `State` to a visual treatment such as normal, good, warning, or unavailable.
- Optionally expose `DetailText` as a tooltip/help line.
- Present default `InMemory` rows as normal and safe.
- Treat missing SQLite path or database rows as normal/not-applicable when SQLite is not the configured backend.

Refresh behavior must stay explicit and read-only:

- A future Refresh button/action may call the presenter refresh function.
- Refresh means re-query current SQLUICore status/display rows only.
- Refresh must not run from startup, construction, timers, tick, polling loops, latent refresh loops, or automatic runtime initialization unless a later PR intentionally adds a safe optional UI path.
- Refresh, validation, and apply-preview generation must not initialize a provider, create a repository, create directories, create databases, apply schema, run migrations, copy seed databases, reset databases, delete files, save settings, or switch backends.

Controls that are out of scope for this first panel contract:

- Backend selector.
- SQLite path editor.
- Provider auto-init toggle.
- Schema initialization or create-database toggle.
- Seed-copy controls.
- Settings save/apply flow.
- Reset/delete button.
- Migration execution controls.

The draft settings model, apply-intent preview, apply-preview display rows, validation display rows, sample adapter, and C++ widget shell may be used by future controls, but adding them does not add those controls. UI surfaces should keep treating draft validation and apply preview as dry-run/pending-state steps until a later PR adds explicit Apply/Cancel behavior.

Ownership boundaries:

- UI/widgets consume SQLUICore status/display rows; they do not own persistence internals.
- UI/widgets do not know SQL strings, schema tables, migration ids, seed-copy policy, sidecar naming, SQLite file-size checks, or deletion rules.
- UI/widgets do not perform direct file existence, file size, sidecar, or schema checks.
- Future reset/delete UI must call SQLUICore database management helper/policy surfaces and must not delete files directly.

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

Status refresh should be read-only and caller-invoked. It should not create directories, create databases, initialize schema, copy seed databases, delete files, poll/tick automatically, or change the active repository.

Basic file status should use the closed-file database management helper. The migration/status portion may open an existing SQLite database read-only through the SQLUICore schema versioning helper. It must not apply migrations or create missing database files.

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

The first proof slice implements this as a SQLUICore status snapshot plus a display-row adapter and smoke coverage, not a widget. The next UI PR can bind a small panel to those rows without adding settings editing or destructive actions.

The read-only panel contract above is now the recipe for that next UI PR. It should bind rows from `USQLUISamplePersistenceStatusPanelWidget`, `USQLUISamplePersistenceStatusPanelAdapter`, `USQLUISamplePersistenceStatusPresenter`, or directly from `USQLUIPersistenceStatusDisplayLibrary`, keep refresh caller-invoked only, and avoid backend selection, path editing, settings save/apply, reset/delete, and startup wiring.

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
