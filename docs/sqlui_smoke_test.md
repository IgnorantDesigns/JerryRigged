# SQLUI Smoke Test

The SQLUI sample smoke test commandlet runs the current sample runtime widget pipeline in a transient commandlet world. It creates the sample widget catalog, variable store, runtime context, and layout request, then reports whether the root widget and pipeline steps succeeded.

By default, the smoke test uses the existing in-memory C++ layout document. It can also run the built-in SQLUISamples JSON layout fixture, which deserializes a tiny `SQLUI.FilterBox` layout through SQLUICore's layout JSON helpers before running the same runtime widget pipeline.

The in-memory repository smoke path reuses that same JSON fixture, selects the in-memory backend through the SQLUICore layout repository factory, saves the deserialized layout into `USQLUIInMemoryLayoutRepository`, verifies `ListLayouts` includes the saved metadata, loads it back by layout id, verifies `RemoveLayout` removes it, verifies `ListLayouts` no longer includes it, exercises `ClearLayouts`, and then runs the runtime widget pipeline with the loaded document.

The JSON file repository smoke path also reuses the JSON fixture, selects the JSON-file backend through the SQLUICore layout repository factory, saves the deserialized layout into `USQLUIJsonFileLayoutRepository`, verifies `ListLayouts` includes the saved metadata, loads it back by layout id, verifies `RemoveLayout` removes it, verifies `ListLayouts` no longer includes it, exercises `ClearLayouts`, and then runs the runtime widget pipeline with the loaded document.

Both repository smoke paths also select the unavailable backend through the factory and verify load/save report `bBackendUnavailable` cleanly. This keeps unavailable persistence explicit without adding SQLite.

The optional SQLiteCore probe opens and closes a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteCoreProbe`, then removes the probe database file. This proves engine `SQLiteCore` can use a runtime-writable SQLUI smoke-test path without adding a layout repository, migrations, schema tables, async database workers, or repository factory behavior.

The optional database async probe runs a tiny SQLUICore-owned database-boundary request on a background thread and delivers the result back through the game-thread callback path. It does not open SQLite, run SQL, write database files, add migrations, or change repository selection.

The optional SQLite migration probe opens a temporary SQLite database under `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe`, creates the smoke-only migration tracking table, applies and records one probe migration, verifies the migration row, closes the database, and removes the probe database file. This is not the planned SQLUI layout schema migration and does not add a SQLite layout repository.

This is a local developer workflow only. It is not CI yet, and it does not assume Unreal Engine is installed on GitHub Actions or any build agent.

The smoke test does not edit maps, levels, Content, persistent database files, or the viewport. It does not add SQLite layout repository behavior or attach widgets to the viewport. The JSON file repository smoke path writes only under `Saved\SQLUI\SmokeTests\Layouts`, removes its saved layout after loading it, and clears remaining layouts in that smoke-test repository directory. The SQLiteCore probe writes only under `Saved\SQLUI\SmokeTests\SQLiteCoreProbe` and removes `SQLiteCoreProbe.db` after the check. The SQLite migration probe writes only under `Saved\SQLUI\SmokeTests\SQLiteMigrationProbe` and removes `SQLiteMigrationProbe.db` after the check. The database async probe does not perform file I/O.

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

Some optional pipeline steps may log `Skipped` depending on the current sample request. Failures are logged with `SQLUI sample smoke test commandlet failed.` and the script returns a non-zero exit code.

Useful logs appear in the command window. Unreal may also write local generated logs under `Saved\Logs`, such as `Saved\Logs\JerryRigged.log`.

## Common Local Issues

- Quote paths that contain spaces, especially `C:\Program Files\Epic Games\...`.
- If PowerShell blocks the script, run it with `-ExecutionPolicy Bypass` as shown above.
- If `UnrealEditor-Cmd.exe` is not found, check `-EngineRoot` or pass `-UnrealEditorCmdPath`.
- If the commandlet cannot be found, build `JerryRiggedEditor` first.
- Visual Studio, MSVC, Windows SDK, or Unreal toolchain prompts can appear on first build. Install the C++ desktop workload and the engine-required Windows SDK if prompted.
- Windows firewall prompts can appear the first time Unreal tools run locally. The smoke test itself does not require network access.
