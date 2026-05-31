# SQLUI SQLite Layout Schema Draft

This document describes the SQLite layout schema used by SQLUI's SQLite repository work. The original schema draft was documentation-only; SQLUICore now has an initial schema migration helper, schema-init hardening coverage, SQLite read/list/load probes, and `USQLUISQLiteLayoutRepository` operations that use this schema for save, list, load, soft-delete remove, and scoped clear behavior.

For the consolidated current implementation status, see [`sqlui_sqlite_runtime_status.md`](sqlui_sqlite_runtime_status.md).

## Purpose

The SQLite repository should provide durable runtime layout storage behind the existing SQLUI repository boundary. Widgets, widget factories, and runtime pipeline code should continue to work with layout documents, repositories, runtime contexts, variable stores, and action systems. They should not know SQL table names, SQLite connection objects, file paths, or concrete storage classes.

The proposed schema is meant to support the current repository lifecycle plus later layout history, checkpoints, previews, tags, and searchable metadata.

## Non-Goals

- Do not make SQLite the default repository backend.
- Do not run migrations inside `USQLUILayoutRepositoryFactory`; the factory passes settings only.
- Do not treat local smoke coverage as packaged-build validation.
- Do not expose SQL or schema details to `SQLUI.Widgets`.
- Do not replace JSON document validation with database constraints.
- Do not use `Content/`, maps, or assets as writable runtime layout storage.
- Do not run database work on the game thread.
- Do not require all future lifecycle features to ship with the first SQLite repository.

## Storage Paths

Writable SQLite files should live under `Saved/SQLUI/...`.

Recommended default path:

```text
Saved/SQLUI/Layouts/Layouts.db
```

Smoke tests or sample-specific stores should use narrower scopes such as:

```text
Saved/SQLUI/SmokeTests/Layouts/Layouts.db
Saved/SQLUI/SmokeTests/LayoutSchemaMigrationProbe/LayoutSchemaMigrationProbe.db
Saved/SQLUI/SmokeTests/LayoutReadProbe/LayoutReadProbe.db
Saved/SQLUI/Samples/LayoutDrivenFilterList/Layouts.db
```

The repository factory now supports explicit SQLite selection through `ESQLUILayoutRepositoryBackend::SQLite` and passes `SQLiteSettings` into `USQLUISQLiteLayoutRepository`. The factory does not run migrations or create database files. A missing database may be created and initialized only by a writable repository when `bInitializeSchemaIfMissing = true` and `bCreateDatabaseIfMissing = true`.

The layout schema migration smoke proof uses `Saved/SQLUI/SmokeTests/LayoutSchemaMigrationProbe/LayoutSchemaMigrationProbe.db` only as temporary runtime output and removes the database and SQLite sidecar files after verification.

The layout read smoke proof uses `Saved/SQLUI/SmokeTests/LayoutReadProbe/LayoutReadProbe.db` only as temporary runtime output. It applies the same planned schema, inserts one valid probe-only layout into `layouts`, `layout_revisions`, and `layout_tags`, verifies list/load style reads, deserializes and validates the loaded document JSON, then removes the database and SQLite sidecar files after verification.

## Seed Copy Expectations

If seed databases are added later, source-controlled seed files may live in a project or plugin seed-data folder, but they should be treated as read-only inputs. Before any mutation, the runtime repository should copy the seed database into `Saved/SQLUI/...` and open the writable copy.

Seed copy rules:

- Never write to `Content/`, plugin content, or source-controlled seed databases at runtime.
- Create parent directories under `Saved/SQLUI/...` before copying.
- Prefer explicit version checks before using a copied seed.
- If a writable copy already exists, migrate it instead of overwriting it.
- If seed copy fails, return a backend-unavailable or save/load failure result with a clear `ErrorMessage`.

## Schema Overview

The SQLite database should enable foreign keys when opening a connection:

```sql
PRAGMA foreign_keys = ON;
```

Timestamps are stored as UTC ISO-8601 text. Boolean values are stored as integer `0` or `1`. `document_json` stores the canonical SQLUI layout document JSON for a specific revision or preview.

## Tables

### `sqlui_schema_migrations`

Tracks applied schema migrations.

| Column | Type | Constraints | Notes |
| --- | --- | --- | --- |
| `migration_id` | `TEXT` | `PRIMARY KEY` | Stable migration identifier, such as `001_initial_layout_schema`. |
| `applied_at_utc` | `TEXT` | `NOT NULL` | UTC timestamp when the migration completed. |
| `description` | `TEXT` | | Human-readable migration summary. |

### `layouts`

Stores current layout metadata and soft-delete state.

| Column | Type | Constraints | Notes |
| --- | --- | --- | --- |
| `layout_id` | `TEXT` | `PRIMARY KEY` | Matches `FSQLUILayoutMetadata.LayoutId`. |
| `display_name` | `TEXT` | `NOT NULL` | Layout picker label. |
| `description` | `TEXT` | | Layout description. |
| `created_by` | `TEXT` | | Creator or system label. |
| `created_at_utc` | `TEXT` | | Creation timestamp. |
| `updated_at_utc` | `TEXT` | | Last metadata or revision update timestamp. |
| `current_revision` | `INTEGER` | | Revision loaded by default. |
| `schema_version` | `INTEGER` | `NOT NULL` | SQLUI document schema version for current revision. |
| `search_text` | `TEXT` | | Denormalized search text for display name, description, tags, and selected metadata. |
| `b_deleted` | `INTEGER` | `NOT NULL DEFAULT 0` | Soft-delete flag for normal remove behavior. |

### `layout_revisions`

Stores immutable revision documents for each layout.

| Column | Type | Constraints | Notes |
| --- | --- | --- | --- |
| `layout_id` | `TEXT` | `NOT NULL` | Parent layout id. |
| `revision` | `INTEGER` | `NOT NULL` | Monotonic revision number per layout. |
| `label` | `TEXT` | | Optional version label. |
| `document_json` | `TEXT` | `NOT NULL` | Canonical serialized `FSQLUILayoutDocument`. |
| `document_hash` | `TEXT` | | Optional content hash for duplicate detection or audit. |
| `created_by` | `TEXT` | | Author or system label. |
| `created_at_utc` | `TEXT` | | Revision creation timestamp. |
| `change_note` | `TEXT` | | Optional save note. |

Constraints:

```sql
PRIMARY KEY (layout_id, revision),
FOREIGN KEY (layout_id) REFERENCES layouts(layout_id)
```

### `layout_tags`

Stores normalized tags for layout filtering.

| Column | Type | Constraints | Notes |
| --- | --- | --- | --- |
| `layout_id` | `TEXT` | `NOT NULL` | Parent layout id. |
| `tag` | `TEXT` | `NOT NULL` | Exact tag value. |

Constraints:

```sql
PRIMARY KEY (layout_id, tag),
FOREIGN KEY (layout_id) REFERENCES layouts(layout_id)
```

### `layout_checkpoints`

Stores named checkpoints for durable known-good revision references. This can be deferred until checkpoint UI or workflow support exists.

| Column | Type | Constraints | Notes |
| --- | --- | --- | --- |
| `checkpoint_id` | `TEXT` | `PRIMARY KEY` | Stable generated checkpoint id. |
| `layout_id` | `TEXT` | `NOT NULL` | Parent layout id. |
| `revision` | `INTEGER` | `NOT NULL` | Referenced revision. |
| `label` | `TEXT` | | Human-readable checkpoint label. |
| `created_by` | `TEXT` | | Author or system label. |
| `created_at_utc` | `TEXT` | | Checkpoint creation timestamp. |
| `notes` | `TEXT` | | Optional notes. |

Constraints:

```sql
FOREIGN KEY (layout_id, revision) REFERENCES layout_revisions(layout_id, revision)
```

### `layout_previews`

Stores temporary draft documents for preview sessions. This can be deferred until preview/edit workflows exist.

| Column | Type | Constraints | Notes |
| --- | --- | --- | --- |
| `preview_id` | `TEXT` | `PRIMARY KEY` | Stable generated preview id. |
| `layout_id` | `TEXT` | `NOT NULL` | Parent layout id. |
| `base_revision` | `INTEGER` | | Revision the preview started from. |
| `document_json` | `TEXT` | `NOT NULL` | Draft layout document JSON. |
| `created_by` | `TEXT` | | Author or system label. |
| `created_at_utc` | `TEXT` | | Preview creation timestamp. |
| `expires_at_utc` | `TEXT` | | Optional cleanup deadline. |

Constraints:

```sql
FOREIGN KEY (layout_id) REFERENCES layouts(layout_id)
```

## Suggested Initial DDL

The SQLUICore layout schema migration smoke proof applies the layout tables and indexes below as migration `001_initial_layout_schema`. The shared migration runner owns creation and recording of `sqlui_schema_migrations`.

```sql
CREATE TABLE IF NOT EXISTS sqlui_schema_migrations (
	migration_id TEXT PRIMARY KEY,
	applied_at_utc TEXT NOT NULL,
	description TEXT
);

CREATE TABLE IF NOT EXISTS layouts (
	layout_id TEXT PRIMARY KEY,
	display_name TEXT NOT NULL,
	description TEXT,
	created_by TEXT,
	created_at_utc TEXT,
	updated_at_utc TEXT,
	current_revision INTEGER,
	schema_version INTEGER NOT NULL,
	search_text TEXT,
	b_deleted INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS layout_revisions (
	layout_id TEXT NOT NULL,
	revision INTEGER NOT NULL,
	label TEXT,
	document_json TEXT NOT NULL,
	document_hash TEXT,
	created_by TEXT,
	created_at_utc TEXT,
	change_note TEXT,
	PRIMARY KEY (layout_id, revision),
	FOREIGN KEY (layout_id) REFERENCES layouts(layout_id)
);

CREATE TABLE IF NOT EXISTS layout_tags (
	layout_id TEXT NOT NULL,
	tag TEXT NOT NULL,
	PRIMARY KEY (layout_id, tag),
	FOREIGN KEY (layout_id) REFERENCES layouts(layout_id)
);

CREATE TABLE IF NOT EXISTS layout_checkpoints (
	checkpoint_id TEXT PRIMARY KEY,
	layout_id TEXT NOT NULL,
	revision INTEGER NOT NULL,
	label TEXT,
	created_by TEXT,
	created_at_utc TEXT,
	notes TEXT,
	FOREIGN KEY (layout_id, revision) REFERENCES layout_revisions(layout_id, revision)
);

CREATE TABLE IF NOT EXISTS layout_previews (
	preview_id TEXT PRIMARY KEY,
	layout_id TEXT NOT NULL,
	base_revision INTEGER,
	document_json TEXT NOT NULL,
	created_by TEXT,
	created_at_utc TEXT,
	expires_at_utc TEXT,
	FOREIGN KEY (layout_id) REFERENCES layouts(layout_id)
);
```

## Recommended Indexes

```sql
CREATE INDEX IF NOT EXISTS idx_layouts_display_name ON layouts(display_name);
CREATE INDEX IF NOT EXISTS idx_layouts_updated_at_utc ON layouts(updated_at_utc);
CREATE INDEX IF NOT EXISTS idx_layouts_b_deleted ON layouts(b_deleted);
CREATE INDEX IF NOT EXISTS idx_layout_revisions_layout_revision ON layout_revisions(layout_id, revision);
CREATE INDEX IF NOT EXISTS idx_layout_tags_tag ON layout_tags(tag);
CREATE INDEX IF NOT EXISTS idx_layout_previews_layout_id ON layout_previews(layout_id);
```

Optional later indexes:

- `layouts(search_text)` if simple `LIKE` searches are enough.
- An FTS virtual table if full-text search becomes important.
- `layout_previews(expires_at_utc)` for cleanup jobs.
- `layout_checkpoints(layout_id, revision)` for history views.

## Unique Constraints

The schema starts with these uniqueness rules:

- `layouts.layout_id` is unique by primary key.
- `(layout_id, revision)` is unique by `layout_revisions` primary key.
- `(layout_id, tag)` is unique by `layout_tags` primary key.
- `checkpoint_id` is unique by primary key.
- `preview_id` is unique by primary key.

`display_name` is intentionally not unique. Multiple layouts can share a name, and list or picker UI should disambiguate by metadata or layout id.

## Revision and History Semantics

`SaveLayout` should create a new immutable row in `layout_revisions` and update `layouts.current_revision` to that revision. The first save for a layout should create revision `1`. Later saves should use `MAX(revision) + 1` for the same `layout_id`.

`layouts.current_revision` points to the revision used by default `LoadLayout`. Older revisions remain available for future history APIs. Normal saves should not overwrite prior `document_json` values.

The repository should store the full serialized layout document per revision. This keeps load behavior simple and preserves compatibility with the existing JSON validation boundary. Delta storage can be reconsidered later, but it is not part of this draft.

## Remove and Clear Semantics

Normal `RemoveLayout` should be a soft delete for the SQLite backend:

- Set `layouts.b_deleted = 1`.
- Update `layouts.updated_at_utc`.
- Keep `layout_revisions`, `layout_tags`, `layout_checkpoints`, and `layout_previews` unless cleanup policy says otherwise.
- Return `bSucceeded = true`.
- Return `bRemoved = true` only when a non-deleted layout was changed to deleted.
- Return `bRemoved = false` when the layout id does not exist or is already deleted, with an explanatory `ErrorMessage` matching current repository no-op style.

`ListLayouts` should exclude `b_deleted = 1` rows by default.

`LoadLayout` should not load soft-deleted layouts by default. If future restore/history APIs need deleted rows, they should be explicit.

`ClearLayouts` is a destructive cleanup operation for the selected repository scope. It should hard-delete rows for the configured database, including previews, checkpoints, tags, revisions, and layouts. It should return `RemovedCount` as the number of layout records cleared from `layouts`, not the total number of child rows removed.

## Tags and Search Metadata

Tags from `FSQLUILayoutMetadata.Tags` should be normalized into `layout_tags`.

`layouts.search_text` should be denormalized from fields useful to layout pickers and search screens, such as:

- `layout_id`
- `display_name`
- `description`
- `created_by`
- tags
- selected `SearchMetadata` key/value pairs

The canonical copy of full metadata still lives inside `document_json` for each revision. Columns and tag rows exist so list and search operations do not need to load every document body.

## Preview and Checkpoint Semantics

Checkpoints are durable named references to existing revisions. They should not copy document JSON because the referenced revision is immutable.

Previews are temporary draft documents. `layout_previews.document_json` can differ from any saved revision and should be validated before storage and after load. Expired previews can be cleaned by a later repository maintenance operation.

Deferred future operations:

- `SaveCheckpoint` inserts a row in `layout_checkpoints` for an existing `(layout_id, revision)`.
- `ApplyPreview` validates preview JSON, saves it as a new `layout_revisions` row, updates `layouts.current_revision`, and removes or expires the preview.
- `DiscardPreview` deletes one preview row.

## Repository Operation Mapping

### `SaveLayout`

Expected flow:

1. Validate `FSQLUILayoutDocument` with SQLUI layout validation before opening a write transaction.
2. Serialize the document to canonical JSON.
3. Begin a transaction.
4. Insert or update `layouts` metadata with `b_deleted = 0`.
5. Compute the next revision for `layout_id`.
6. Insert `layout_revisions`.
7. Replace `layout_tags` for that layout.
8. Update `layouts.current_revision`, `schema_version`, `search_text`, and `updated_at_utc`.
9. Commit.

Failure mapping:

- Validation failure: `bSucceeded = false`, validation payload populated.
- Backend unavailable or database open failure: `bBackendUnavailable = true`, `bSucceeded = false`.
- SQL write failure: rollback and return `bSucceeded = false` with `ErrorMessage`.

### `LoadLayout`

Expected flow:

1. Query `layouts` where `layout_id = ?` and `b_deleted = 0`.
2. Join or query `layout_revisions` using `current_revision`.
3. Deserialize `document_json`.
4. Validate the loaded document before returning it.

Failure mapping:

- Missing or deleted layout: `bSucceeded = false`, `bBackendUnavailable = false`, clear not-found `ErrorMessage`.
- Invalid stored JSON: `bSucceeded = false`, validation payload populated.
- Backend unavailable: `bBackendUnavailable = true`, `bSucceeded = false`.

### `ListLayouts`

Expected flow:

1. Query `layouts` where `b_deleted = 0`.
2. Apply optional search or tag filters later if the repository contract expands.
3. Return `FSQLUILayoutMetadata` values from metadata columns plus tags from `layout_tags`.

Default ordering should match current repository behavior where practical:

```sql
ORDER BY display_name COLLATE NOCASE ASC, layout_id COLLATE NOCASE ASC
```

### `RemoveLayout`

Expected flow:

1. Update `layouts` set `b_deleted = 1` for a matching non-deleted `layout_id`.
2. Return `bRemoved = true` if one row changed.
3. Return successful no-op semantics when no active layout existed.

This is intentionally a soft delete.

### `ClearLayouts`

Expected flow:

1. Begin a transaction.
2. Count rows in `layouts` for the selected repository scope.
3. Delete child rows from `layout_previews`, `layout_checkpoints`, `layout_tags`, and `layout_revisions`.
4. Delete rows from `layouts`.
5. Commit.
6. Return the counted layout row count as `RemovedCount`.

This is intentionally destructive cleanup for smoke tests, local reset workflows, and scoped repository cleanup.

### Future `LoadHistory`

Query `layout_revisions` for a `layout_id`, ordered by `revision DESC` or `created_at_utc DESC`. It should include revision metadata and optionally omit `document_json` until a specific revision is requested.

### Future `SaveCheckpoint`

Validate that `(layout_id, revision)` exists, then insert a `layout_checkpoints` row. Duplicate checkpoint ids should fail clearly.

### Future `ApplyPreview`

Load and validate `layout_previews.document_json`, save it as a new revision through the same transaction path as `SaveLayout`, then delete or mark the preview as applied. If validation fails, leave the preview untouched.

## Migration and Versioning Approach

The first SQLite implementation should apply named migrations inside a transaction and record them in `sqlui_schema_migrations`.

The current layout schema migration helper applies the planned initial DDL to temporary smoke databases and to explicitly configured repository databases when schema initialization is enabled. Layout read and repository smoke paths seed or save valid layouts into that schema and verify list/load behavior.

Schema initialization is hardened for the current initial migration:

- Missing databases fail when creation is disabled.
- Empty databases initialize when creation is enabled.
- Already-initialized databases succeed idempotently without duplicate migration rows.
- Complete schemas with a missing initial migration row are recorded non-destructively.
- Partial schemas fail clearly and report missing expected objects.
- Read-only repositories reject writes before schema initialization can create database files.

Migration rules:

- Every migration has a stable `migration_id`.
- Each migration is idempotent or guarded by `sqlui_schema_migrations`.
- Apply migrations before repository operations.
- Fail closed if a required migration cannot apply.
- Never silently drop user layout data.
- Back up or copy seed databases before mutating them.
- Keep document schema upgrades separate from database schema migrations when possible.

`layouts.schema_version` stores the SQLUI layout document schema version for the current revision. Database migration version and document schema version are related but separate concerns.

## Validation Boundaries

The SQLite repository should keep the same validation boundaries as the current repositories:

- Validate `FSQLUILayoutDocument` before `SaveLayout` writes anything.
- Serialize only validated documents.
- Deserialize and validate `document_json` after `LoadLayout`.
- Treat invalid stored documents as load failures with validation details.
- Preserve validation warnings in result payloads when available.

Database constraints protect storage shape. SQLUI layout validation protects runtime document semantics.

## Threading and Async Boundaries

SQLite work must not run on the game thread.

Expected implementation shape:

- Repository public methods can keep the existing callback contract.
- File open, migrations, reads, writes, and cleanup should run behind an async or background boundary.
- Completion callbacks should marshal results back to the appropriate game-thread boundary before touching UObjects.
- Transactions should stay inside the worker-side database operation.
- The repository should guard shutdown so callbacks do not access invalid UObjects.

The current in-memory and JSON-file repositories complete immediately, but the SQLite backend should use the same interface without blocking gameplay.

## Failure and Result Semantics

The SQLite repository should match current result types:

- `bSucceeded = true` only when the requested operation completed successfully.
- `bBackendUnavailable = true` only when the database backend is unavailable, cannot open, cannot migrate, or is not configured.
- Missing layout ids are normal operation failures, not backend-unavailable failures.
- `ErrorMessage` should explain the failure or useful no-op.
- `Validation` should be populated when document validation or deserialization validation was attempted.
- `SavedLayoutId` should echo the document layout id for save attempts.
- `RemovedLayoutId` should echo the requested layout id for remove attempts.
- `RemovedCount` should report the number of layouts cleared, not every child row.

## Deferred Decisions

These decisions should be made when implementation starts:

- Concrete SQLite plugin or backend.
- Exact async service integration.
- Database path settings shape in `FSQLUILayoutRepositoryFactorySettings`.
- Whether FTS is needed for search.
- Restore API for soft-deleted layouts.
- Retention policy for old previews and revisions.
- Whether document hashes use SHA-1, SHA-256, or another stable hash available in Unreal.
