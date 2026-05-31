# SQLUI Smoke Test

The SQLUI sample smoke test commandlet runs the current sample runtime widget pipeline in a transient commandlet world. It creates the sample widget catalog, variable store, runtime context, and layout request, then reports whether the root widget and pipeline steps succeeded.

By default, the smoke test uses the existing in-memory C++ layout document. It can also run the built-in SQLUISamples JSON layout fixture, which deserializes a tiny `SQLUI.FilterBox` layout through SQLUICore's layout JSON helpers before running the same runtime widget pipeline.

The in-memory repository smoke path reuses that same JSON fixture, selects the in-memory backend through the SQLUICore layout repository factory, saves the deserialized layout into `USQLUIInMemoryLayoutRepository`, verifies `ListLayouts` includes the saved metadata, loads it back by layout id, verifies `RemoveLayout` removes it, verifies `ListLayouts` no longer includes it, exercises `ClearLayouts`, and then runs the runtime widget pipeline with the loaded document.

The JSON file repository smoke path also reuses the JSON fixture, selects the JSON-file backend through the SQLUICore layout repository factory, saves the deserialized layout into `USQLUIJsonFileLayoutRepository`, verifies `ListLayouts` includes the saved metadata, loads it back by layout id, verifies `RemoveLayout` removes it, verifies `ListLayouts` no longer includes it, exercises `ClearLayouts`, and then runs the runtime widget pipeline with the loaded document.

Both repository smoke paths also select the unavailable backend through the factory and verify load/save report `bBackendUnavailable` cleanly. This keeps unavailable persistence explicit without adding SQLite.

The optional SQLiteCore probe opens and closes a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteCoreProbe`, then removes the probe database file. This proves engine `SQLiteCore` can use a runtime-writable SQLUI smoke-test path without adding a layout repository, migrations, schema tables, async database workers, or repository factory behavior.

The optional database async probe runs a tiny SQLUICore-owned database-boundary request on a background thread and delivers the result back through the game-thread callback path. It does not open SQLite, run SQL, write database files, add migrations, or change repository selection.

The optional SQLite migration probe opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe`, creates the smoke-only migration tracking table, applies and records one probe migration, verifies the migration row, closes the database, and removes the probe database file. This is not the planned SQLUI layout schema migration and does not add a SQLite layout repository.

The optional SQLite layout schema migration probe opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\LayoutSchemaMigrationProbe`, applies the planned initial layout schema through the SQLUICore migration runner, verifies the expected layout tables and indexes exist, closes the database, and removes the probe database file. This proves the schema DDL can apply locally without adding a SQLite layout repository or repository factory selection.

The optional SQLite layout read probe opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\LayoutReadProbe`, applies the planned initial layout schema, seeds one probe-only layout document into `layouts`, `layout_revisions`, and `layout_tags`, verifies list-style metadata and current-revision document load queries, deserializes and validates the loaded JSON, closes the database, and removes the probe database file. This proves read/list/load mapping against the schema without adding a SQLite layout repository or repository factory selection.

The optional SQLite read-only layout repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteReadOnlyRepository`, prepares it with the planned schema and one probe-only layout, then reads it through `USQLUISQLiteLayoutRepository`. It verifies `ListLayouts` metadata and tags, verifies `LoadLayout` deserializes and validates the current document, verifies `SaveLayout`, `RemoveLayout`, and `ClearLayouts` are rejected while the repository is read-only, closes all database handles, and removes the probe database file.

The optional SQLite SaveLayout repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteSaveLayoutRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false`, saves one probe-only layout, verifies `ListLayouts` metadata and tags, verifies `LoadLayout`, saves the same layout id again with updated metadata, verifies the latest revision is loaded, and removes the probe database file. This proves the SaveLayout SQLite repository operation without adding SQLite factory selection, async SQLite workers, or persistent database files.

The optional SQLite RemoveLayout repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteRemoveLayoutRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false`, saves one probe-only layout, verifies it is listed and loadable, soft-deletes it through `RemoveLayout`, verifies `ListLayouts` and `LoadLayout` no longer expose the layout, verifies revision history remains present, and removes the probe database file. This proves SQLite soft-delete semantics without adding SQLite factory selection, async SQLite workers, or persistent database files.

The optional SQLite ClearLayouts repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteClearLayoutsRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false`, saves two probe-only layouts, soft-deletes one through `RemoveLayout`, destructively clears the selected repository scope through `ClearLayouts`, verifies active and soft-deleted layout rows plus dependent schema rows are gone, and removes the probe database file. This proves scoped cleanup semantics without adding SQLite factory selection, async SQLite workers, or persistent database files.

The optional SQLite full lifecycle repository smoke path opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteFullLifecycleRepository`, prepares it with the planned schema, configures `USQLUISQLiteLayoutRepository` with `bReadOnly = false`, saves one layout, lists and loads it, saves it again as revision 2 with updated metadata and tags, saves a second layout, soft-deletes the first layout, verifies revision history remains, destructively clears the selected repository scope, verifies schema rows are empty, and removes the probe database file. This combines the currently supported SQLite repository operations without adding SQLite factory selection, async SQLite workers, or persistent database files.

This is a local developer workflow only. It is not CI yet, and it does not assume Unreal Engine is installed on GitHub Actions or any build agent.

The smoke test does not edit maps, levels, Content, persistent database files, or the viewport. It does not add SQLite factory selection or attach widgets to the viewport. The JSON file repository smoke path writes only under `Saved\SQLUI\SmokeTests\Layouts`, removes its saved layout after loading it, and clears remaining layouts in that smoke-test repository directory. The SQLiteCore probe writes only under `Saved\SQLUI\SmokeTests\SQLiteCoreProbe` and removes `SQLiteCoreProbe.db` after the check. The SQLite migration probe writes only under `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe` and removes `SQLiteMigrationProbe.db` after the check. The SQLite layout schema migration probe writes only under `Saved\SQLUI\SmokeTests\LayoutSchemaMigrationProbe` and removes `LayoutSchemaMigrationProbe.db` after the check. The SQLite layout read probe writes only under `Saved\SQLUI\SmokeTests\LayoutReadProbe` and removes `LayoutReadProbe.db` after the check. The SQLite read-only layout repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteReadOnlyRepository` and removes `SQLiteReadOnlyRepository.db` after the check. The SQLite SaveLayout repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteSaveLayoutRepository` and removes `SQLiteSaveLayoutRepository.db` after the check. The SQLite RemoveLayout repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteRemoveLayoutRepository` and removes `SQLiteRemoveLayoutRepository.db` after the check. The SQLite ClearLayouts repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteClearLayoutsRepository` and removes `SQLiteClearLayoutsRepository.db` after the check. The SQLite full lifecycle repository smoke path writes only under `Saved\SQLUI\SmokeTests\SQLiteFullLifecycleRepository` and removes `SQLiteFullLifecycleRepository.db` after the check. The database async probe does not perform file I/O.

## Build JerryRiggedEditor

Build the editor target before running the commandlet so the SQLUISamples module and commandlet are available.

From the repo root:

```powershell
$EngineRoot = "C:\Program Files\Epic Games\UE_5.7"
$ProjectPath = (Resolve-Path .\JerryRigged.uproject).Path
& "$EngineRoot\Engine\Build\BatchFiles\Build.bat" JerryRiggedEditor Win64 Development "-Project=$ProjectPath" -WaitMutex -NoHotReloadFromIDE
```

If your engine is installed somewhere else, change `$EngineRoot`.

## Run The Default Smoke Test

From the repo root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"
```

The script derives `UnrealEditor-Cmd.exe` from `-EngineRoot`:

```text
<EngineRoot>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
```

If you need to point at a project file outside the default repo layout, pass `-ProjectPath`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -ProjectPath "D:\Projects\JerryRigged\JerryRigged.uproject"
```

If you already know the exact editor commandlet executable path, pass `-UnrealEditorCmdPath` instead:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -UnrealEditorCmdPath "C:\UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
```

## Run The JSON Fixture Smoke Test

The JSON fixture path keeps the same transient commandlet flow, but enables the built-in SQLUISamples JSON layout fixture with `-UseJsonLayoutFixture`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseJsonLayoutFixture
```

The commandlet also accepts `-JsonLayoutFixture` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

## Run The In-Memory Repository Smoke Test

The in-memory repository path keeps the same transient commandlet flow, deserializes the built-in SQLUISamples JSON layout fixture, selects `InMemory` through the repository factory, saves it into `USQLUIInMemoryLayoutRepository`, verifies list-after-save, loads it back by `LayoutId`, checks remove/list-after-remove/clear repository operations, and runs the same runtime widget pipeline with the loaded layout document:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseInMemoryLayoutRepository
```

The commandlet also accepts `-InMemoryLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is still sample scaffolding only. It does not use SQLite, disk persistence, file I/O, maps, Content, or viewport attachment.

## Run The JSON File Repository Smoke Test

The JSON file repository path keeps the same transient commandlet flow, deserializes the built-in SQLUISamples JSON layout fixture, selects `JsonFile` through the repository factory, saves it into `USQLUIJsonFileLayoutRepository`, verifies list-after-save, loads it back by `LayoutId`, checks remove/list-after-remove/clear repository operations, and runs the same runtime widget pipeline with the loaded layout document:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseJsonFileLayoutRepository
```

The commandlet also accepts `-JsonFileLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is still sample scaffolding only. It does not use SQLite, Content, maps, or viewport attachment. It writes temporary layout files under `Saved\SQLUI\SmokeTests\Layouts`, removes the saved fixture layout after the load step, and verifies `ClearLayouts` removes the same number of remaining layouts reported by the post-remove list.

## Run The SQLiteCore Probe

The SQLiteCore probe keeps the same transient commandlet flow, opens a temporary database under `Saved\SQLUI\SmokeTests\SQLiteCoreProbe`, runs a safe SQLiteCore integrity check, closes the database, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteCoreProbe
```

The commandlet also accepts `-SQLiteCoreProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a backend availability proof only. It does not add a SQLite layout repository, repository factory selection, schema tables, migrations, async database work, Content, maps, or persistent database files.

## Run The Database Async Probe

The database async probe keeps the same transient commandlet flow, enqueues a plain SQLUICore database-boundary request onto a background task, marshals the result back to the game thread, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseDatabaseAsyncProbe
```

The commandlet also accepts `-DatabaseAsyncProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is an async-boundary proof only. It does not open SQLite, create database files, run SQL, add migrations, implement a SQLite layout repository, change repository factory selection, modify Content, or edit maps.

## Run The SQLite Migration Probe

The SQLite migration probe keeps the same transient commandlet flow, opens a temporary database under `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe`, creates a smoke-only `sqlui_schema_migrations` table, applies a tiny probe migration, records and verifies that migration, closes the database, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteMigrationProbe
```

The commandlet also accepts `-SQLiteMigrationProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a migration-runner proof only. The probe migration is clearly separate from the planned layout schema. It does not implement SQLite layout persistence, add real layout schema migrations, change repository factory selection, modify Content, edit maps, or leave persistent database files behind.

## Run The SQLite Layout Schema Migration Probe

The SQLite layout schema migration probe keeps the same transient commandlet flow, applies the planned initial layout schema to a temporary database under `Saved\SQLUI\SmokeTests\LayoutSchemaMigrationProbe`, verifies expected tables and indexes, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteLayoutSchemaMigrationProbe
```

The commandlet also accepts `-SQLiteLayoutSchemaMigrationProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a layout-schema migration proof only. It does not implement SQLite layout persistence, add repository factory selection, modify Content, edit maps, or leave persistent database files behind.

## Run The SQLite Layout Read Probe

The SQLite layout read probe keeps the same transient commandlet flow, applies the planned initial layout schema to a temporary database under `Saved\SQLUI\SmokeTests\LayoutReadProbe`, seeds one probe-only layout document, verifies list-style metadata and current-revision load queries, deserializes and validates the loaded document JSON, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteLayoutReadProbe
```

The commandlet also accepts `-SQLiteLayoutReadProbe` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a read/list/load mapping proof only. It does not implement SQLite layout repository persistence, repository callbacks, repository factory selection, `SaveLayout`, `RemoveLayout`, `ClearLayouts`, Content changes, map edits, or persistent database files.

## Run The SQLite Read-Only Layout Repository Smoke Test

The SQLite read-only layout repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteReadOnlyRepository`, instantiates `USQLUISQLiteLayoutRepository` directly against that database, verifies `ListLayouts` metadata and tags, verifies `LoadLayout` deserializes and validates the current document JSON, verifies `SaveLayout`, `RemoveLayout`, and `ClearLayouts` are rejected as read-only operations, verifies the seeded layout remains readable afterward, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteReadOnlyLayoutRepository
```

The commandlet also accepts `-SQLiteReadOnlyLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a read-only repository proof only. It does not add `ESQLUILayoutRepositoryBackend::SQLite`, repository factory selection, writable SQLite operations, Content changes, map edits, or persistent database files. Writable `SaveLayout`, `RemoveLayout`, and `ClearLayouts` behavior are covered by separate optional smoke paths.

## Run The SQLite SaveLayout Repository Smoke Test

The SQLite SaveLayout repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteSaveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, saves one probe-only layout, verifies `ListLayouts` metadata and tags, verifies `LoadLayout` deserializes and validates the saved document JSON, saves the same layout id again with updated metadata, verifies the latest revision is loaded, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteSaveLayoutRepository
```

The commandlet also accepts `-SQLiteSaveLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a SaveLayout repository proof only. It does not add `ESQLUILayoutRepositoryBackend::SQLite`, repository factory selection, async SQLite workers, Content changes, map edits, or persistent database files. Soft-delete `RemoveLayout` and destructive `ClearLayouts` behavior are covered by separate optional smoke paths.

## Run The SQLite RemoveLayout Repository Smoke Test

The SQLite RemoveLayout repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteRemoveLayoutRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, saves one probe-only layout, verifies it is listed and loadable, soft-deletes it, verifies the removed layout is absent from list/load results while revision history remains preserved, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteRemoveLayoutRepository
```

The commandlet also accepts `-SQLiteRemoveLayoutRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a RemoveLayout repository proof only. It does not add `ESQLUILayoutRepositoryBackend::SQLite`, repository factory selection, async SQLite workers, Content changes, map edits, or persistent database files. Destructive `ClearLayouts` behavior is covered by a separate optional smoke path.

## Run The SQLite ClearLayouts Repository Smoke Test

The SQLite ClearLayouts repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteClearLayoutsRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, saves two probe-only layouts, verifies both are listed, soft-deletes one layout, verifies only the active layout remains listed, clears the repository scope, verifies active and soft-deleted layout rows plus dependent schema rows are gone, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteClearLayoutsRepository
```

The commandlet also accepts `-SQLiteClearLayoutsRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a ClearLayouts repository proof only. It does not add `ESQLUILayoutRepositoryBackend::SQLite`, repository factory selection, async SQLite workers, Content changes, map edits, or persistent database files.

## Run The SQLite Full Lifecycle Repository Smoke Test

The SQLite full lifecycle repository path keeps the same transient commandlet flow, prepares a temporary database under `Saved\SQLUI\SmokeTests\SQLiteFullLifecycleRepository`, instantiates `USQLUISQLiteLayoutRepository` directly with `bReadOnly = false`, saves one probe-only layout, verifies list/load behavior, saves that layout again as revision 2 with updated metadata and tags, saves a second layout, soft-deletes the first layout, verifies the second layout remains readable, verifies the first layout's revision history remains preserved, clears the repository scope, verifies schema rows are empty, removes the probe database file, and then runs the same default runtime widget pipeline:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteFullLifecycleRepository
```

The commandlet also accepts `-SQLiteFullLifecycleRepository` directly as an alias when invoking `UnrealEditor-Cmd.exe`.

This path is a combined currently-supported lifecycle proof only. It does not add `ESQLUILayoutRepositoryBackend::SQLite`, repository factory selection, async SQLite workers, Content changes, map edits, or persistent database files.

## Expected Results

The script prints the exact command it runs, then returns the same exit code as the commandlet. It passes `-DDC-AllowNoActiveStores` so this transient smoke-test commandlet does not require a writable local Derived Data Cache.

On success, the commandlet exits with code `0`. Look for log lines like:

```text
SQLUI sample smoke test commandlet started.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
SQLUI sample smoke test step '<StepName>': Succeeded
```

For the JSON fixture smoke test, also look for:

```text
SQLUI sample smoke test JSON fixture selected: true
SQLUI sample smoke test JSON fixture parse succeeded.
SQLUI sample smoke test JSON fixture validation succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

For the in-memory repository smoke test, also look for:

```text
SQLUI sample smoke test JSON fixture selected: true
SQLUI sample smoke test JSON fixture parse succeeded.
SQLUI sample smoke test JSON fixture validation succeeded.
SQLUI sample smoke test layout repository factory unavailable backend succeeded. SaveBackendUnavailable=true LoadBackendUnavailable=true
SQLUI sample smoke test in-memory layout repository selected.
SQLUI sample smoke test in-memory layout repository save succeeded.
SQLUI sample smoke test in-memory layout repository load succeeded.
SQLUI sample smoke test in-memory layout repository list after save succeeded. LayoutCount=1 MetadataFound=true
SQLUI sample smoke test in-memory layout repository remove succeeded. Removed=true
SQLUI sample smoke test in-memory layout repository list after remove succeeded. LayoutCount=0 MetadataAbsent=true
SQLUI sample smoke test in-memory layout repository clear succeeded. RemovedCount=0 ExpectedRemovedCount=0
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

For the JSON file repository smoke test, also look for:

```text
SQLUI sample smoke test JSON fixture selected: true
SQLUI sample smoke test JSON fixture parse succeeded.
SQLUI sample smoke test JSON fixture validation succeeded.
SQLUI sample smoke test layout repository factory unavailable backend succeeded. SaveBackendUnavailable=true LoadBackendUnavailable=true
SQLUI sample smoke test JSON file layout repository selected.
SQLUI sample smoke test JSON file layout repository save succeeded.
SQLUI sample smoke test JSON file layout repository load succeeded.
SQLUI sample smoke test JSON file layout repository list after save succeeded. LayoutCount=1 MetadataFound=true
SQLUI sample smoke test JSON file layout repository remove succeeded. Removed=true
SQLUI sample smoke test JSON file layout repository list after remove succeeded. LayoutCount=0 MetadataAbsent=true
SQLUI sample smoke test JSON file layout repository clear succeeded. RemovedCount=0 ExpectedRemovedCount=0
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

If the JSON file repository smoke directory already contains valid layout files, the listed counts can be higher; the clear step still expects `RemovedCount` to match the post-remove list count.

For the SQLiteCore probe, also look for:

```text
SQLUI SQLiteCore probe selected: true
SQLUI SQLiteCore probe database opened: true
SQLUI SQLiteCore probe database closed: true
SQLUI SQLiteCore probe database removed: true
SQLUI SQLiteCore probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, `Saved\SQLUI\SmokeTests\SQLiteCoreProbe\SQLiteCoreProbe.db` should not exist.

For the database async probe, also look for:

```text
SQLUI database async probe selected: true
SQLUI database async probe background work completed: true
SQLUI database async probe callback delivered: true
SQLUI database async probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

For the SQLite migration probe, also look for:

```text
SQLUI SQLite migration probe selected: true
SQLUI SQLite migration probe database opened: true
SQLUI SQLite migration probe migration table created: true
SQLUI SQLite migration probe migration applied: true
SQLUI SQLite migration probe migration recorded: true
SQLUI SQLite migration probe database closed: true
SQLUI SQLite migration probe database removed: true
SQLUI SQLite migration probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe\SQLiteMigrationProbe.db` should not exist.

For the SQLite layout schema migration probe, also look for:

```text
SQLUI SQLite layout schema migration probe selected: true
SQLUI SQLite layout schema migration probe migration succeeded: true
SQLUI SQLite layout schema migration probe layouts table exists: true
SQLUI SQLite layout schema migration probe layout revisions table exists: true
SQLUI SQLite layout schema migration probe layout tags table exists: true
SQLUI SQLite layout schema migration probe layout checkpoints table exists: true
SQLUI SQLite layout schema migration probe layout previews table exists: true
SQLUI SQLite layout schema migration probe expected indexes exist: true
SQLUI SQLite layout schema migration probe database removed: true
SQLUI SQLite layout schema migration probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, `Saved\SQLUI\SmokeTests\LayoutSchemaMigrationProbe\LayoutSchemaMigrationProbe.db` should not exist.

For the SQLite layout read probe, also look for:

```text
SQLUI SQLite layout read probe selected: true
SQLUI SQLite layout read probe schema migration succeeded: true
SQLUI SQLite layout read probe seed inserted: true
SQLUI SQLite layout read probe list succeeded: true
SQLUI SQLite layout read probe listed metadata found: true
SQLUI SQLite layout read probe load succeeded: true
SQLUI SQLite layout read probe loaded document valid: true
SQLUI SQLite layout read probe database removed: true
SQLUI SQLite layout read probe succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the probe succeeds, `Saved\SQLUI\SmokeTests\LayoutReadProbe\LayoutReadProbe.db` should not exist.

For the SQLite read-only layout repository smoke test, also look for:

```text
SQLUI SQLite read-only layout repository selected: true
SQLUI SQLite read-only layout repository prepared database: true
SQLUI SQLite read-only layout repository list succeeded: true
SQLUI SQLite read-only layout repository listed metadata found: true
SQLUI SQLite read-only layout repository load succeeded: true
SQLUI SQLite read-only layout repository loaded document valid: true
SQLUI SQLite read-only layout repository save rejected: true
SQLUI SQLite read-only layout repository remove rejected: true
SQLUI SQLite read-only layout repository clear rejected: true
SQLUI SQLite read-only layout repository list after rejected writes succeeded: true
SQLUI SQLite read-only layout repository load after rejected writes succeeded: true
SQLUI SQLite read-only layout repository database removed: true
SQLUI SQLite read-only layout repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteReadOnlyRepository\SQLiteReadOnlyRepository.db` should not exist.

For the SQLite SaveLayout repository smoke test, also look for:

```text
SQLUI SQLite SaveLayout repository selected: true
SQLUI SQLite SaveLayout repository database prepared: true
SQLUI SQLite SaveLayout repository first save succeeded: true
SQLUI SQLite SaveLayout repository list after save succeeded: true
SQLUI SQLite SaveLayout repository listed saved metadata found: true
SQLUI SQLite SaveLayout repository load after save succeeded: true
SQLUI SQLite SaveLayout repository loaded document valid: true
SQLUI SQLite SaveLayout repository second save succeeded: true
SQLUI SQLite SaveLayout repository list after second save succeeded: true
SQLUI SQLite SaveLayout repository load after second save succeeded: true
SQLUI SQLite SaveLayout repository latest revision loaded: true
SQLUI SQLite SaveLayout repository database removed: true
SQLUI SQLite SaveLayout repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteSaveLayoutRepository\SQLiteSaveLayoutRepository.db` should not exist.

For the SQLite RemoveLayout repository smoke test, also look for:

```text
SQLUI SQLite RemoveLayout repository selected: true
SQLUI SQLite RemoveLayout repository database prepared: true
SQLUI SQLite RemoveLayout repository save succeeded: true
SQLUI SQLite RemoveLayout repository list before remove succeeded: true
SQLUI SQLite RemoveLayout repository listed metadata found before remove: true
SQLUI SQLite RemoveLayout repository load before remove succeeded: true
SQLUI SQLite RemoveLayout repository remove succeeded: true
SQLUI SQLite RemoveLayout repository removed: true
SQLUI SQLite RemoveLayout repository list after remove succeeded: true
SQLUI SQLite RemoveLayout repository metadata absent after remove: true
SQLUI SQLite RemoveLayout repository load after remove failed as expected: true
SQLUI SQLite RemoveLayout repository revisions preserved: true
SQLUI SQLite RemoveLayout repository database removed: true
SQLUI SQLite RemoveLayout repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteRemoveLayoutRepository\SQLiteRemoveLayoutRepository.db` should not exist.

For the SQLite ClearLayouts repository smoke test, also look for:

```text
SQLUI SQLite ClearLayouts repository selected: true
SQLUI SQLite ClearLayouts repository database prepared: true
SQLUI SQLite ClearLayouts repository first save succeeded: true
SQLUI SQLite ClearLayouts repository second save succeeded: true
SQLUI SQLite ClearLayouts repository list before remove succeeded: true
SQLUI SQLite ClearLayouts repository both metadata entries found before remove: true
SQLUI SQLite ClearLayouts repository remove succeeded: true
SQLUI SQLite ClearLayouts repository removed: true
SQLUI SQLite ClearLayouts repository list before clear succeeded: true
SQLUI SQLite ClearLayouts repository active metadata preserved before clear: true
SQLUI SQLite ClearLayouts repository removed metadata absent before clear: true
SQLUI SQLite ClearLayouts repository clear succeeded: true
SQLUI SQLite ClearLayouts repository removed count matched expected: true
SQLUI SQLite ClearLayouts repository list after clear succeeded: true
SQLUI SQLite ClearLayouts repository empty after clear: true
SQLUI SQLite ClearLayouts repository loads after clear failed as expected: true
SQLUI SQLite ClearLayouts repository tables empty after clear: true
SQLUI SQLite ClearLayouts repository database removed: true
SQLUI SQLite ClearLayouts repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteClearLayoutsRepository\SQLiteClearLayoutsRepository.db` should not exist.

For the SQLite full lifecycle repository smoke test, also look for:

```text
SQLUI SQLite full lifecycle repository selected: true
SQLUI SQLite full lifecycle repository database prepared: true
SQLUI SQLite full lifecycle repository first save succeeded: true
SQLUI SQLite full lifecycle repository list after first save succeeded: true
SQLUI SQLite full lifecycle repository first metadata found after first save: true
SQLUI SQLite full lifecycle repository load after first save succeeded: true
SQLUI SQLite full lifecycle repository first revision loaded: true
SQLUI SQLite full lifecycle repository second revision save succeeded: true
SQLUI SQLite full lifecycle repository latest revision loaded: true
SQLUI SQLite full lifecycle repository second layout save succeeded: true
SQLUI SQLite full lifecycle repository list before remove succeeded: true
SQLUI SQLite full lifecycle repository both metadata entries found before remove: true
SQLUI SQLite full lifecycle repository remove succeeded: true
SQLUI SQLite full lifecycle repository removed: true
SQLUI SQLite full lifecycle repository list after remove succeeded: true
SQLUI SQLite full lifecycle repository removed metadata absent after remove: true
SQLUI SQLite full lifecycle repository second metadata preserved after remove: true
SQLUI SQLite full lifecycle repository removed load failed as expected: true
SQLUI SQLite full lifecycle repository second load after remove succeeded: true
SQLUI SQLite full lifecycle repository revision history preserved after remove: true
SQLUI SQLite full lifecycle repository clear succeeded: true
SQLUI SQLite full lifecycle repository removed count matched expected: true
SQLUI SQLite full lifecycle repository list after clear succeeded: true
SQLUI SQLite full lifecycle repository empty after clear: true
SQLUI SQLite full lifecycle repository loads after clear failed as expected: true
SQLUI SQLite full lifecycle repository tables empty after clear: true
SQLUI SQLite full lifecycle repository database removed: true
SQLUI SQLite full lifecycle repository succeeded.
SQLUI sample smoke test commandlet succeeded.
SQLUI sample smoke test root widget valid: true
SQLUI sample smoke test created widget count: 1
```

After the smoke test succeeds, `Saved\SQLUI\SmokeTests\SQLiteFullLifecycleRepository\SQLiteFullLifecycleRepository.db` should not exist.

Some optional pipeline steps may log `Skipped` depending on the current sample request. Failures are logged with `SQLUI sample smoke test commandlet failed.` and the script returns a non-zero exit code.

Useful logs appear in the command window. Unreal may also write local generated logs under `Saved\Logs`, such as `Saved\Logs\JerryRigged.log`.

## Common Local Issues

- Quote paths that contain spaces, especially `C:\Program Files\Epic Games\...`.
- If PowerShell blocks the script, run it with `-ExecutionPolicy Bypass` as shown above.
- If `UnrealEditor-Cmd.exe` is not found, check `-EngineRoot` or pass `-UnrealEditorCmdPath`.
- If the commandlet cannot be found, build `JerryRiggedEditor` first.
- Visual Studio, MSVC, Windows SDK, or Unreal toolchain prompts can appear on first build. Install the C++ desktop workload and the engine-required Windows SDK if prompted.
- Windows firewall prompts can appear the first time Unreal tools run locally. The smoke test itself does not require network access.
