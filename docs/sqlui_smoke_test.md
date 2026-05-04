# SQLUI Smoke Test

The SQLUI sample smoke test commandlet runs the current sample runtime widget pipeline in a transient commandlet world. It creates the sample widget catalog, variable store, runtime context, and layout request, then reports whether the root widget and pipeline steps succeeded.

By default, the smoke test uses the existing in-memory C++ layout document. It can also run the built-in SQLUISamples JSON layout fixture, which deserializes a tiny `SQLUI.FilterBox` layout through SQLUICore's layout JSON helpers before running the same runtime widget pipeline.

This is a local developer workflow only. It is not CI yet, and it does not assume Unreal Engine is installed on GitHub Actions or any build agent.

The smoke test does not edit maps, levels, Content, SQLite databases, or the viewport. It does not add database behavior or attach widgets to the viewport.

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

## Expected Results

The script prints the exact command it runs, then returns the same exit code as the commandlet.

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

Some optional pipeline steps may log `Skipped` depending on the current sample request. Failures are logged with `SQLUI sample smoke test commandlet failed.` and the script returns a non-zero exit code.

Useful logs appear in the command window. Unreal may also write local generated logs under `Saved\Logs`, such as `Saved\Logs\JerryRigged.log`.

## Common Local Issues

- Quote paths that contain spaces, especially `C:\Program Files\Epic Games\...`.
- If PowerShell blocks the script, run it with `-ExecutionPolicy Bypass` as shown above.
- If `UnrealEditor-Cmd.exe` is not found, check `-EngineRoot` or pass `-UnrealEditorCmdPath`.
- If the commandlet cannot be found, build `JerryRiggedEditor` first.
- Visual Studio, MSVC, Windows SDK, or Unreal toolchain prompts can appear on first build. Install the C++ desktop workload and the engine-required Windows SDK if prompted.
- Windows firewall prompts can appear the first time Unreal tools run locally. The smoke test itself does not require network access.
