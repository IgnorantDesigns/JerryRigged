# SQLUI Persistence Settings Editing And Reset Plan

This document plans the next SQLUI persistence settings UX phase after the completed read-only status foundation. PR #105 was a docs-only design and sequencing PR. The first implementation slices now add a non-mutating SQLUICore draft/pending settings model, validation-only behavior, UI-safe validation display rows, a dry-run apply-intent preview, a non-mutating apply/cancel contract, UI-safe apply-preview display rows, UI-safe apply/cancel contract display rows, a callable actual apply entrypoint skeleton that still returns unavailable/not implemented without mutation, UI-safe apply-result display rows, SQLUISamples sample/dev adapters for validation, apply-preview, apply/cancel contract, and apply-result display rows, and optional C++ UMG widget shell contracts for future Blueprint binding, including the apply-result shell; they still do not implement settings editing UI controls, backend selector controls, SQLite path editor controls, provider auto-init toggle controls, reset/delete behavior, settings save/apply execution, startup wiring, widget blueprint assets, config, Build.cs changes, plugin descriptor changes, maps, Content assets, packaged outputs, or packaged runtime behavior.

Related docs:

- [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) defines the broader persistence settings UX policy.
- [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md) documents the current read-only UMG binding recipe.
- [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) documents the validation-only draft settings UMG binding recipe.
- [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md) documents the dry-run apply-preview UMG binding recipe.
- [`sqlui_persistence_settings_apply_contract_umg_usage.md`](sqlui_persistence_settings_apply_contract_umg_usage.md) documents the apply/cancel contract UMG binding recipe.
- [`sqlui_persistence_settings_apply_result_umg_usage.md`](sqlui_persistence_settings_apply_result_umg_usage.md) documents the apply-result UMG binding recipe.
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

That first implementation slice now exists as `FSQLUIPersistenceSettingsDraft`, `FSQLUIPersistenceSettingsValidationResult`, `FSQLUIPersistenceSettingsApplyPreviewResult`, `FSQLUIPersistenceSettingsApplyContractResult`, `FSQLUIPersistenceSettingsCancelPreviewResult`, `FSQLUIPersistenceSettingsApplyRequest`, `FSQLUIPersistenceSettingsApplyResult`, and `USQLUIPersistenceSettingsDraftLibrary`. It can create a draft from current/default runtime settings, reset a draft value back to current values, validate pending backend/path/provider-auto-init policy, build a dry-run apply-intent preview, report non-executing future-apply readiness, describe cancel/discard as a pure value preview, and accept an actual apply request that returns unavailable/not implemented without applying anything. `USQLUIPersistenceSettingsDraftDisplayLibrary`, `USQLUIPersistenceSettingsApplyPreviewDisplayLibrary`, `USQLUIPersistenceSettingsApplyContractDisplayLibrary`, and `USQLUIPersistenceSettingsApplyResultDisplayLibrary` format validation results, apply-preview results, apply/cancel contract results, and apply entrypoint result data into UI-safe summary/row data for future sample or settings panels. `USQLUISamplePersistenceSettingsDraftPresenter`, `USQLUISamplePersistenceSettingsApplyPreviewPresenter`, `USQLUISamplePersistenceSettingsApplyContractPresenter`, `USQLUISamplePersistenceSettingsApplyResultPresenter`, `USQLUISamplePersistenceSettingsDraftPanelWidget`, `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget`, `USQLUISamplePersistenceSettingsApplyContractPanelWidget`, and `USQLUISamplePersistenceSettingsApplyResultPanelWidget` provide optional SQLUISamples sample/dev-facing consumption surfaces for validation/apply-preview/apply-contract/apply-result rows. These draft, preview, contract, apply-request skeleton, apply-result display, display, adapter, and widget-shell helpers do not write config, change settings, initialize providers/repositories, create SQLite database files, create directories, open databases for writing, run migrations, copy seed databases, delete files, add settings controls, or change startup behavior. The safe Blueprint/UMG binding recipes are documented in [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) for draft validation, [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md) for dry-run apply-preview display, [`sqlui_persistence_settings_apply_contract_umg_usage.md`](sqlui_persistence_settings_apply_contract_umg_usage.md) for apply/cancel contract display, and [`sqlui_persistence_settings_apply_result_umg_usage.md`](sqlui_persistence_settings_apply_result_umg_usage.md) for apply-result display.

## Completed Apply/Cancel Contract Foundation

The non-mutating persistence settings apply/cancel contract UI foundation is complete through the #105-#124 sequence. It is a display, adapter, widget-shell, and documentation checkpoint only; it is still not settings editing and still does not implement actual Apply, Save, Cancel mutation, or config writes.

- #105 documented this editing/reset UX plan.
- #106 added the SQLUICore validation-only draft/pending settings model.
- #107 added UI-safe validation display rows and summary.
- #108 added the optional SQLUISamples draft validation presenter/adapter.
- #109 added the optional SQLUISamples C++ UMG widget shell for draft validation display.
- #110 documented safe UMG subclassing and binding for the validation shell and shared validation/preview boundaries.
- #111 records the final non-mutating draft validation foundation checkpoint.
- #112 added the non-mutating dry-run apply-intent preview.
- #113 added UI-safe apply-preview display rows/summary that format what a future Apply would do without applying anything.
- #114 added the optional SQLUISamples sample/dev-facing apply-preview presenter that consumes those SQLUICore display rows and caches rows/summary strings for future sample UI work.
- #115 added the optional apply-preview C++ UMG widget shell that delegates to the apply-preview presenter and caches rows/summary/preview flags for future Blueprint binding.
- #116 documented safe subclassing and binding for the #115 `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget` shell without adding actual Apply/Save behavior or settings controls.
- #117 records the final non-mutating apply-preview foundation checkpoint.
- #118 added the non-mutating apply/cancel contract: non-executing future-apply readiness plus cancel/discard value preview data without actual Apply, Save, config-write, or live Cancel behavior.
- #119 added apply/cancel contract display rows that format readiness and cancel/discard value preview into UI-safe rows/summary without adding settings controls or mutation.
- #120 documented the latest Unreal Editor null-RHI validation checkpoint for this persistence/settings foundation.
- #121 added the optional SQLUISamples apply/cancel contract sample adapter for those UI-safe contract rows without adding actual Apply, settings controls, widget assets, startup wiring, or persistence lifecycle mutation.
- #122 added the optional apply/cancel contract C++ UMG widget shell over that sample adapter without adding actual Apply, settings controls, widget assets, startup wiring, or persistence lifecycle mutation.
- #123 documented safe future Blueprint subclassing/binding for that shell without adding widget assets, settings controls, startup wiring, viewport attachment, auto-refresh, or persistence lifecycle mutation.
- #124 records the final docs-only non-mutating apply/cancel contract UI foundation checkpoint before any actual Apply/Save/config-write behavior or settings controls.
- `-UsePersistenceSettingsDraftProbe` validates the draft model, apply/cancel contract, validation display rows, apply preview display rows, apply/cancel contract display rows, apply-result display rows, validation/apply-preview/apply-contract/apply-result sample adapters, and validation/apply-preview/apply-contract/apply-result widget-shell contracts without widget blueprint assets, maps, viewport attachment, startup wiring, settings mutation, or provider/repository initialization.

This checkpoint is still not settings editing. It adds no backend selector UI, SQLite path editor UI, provider auto-init control, actual Apply/Cancel execution, settings save/config-write behavior, reset/delete UX, widget blueprint asset, map, startup wiring, viewport attachment, polling, ticking, auto-refresh, provider/repository initialization, migration, seed-copy behavior, or default config change. Refresh/build/validation/preview/contract generation remains caller-invoked only.

The next mutating apply/cancel phase must remain SQLUICore-first. Future implementation should be split into small PRs: first mutating behavior behind the SQLUICore request/result skeleton, then config-write behavior only after validation/preview/contract surfaces are stable, then UI controls only after SQLUICore behavior has focused smoke coverage. Widgets should keep consuming SQLUICore display/status/contract data and should not write config, initialize providers/repositories, create databases or directories, run migrations, copy seeds, or delete files directly. Any startup/default map/config/provider lifecycle change needs packaged validation.

## Completed Apply Result Foundation

The non-mutating persistence settings apply entrypoint/result UI foundation is complete through the #105-#131 sequence. It builds on the completed validation, apply-preview, and apply/cancel contract foundations, then adds the #125 actual apply implementation gate, #126 callable `RequestPersistenceSettingsApply` skeleton, #127 UI-safe apply-result display rows/summary, #128 optional SQLUISamples apply-result sample adapter, #129 optional apply-result C++ UMG widget shell, #130 apply-result UMG usage guide, this #131 final docs-only checkpoint, and `-UsePersistenceSettingsDraftProbe` smoke coverage.

This closes the display/adapter/widget-shell chain for apply results before actual config-write behavior begins. It is still not settings editing. It adds no actual apply/save/config-write behavior, backend selector UI, SQLite path editor UI, provider auto-init control, reset/delete UX, widget Blueprint asset, map, startup wiring, viewport attachment, polling, ticking, timer, auto-refresh, provider/repository initialization, migration, seed-copy behavior, or default config change. Refresh/build/validation/preview/contract/apply-result generation remains caller-invoked only.

## Smoke-Owned Apply Config Target Checkpoint

PR #132 added the first write-capable scaffold after the non-mutating foundation, but it is limited to an explicit SQLUICore smoke-owned config target. It gives future Apply work a concrete `FSQLUIPersistenceSettingsApplyConfigTarget` abstraction and lets editor smoke tests write only to a temporary `Saved/SQLUI/SmokeTests` ini artifact, then remove that artifact. The production/default `RequestPersistenceSettingsApply` entrypoint still returns unavailable/not implemented, does not write config, and does not change live settings.

The scaffold is not a user/runtime Apply implementation. It does not write committed `Config` defaults, generated `Saved/Config`, user/global editor settings, real runtime settings, or project defaults. It does not create SQLite databases, open databases for writing, run migrations, copy seeds, initialize providers/repositories, reset/delete files, add settings controls, change startup behavior, make SQLite the default backend, or enable provider auto-init by default.

`-UsePersistenceSettingsDraftProbe` now covers the target boundary: default/runtime targets are refused, unsafe project config paths are refused, invalid drafts do not mutate the smoke target, valid drafts write only expected narrow values to the smoke-owned target, repo config snapshots remain unchanged, SQLite DB files are not created, provider auto-init policy remains unchanged, lifecycle work does not run, and the smoke-owned target is cleaned up.

The apply entrypoint skeleton and apply-result display path remain non-mutating. They can report validation-blocked, no-change, preview-only, and unavailable/not-implemented result states; they do not write config, apply settings, change provider auto-init policy, initialize providers/repositories, create directories or database files, open databases for writing, run migrations, copy seed databases, reset/delete data, or change startup behavior.

This checkpoint proves a narrow set of mechanics only:

- SQLUICore can distinguish default/runtime targets from explicit smoke/test-owned write targets.
- Smoke tests can exercise isolated config serialization without touching repo `Config`, generated `Saved/Config`, committed defaults, or user/global editor settings.
- Draft validation runs before any smoke-owned write.
- Invalid drafts are refused without mutating the smoke-owned target.
- No-change drafts remain no-op writes.
- Smoke-owned artifacts can be cleaned deterministically.
- Runtime/default Apply stays unavailable and does not call the smoke-owned writer.

The first actual config-write implementation must now build on this foundation. It should use SQLUICore helper/policy surfaces, keep writes narrow and explicit, keep widgets ignorant of SQL, schema, migrations, seed-copy policy, sidecar internals, deletion behavior, and provider/repository lifecycle, keep widgets from writing config directly, keep runtime DB writes under `Saved/SQLUI`, validate drafts before writing any config, refuse invalid drafts without mutation, support no-change/no-op behavior, keep validation failures user-readable and non-destructive, avoid silently initializing providers/repositories, and show restart/reopen/reinitialize-required messaging instead of forcing lifecycle changes. Reset/delete UX remains separate and must route through SQLUICore database management helper/policy surfaces.

Future mutating work should stay split into small PRs: first a real target policy decision, then a narrow backend/provider-auto-init config-write slice only when safe, then validation/no-op/failure paths, then smoke-owned versus real-target separation checks, and then UI controls only after SQLUICore behavior is fully tested. Any such path needs smoke coverage, config diff/snapshot checks, cleanup/restore for smoke-owned artifacts, and packaged validation if startup behavior, default maps, config wiring, provider lifecycle, viewport flow, packaged runtime behavior, or committed/default runtime config changes.

## Actual Apply Implementation Gate

PR #125 is documentation-only: it records this actual apply implementation gate and adds no runtime code, scripts, config changes, smoke flags, UI controls, settings editing, or apply/save/config-write behavior.

The first SQLUICore actual apply entrypoint skeleton now exists as `RequestPersistenceSettingsApply`. It accepts `FSQLUIPersistenceSettingsApplyRequest`, evaluates the existing apply contract, returns `FSQLUIPersistenceSettingsApplyResult`, and deliberately keeps execution unavailable/not implemented. It can report validation-blocked, no-change, and preview-only states, but `bSucceeded` remains false, `bActualApplyImplemented` remains false, and all side-effect flags remain false.

Actual apply/save/config-write behavior has not been implemented yet. The current `PreviewPersistenceSettingsDraftApply`, `BuildPersistenceSettingsApplyContract`, `BuildPersistenceSettingsCancelPreview`, and `RequestPersistenceSettingsApply` helpers remain non-mutating report/preview/refusal paths. They can validate and describe a future apply, but they do not save settings, write config, initialize provider or repository state, create directories, create database files, open databases for writing, run migrations, copy seed databases, delete files, or change startup behavior.

The first actual apply implementation must be SQLUICore-first. Widgets and SQLUISamples must keep acting as presentation and intent-capture layers; they must not write config, mutate `USQLUILayoutRepositoryRuntimeSettings`, initialize providers/repositories, call repository factory behavior directly for apply, or perform database/file lifecycle work. A future mutating entrypoint should validate an `FSQLUIPersistenceSettingsDraft`, refuse invalid drafts without mutation, write only explicitly scoped persistence settings, and return user-readable success/failure/no-op/restart/reopen/reinitialize messaging.

If the current architecture does not yet provide a safe write target, the first implementation PR must stop at a clear not-implemented or dry-run-only result instead of inventing unsafe config writes.

### Config-Write Boundaries

Future config writes must be explicit and narrow. They should use SQLUICore runtime settings/config policy surfaces already used by the resolver and settings policy where practical, and they must not mutate committed defaults such as `DefaultEngine.ini` in normal runtime/user apply paths.

Smoke coverage for config writes must use a smoke-owned config context or another explicitly isolated test surface, then clean up or restore any temporary artifacts. Smoke must also prove the repo default config remains safe after the test: `InMemory` remains the default backend, SQLite is not enabled by default, provider auto-init is not enabled by default, and no committed config creates SQLite database files.

Actual apply code must never write user/global editor settings unexpectedly. If the intended target is unclear, the implementation must report that apply is unavailable rather than choosing a broad or persistent write location.

### Lifecycle Boundaries

Actual apply/save/config-write behavior must not initialize or reinitialize the runtime provider/repository by default. It must not create SQLite database files or directories, open database files for writing, run migrations, copy seed databases, delete DB files or sidecars, or change the currently active repository as a hidden side effect.

When a settings change requires a restart, editor reopen, provider reset, or provider reinitialization, apply should return clear messaging instead of forcing lifecycle work from the apply path. Any future lifecycle-changing apply path needs a separately scoped PR, focused smoke coverage, and packaged validation if startup/config/default-map/provider lifecycle or packaged runtime behavior changes.

### Reset/Delete Separation

Reset/delete UX remains separate from settings apply/save. Reset/delete must use `FSQLUILayoutRepositoryDatabaseManagement` or another SQLUICore-owned policy surface, never widget-owned file deletion. Apply/save must not reset, clear, delete, or overwrite database files as a side effect.

`RemoveLayout`, `ClearLayouts`, and database reset are still distinct concepts. Apply/save changes configuration intent; it must not perform repository row cleanup or database file maintenance unless a future PR explicitly scopes and validates a separate maintenance action.

### First Mutating PR Sequence

Recommended sequence for the first actual apply implementation work:

1. Add validation-gated mutating apply behavior behind the existing SQLUICore request/result skeleton only when the target is explicit and safe.
2. Add config write for a narrow subset only when the target is explicit and safe, such as backend/provider-auto-init fields if the current settings policy supports smoke-owned writes.
3. Add smoke coverage for a successful apply into a smoke-owned temporary config context with cleanup/restore.
4. Add failed validation, no-change, and no-op smoke paths.
5. Add restart/reopen/reinitialize-required result messaging without forcing lifecycle changes.
6. Add SQLUISamples sample adapter or UI-shell consumption only after SQLUICore apply behavior is tested.
7. Add actual UI controls only after the SQLUICore apply path and sample consumption path are stable.
8. Run packaged validation for any PR that changes startup behavior, default maps, config wiring, provider lifecycle, packaged runtime behavior, or actual config writes that can affect packaged startup.

### Required Validation For First Mutating Apply PRs

Future mutating apply PRs should include focused validation for both success and refusal paths:

- `Build.bat JerryRiggedEditor Win64 Development`.
- `RunSQLUISmokeTest.ps1 -Help`.
- Default SQLUI smoke.
- `-UsePersistenceStatusSampleSurfaceProbe`.
- `-UsePersistenceSettingsDraftProbe`.
- A new or extended apply/config-write smoke probe.
- Valid apply in a smoke-owned config context.
- Invalid draft refusal with no mutation.
- No-change/no-op apply behavior.
- No SQLite database files or directories created by config write alone.
- No migrations, seed copy, provider initialization, repository initialization, or repository reinitialization from config write alone.
- Smoke-owned config cleanup/restore.
- Proof that repo default config still keeps `InMemory` default, SQLite disabled by default, and provider auto-init disabled by default.
- Packaged validation if startup behavior, default maps, config wiring, provider lifecycle, or packaged runtime behavior changes.

### Manual Editor Validation Recommendation

Before the first real config-write PR merges, a human should perform a manual editor inspection from clean `main` if practical. That inspection should verify the future UI or sample flow does not save maps/assets/config unintentionally and does not create database files from display, validation, preview, contract, or apply refusal paths.

Local throwaway widget Blueprints are acceptable for manual inspection only when they are not committed. Do not claim manual validation happened unless it actually did.

## Dry-Run Apply-Intent Preview Slice

The first apply/cancel phase code slices are non-mutating only. `USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply` evaluates a validated draft and reports whether a future Apply would have changes, whether backend/SQLite/provider-auto-init policy would change, and whether restart/reopen/reinitialize messaging should be shown. `BuildPersistenceSettingsApplyContract` wraps that preview with an explicit availability/readiness contract and reports that actual Apply execution is not implemented. `RequestPersistenceSettingsApply` is the callable apply entrypoint skeleton that returns unavailable/not implemented, validation-blocked, no-change, or preview-only result data without touching live settings. `BuildPersistenceSettingsCancelPreview` reports whether cancel would discard pending values and returns the draft value that would result, without touching live settings.

The preview, contract, apply request skeleton, apply-result display rows, optional SQLUISamples apply-preview presenter, optional SQLUISamples apply/cancel contract presenter, optional SQLUISamples apply-result presenter, optional apply-preview widget shell, optional apply/cancel contract widget shell, and optional apply-result widget shell deliberately use future-oriented wording such as "would change," "not implemented," "not applied," "not saved," "config not written," and "database files not created." They do not apply settings, save config, create directories or database files, open databases for writing, run migrations, copy seed databases, initialize providers/repositories, reset databases, delete files, add settings controls, change defaults, or change startup behavior. `-UsePersistenceSettingsDraftProbe` covers the preview, contract, apply request skeleton, apply-result display rows, apply-result adapter, and apply-result widget shell contract alongside draft validation/display rows, apply-preview display rows/adapters, apply/cancel contract display rows/adapter, and widget shell contracts, and verifies default/current no-change previews, backend/SQLite/provider-auto-init change previews, invalid backend/path rejection, cancel/discard value preview, deterministic output, sidecar preservation, config preservation, and smoke-owned cleanup.

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

- Pending settings model can change values without mutating live runtime state. The first validation/preview/contract-only smoke path is `-UsePersistenceSettingsDraftProbe`, which also verifies the dry-run apply-intent preview, non-mutating apply/cancel contract, unavailable apply request skeleton, UI-safe validation display rows, UI-safe apply-preview display rows, UI-safe apply/cancel contract display rows, UI-safe apply-result display rows, SQLUISamples sample adapters that consume validation, apply-preview, apply/cancel contract, and apply-result rows, and the optional validation/apply-preview/apply-contract/apply-result C++ UMG widget shell contracts by reflection without widget blueprint assets, maps, viewport attachment, or startup wiring.
- Cancel preview reports that pending values would be discarded without mutating live state.
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

1. Add this docs-only plan and the actual apply implementation gate.
2. Add a non-mutating pending settings model or view-model. Complete as the SQLUICore persistence settings draft model.
3. Add validation-only SQLUICore policy helpers for editable settings. Complete for the first backend/path/provider-auto-init draft checks.
4. Add UI-safe validation display rows for draft validation results. Complete as `USQLUIPersistenceSettingsDraftDisplayLibrary`.
5. Add a sample/dev-facing SQLUISamples adapter for validation display rows. Complete as `USQLUISamplePersistenceSettingsDraftPresenter`.
6. Add an optional C++ UMG widget shell contract for draft validation rows. Complete as `USQLUISamplePersistenceSettingsDraftPanelWidget`.
7. Add a non-mutating dry-run apply-intent preview. Complete as `FSQLUIPersistenceSettingsApplyPreviewResult` and `PreviewPersistenceSettingsDraftApply`.
8. Add UI-safe apply-preview display rows, a sample/dev adapter, and an optional C++ UMG widget shell for preview rows. Complete as the apply-preview display/presenter/widget-shell path.
9. Add a non-mutating Apply/Cancel contract without widget controls. Complete as `FSQLUIPersistenceSettingsApplyContractResult`, `FSQLUIPersistenceSettingsCancelPreviewResult`, and the `USQLUIPersistenceSettingsDraftLibrary` contract helpers.
10. Add UI-safe apply/cancel contract display rows for future settings panels. Complete as a SQLUICore-only display formatter with smoke coverage through `-UsePersistenceSettingsDraftProbe`.
11. Add UI-safe apply-result display rows for the unavailable apply entrypoint skeleton. Complete as a SQLUICore-only display formatter with smoke coverage through `-UsePersistenceSettingsDraftProbe`.
12. Add an optional SQLUISamples sample/dev adapter for apply-result display rows. Complete as `USQLUISamplePersistenceSettingsApplyResultPresenter`.
13. Add an optional C++ UMG widget shell contract for apply-result rows. Complete as `USQLUISamplePersistenceSettingsApplyResultPanelWidget`.
14. Add a focused safe UMG usage guide for apply-result rows. Complete as [`sqlui_persistence_settings_apply_result_umg_usage.md`](sqlui_persistence_settings_apply_result_umg_usage.md).
15. Add mutating Apply execution behind the existing helper APIs without widget controls, following the actual apply implementation gate above.
16. Add a backend selector UI shell that edits pending state only.
17. Add SQLite path display/edit validation that does not create files.
18. Add reset UX confirmation and smoke coverage through SQLUICore database management helpers.
19. Add packaged validation for any startup, config, default map, viewport, or packaged lifecycle integration.

Each implementation PR should keep the blast radius narrow and should not combine default policy changes with UI scaffolding.

## Packaged Validation Policy

The validation-only draft model, dry-run apply-intent preview, non-mutating apply/cancel contract, unavailable apply request skeleton, UI-safe validation/apply-preview/apply-contract/apply-result display rows, sample/dev-facing adapters, and optional C++ UMG widget shells do not require full packaged validation because they do not touch maps, config, startup behavior, package behavior, viewport attachment, provider lifecycle, or packaged lifecycle flow. They add runtime code and editor commandlet smoke coverage only for non-mutating validation/preview/contract/apply-result display formatting, sample consumption, and reflection-validated binding contracts.

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
