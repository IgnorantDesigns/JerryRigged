# SQLUI Persistence Settings Apply Preview UMG Usage

This document is the focused Blueprint/UMG binding recipe for the optional SQLUISamples persistence settings apply-preview widget shell.

PR #115 added `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget` as a C++ `UUserWidget` shell for dry-run apply-preview display. PR #116 is docs-only: it adds this guide for how future local Blueprint/UMG work can subclass or bind to that shell safely, without adding widget Blueprint assets, maps, startup wiring, viewport attachment, polling, ticking, settings controls, actual Apply/Save behavior, reset/delete behavior, provider lifecycle behavior, migrations, seed-copy behavior, directory creation, or database file creation.

Related docs:

- [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) documents the companion draft validation widget-shell binding recipe.
- [`sqlui_persistence_settings_apply_contract_umg_usage.md`](sqlui_persistence_settings_apply_contract_umg_usage.md) documents the companion apply/cancel contract widget-shell binding recipe.
- [`sqlui_persistence_settings_apply_result_umg_usage.md`](sqlui_persistence_settings_apply_result_umg_usage.md) documents the companion apply-result widget-shell binding recipe.
- [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) defines the broader future persistence settings UX.
- [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md) plans the future mutating settings editing, apply/cancel, backend selection, SQLite path, provider auto-init, and reset/delete UX phase.
- [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md) documents the read-only status-panel UMG binding recipe.
- [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md) summarizes current SQLite runtime status and safety boundaries.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) describes repository and UI/storage ownership boundaries.
- [`sqlui_smoke_test.md`](sqlui_smoke_test.md) lists the local smoke command that validates the apply-preview display rows, sample presenter, and widget shell contract.

## Existing Apply Preview Stack

The current apply-preview stack is intentionally non-mutating:

- #112: `USQLUIPersistenceSettingsDraftLibrary::PreviewPersistenceSettingsDraftApply` builds `FSQLUIPersistenceSettingsApplyPreviewResult`, which reports what a future Apply would do without applying or saving settings.
- #113: `USQLUIPersistenceSettingsApplyPreviewDisplayLibrary` converts dry-run preview results into `FSQLUIPersistenceSettingsApplyPreviewDisplayRow` values and a `FSQLUIPersistenceSettingsApplyPreviewDisplaySummary`.
- #114: `USQLUISamplePersistenceSettingsApplyPreviewPresenter` is the optional SQLUISamples sample/dev-facing adapter. It delegates to SQLUICore display helpers and caches rows, formatted lines, summary text, and preview flags for simple sample or Blueprint consumption.
- #115: `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget` is the optional SQLUISamples C++ `UUserWidget` shell over that presenter. It creates no visual layout, adds no widget Blueprint asset, and is not wired into startup, maps, config, timers, tick, polling, or the viewport.
- The non-mutating apply/cancel contract in `USQLUIPersistenceSettingsDraftLibrary` reports future apply readiness, explicitly marks actual Apply execution unavailable/not implemented, and describes cancel/discard as value preview only.
- `USQLUIPersistenceSettingsApplyContractDisplayLibrary` formats that apply/cancel contract into UI-safe rows/summary without running Apply, saving settings, writing config, or mutating live state.
- `USQLUISamplePersistenceSettingsApplyContractPanelWidget` is the optional SQLUISamples C++ `UUserWidget` shell over the apply/cancel contract presenter. Its dedicated safe binding recipe is [`sqlui_persistence_settings_apply_contract_umg_usage.md`](sqlui_persistence_settings_apply_contract_umg_usage.md).
- `USQLUISamplePersistenceSettingsApplyResultPanelWidget` is the optional SQLUISamples C++ `UUserWidget` shell over the apply-result presenter. Its dedicated safe binding recipe is [`sqlui_persistence_settings_apply_result_umg_usage.md`](sqlui_persistence_settings_apply_result_umg_usage.md).

Future UI should consume the display rows, display summary, summary text, and preview flags only. It should not duplicate apply-preview logic, draft validation, path policy, backend policy, provider lifecycle policy, file checks, SQLite schema knowledge, migration policy, seed-copy policy, or persistence policy in Blueprint or widget code.

The apply-preview shell is sample/dev-facing. It is not a full settings screen and it is not an actual Apply UI.

## Foundation Checkpoint

The non-mutating apply-preview UI foundation is complete through #112 dry-run apply-intent preview, #113 apply-preview display rows/summary, #114 optional SQLUISamples apply-preview presenter, #115 optional C++ apply-preview UMG widget shell, this #116 usage guide, the #117 final foundation checkpoint, the non-mutating apply/cancel contract, UI-safe apply/cancel contract display rows/summary, the optional apply/cancel contract presenter and C++ UMG widget shell, and `-UsePersistenceSettingsDraftProbe` smoke coverage. That checkpoint builds on the #105-#111 validation-only draft foundation, but it still adds no settings editing, actual Apply/Cancel execution, config writes, backend selector controls, SQLite path editor controls, provider auto-init controls, reset/delete UX, widget blueprint assets, maps, startup wiring, viewport attachment, polling, ticking, auto-refresh, provider/repository initialization, migrations, seed-copy behavior, directory creation, database file creation, or default policy changes.

## Shell API Shape

The shell exposes explicit caller-invoked methods:

- `RefreshDefaultPersistenceSettingsApplyPreviewPanel()`
- `RefreshCurrentPersistenceSettingsApplyPreviewPanel()`
- `BuildPersistenceSettingsApplyPreviewPanel(const FSQLUIPersistenceSettingsDraft& Draft)`

The shell exposes cached data through Blueprint-readable properties and pure getters:

- `Rows` / `GetRows()`
- `FormattedLines` / `GetFormattedLines()`
- `LastRefreshResult` / `GetLastRefreshResult()`
- `SummaryText` / `GetSummaryText()`
- `bCanApplyInFuture`
- `bIsValid`
- `bHasChanges`
- `bHasErrors`
- `bHasWarnings`
- `bRequiresRestartOrReinitialize`
- `bWouldNeedProviderReinitialize`
- `bWouldNeedRepositoryReopen`

Use `FormattedLines` only for simple sample text or commandlet-style output. Product UI should prefer structured rows and summary fields.

## Future Blueprint Recipe

A future Blueprint/UMG PR can use the shell like this:

1. Create a local widget Blueprint subclass of `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget`.
2. Add simple text/list bindings for the apply-preview rows and summary.
3. Bind a list view or repeated row widget to `Rows` or `GetRows()`.
4. For each `FSQLUIPersistenceSettingsApplyPreviewDisplayRow`, display `Label` and `Value`.
5. Optionally map `State` to a visual treatment such as normal, good, warning, or error.
6. Optionally expose `DetailText` as a tooltip or small help line.
7. Show `SummaryText` for the overall preview state.
8. Show `bCanApplyInFuture`, `bHasChanges`, `bWouldNeedProviderReinitialize`, `bWouldNeedRepositoryReopen`, and `bRequiresRestartOrReinitialize` only as dry-run preview flags.
9. Show "would change backend," "would change SQLite path," "would affect provider auto-init," and similar pending-change details only when they are exposed through the shell's rows, summary, or underlying SQLUICore preview/display model.
10. Add a local test button only if it calls one of the explicit refresh/build methods.
11. Treat every row and flag as display-only.

Do not call refresh/build automatically from construction, `NativeConstruct`, `PreConstruct`, `Tick`, timers, startup, map load, config load, or a polling loop unless a later PR explicitly scopes a safe optional UI path. Refresh/build is report generation, not lifecycle work.

## Display Semantics

The apply-preview UI should preserve future-oriented wording:

- Default `InMemory` preview is safe and should not be shown as an error.
- A current/default preview with no pending changes should read as "No changes to apply" or equivalent.
- A SQLite draft or apply preview does not mean SQLite is active.
- Provider auto-init pending display, if shown, does not mean startup behavior changed.
- Missing SQLite database/path should not be alarming when SQLite is not active.
- Unknown backend or invalid SQLite path messages should remain user-readable.
- Restart, reopen, and reinitialize messages are pending requirements only.

Use careful words such as `Would change`, `Would require`, `Preview only`, `Not applied`, `Not saved`, `Pending`, and `No changes to apply`.

Do not imply that the widget has applied settings, saved config, created directories, initialized a provider, opened a database for writing, opened or created a database, run migrations, copied seeds, reset state, deleted files, changed the active repository, or changed startup behavior.

Do not surface low-level SQL strings, schema details, migration internals, sidecar naming, or direct file-status checks from widget code. Those details stay behind SQLUICore policy/display surfaces.

## Out Of Scope

This usage guide does not introduce:

- Apply button.
- Save button.
- Cancel button.
- Settings apply/save/config-write behavior.
- Settings save/apply flow.
- Backend selector UI controls.
- SQLite path editor UI controls.
- Provider auto-init toggle/control.
- Reset/delete button.
- Destructive actions.
- Migration controls.
- Seed-copy controls.
- Widget Blueprint assets.
- Maps.
- Startup, config, or viewport wiring.
- Polling, ticking, timers, or auto-refresh.

Future settings editing, apply/cancel, and reset/delete UX must be scoped separately and should follow [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md). Future reset/delete behavior must go through SQLUICore database management helper/policy surfaces instead of widget-owned file deletion.

## Ownership Boundaries

SQLUICore owns draft validation, dry-run apply-preview logic, and display formatting. SQLUISamples demonstrates optional sample/dev-facing consumption. JerryRigged remains a thin host.

Widgets and sample UI must not:

- Know SQL strings.
- Know SQLite schema details.
- Know migration ids or versioning internals.
- Know seed-copy policy internals.
- Know SQLite sidecar internals.
- Perform direct file existence, file size, sidecar, schema, or migration checks.
- Initialize providers or repositories.
- Create directories or database files.
- Open databases for writing.
- Run migrations.
- Copy seed databases.
- Initialize provider/repository state.
- Write config.
- Apply or save settings.
- Delete files.

Widgets should consume SQLUICore-provided rows and summaries. They should not own persistence policy.

## Manual Local Checklist

For a future local/manual Blueprint exploration, keep the asset local unless a later PR explicitly scopes committing it:

- Create a local widget Blueprint subclass of `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget`.
- Add simple list/text bindings for row `Label`, `Value`, `State`, and `DetailText`.
- Bind summary text to `SummaryText`.
- Optionally show preview flags such as `bCanApplyInFuture`, `bHasChanges`, `bWouldNeedProviderReinitialize`, `bWouldNeedRepositoryReopen`, and `bRequiresRestartOrReinitialize`.
- Call an explicit refresh/build method manually from a local test button if needed.
- Do not wire the widget into startup, default maps, config, GameInstance, or viewport by default.
- Do not add Apply, Save, Cancel, Reset, Delete, backend selector, SQLite path editor, provider auto-init, migration, or seed-copy controls in this local shell.
- Do not commit the local widget Blueprint asset unless a future PR explicitly scopes that asset.
- Confirm default `InMemory` display is safe.
- Confirm preview wording says "would change" or "not applied," not "applied" or "saved."
- Confirm no database files are created by apply-preview display.
- Confirm the smoke probe leaves no draft/status probe database or sidecar files behind.

## Smoke Coverage

The existing `-UsePersistenceSettingsDraftProbe` smoke path validates the SQLUICore draft model, dry-run apply preview, non-mutating apply/cancel contract, default unavailable apply behavior, guarded backend-only apply behavior, apply-preview display-row formatting, apply/cancel contract display-row formatting, apply-result display-row formatting, `USQLUISamplePersistenceSettingsApplyPreviewPresenter`, `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget`, `USQLUISamplePersistenceSettingsApplyContractPresenter`, `USQLUISamplePersistenceSettingsApplyContractPanelWidget`, `USQLUISamplePersistenceSettingsApplyResultPresenter`, and `USQLUISamplePersistenceSettingsApplyResultPanelWidget` contracts by reflection.

It verifies the apply-preview shell derives from `UUserWidget`, that refresh/build functions are Blueprint-callable and not `BlueprintPure`, that cached getters are `BlueprintPure`, and that cached row/formatted-line/result/summary/flag properties are Blueprint-visible. It does this without a widget Blueprint asset, map, viewport instance, startup wiring, polling, or automatic refresh.

The same probe verifies default/current `InMemory` previews and contracts are safe, default Apply execution is reported as unavailable/not implemented, the guarded backend-only Apply request writes only the backend value, unknown backends and empty SQLite paths are shown as blocked/error preview rows and blocked apply contracts/contract-display rows, pending SQLite paths can be previewed without database creation, provider auto-init changes remain pending policy, cancel/discard is represented as value preview only, repeated preview/contract/display/adapter output is deterministic, and a smoke-owned sidecar is not deleted by preview/contract/display/adapter generation.

This guide adds no new smoke flag. Cleanup expectations remain unchanged: the probe removes only smoke-owned database and sidecar files under `Saved/SQLUI/SmokeTests/PersistenceSettingsDraft`.

## Remaining Work

Still future work:

- Actual widget Blueprint assets.
- Visual layout.
- Startup/map/config integration.
- Settings editing controls.
- Backend selection UI.
- SQLite path editing UI.
- Provider auto-init controls.
- Actual Apply/Cancel behavior.
- Settings save/config-write behavior.
- Reset/delete UX.
- Product startup policy.

Packaged validation is not required for this docs-only guide because it does not touch runtime code, maps, config, startup behavior, provider lifecycle, viewport attachment, or packaged runtime behavior. Future PRs that wire widgets into startup/default maps/config, write config, or change provider lifecycle behavior should include packaged validation appropriate to that behavior.
