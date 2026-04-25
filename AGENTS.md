# AGENTS.md

## Mission
Build SQLUI / JerryRigged as a reusable Unreal Engine C++ plugin plus host project for runtime UMG generated from SQLite and JSON layout data.

## Working Rules
- Keep core logic in reusable plugins and keep `JerryRigged` thin.
- Do not perform database work on the game thread.
- Keep widgets talking to repositories, variable stores, and action systems, not raw SQL.
- Prefer boring, reusable architecture over prototype shortcuts.
- Keep milestone 1 limited to structure, registration, project wiring, and compile safety.

## Guardrails
- Do not edit `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `.vs/`, or generated files.
- Follow existing Unreal naming and style.
- Keep changes minimal and reusable.
- Do not add editor tooling, telemetry, warmups, packaging helpers, or commandlets in milestone 1.
- Do not choose or integrate a concrete SQLite backend in milestone 1.

## Milestone 1 Done
- One umbrella plugin named `SQLUI` exists.
- `SQLUI.Core`, `SQLUI.Widgets`, and `SQLUI.Samples` exist as minimal scaffolding.
- `JerryRigged` remains a thin host shell.
- The host project is wired to the plugin with only thin integration if needed.
- The project compiles cleanly and is organized for later milestones.
