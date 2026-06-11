# SQLUI Persistence Settings Draft UMG Usage

This document is the focused Blueprint/UMG binding recipe for the optional SQLUISamples persistence settings draft validation widget shell.

PR #110 was docs-only. It documents how future UI work can subclass or bind to the #109 `USQLUISamplePersistenceSettingsDraftPanelWidget` C++ shell without adding actual widget assets. A later SQLUICore dry-run apply-intent preview now reports what a future Apply would do, but the widget shell still does not add maps, startup wiring, viewport attachment, polling, settings editing, actual apply/save behavior, reset/delete behavior, provider lifecycle behavior, migrations, seed-copy behavior, or database file creation.

Related docs:

- [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) defines the broader future persistence settings UX.
- [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md) plans the future mutating settings editing, apply/cancel, backend selection, SQLite path, provider auto-init, and reset/delete UX phase.
- [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md) documents the read-only status-panel UMG binding recipe.
- [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md) summarizes current SQLite runtime status and safety boundaries.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) describes repository and UI/storage ownership boundaries.
- [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md) documents the focused apply-preview widget-shell binding recipe.
- [`sqlui_smoke_test.md`](sqlui_smoke_test.md) lists the local smoke command that validates the draft model, apply preview, non-mutating apply/cancel contract, apply-preview display rows, validation display rows, sample presenters, and validation/apply-preview/apply-contract widget shells.

## Existing Draft Validation Stack

The current stack is intentionally validation/preview-only:

- #106: `USQLUIPersistenceSettingsDraftLibrary` creates and validates non-mutating draft settings.
- Dry-run apply-intent preview: `PreviewPersistenceSettingsDraftApply` reports what a future Apply would do without applying or saving settings.
- #107: `USQLUIPersistenceSettingsDraftDisplayLibrary` converts validation results into `FSQLUIPersistenceSettingsValidationDisplayRow` values and a display summary.
- #108: `USQLUISamplePersistenceSettingsDraftPresenter` is the SQLUISamples sample/dev adapter for those rows; it stores display rows, formatted lines, summary text, and validation flags for sample/dev and Blueprint-facing use.
- Apply-preview display adapter: `USQLUISamplePersistenceSettingsApplyPreviewPresenter` is the SQLUISamples sample/dev adapter for dry-run apply-preview display rows; it stores rows, formatted lines, summary text, and preview flags for sample/dev and Blueprint-facing use without applying anything.
- Apply/cancel contract display adapter: `USQLUISamplePersistenceSettingsApplyContractPresenter` is the SQLUISamples sample/dev adapter for non-mutating apply/cancel contract display rows; it stores rows, formatted lines, summary text, and contract flags for sample/dev and Blueprint-facing use without applying anything.
- #109: `USQLUISamplePersistenceSettingsDraftPanelWidget` is the optional C++ `UUserWidget` shell over the #108 presenter/adapter.
- Apply-preview panel shell: `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget` is the optional C++ `UUserWidget` shell over the apply-preview presenter/adapter. It creates no visual layout, adds no widget blueprint asset, is not wired into startup/maps/config/viewport, and exposes only caller-invoked refresh/build plus cached display data.
- Apply/cancel contract panel shell: `USQLUISamplePersistenceSettingsApplyContractPanelWidget` is the optional C++ `UUserWidget` shell over the apply/cancel contract presenter/adapter. It follows the same caller-invoked, no-asset, no-viewport, no-lifecycle-refresh pattern.
- #110: this guide documents safe future Blueprint subclassing and binding for the validation shell and the shared validation/preview-only boundaries.
- Apply-preview usage guide: [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md) documents the focused safe binding recipe for `USQLUISamplePersistenceSettingsApplyPreviewPanelWidget`.

The #109 shell is sample/dev-facing and delegates to the #108 SQLUISamples presenter/adapter, which consumes SQLUICore draft validation/display surfaces from #106 and #107. Future UI should consume the display rows, display summary, summary text, and validation flags only. It should not duplicate draft validation, path policy, backend policy, provider lifecycle policy, file checks, or persistence policy in Blueprint or widget code.

The shell creates no visual layout, adds no widget blueprint asset, and is not wired into startup, maps, config, timers, tick, polling, or the viewport.

The shell exposes explicit refresh/build methods:

- `RefreshDefaultPersistenceSettingsDraftPanel()`
- `RefreshCurrentPersistenceSettingsDraftPanel()`
- `BuildPersistenceSettingsDraftPanel(const FSQLUIPersistenceSettingsDraft& Draft)`

The shell exposes cached data through Blueprint-readable properties and pure getters:

- `Rows` / `GetRows()`
- `FormattedLines` / `GetFormattedLines()`
- `LastRefreshResult` / `GetLastRefreshResult()`
- `SummaryText` / `GetSummaryText()`
- `bIsValid`
- `bHasErrors`
- `bHasWarnings`
- `bRequiresRestartOrReinitialize`

## Foundation Checkpoint

The persistence settings draft validation UMG foundation is complete as a binding scaffold. It includes the #105 settings editing/reset UX plan, the #106 SQLUICore draft model and validation result, the #107 SQLUICore display rows and summary, the #108 optional SQLUISamples validation presenter/adapter, the #109 optional C++ UMG widget shell, this #110 usage guide, and the #111 final checkpoint. The follow-up apply-preview UI foundation is complete through #112 dry-run apply-intent preview, #113 UI-safe apply-preview display rows/summary, #114 optional SQLUISamples apply-preview presenter/adapter, #115 optional SQLUISamples apply-preview C++ UMG widget shell, #116 apply-preview usage guide, the #117 final checkpoint, the non-mutating apply/cancel contract, UI-safe apply/cancel contract display rows, optional SQLUISamples apply/cancel contract presenter/widget shell, and `-UsePersistenceSettingsDraftProbe` non-asset smoke coverage for the draft model, apply preview, apply/cancel contract, validation/apply-preview/apply-contract display rows, presenters/adapters, and widget-shell contracts.

This remains sample/dev-facing and validation/preview-only. It is not a full settings screen and does not add settings editing controls, backend selector controls, SQLite path editing controls, provider auto-init toggles, actual Apply/Cancel behavior, settings save behavior, config writes, reset/delete actions, migration controls, seed-copy controls, provider/repository initialization, database creation, widget blueprint assets, maps, startup wiring, viewport attachment, timers, tick, polling, or auto-refresh.

A valid draft or successful display refresh means only that validation/display data was produced. It does not mean settings were applied, saved, written to config, or made active.

The apply-intent preview, apply-preview display rows, apply/cancel contract display rows, apply-preview sample presenter, apply/cancel contract sample presenter, apply-preview widget shell, and apply/cancel contract widget shell are report data only. They may use "would change," "not implemented," and "not applied" messaging, but they do not save settings, write config, initialize providers/repositories, create directories or database files, open databases for writing, run migrations, copy seeds, reset databases, or delete files.

Future editable settings or reset/delete UX should build on SQLUICore policy helpers and the dedicated plan in [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md). Widgets should keep using draft display rows and must not know SQL, schema, migration ids, seed-copy policy, sidecar internals, direct file deletion, live repository mutation rules, provider lifecycle rules, or config-write details.

## Future Blueprint Recipe

A future Blueprint/UMG PR can use the shell like this:

1. Create a widget blueprint subclass of `USQLUISamplePersistenceSettingsDraftPanelWidget`.
2. Add a simple visual list or text layout for validation rows.
3. Bind the visual list to `Rows` or `GetRows()`.
4. For each `FSQLUIPersistenceSettingsValidationDisplayRow`, display `Label` and `Value`.
5. Optionally map `State` to a visual treatment such as normal, good, warning, or error.
6. Optionally expose `DetailText` as a tooltip or small help line.
7. Show `SummaryText` for the overall pending/draft state.
8. Show backend, validation summary, errors/warnings, and restart/reinitialize information when those values are exposed by the rows or summary.
9. Add a Refresh button only if it calls one of the explicit refresh/build methods.
10. Treat refresh/build as caller-invoked validation display work only.

Use `FormattedLines` only for simple sample text or commandlet-style output. Product UI should prefer structured rows.

Rows and summary data are display-only. They are not settings controls and should not be used as proof that a backend, SQLite path, provider auto-init policy, or runtime repository has changed.

## Refresh And Build Boundaries

Refresh/build is validation-only draft display work. Apply-preview generation is dry-run report work. Neither is any of the following:

- Polling.
- Ticking.
- Timer-driven refresh.
- Startup initialization.
- Viewport attachment.
- Provider initialization.
- Repository initialization.
- Database creation.
- Schema initialization.
- Migration.
- Seed copy.
- Reset/delete.
- Settings apply.
- Settings save.
- Backend selection.
- Treating apply-preview output as live applied state.

Do not call refresh/build automatically from construction, `NativeConstruct`, `PreConstruct`, `Tick`, timers, startup, map load, config load, or a latent refresh loop unless a later PR explicitly scopes a safe optional UI path.

## Display Semantics

The UI should display the row model without reinterpreting storage internals:

- Default `InMemory` draft state is normal and safe, not an error.
- SQLite draft state is pending until explicitly applied by future work.
- A pending SQLite path should be display-only and must not create directories or database files.
- A missing SQLite database is not alarming unless SQLite is the active/applied backend and the future product policy says it should exist.
- Provider auto-init changes are pending policy choices, not live provider lifecycle changes.
- Unknown or unsupported backend values should be shown as validation errors.
- Empty SQLite paths should be shown as validation errors only for pending SQLite selections.
- Restart or reinitialize requirements should be shown as pending requirements, not performed by the widget.

Use careful words such as `Draft`, `Pending`, `Would change`, `Would require`, `Validation only`, `Preview only`, and `Not applied`. Do not imply that the widget has applied, saved, initialized, migrated, copied, reset, deleted, or changed live runtime state.

## Out-Of-Scope Controls

This usage guide does not introduce:

- Backend selector controls.
- SQLite path editor controls.
- Provider auto-init toggles.
- Settings save/apply controls.
- Cancel controls.
- Reset/delete buttons.
- Migration controls.
- Seed-copy controls.
- Direct file browser/delete behavior.

Future settings editing and reset/delete UX must be scoped separately and should follow [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md). Future reset behavior must go through SQLUICore database management helper/policy surfaces instead of widget-owned file deletion.

## Ownership Boundaries

Widgets and sample UI should consume validation rows. They should not own persistence internals.

Widgets must not:

- Know SQL strings.
- Know SQLite schema details.
- Know migration ids or versioning internals.
- Know seed-copy policy internals.
- Know SQLite sidecar internals.
- Perform file existence, file size, sidecar, or schema checks directly.
- Initialize providers or repositories.
- Create database files.
- Run migrations.
- Copy seed databases.
- Delete files.
- Apply or save settings.

JerryRigged remains a thin host. SQLUICore remains the storage/policy boundary and should not gain UMG, Slate, SlateCore, editor-only, or widget dependencies for this UI surface. SQLUISamples may provide optional sample/dev widget shells, but those shells should remain presentation scaffolds.

## Manual Local Checklist

For a future local/manual Blueprint exploration, keep the asset local unless a later PR explicitly scopes committing it:

- Create a local widget blueprint subclass of `USQLUISamplePersistenceSettingsDraftPanelWidget`.
- Add a simple list/text layout for `Label` and `Value` rows.
- Optionally show `State`, `DetailText`, and `SummaryText`.
- Add a Refresh button that calls an explicit refresh/build method.
- Do not wire the widget into startup, default maps, config, GameInstance, or viewport by default.
- Do not add Apply, Save, Cancel, Reset, Delete, backend selector, SQLite path editor, provider auto-init, migration, or seed-copy controls in this local shell.
- Do not commit the local widget asset unless a future PR explicitly scopes that asset.
- Confirm the default `InMemory` draft displays as safe.
- Confirm a pending SQLite draft does not create SQLite database files.
- Confirm no persistence settings draft probe DB or sidecar files remain after smoke tests.

## Smoke Coverage

The existing `-UsePersistenceSettingsDraftProbe` smoke path validates the SQLUICore draft model, validation result, validation display-row formatting, apply-preview display-row formatting, apply/cancel contract display-row formatting, SQLUISamples validation/apply-preview/apply-contract presenters, and validation/apply-preview/apply-contract C++ widget shell contracts by reflection. It checks that the widget shells derive from `UUserWidget`, that refresh/build functions are Blueprint-callable and not `BlueprintPure`, that cached widget-shell getters are `BlueprintPure`, and that cached row/formatted-lines/result/summary/flag properties are Blueprint-visible.

The smoke path validates that contract without requiring committed widget blueprint assets, maps, startup wiring, or viewport attachment. It also keeps refresh/build caller-invoked and confirms default `InMemory` validation/apply-preview display does not initialize providers/repositories or create SQLite database files. The focused apply-preview shell recipe lives in [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md).

This guide adds no new smoke flag. Cleanup expectations remain unchanged: the probe removes only smoke-owned database and sidecar files under `Saved/SQLUI/SmokeTests/PersistenceSettingsDraft`.

## Remaining Work

Still future work:

- Actual widget blueprint assets.
- Visual layout.
- Startup/map/config integration.
- Settings editing controls.
- Backend selection UI.
- SQLite path editing UI.
- Provider auto-init controls.
- Actual Apply/Cancel behavior.
- Settings save behavior.
- Reset/delete UX.
- Product startup policy.

The next mutating settings/reset phase is planned in [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md), but no editing, actual apply/save, provider lifecycle, or reset/delete behavior exists in the current validation/preview-only widget shell.
