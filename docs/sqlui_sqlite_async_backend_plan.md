# SQLUI SQLite Async Backend Plan

This document drafts the async and backend boundary for a future SQLite-backed SQLUI layout repository. The original plan was documentation-only. The current proof work includes minimal `SQLiteCore` availability and open/close probes, a SQLUICore-owned async boundary/probe that runs plain database-style work on a background task and delivers the result back through a game-thread callback, and a smoke-only migration-runner probe. It still does not add SQLite layout persistence, real layout schema migrations, SQLite repository selection, widgets, maps, assets, CI, or persistent database files.

## Purpose

The future SQLite repository should provide durable layout persistence while preserving the current SQLUI repository contract and keeping widgets unaware of storage details.

The async/backend plan defines how the repository should handle:

- Game-thread and worker-thread responsibilities.
- Database connection, transaction, migration, seed-copy, and shutdown ownership.
- Result delivery through the existing callback-style repository APIs.
- Backend-unavailable behavior and error mapping.
- Selection criteria for choosing a concrete SQLite implementation later.

The schema itself is drafted separately in [`sqlui_sqlite_layout_schema.md`](sqlui_sqlite_layout_schema.md).

## Non-Goals

- Do not implement SQLite layout persistence in this step.
- Do not choose a concrete SQLite plugin or library beyond the already-proven engine `SQLiteCore` candidate.
- Do not add new SQLite module dependencies or modify `Build.cs`.
- Do not add SQLite repository C++ code, real layout schema migrations, or persistent database files.
- Do not expand the async scaffold beyond a small proof that can run plain background work and marshal a result safely.
- Do not expose SQL, table names, database paths, worker objects, or SQLite connection details to `SQLUI.Widgets`.
- Do not change existing smoke-test behavior unless an optional probe flag is explicitly passed.
- Do not require the first SQLite implementation to ship history, checkpoints, previews, or search UI.

## Design Principles

The SQLite repository should act like another implementation of the existing layout repository boundary, not like a new widget-facing subsystem.

Recommended principles:

- Repository public methods keep the current callback contract.
- Public methods perform lightweight request validation on the game thread.
- File and database open, seed copy, migrations, queries, writes, deletes, and cleanup run behind a background boundary.
- Result callbacks marshal back to the game thread before touching UObjects or user-facing runtime objects.
- Transactions stay inside worker-side database operations.
- Shutdown invalidates pending callbacks or converts them into safe failure results.
- The repository never blocks gameplay waiting on SQLite.

## Async and Threading Model

SQLite work must not run on the game thread.

The recommended shape is a small SQLUI Core-owned worker boundary:

1. A repository `UObject` receives public `LoadLayout`, `SaveLayout`, `ListLayouts`, `RemoveLayout`, and `ClearLayouts` requests.
2. The repository validates cheap request preconditions on the game thread.
3. The repository packages immutable request data for worker execution.
4. A background database boundary opens or reuses the SQLite connection, runs migrations if needed, executes SQL, and builds plain result data.
5. Completion is marshaled back to the game thread.
6. The repository converts plain result data into the existing repository result structs and invokes the caller callback if it is still safe to do so.

This boundary can be implemented with whichever Unreal async primitive best fits the chosen backend later. The design requirement is the boundary itself, not a specific task API.

The current scaffold proves only the boundary shape. `FSQLUIDatabaseAsyncRunner` accepts immutable request data, runs a simulated database-style task on a background thread, and marshals `FSQLUIDatabaseAsyncResult` back to the game thread before invoking the callback. It deliberately does not open SQLite, execute SQL, write files, manage migrations, or expose a repository backend.

## Game-Thread Responsibilities

The repository `UObject` and public API layer should own game-thread work:

- Accept repository requests from runtime code.
- Validate lightweight parameters such as empty layout ids, invalid document metadata, or missing callbacks when the current contract requires one.
- Run SQLUI layout document validation before enqueueing `SaveLayout` writes.
- Serialize or copy request data only if doing so is cheap and does not touch SQLite.
- Track repository lifetime, shutdown state, and request generation tokens.
- Deliver callbacks on the game thread.
- Avoid touching SQLite connection objects directly.

The game-thread layer should not:

- Open database files.
- Run migrations.
- Execute SQL.
- Wait synchronously for a worker result.
- Hold database transactions.
- Perform seed-copy or database cleanup.

## Worker-Thread Responsibilities

Worker-side database code should own storage work:

- Resolve and create database directories under `Saved/SQLUI/...`.
- Copy seed databases into writable runtime storage when seed support is added.
- Open, configure, and close SQLite connections.
- Enable required SQLite pragmas, such as foreign keys, if supported by the chosen backend.
- Apply migrations before repository operations.
- Execute read and write statements.
- Own SQL transactions.
- Convert database rows into plain data transfer structures.
- Report backend, migration, SQL, serialization, and file-system errors without touching UObjects.

Worker tasks should be cancellable or safely discardable at the result-delivery boundary. They do not need to interrupt SQLite mid-statement in the first implementation, but they must not call stale UObjects after shutdown.

## Repository Public API Expectations

The SQLite repository should preserve the existing repository surface:

- `LoadLayout(LayoutId, Callback)`
- `SaveLayout(Document, Callback)`
- `ListLayouts(Callback)`
- `RemoveLayout(LayoutId, Callback)`
- `ClearLayouts(Callback)`

The callback contract should remain valid whether the concrete repository completes immediately, like the current in-memory and JSON-file repositories, or completes asynchronously, like the future SQLite implementation.

Public calls should return quickly. If a request cannot be enqueued because the backend is unavailable, the repository is shutting down, or required settings are invalid, it should deliver a failure result through the same callback path.

## Callback Delivery Rules

Callbacks should be delivered on the game thread.

Delivery rules:

- Invoke each callback at most once.
- Preserve current result structs and fields.
- Never invoke callbacks directly from the database worker.
- Do not touch UObjects, widget instances, worlds, or game-instance state from worker-side code.
- If the repository is destroyed or shut down before completion, either suppress stale callbacks or deliver a safe failure result, depending on what is safest for the call site.
- Prefer deterministic logging when callbacks are skipped because a request became stale.

The first SQLite implementation should document which shutdown behavior it chooses. Suppressing stale callbacks is acceptable when the owning UObject is gone. Returning a failure result is preferable when the repository is still alive but has entered an explicit shutdown state.

## UObject Lifetime and Shutdown Safety

The repository `UObject` should own the public lifetime, but it should not rely on worker callbacks holding raw UObject pointers.

Recommended safety mechanisms:

- Store pending requests with monotonically increasing request ids or generation tokens.
- Capture weak object references or plain data in game-thread completion lambdas.
- Mark the repository as shutting down before releasing worker resources.
- Invalidate pending request tokens during shutdown.
- Drain, cancel, or detach worker tasks in a controlled way.
- Close the database connection only on the worker side after queued operations are complete or safely abandoned.

`BeginDestroy`, subsystem shutdown, or an explicit repository shutdown method should prevent new requests from being accepted and should make pending completions harmless.

## Cancellation and Stale Callback Behavior

Initial cancellation can be cooperative and boundary-based.

Expected behavior:

- Requests accepted before shutdown may finish on the worker.
- Results for stale requests should be ignored if their repository generation no longer matches.
- Results should not resurrect destroyed UObjects.
- If a newer request supersedes an older one in a future API, the older callback should return a clear stale/cancelled failure or be explicitly suppressed by documented policy.
- Worker code should check cancellation tokens before expensive multi-step work such as seed copy, migration, or clear operations when practical.

The current repository result types do not have a dedicated cancellation field. Until one exists, cancellation should map to `bSucceeded = false` with a clear `ErrorMessage`, unless the callback is suppressed because the owner no longer exists.

## Database Connection Ownership

Connection ownership should be isolated behind the SQLUI Core worker boundary.

Recommended model:

- One repository instance owns one database connection context for its configured database file.
- The connection opens lazily on the first operation or during explicit repository initialization.
- The connection is configured once after open, including foreign-key behavior and any backend-specific pragmas.
- The connection is used from one worker queue or otherwise protected so SQLite operations are serialized per repository database.
- The connection is closed on the worker side during shutdown.

The repository public API should not expose the connection object. Widgets and sample code should never receive a SQLite handle.

## Transaction Ownership

Transactions should be created and completed inside worker-side database operations.

Expected rules:

- `SaveLayout` uses one transaction for metadata, revision, tag, and current-revision updates.
- `RemoveLayout` uses a transaction when updating soft-delete state and related metadata.
- `ClearLayouts` uses one transaction for destructive scoped cleanup.
- Migration batches use transactions where the chosen backend and SQLite migration rules allow it.
- Read-only `LoadLayout` and `ListLayouts` do not need explicit write transactions.

The game-thread repository layer should not begin, commit, roll back, or hold transactions.

## Migration Execution Timing

Migrations should run before repository operations use the database.

Recommended timing:

1. Resolve the writable database path under `Saved/SQLUI/...`.
2. Copy a seed database if configured and needed.
3. Open the database connection.
4. Configure connection pragmas.
5. Apply required migrations.
6. Mark the database context ready for normal operations.

Migrations can run lazily on the first operation, but they should be serialized so multiple initial repository requests do not attempt to migrate the same database concurrently.

Migration failures should fail closed. A repository operation should not continue against a partially migrated or unknown schema.

## Seed-Copy Timing

Seed-copy support is deferred, but the timing should be defined before implementation.

Expected seed-copy rules:

- Source-controlled seed databases are read-only inputs.
- Writable copies live under `Saved/SQLUI/...`.
- Seed copy runs before opening the writable database for mutation.
- If a writable copy already exists, migrate it instead of overwriting it.
- If the seed copy fails, return a backend-unavailable or operation failure result with a clear `ErrorMessage`.
- Seed copy should run on the worker side because it is file-system work and can be slow.

The repository factory may later accept seed path settings, but widgets should never see those paths.

## Error and Result Mapping

The SQLite repository should match current repository result semantics.

General mapping:

- Backend missing, disabled, not configured, unable to open, unable to migrate, or unable to copy seed data: `bBackendUnavailable = true` where the result type supports it, `bSucceeded = false`, with a clear `ErrorMessage`.
- Validation failure before save: `bSucceeded = false`, validation payload populated, backend unavailable false.
- Invalid stored JSON after load: `bSucceeded = false`, validation payload populated, backend unavailable false.
- Missing layout id: operation failure or no-op according to existing repository semantics, not backend unavailable.
- SQL constraint or transaction failure: `bSucceeded = false`, backend unavailable false unless the connection itself has become unusable.
- Shutdown/cancelled while repository is alive: `bSucceeded = false` with an explanatory `ErrorMessage`.

Operation-specific expectations:

- `SaveLayout` returns `SavedLayoutId` when a layout id was known, even on validation failures where useful.
- `LoadLayout` returns a document only after deserialization and validation succeed.
- `ListLayouts` returns metadata only for active non-deleted layouts by default.
- `RemoveLayout` uses soft-delete semantics and sets `bRemoved = true` only when an active layout became deleted.
- `ClearLayouts` reports `RemovedCount` as the number of layout records cleared from the selected repository scope.

## Backend-Unavailable Cases

The future SQLite backend should report clean unavailability when:

- The project was built without the selected SQLite dependency.
- The selected SQLite module fails to load or initialize.
- The database path is invalid or outside the allowed runtime storage boundary.
- The repository cannot create the parent directory under `Saved/SQLUI/...`.
- The database file cannot be opened.
- Required migrations cannot be applied.
- A seed database is required but missing or cannot be copied.
- The repository is already shutting down before work can be enqueued.

Unavailable-backend failures should be explicit and actionable in logs. They should not look like layout validation failures.

## Logging Expectations

SQLite logging should be clear enough for smoke tests and packaged runtime diagnostics without dumping user layout JSON by default.

Recommended log events:

- Repository backend selected and configured.
- Database path resolved under `Saved/SQLUI/...`.
- Seed copy started, skipped, succeeded, or failed.
- Database open succeeded or failed.
- Migration started, applied, skipped, succeeded, or failed.
- Repository operations started and completed at a concise level for smoke paths.
- Remove soft-deleted an active layout or found no active layout.
- Clear completed with `RemovedCount`.
- Callback skipped because the repository was destroyed or the request was stale.

Do not log full `document_json` bodies by default. If verbose document logging is added later, it should be opt-in.

## Smoke-Test Implications

Existing smoke paths should remain unchanged until SQLite is implemented.

The optional database async probe exercises the minimal SQLUICore async boundary without touching SQLite or the filesystem. In commandlet smoke runs, the harness pumps the game-thread task queue while waiting for the probe callback because there is no normal gameplay tick while the commandlet stack is blocked. Runtime code should use the callback asynchronously rather than copying that blocking smoke-test wait pattern.

The optional SQLite migration probe exercises only a smoke-safe migration-runner slice. It opens a temporary database under `Saved/SQLUI/SmokeTests/SQLiteMigrationProbe`, creates and records a probe migration in `sqlui_schema_migrations`, verifies the row, closes the database, and removes the file. That probe is intentionally separate from the real layout schema migration and does not change repository factory selection or implement SQLite layout persistence.

When the SQLite backend is added later, smoke coverage should prove:

- Repository factory selection creates the SQLite repository only when the backend is available.
- Backend-unavailable behavior is clean when SQLite support is absent or disabled.
- Runtime files are written only under `Saved/SQLUI/...`.
- `SaveLayout`, `LoadLayout`, `ListLayouts`, `RemoveLayout`, and `ClearLayouts` preserve the same lifecycle semantics as current repositories.
- SQLite operations complete without blocking the runtime widget pipeline.
- Result callbacks are delivered on the game thread.
- Shutdown or stale callback behavior is safe and logged.

SQLite smoke tests should not add maps, Content assets, editor tooling, CI, or durable database files to source control.

## Repository Factory Integration

`USQLUILayoutRepositoryFactory` is the correct runtime selection boundary for a future SQLite implementation.

Expected future fit:

- Add a new `ESQLUILayoutRepositoryBackend::SQLite` value only when SQLite implementation work begins.
- Extend `FSQLUILayoutRepositoryFactorySettings` with SQLite-specific settings only as needed, such as database base directory, database file name, seed path, or migration policy.
- Keep default settings pointed at existing backends until SQLite is ready.
- Return `USQLUILayoutRepository` or a clear unavailable result path when SQLite is selected but not compiled, disabled, or misconfigured.
- Keep widget and sample runtime code dependent on repository interfaces and factory settings, not concrete SQLite classes.

The factory should remain a small object-construction boundary. It should not execute migrations or open database files directly on the game thread.

## Backend Selection Criteria

A concrete SQLite implementation should be chosen later using explicit criteria.

Selection criteria:

- Unreal Engine 5.7 compatibility.
- Runtime support, not editor-only support.
- Packaged build behavior across the target platforms.
- Threading support and clear rules for connection ownership.
- Ability to control database file paths under `Saved/SQLUI/...`.
- Ability to run raw SQL migrations safely or equivalent migration support.
- Transaction support with reliable rollback behavior.
- Error reporting quality for open, migration, query, constraint, and transaction failures.
- License suitability for the project.
- Minimal dependency footprint.
- Long-term maintainability and update path.
- Compatibility with Unreal build, module, and packaging workflows.
- Testability in commandlet or smoke-test style flows without editor-only dependencies.

No backend should be selected merely because it is convenient for editor tooling. The first SQLite implementation should serve runtime repository behavior.

## Implementation Phases

SQLite implementation should proceed in small, reviewable phases.

### Phase 1: Backend Evaluation and Wiring Proposal

Evaluate candidate SQLite backends against the selection criteria. Propose the minimal plugin or module wiring, packaging behavior, and settings shape before changing build files.

### Phase 2: Async Database Service Boundary

Use the minimal SQLUI Core-owned async scaffold as the starting point, then harden it for real SQLite work. The implementation PR should decide whether the simple task-graph runner remains sufficient or whether SQLite needs a serialized worker queue, explicit cancellation tokens, or a longer-lived database service boundary.

### Phase 3: Migration Runner

Build on the smoke-only migration-runner proof by adding executable migrations for the agreed layout schema and recording them in `sqlui_schema_migrations`. Ensure migration failures produce clear unavailable-backend results.

### Phase 4: Read-Only Repository Operations

Implement read-only SQLite repository behavior for `LoadLayout` and `ListLayouts`, including deserialization and validation after load.

### Phase 5: Write Repository Operations

Implement `SaveLayout`, `RemoveLayout`, and `ClearLayouts`, including transaction ownership, revision creation, soft-delete behavior, tag updates, and clear-count semantics.

### Phase 6: Smoke Coverage

Extend smoke coverage for SQLite selection and repository operations only after implementation exists. Keep tests scoped to `Saved/SQLUI/...`.

### Phase 7: Optional Lifecycle Features

Add history, checkpoints, previews, restore, retention, or search improvements when product workflows require them.

## Open Decisions for Implementation

The following decisions remain intentionally deferred:

- Which SQLite backend to use.
- Whether SQLite support is always compiled or feature-gated.
- Exact async primitive or worker queue implementation.
- Database path settings names and defaults.
- Seed database settings shape.
- Whether migrations run lazily or during explicit initialization.
- Cancellation policy for still-running worker operations.
- Retention policy for old revisions and previews.
- Whether full-text search is worth the extra schema and packaging surface.
