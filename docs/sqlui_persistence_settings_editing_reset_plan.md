# SQLUI Persistence Settings Editing And Reset Plan

This document plans the next SQLUI persistence settings UX phase after the completed read-only status foundation. PR #105 was a docs-only design and sequencing PR. The first implementation slices now add a non-mutating SQLUICore draft/pending settings model, validation-only behavior, UI-safe validation display rows, a dry-run apply-intent preview, a SQLUISamples sample/dev adapter that consumes those display rows, and an optional C++ UMG widget shell contract for future Blueprint binding; they still do not implement settings editing UI controls, backend selector controls, SQLite path editor controls, provider auto-init toggle controls, reset/delete behavior, settings save/apply behavior, startup wiring, widget blueprint assets, config, Build.cs changes, plugin descriptor changes, maps, Content assets, packaged outputs, or packaged runtime behavior.

Related docs:

- [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) defines the broader persistence settings UX policy.
- [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md) documents the current read-only UMG binding recipe.
- [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) documents the validation/preview-only draft settings UMG binding recipe.
- [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md) summarizes implemented SQLite runtime behavior and safety boundaries.
- [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md) tracks current phase status and next slices.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) describes repository, factory, provider, and widget ownership boundaries.
- [`sqlui_smoke_test.md`](sqlui_smoke_test.md) lists local editor smoke commands.
- [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md) documents local packaged validation and packaged runtime smoke paths.

## Purpose

The next UI phase should let a future app or sample surface edit persistence settings safely without turning the read-only status panel into a hidden persistence lifecycle owner.

The planned surface may eventually support:

- Backend selection.
- SQLite path configuration.
- Provider auto-init policy display/editing.
- Validation before applying settings.
- Pending/apply/cancel semantics.
- User-readable restart, reopen, or reinitialize messaging.
- Reset/delete UX routed through SQLUICore database management helper and policy surfaces.

The goal is a boring, reviewable path from read-only status display to mutating settings UX. Widgets should remain presentation and intent-capture layers. SQLUICore should remain the source of truth for persistence settings policy, path resolution, repository/provider lifecycle, schema/migration state, seed-copy policy, and database management.

## Non-Goals

This planning slice does not add:

- Settings editing.
- Backend selector controls.
- SQLite path editor controls.
- Provider auto-init toggle controls.
- Reset/delete buttons.
- Migration execution controls.
- Seed-copy controls.
- Settings save/apply behavior.
- Startup wiring.
- Provider initialization behavior.
- Repository initialization behavior.
- Widget blueprint assets.
- Maps, viewport attachment, timers, polling, or auto-refresh.
- Default config changes.
- Script or smoke-flag changes.
- Runtime code.

Future implementation PRs should keep each of those behaviors explicitly scoped and separately validated.

## Existing Read-Only Foundation

The current foundation is intentionally read-only:

- SQLUICore exposes `USQLUIPersistenceStatusLibrary` for persistence status snapshots.
- SQLUICore exposes `USQLUIPersistenceStatusDisplayLibrary` for UI-safe display rows.
- SQLUISamples exposes optional sample/dev presenter, panel adapter, and C++ UMG widget shell surfaces.
- Refresh is caller-invoked and re-queries SQLUICore status/display rows.
- The sample surface is not wired into startup, maps, config, viewport, timers, tick, polling, or auto-refresh.

That foundation does not create databases, run migrations, copy seeds, initialize providers or repositories, save settings, switch backends, reset databases, delete files, or change normal startup behavior.

## Non-Negotiable Safety Constraints

Future editable persistence UI must preserve these constraints:

- `InMemory` remains the safe default backend.
- SQLite remains opt-in.
- Provider auto-init remains off by default.
- No default config creates SQLite database files.
- No default config enables provider auto-init.
- SQLite-by-default or provider-auto-init default changes require a separate explicit policy PR.
- Writable runtime database state stays under `Saved/SQLUI/...` unless a later explicit path policy allows otherwise.
- JerryRigged remains a thin host.
- SQLUICore remains the persistence authority.
- SQLUICore stays free of UMG, Slate, SlateCore, widget, and editor-only dependencies for persistence settings policy.
- SQLUISamples may provide optional sample/dev UI scaffolding but must not own persistence internals.
- Widgets do not know SQL strings, schema tables, migration ids, seed-copy internals, SQLite sidecar naming, or raw database worker details.
- Widgets do not perform direct file checks, direct file deletion, provider initialization, repository initialization, schema initialization, migrations, seed copy, or database creation.
- Reset/delete behavior routes through SQLUICore database management helper/policy surfaces, not widget-owned file deletion.
- Settings edits use pending/apply semantics instead of mutating live persistence from bindings.

## Pending / Apply / Cancel Semantics

Editable settings should have an explicit pending state:

- UI controls edit a pending settings model.
- The current applied/runtime state remains visible separately.
- Validation runs against pending values before apply.
- Dry-run or validation-only checks should be available before any mutating apply path is introduced.
- Apply sends a validated request to SQLUICore helper or policy surfaces.
- Cancel discards pending values and restores the current applied display.
- Validation failures are user-readable and non-destructive.
- Applying settings should not silently initialize providers, create repositories, create databases, or run schema initialization unless that behavior is explicitly part of the pending policy and the user confirmed it.
- If a change requires restart, reopen, provider reset, or provider reinitialization, the UI should say so instead of forcing lifecycle changes from a widget binding.

The first implementation should prefer a non-mutating pending model and validation helper before any UI control can change live runtime persistence.

That first implementation slice now exists as `FSQLUIPersistenceSettingsDraft`, `FSQLUIPersistenceSettingsValidationResult`, `FSQLUIPersistenceSettingsApplyPreviewResult`, and `USQLUIPersistenceSettingsDraftLibrary`. It can create a draft from current/default runtime settings, reset a draft value back to current values, validate pending backend/path/provider-auto-init policy, and build a dry-run apply-intent preview without applying anything. `USQLUIPersistenceSettingsDraftDisplayLibrary` formats validation results into UI-safe summary/row data for future sample or settings panels. `USQLUISamplePersistenceSettingsDraftPresenter` and `USQLUISamplePersistenceSettingsDraftPanelWidget` provide optional SQLUISamples sample/dev-facing consumption surfaces for those rows. These draft, preview, display, adapter, and widget-shell helpers do not write config, initialize providers/repositories, create SQLite database files, create directories from display generation or preview generation, open databases for writing, run migrations, copy seed databases, delete files, add settings controls, or change startup behavior. The safe Blueprint/UMG binding recipe for the draft widget shell is documented in [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md).

## Completed Draft Validation Foundation

The non-mutating draft validation UI foundation is complete through the #105-#111 sequence:

- #105 documented this editing/reset UX plan.
- #106 added the SQLUICore validation-only draft/pending settings model.
- #107 added UI-safe validation display rows and summary.
- #108 added the optional SQLUISamples draft validation presenter/adapter.
- #109 added the optional SQLUISamples C++ UMG widget shell for draft validation display.
- #110 documented safe UMG subclassing and binding for that shell.
- #111 records the final non-mutating draft validation foundation checkpoint.
- `-UsePersistenceSettingsDraftProbe` validates the draft model, display rows, sample adapter, and widget-shell contract without widget blueprint assets, maps, viewport attachment, startup wiring, settings mutation, or provider/repository initialization.

This checkpoint is still not settings editing. It adds no backend selector UI, SQLite path editor UI, provider auto-init control, Apply/Cancel behavior, settings save/config-write behavior, reset/delete UX, widget blueprint asset, map, startup wiring, viewport attachment, polling, ticking, auto-refresh, provider/repository initialization, migration, seed-copy behavior, or default config change. Refresh/build/validation/preview remains caller-invoked only.

## Dry-Run Apply-Intent Preview Slice

The first apply/cancel phase code slice is non-mutating only. `USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply` now evaluates a validated draft and reports whether a future Apply would have changes, whether backend/SQLite/provider-auto-init policy would change, and whether restart/reopen/reinitialize messaging should be shown.

The preview deliberately uses future-oriented wording such as "would change" and "not applied." It does not apply settings, save config, create directories or database files, open databases for writing, run migrations, copy seed databases, initialize providers/repositories, reset databases, delete files, add settings controls, change defaults, or change startup behavior. `-UsePersistenceSettingsDraftProbe` covers the preview alongside draft validation/display rows and verifies default/current no-change previews, backend/SQLite/provider-auto-init change previews, invalid backend/path rejection, deterministic output, sidecar preservation, and smoke-owned cleanup.

The next apply/cancel phase should build on this foundation and must keep widgets as presentation/intent-capture layers. Future helpers should use SQLUICore policy surfaces, keep widgets from writing config directly, keep widgets from initializing providers/repositories directly, keep widgets from deleting files directly, keep runtime database writes under `Saved/SQLUI`, show restart/reopen/reinitialize-required messaging instead of forcing lifecycle changes, and keep validation failures user-readable and non-destructive. Reset/delete behavior must route through SQLUICore database management helper/policy surfaces.

## Backend Selection Design

The backend selector should show the current configured backend and allow only supported values:

- `InMemory`
- `JsonFile`
- `SQLite`

`Unavailable` should usually be status-only unless a debug or smoke path explicitly needs it.

Selecting a backend should not by itself:

- Create a database.
- Create directories.
- Run migrations.
- Copy a seed database.
- Initialize a provider.
- Initialize a repository.
- Save settings.
- Change the live repository.

SQLite selection remains explicit and advanced compared with `InMemory`. The selector should make the safe default obvious and should require Apply before any requested backend can become active. If an applied backend change requires restart, reopen, provider reset, or provider reinitialization, the UI should report that requirement clearly instead of forcing lifecycle work from the selector itself.

Backend selection UI must not change default product policy by itself. Any SQLite-by-default or provider-auto-init-by-default change remains out of scope for ordinary settings UI and requires a separate explicit policy PR.

## SQLite Path Editing Design

The path field should distinguish configured input from resolved runtime path:

- Configured value: what the user or settings model stores.
- Resolved path: the SQLUICore-normalized runtime path that would be used.

SQLite path display can remain read-only in early slices. If editing is added later:

- Use SQLUICore resolver/policy surfaces for normalization.
- Prefer relative paths under `Saved/SQLUI/LayoutRepositories`.
- Treat absolute paths as advanced and policy-controlled.
- Validate empty, invalid, unsafe, or source-controlled-looking paths before Apply.
- Do not create files or directories during edit or validation.
- Do not browse, inspect, or delete files directly from the widget.
- Do not perform direct file existence, file size, or SQLite sidecar checks from the widget.
- Do not run migrations, copy seeds, initialize providers, or initialize repositories from path editing or validation.
- Do not allow writes outside `Saved/SQLUI` unless a later explicit policy PR allows and validates that behavior.
- Report missing paths clearly without creating a database.

Path editing should never be a hidden database initialization action.

## Provider Auto-Init Policy Design

Provider auto-init affects startup behavior and should stay advanced:

- It remains off by default.
- It should be shown as runtime policy, not as a simple visual preference.
- It should require explicit Apply.
- It should not be changed in the same PR that wires a UI into default startup unless that PR includes matching startup and packaged validation.
- It should not be bundled silently with unrelated UI work.
- Command-line override policy should remain visible when relevant so developers can understand whether runtime command-line settings can override config-backed settings.

A future UI can explain that provider auto-init only requests startup initialization through the existing settings/subsystem/provider path. It should not make SQLite the default by implication.

## Reset / Delete UX Design

Reset/delete UX is destructive and must stay SQLUICore-mediated.

A future reset control should:

- Show the resolved target database path.
- Show whether the target appears to exist.
- Show whether SQLite sidecar files appear to exist.
- Require confirmation.
- Be disabled for non-SQLite backends unless the product adds a backend-specific reset policy.
- Be disabled when the resolved SQLite path is empty or unsafe.
- Remove only policy-approved runtime database files under `Saved/SQLUI`.
- Remove SQLite sidecars through SQLUICore helper/policy behavior.
- Never delete broad directories.
- Never delete arbitrary user-selected files.
- Never delete seed databases.
- Never infer sidecar names in widget code.
- Never run while the active repository is using the database unless the app has explicitly shut down, reset, or reinitialized the provider according to a SQLUICore policy.

Smoke coverage for future reset/delete behavior should include safe no-op, success, failure, and sidecar cleanup paths. If reset/delete controls are exposed in packaged runtime UI, the matching PR should include packaged validation.

Reset database is different from repository operations:

- `RemoveLayout` soft-deletes one layout row through repository behavior.
- `ClearLayouts` destructively clears layout rows and dependents inside the configured database scope.
- Reset database removes the resolved database file and sidecars through database management policy.

Those actions should have distinct labels, explanations, and confirmations.

## Validation / Testing Plan

Future mutating settings/reset PRs should add focused smoke coverage as behavior appears:

- Pending settings model can change values without mutating live runtime state. The first validation/preview-only smoke path is `-UsePersistenceSettingsDraftProbe`, which also verifies the dry-run apply-intent preview, UI-safe validation display rows, the SQLUISamples sample adapter that consumes them, and the optional C++ UMG widget shell contract by reflection without widget blueprint assets, maps, viewport attachment, or startup wiring.
- Cancel discards pending values.
- Validation rejects empty or unsafe SQLite paths without creating files.
- Backend selection requires Apply.
- SQLite selection does not create a database until explicitly configured behavior does so.
- Provider auto-init remains off by default.
- Default config does not create a SQLite database.
- Default config does not initialize providers.
- Status refresh remains read-only.
- Reset helper removes only the resolved SQLite DB and sidecars.
- Reset helper no-ops safely for missing files.
- Reset helper fails clearly for unsafe or empty paths.
- Smoke-owned database files and sidecars are cleaned up.

Packaged validation should be included when a PR changes startup behavior, default maps, default config, viewport attachment, provider lifecycle, or packaged runtime persistence behavior. Docs-only planning does not require full packaged validation.

## Proposed Implementation Sequence

Recommended future sequence:

1. Add this docs-only plan.
2. Add a non-mutating pending settings model or view-model. Complete as the SQLUICore persistence settings draft model.
3. Add validation-only SQLUICore policy helpers for editable settings. Complete for the first backend/path/provider-auto-init draft checks.
4. Add UI-safe validation display rows for draft validation results. Complete as `USQLUIPersistenceSettingsDraftDisplayLibrary`.
5. Add a sample/dev-facing SQLUISamples adapter for validation display rows. Complete as `USQLUISamplePersistenceSettingsDraftPresenter`.
6. Add an optional C++ UMG widget shell contract for draft validation rows. Complete as `USQLUISamplePersistenceSettingsDraftPanelWidget`.
7. Add a non-mutating dry-run apply-intent preview. Complete as `FSQLUIPersistenceSettingsApplyPreviewResult` and `PreviewPersistenceSettingsDraftApply`.
8. Add actual Apply/Cancel helper APIs without widget controls.
9. Add a backend selector UI shell that edits pending state only.
10. Add SQLite path display/edit validation that does not create files.
11. Add reset UX confirmation and smoke coverage through SQLUICore database management helpers.
12. Add packaged validation for any startup, config, default map, viewport, or packaged lifecycle integration.

Each implementation PR should keep the blast radius narrow and should not combine default policy changes with UI scaffolding.

## Packaged Validation Policy

The validation-only draft model, dry-run apply-intent preview, UI-safe validation display rows, sample/dev-facing adapter, and optional C++ UMG widget shell do not require full packaged validation because they do not touch maps, config, startup behavior, package behavior, viewport attachment, provider lifecycle, or packaged lifecycle flow. They add runtime code and editor commandlet smoke coverage only for non-mutating validation/preview/display formatting, sample consumption, and reflection-validated binding contracts.

Future PRs that wire settings UI into startup, default maps, config-backed provider lifecycle, viewport attachment, or packaged runtime persistence should run packaged validation appropriate to that behavior.

The earlier unresolved `__std_*` packaged linker issue was resolved locally by installing the UE 5.7-preferred Visual Studio 2022 MSVC `14.44.x` toolchain. If similar unresolved MSVC STL helper symbols appear again with MSVC `14.38.x` or Visual Studio Preview toolchains, treat them as local toolchain-selection evidence unless UBT logs point to SQLUI or SQLiteCore objects.

## Remaining Work

This plan still leaves the mutating UI and lifecycle work future-scoped:

- Apply/cancel behavior.
- Backend selector UI.
- SQLite path editing UI.
- Provider auto-init control.
- Reset/delete UX.
- Provider shutdown/reinitialize policy for reset.
- Packaged validation for startup or packaged runtime UI integration.
- Future migration/version upgrade UX beyond the current initial schema policy.
