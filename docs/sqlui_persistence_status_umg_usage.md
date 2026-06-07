# SQLUI Persistence Status UMG Usage

This document is the focused read-only Blueprint/UMG binding recipe for the optional SQLUISamples persistence status widget shell.

It documents how future UI work can subclass or bind to `USQLUISamplePersistenceStatusPanelWidget` without adding actual widget assets in this PR. It does not add maps, startup wiring, viewport attachment, polling, settings editing, reset/delete behavior, provider lifecycle behavior, migrations, seed-copy behavior, or database file creation.

Related docs:

- [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md) defines the broader future persistence settings UX.
- [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md) summarizes current SQLite runtime status and safety boundaries.
- [`sqlui_repository_architecture.md`](sqlui_repository_architecture.md) describes repository and UI/storage ownership boundaries.
- [`sqlui_smoke_test.md`](sqlui_smoke_test.md) lists the local smoke command that validates the presenter, adapter, and widget shell.

## Existing Read-Only Stack

The current stack is intentionally narrow:

- `USQLUIPersistenceStatusLibrary` creates a read-only SQLUICore status snapshot.
- `USQLUIPersistenceStatusDisplayLibrary` converts that snapshot into `FSQLUIPersistenceStatusDisplayRow` values.
- `USQLUISamplePersistenceStatusPresenter` stores rows and formatted lines for sample/dev and Blueprint-facing use.
- `USQLUISamplePersistenceStatusPanelAdapter` delegates to the presenter and stores the latest panel-friendly rows/result.
- `USQLUISamplePersistenceStatusPanelWidget` is the optional C++ `UUserWidget` shell over the adapter.

The shell is sample/dev-facing. It creates no visual layout, adds no widget blueprint asset, and is not wired into startup, maps, config, timers, tick, polling, or the viewport.

The shell exposes explicit refresh methods:

- `RefreshPersistenceStatusPanel()`
- `RefreshPersistenceStatusPanelFromRuntimeConfig(const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)`

The shell exposes cached data through Blueprint-readable properties and pure getters:

- `Rows` / `GetRows()`
- `FormattedLines` / `GetFormattedLines()`
- `LastRefreshResult` / `GetLastRefreshResult()`
- `SummaryText` / `GetSummaryText()`

## Future Blueprint Recipe

A future Blueprint/UMG PR can use the shell like this:

1. Create a widget blueprint subclass of `USQLUISamplePersistenceStatusPanelWidget`.
2. Add a simple visual list or text layout for status rows.
3. Bind the visual list to `Rows` or `GetRows()`.
4. For each `FSQLUIPersistenceStatusDisplayRow`, display `Label` and `Value`.
5. Optionally map `State` to a visual treatment such as normal, good, warning, or unavailable.
6. Optionally expose `DetailText` as a tooltip or small help line.
7. Add a Refresh button only if it calls `RefreshPersistenceStatusPanel()` or the explicit runtime-config refresh method.
8. Treat refresh as a caller-invoked re-query of existing status rows only.

Use `FormattedLines` only for simple sample text or commandlet-style output. Product UI should prefer structured rows.

## Refresh Boundaries

Refresh is read-only status display work. It is not any of the following:

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
- Settings editing.
- Backend selection.

Do not call refresh automatically from construction, `NativeConstruct`, `PreConstruct`, `Tick`, timers, startup, map load, config load, or a latent refresh loop unless a later PR explicitly scopes a safe optional UI path.

## Display Semantics

The UI should display the row model without reinterpreting storage internals:

- `InMemory` default state is normal and safe, not an error.
- SQLite missing path or missing database state is normal when SQLite is not active.
- SQLite paths are display-only in this read-only surface.
- Database existence, file size, sidecar, and schema status are display-only.
- Warnings should reflect the existing row `State` and `DetailText`.
- Avoid alarming copy for safe default state.

The display row model intentionally hides raw SQL, schema table names, migration internals, seed-copy policy, and direct file checks from UMG.

## Out-Of-Scope Controls

This usage guide does not introduce:

- Backend selector controls.
- SQLite path editing.
- Provider auto-init toggles.
- Settings save/apply controls.
- Reset/delete buttons.
- Migration controls.
- Seed-copy controls.
- Direct file browser/delete behavior.

Future settings editing and reset/delete UX must be scoped separately. Future reset behavior must go through SQLUICore database management helper/policy surfaces instead of widget-owned file deletion.

## Ownership Boundaries

Widgets and sample UI should consume status rows. They should not own persistence internals.

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

JerryRigged remains a thin host. SQLUICore remains the storage/policy boundary and should not gain UMG, Slate, SlateCore, editor-only, or widget dependencies for this UI surface.

## Manual Local Checklist

For a future local/manual Blueprint exploration, keep the asset local unless a later PR explicitly scopes committing it:

- Create a local widget blueprint subclass of `USQLUISamplePersistenceStatusPanelWidget`.
- Add a simple list/text layout for `Label` and `Value` rows.
- Optionally show `State` and `DetailText`.
- Add a Refresh button that calls the explicit refresh method.
- Do not wire the widget into startup, default maps, config, GameInstance, or viewport by default.
- Do not commit the local widget asset unless a future PR explicitly scopes that asset.
- Confirm the default `InMemory` state displays as safe.
- Confirm refresh does not create SQLite database files.
- Confirm no persistence status sample surface probe DB or sidecar files remain after smoke tests.

## Smoke Coverage

The existing `-UsePersistenceStatusSampleSurfaceProbe` smoke path validates the presenter, panel adapter, and C++ widget shell contract by reflection. It checks that the widget shell derives from `UUserWidget`, that its refresh functions are Blueprint-callable, and that cached row/result/summary properties are Blueprint-visible.

This guide adds no new smoke flag. Cleanup expectations remain unchanged: the probe removes only smoke-owned database and sidecar files under `Saved/SQLUI/SmokeTests/PersistenceStatusSampleSurface`.

## Remaining Work

Still future work:

- Actual widget blueprint assets.
- Visual layout.
- Startup/map/config integration.
- Settings editing.
- Backend selection UI.
- SQLite path editing UI.
- Reset/delete UX.
- Product startup policy.
