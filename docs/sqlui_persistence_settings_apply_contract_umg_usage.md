# SQLUI Persistence Settings Apply/Cancel Contract UMG Usage

This document is the focused Blueprint/UMG binding recipe for the optional SQLUISamples persistence settings apply/cancel contract widget shell.

PR #122 added `USQLUISamplePersistenceSettingsApplyContractPanelWidget` as a C++ `UUserWidget` shell for non-mutating apply/cancel contract display. PR #123 is docs-only: it documents how future local Blueprint/UMG work can subclass or bind to that shell safely, without adding widget Blueprint assets, maps, startup wiring, viewport attachment, polling, ticking, settings controls, actual Apply/Save behavior, reset/delete behavior, provider lifecycle behavior, migrations, seed-copy behavior, directory creation, or database file creation.

Related docs:

- [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md) documents the validation-only draft settings widget-shell binding recipe.
- [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md) documents the dry-run apply-preview widget-shell binding recipe.
- [`sqlui_persistence_settings_apply_result_umg_usage.md`](sqlui_persistence_settings_apply_result_umg_usage.md) documents the apply-result widget-shell binding recipe.
- [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) defines the broader future persistence settings UX.
- [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md) plans the future mutating settings editing, apply/cancel, backend selection, SQLite path, provider auto-init, and reset/delete UX phase.
- [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md) documents the read-only status-panel UMG binding recipe.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) describes repository and UI/storage ownership boundaries.
- [`sqlui_smoke_test.md`](sqlui_smoke_test.md) lists the local smoke command that validates the apply/cancel contract display rows, sample presenter, and widget shell contract.

## Existing Apply/Cancel Contract Stack

The current apply/cancel contract stack is intentionally non-mutating:

- #118: `USQLUIPersistenceSettingsDraftLibrary::BuildPersistenceSettingsApplyContract` builds `FSQLUIPersistenceSettingsApplyContractResult`, which reports whether a future Apply could run, why it is blocked, and that actual Apply execution is unavailable/not implemented.
- #118: `USQLUIPersistenceSettingsDraftLibrary::BuildPersistenceSettingsCancelPreview` builds `FSQLUIPersistenceSettingsCancelPreviewResult`, which describes whether pending values would be discarded and what the draft value would become after cancel without mutating live state.
- #119: `USQLUIPersistenceSettingsApplyContractDisplayLibrary` formats the contract and cancel preview into `FSQLUIPersistenceSettingsApplyContractDisplayRow` values plus `FSQLUIPersistenceSettingsApplyContractDisplaySummary`.
- #121: `USQLUISamplePersistenceSettingsApplyContractPresenter` is the optional SQLUISamples sample/dev-facing adapter. It delegates to SQLUICore display helpers and caches rows, formatted lines, summary text, and contract flags for simple sample or Blueprint consumption.
- #122: `USQLUISamplePersistenceSettingsApplyContractPanelWidget` is the optional SQLUISamples C++ `UUserWidget` shell over that presenter/adapter. It creates no visual layout, adds no widget Blueprint asset, and is not wired into startup, maps, config, timers, tick, polling, or the viewport.
- #129: `USQLUISamplePersistenceSettingsApplyResultPanelWidget` is the optional SQLUISamples C++ `UUserWidget` shell over the apply-result presenter. Its focused safe binding recipe is [`sqlui_persistence_settings_apply_result_umg_usage.md`](sqlui_persistence_settings_apply_result_umg_usage.md).

Future UI should consume display rows, display summary, summary text, and contract flags only. It should not duplicate draft validation, dry-run apply-preview logic, apply/cancel contract logic, path policy, backend policy, provider lifecycle policy, file checks, SQLite schema knowledge, migration policy, seed-copy policy, sidecar policy, or persistence policy in Blueprint or widget code.

The apply/cancel contract shell is sample/dev-facing. It is not a full settings screen, not an actual Apply UI, and not an actual Cancel or discard UI.

## Shell API Shape

The shell exposes explicit caller-invoked methods:

- `RefreshDefaultPersistenceSettingsApplyContractPanel()`
- `RefreshCurrentPersistenceSettingsApplyContractPanel()`
- `BuildPersistenceSettingsApplyContractPanel(const FSQLUIPersistenceSettingsDraft& Draft)`

The shell exposes cached data through Blueprint-readable properties and pure getters:

- `Rows` / `GetRows()`
- `FormattedLines` / `GetFormattedLines()`
- `LastRefreshResult` / `GetLastRefreshResult()`
- `SummaryText` / `GetSummaryText()`
- `bCanApplyInFuture`
- `bActualApplyImplemented`
- `bCanExecuteApplyNow`
- `bIsValid`
- `bHasChanges`
- `bHasErrors`
- `bHasWarnings`
- `bRequiresRestartOrReinitialize`
- `bWouldNeedProviderReinitialize`
- `bWouldNeedRepositoryReopen`
- `bWouldDiscardChangesOnCancel`
- `Availability`

Use `FormattedLines` only for simple sample text or commandlet-style output. Product UI should prefer structured rows and summary fields.

## Future Blueprint Recipe

A future Blueprint/UMG PR can use the shell like this:

1. Create a local widget Blueprint subclass of `USQLUISamplePersistenceSettingsApplyContractPanelWidget`.
2. Add simple text/list bindings for the apply/cancel contract rows and summary.
3. Bind a list view or repeated row widget to `Rows` or `GetRows()`.
4. For each `FSQLUIPersistenceSettingsApplyContractDisplayRow`, display `Label` and `Value`.
5. Optionally map `State` to a visual treatment such as normal, good, warning, or error.
6. Optionally expose `DetailText` as a tooltip or small help line.
7. Show `SummaryText` for the overall contract state.
8. Show `bActualApplyImplemented`, `bCanExecuteApplyNow`, `bCanApplyInFuture`, `bHasChanges`, `bWouldNeedProviderReinitialize`, `bWouldNeedRepositoryReopen`, `bRequiresRestartOrReinitialize`, and `bWouldDiscardChangesOnCancel` only as report/preview flags.
9. Show "actual Apply unavailable/not implemented," "can apply in future," "has changes," "blocked by validation," "would change backend," "would change SQLite path," "would affect provider auto-init," and "requires restart/reopen/reinitialize" only when those meanings are exposed by the shell rows, summary, or underlying SQLUICore contract/display model.
10. Add a local test button only if it calls one of the explicit refresh/build methods.
11. Treat every row and flag as display-only.

Do not call refresh/build automatically from construction, `NativeConstruct`, `PreConstruct`, `Tick`, timers, startup, map load, config load, or a polling loop unless a later PR explicitly scopes a safe optional UI path. Refresh/build is report generation, not lifecycle work.

## Display Semantics

The apply/cancel contract UI should preserve future-oriented wording:

- Default `InMemory` contract display is safe and should not be shown as an error.
- SQLite remains explicit opt-in and `InMemory` remains the default backend.
- A current/default contract with no pending changes should read as "No changes to apply" or equivalent.
- A SQLite draft or apply/cancel contract display does not mean SQLite is active.
- Provider auto-init pending display, if shown, does not mean startup behavior changed.
- Missing SQLite database/path should not be alarming when SQLite is not active.
- Unknown backend or invalid SQLite path messages should remain user-readable.
- Restart, reopen, and reinitialize messages are pending requirements only.
- Cancel/discard rows describe what would be discarded, not a live mutation.

Use careful words such as `Apply unavailable`, `Apply not implemented`, `Would be ready for future apply`, `Would discard pending changes`, `Preview only`, `Not applied`, `Not saved`, `No changes to apply`, and `Blocked by validation`.

Do not imply that the widget has applied settings, saved config, created directories, initialized a provider, opened a database for writing, opened or created a database, run migrations, copied seeds, reset state, deleted files, changed the active repository, changed startup behavior, or discarded live state.

Do not surface low-level SQL strings, schema details, migration internals, sidecar naming, file-size checks, or direct file-status checks from widget code. Those details stay behind SQLUICore policy/display surfaces.

## Out Of Scope

This usage guide does not introduce:

- Apply button.
- Save button.
- Cancel button that mutates state.
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

SQLUICore owns draft validation, dry-run apply preview, apply/cancel contract/readiness, cancel-preview value semantics, and display formatting. SQLUISamples demonstrates optional sample/dev-facing consumption. JerryRigged remains a thin host.

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

- Create a local widget Blueprint subclass of `USQLUISamplePersistenceSettingsApplyContractPanelWidget`.
- Add simple list/text bindings for row `Label`, `Value`, `State`, and `DetailText`.
- Bind summary text to `SummaryText`.
- Optionally show contract flags such as `bCanApplyInFuture`, `bActualApplyImplemented`, `bCanExecuteApplyNow`, `bHasChanges`, `bWouldNeedProviderReinitialize`, `bWouldNeedRepositoryReopen`, `bRequiresRestartOrReinitialize`, `bWouldDiscardChangesOnCancel`, and `Availability`.
- Call an explicit refresh/build method manually from a local test button if needed.
- Do not wire the widget into startup, default maps, config, GameInstance, or viewport by default.
- Do not add Apply, Save, Cancel, Reset, Delete, backend selector, SQLite path editor, provider auto-init, migration, or seed-copy controls in this local shell.
- Do not commit the local widget Blueprint asset unless a future PR explicitly scopes that asset.
- Confirm default `InMemory` display is safe.
- Confirm contract wording says "Apply unavailable," "Apply not implemented," "would change," "would discard," "not applied," or "not saved," not "applied" or "saved."
- Confirm no database files are created by apply/cancel contract display.
- Confirm the smoke probe leaves no draft/status probe database or sidecar files behind.

## Smoke Coverage

The existing `-UsePersistenceSettingsDraftProbe` smoke path validates the SQLUICore draft model, validation display rows/summary, dry-run apply preview, apply-preview display-row formatting, non-mutating apply/cancel contract, apply/cancel contract display-row formatting, unavailable apply entrypoint skeleton, apply-result display-row formatting, `USQLUISamplePersistenceSettingsDraftPresenter`, `USQLUISamplePersistenceSettingsDraftPanelWidget`, `USQLUISamplePersistenceSettingsApplyPreviewPresenter`, `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget`, `USQLUISamplePersistenceSettingsApplyContractPresenter`, `USQLUISamplePersistenceSettingsApplyContractPanelWidget`, `USQLUISamplePersistenceSettingsApplyResultPresenter`, and `USQLUISamplePersistenceSettingsApplyResultPanelWidget` contracts by reflection.

It verifies the apply/cancel contract shell derives from `UUserWidget`, that refresh/build functions are Blueprint-callable and not `BlueprintPure`, that cached getters are `BlueprintPure`, and that cached row/formatted-line/result/summary/flag properties are Blueprint-visible. It does this without a widget Blueprint asset, map, viewport instance, startup wiring, polling, ticking, or automatic refresh.

The same probe verifies default/current `InMemory` contracts are safe, actual Apply execution is reported as unavailable/not implemented, unknown backends and empty SQLite paths are shown as blocked/error preview rows and blocked apply contracts/contract-display rows, pending SQLite paths can be previewed without database creation, provider auto-init changes remain pending policy, cancel/discard is represented as value preview only, repeated preview/contract/display/adapter output is deterministic, and a smoke-owned sidecar is not deleted by preview/contract/display/adapter generation.

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
