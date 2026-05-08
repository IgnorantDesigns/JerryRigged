# PROJECT_BRIEF.md

## Product
SQLUI / JerryRigged is a reusable Unreal Engine plugin plus host app that builds UMG widget trees at runtime from SQLite-backed data and JSON layout documents.

## Why This Exists
- Replace earlier messy prototypes with a clean reusable architecture.
- Power the JerryRigged app while keeping the core reusable for future projects.
- Enable fast layout iteration, saved and reloaded layouts, history, checkpoints, tags, and search.
- Support future per-user customization of layouts and behavior.

## Goals
- Keep database work off the game thread.
- Split app data and layout data into `App.db` and `Layouts.db`.
- Make runtime widgets talk to repositories, variable stores, and action systems instead of raw SQL.
- Keep the runtime cross-platform and degrade gracefully if SQLite is unavailable.
- Copy seed databases to `Saved` on first run.
- Keep core logic in reusable plugins rather than tying it to the host game.

## Non-Goals
- Hard-coded one-off UI flows as the main architecture.
- Milestone 1 implementation of the DB backend, layout repository, editor builder, telemetry, warmups, packaging helpers, or commandlets.
- Picking a concrete SQLite backend in milestone 1.

## Architecture Direction
- `SQLUI.Core` owns async DB boundaries, settings and seed copy flow, self-checks, layout structs, JSON serialization, repositories, variable store, action registry, focus manager, widget catalog or class registry, and layout factory boundaries.
- `SQLUI.Widgets` owns reusable runtime widgets, row item types, and viewmodels.
- `SQLUI.Samples` owns minimal sample host scaffolding and later sample assets.
- `JerryRigged` stays a thin host project layer.

Related architecture doc: [SQLUI repository architecture](docs/sqlui_repository_architecture.md).

## First Success Criteria
- The `SQLUI` plugin and three milestone-1 modules exist.
- Project and plugin registration are in place.
- `SQLUI.Samples` exists only as minimal scaffolding.
- The current repo compiles cleanly with minimal reusable structure.

## Open Questions
- Should `SQLUI.Samples` stay shippable later or become dev-only once the core stabilizes?
- Where should seed databases live in source control long-term: plugin resources, plugin content, or host-project-owned files?
- Should layout documents get a schema version in the first read milestone, and if so should it be an integer revision or semantic version?
- Which platforms matter first for graceful SQLite fallback behavior?
