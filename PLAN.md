# PLAN.md

## Milestone 1: Skeleton And Compile
Create the `SQLUI` plugin, add `SQLUI.Core`, `SQLUI.Widgets`, and `SQLUI.Samples`, register them with the host project, keep `JerryRigged` thin, and reach a clean compile.

## Milestone 2: Startup Safety Path
Add settings, seed database copy-to-`Saved`, SQLite availability checks, graceful fallback behavior, and a startup self-check path.

## Milestone 3: First Layout Read Slice
Add layout structs, JSON serialization, repository boundaries, and an async path to load one layout document and build a first runtime widget tree.

## Milestone 4: Data-Driven Widget Slice
Add `FilterBox`, `SQLList`, row item types, viewmodels, variable store, and action registry wiring so widgets talk to services instead of SQL.

## Milestone 5: Layout Lifecycle
Add version history, checkpoints, preview sessions, tags, and search while keeping editor tooling, telemetry, warmups, packaging helpers, and commandlets out of scope until later phases.

## Open Questions
- Should `SQLUI.Samples` remain a runtime-facing sample module or move to a dev-only sample path later?
- Where should seed databases live in source control long-term?
- Do layout documents need a schema version field in the first read milestone?
- Which platforms should define milestone-2 fallback behavior first?
