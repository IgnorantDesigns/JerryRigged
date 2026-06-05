# SQLUI Packaged Build Validation

This document describes the local packaged-build validation scaffold for JerryRigged and SQLUI SQLite readiness checks.

For the concise SQLite phase status and roadmap, see [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md).

## Purpose

`Scripts/RunSQLUIPackagedBuildValidation.ps1` runs Unreal AutomationTool `BuildCookRun` for the JerryRigged project. The goal is to verify that the project can build, cook, stage, package, and archive locally with the SQLUI plugin and engine `SQLiteCore` wiring enabled.

When `-RunPackagedSQLiteSmoke` is passed, the script also launches the packaged executable with `-SQLUIPackagedRuntimeSQLiteSmoke` and verifies the runtime log contains the packaged SQLite lifecycle success line.

When `-RunPackagedProviderStartupSmoke` is passed, the script launches the packaged executable with `-SQLUIRuntimeProviderStartupSmoke` plus explicit command-line repository settings. That smoke proves packaged startup/runtime code can intentionally create `USQLUILayoutRepositoryRuntimeProvider`, initialize it through command-line config, use the active repository through the repository contract for save/load, verify SQLite list readback in smoke-only code, then reset the provider and remove the smoke database.

When `-RunPackagedProviderSubsystemSmoke` is passed, the script launches the packaged executable with `-SQLUIRuntimeProviderSubsystemSmoke`, `-SQLUILayoutRepositoryProviderAutoInit`, and explicit SQLite repository command-line settings. That smoke proves the passive SQLUICore `UGameInstanceSubsystem` can act as the app-level provider holder, opt into command-line initialization, use the active repository through the repository contract, reset the provider, and remove the smoke database.

This validation is intended to catch packaging and dependency issues around:

- JerryRigged as the host project.
- The SQLUI plugin modules.
- SQLUICore and SQLUISamples compile/package compatibility.
- Engine `SQLiteCore` plugin/module wiring.
- Current local project packaging assumptions.

## Scope

This is local validation only. It is not CI, and it does not assume an Unreal build agent exists on GitHub Actions or any other hosted runner.

This scaffold does not:

- Add GitHub Actions or CI.
- Launch the packaged executable by default.
- Run packaged runtime SQLite lifecycle smoke unless `-RunPackagedSQLiteSmoke` is explicitly passed.
- Run packaged runtime provider startup smoke unless `-RunPackagedProviderStartupSmoke` is explicitly passed.
- Run packaged runtime provider subsystem smoke unless `-RunPackagedProviderSubsystemSmoke` is explicitly passed.
- Add maps, Content assets, packaged outputs, database files, or generated files to source control.
- Change normal game startup behavior unless an explicit packaged smoke flag such as `-SQLUIPackagedRuntimeSQLiteSmoke`, `-SQLUIRuntimeProviderStartupSmoke`, or `-SQLUIRuntimeProviderSubsystemSmoke` is present.

Run this validation before treating SQLite as packaged-runtime-ready, but do not treat a successful package alone as proof of packaged SQLite lifecycle behavior. Use `-RunPackagedSQLiteSmoke` for the packaged runtime SQLite repository lifecycle proof, `-RunPackagedProviderStartupSmoke` for the direct provider startup proof, and `-RunPackagedProviderSubsystemSmoke` for the passive subsystem startup proof.

## Output Location

By default, the script writes stage and archive output under:

```text
Saved/SQLUI/PackagedValidation/<Platform>/<Configuration>/
```

The default `Win64` / `Development` paths are:

```text
Saved/SQLUI/PackagedValidation/Win64/Development/Stage
Saved/SQLUI/PackagedValidation/Win64/Development/Archive
```

The default packaged runtime smoke log path is:

```text
Saved/SQLUI/PackagedValidation/Win64/Development/RuntimeSmoke/SQLUIPackagedRuntimeSQLiteSmoke.log
```

The default packaged runtime provider startup smoke log path is:

```text
Saved/SQLUI/PackagedValidation/Win64/Development/RuntimeSmoke/SQLUIPackagedRuntimeProviderStartupSmoke.log
```

The default packaged runtime provider subsystem smoke log path is:

```text
Saved/SQLUI/PackagedValidation/Win64/Development/RuntimeSmoke/SQLUIPackagedRuntimeProviderSubsystemSmoke.log
```

Unreal may still write normal local build artifacts such as logs, build products, Derived Data Cache, or intermediate files according to standard Unreal packaging behavior. Those outputs are generated files and should not be committed.

## Run Validation

From the repo root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"
```

To remove only the packaged-validation stage/archive output before running:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput
```

To build/package and then run the packaged SQLite lifecycle smoke:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput -RunPackagedSQLiteSmoke
```

To build/package and then run the packaged runtime provider startup smoke:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput -RunPackagedProviderStartupSmoke
```

To build/package and then run the packaged runtime provider subsystem smoke:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput -RunPackagedProviderSubsystemSmoke
```

To run all packaged runtime smoke paths after one BuildCookRun:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput -RunPackagedSQLiteSmoke -RunPackagedProviderStartupSmoke -RunPackagedProviderSubsystemSmoke
```

To use a direct AutomationTool path:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -RunUATPath "C:\UE\Engine\Build\BatchFiles\RunUAT.bat"
```

To override platform, configuration, or output directories:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -Platform Win64 -Configuration Development -ArchiveDirectory ".\Saved\SQLUI\PackagedValidation\Win64\Development\Archive"
```

Custom archive and stage directories must resolve under `Saved/SQLUI/PackagedValidation`.

Show script help:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -Help
```

## Script Behavior

The script:

- Resolves `RunUAT.bat` from `-EngineRoot` or `-RunUATPath`.
- Resolves `-ProjectPath`, defaulting to `JerryRigged.uproject`.
- Defaults to `Win64` and `Development`.
- Prints the resolved `RunUAT.bat`, project path, platform, configuration, and shell-visible `cl.exe` / `link.exe` versions when those tools are available on `PATH`.
- Prints the exact `RunUAT BuildCookRun` command before running it.
- Runs `BuildCookRun` with `-nop4`, `-utf8output`, `-stage`, `-build`, `-cook`, `-pak`, `-package`, and `-archive` by default.
- Uses `Saved/SQLUI/PackagedValidation/...` for stage and archive output by default.
- Returns the AutomationTool exit code unless packaged runtime smoke is requested and BuildCookRun succeeds.
- Refuses `-CleanPackageOutput` outside `Saved/SQLUI/PackagedValidation`.
- When `-RunPackagedSQLiteSmoke` is passed, locates the packaged executable, launches it with `-SQLUIPackagedRuntimeSQLiteSmoke`, waits for completion, and verifies the runtime smoke log.
- When `-RunPackagedProviderStartupSmoke` is passed, locates the packaged executable, launches it with `-SQLUIRuntimeProviderStartupSmoke` and explicit SQLite repository command-line settings, waits for completion, and verifies the runtime provider startup smoke log.
- When `-RunPackagedProviderSubsystemSmoke` is passed, locates the packaged executable, launches it with `-SQLUIRuntimeProviderSubsystemSmoke`, `-SQLUILayoutRepositoryProviderAutoInit`, and explicit SQLite repository command-line settings, waits for completion, and verifies the runtime provider subsystem smoke log.
- When multiple packaged runtime smoke switches are passed, runs them sequentially with separate logs after the same BuildCookRun.

Optional switches:

- `-NoBuild`: omits `-build`; use only when build outputs already exist.
- `-NoCook`: omits `-cook`; use only when cooked content already exists.
- `-SkipPackage`: omits `-pak`, `-package`, `-archive`, and `-archivedirectory`; the command still stages.
- `-CleanPackageOutput`: removes only resolved validation stage/archive directories before running.
- `-RunPackagedSQLiteSmoke`: launches the packaged executable after BuildCookRun succeeds and verifies the packaged runtime SQLite lifecycle log.
- `-PackagedSmokeTimeoutSeconds`: overrides the packaged runtime smoke process timeout. The default is `120`.
- `-PackagedSmokeLogPath`: overrides the packaged runtime smoke log path. The resolved path must stay under `Saved/SQLUI/PackagedValidation`.
- `-RunPackagedProviderStartupSmoke`: launches the packaged executable after BuildCookRun succeeds and verifies the packaged runtime provider startup log.
- `-PackagedProviderStartupSmokeTimeoutSeconds`: overrides the packaged runtime provider startup smoke process timeout. The default is `120`.
- `-PackagedProviderStartupSmokeLogPath`: overrides the packaged runtime provider startup smoke log path. The resolved path must stay under `Saved/SQLUI/PackagedValidation`.
- `-RunPackagedProviderSubsystemSmoke`: launches the packaged executable after BuildCookRun succeeds and verifies the packaged runtime provider subsystem log.
- `-PackagedProviderSubsystemSmokeTimeoutSeconds`: overrides the packaged runtime provider subsystem smoke process timeout. The default is `120`.
- `-PackagedProviderSubsystemSmokeLogPath`: overrides the packaged runtime provider subsystem smoke log path. The resolved path must stay under `Saved/SQLUI/PackagedValidation`.

## Interpreting Results

Success means AutomationTool returned exit code `0` for the requested BuildCookRun command. That proves the selected local machine, engine install, project checkout, SQLUI modules, and SQLiteCore wiring survived the requested package path.

When `-RunPackagedSQLiteSmoke` is enabled, success also requires the packaged process to exit cleanly and the runtime smoke log to contain:

```text
SQLUI packaged runtime SQLite smoke succeeded.
```

Failure means AutomationTool returned a non-zero exit code. Check the command output and Unreal logs for the first meaningful build, cook, staging, package, plugin, or dependency error. Do not treat script failure as a SQLUI runtime failure unless the logs point to SQLUI or SQLiteCore specifically.

With `-RunPackagedSQLiteSmoke`, failure can also mean the packaged executable could not be found, timed out, returned a non-zero exit code, failed to write the smoke log, logged `SQLUI packaged runtime SQLite smoke failed:`, or never logged the success line.

With `-RunPackagedProviderStartupSmoke`, failure can also mean the packaged executable could not be found, timed out, returned a non-zero exit code, failed to write the provider startup smoke log, logged `SQLUI packaged runtime provider startup smoke failed:`, or never logged the provider startup success line.

With `-RunPackagedProviderSubsystemSmoke`, failure can also mean the packaged executable could not be found, timed out, returned a non-zero exit code, failed to write the provider subsystem smoke log, logged `SQLUI packaged runtime provider subsystem smoke failed:`, or never logged the provider subsystem success line.

## Packaged Runtime SQLite Smoke

`-RunPackagedSQLiteSmoke` is the first packaged runtime SQLUI SQLite lifecycle proof. It runs only when the packaged executable is launched with:

```text
-SQLUIPackagedRuntimeSQLiteSmoke
```

The flag is inspected by the runtime SQLUISamples module. Normal startup is unchanged when the flag is absent.

The packaged runtime smoke resolves a database under the packaged runtime project saved directory:

```text
<ProjectSavedDir>/SQLUI/PackagedRuntimeSmoke/SQLiteLifecycle/SQLUIPackagedRuntimeSQLiteLifecycle.db
```

It deletes any existing smoke database and SQLite sidecar files, creates a SQLite repository through `USQLUILayoutRepositoryFactory`, enables schema initialization and database creation through repository settings, then verifies:

- `SaveLayout`
- `ListLayouts`
- `LoadLayout`
- `RemoveLayout`
- `ClearLayouts`
- database and sidecar cleanup

The packaged runtime smoke requests process exit when finished. Success is recorded by both a clean packaged process exit and the runtime log success line.

## Packaged Runtime Provider Startup Smoke

`-RunPackagedProviderStartupSmoke` proves the intended explicit startup integration shape for `USQLUILayoutRepositoryRuntimeProvider`. It runs only when the packaged executable is launched with:

```text
-SQLUIRuntimeProviderStartupSmoke
```

The script also passes explicit repository command-line settings:

```text
-SQLUILayoutRepositoryBackend=SQLite
-SQLUISQLiteLayoutRepositoryPath="PackagedRuntimeSmoke/RuntimeProviderStartup/RuntimeProviderStartup.db"
-SQLUISQLiteLayoutRepositoryInitializeSchema
-SQLUISQLiteLayoutRepositoryCreateDatabase
```

The relative SQLite path is resolved by `FSQLUILayoutRepositoryRuntimeConfigResolver` under the packaged runtime saved directory:

```text
<ProjectSavedDir>/SQLUI/LayoutRepositories/PackagedRuntimeSmoke/RuntimeProviderStartup/RuntimeProviderStartup.db
```

The runtime flag is inspected by the SQLUISamples module after engine-loop initialization. Normal startup is unchanged when the flag is absent.

The provider startup smoke creates `USQLUILayoutRepositoryRuntimeProvider`, initializes it from the packaged command line, verifies the active backend is SQLite, saves and loads one probe layout through the base repository callback methods, verifies SQLite metadata/tags listing in smoke-only code, resets the provider, removes the smoke database and sidecars, and requests process exit.

Success requires the packaged process to exit cleanly and the runtime log to contain:

```text
SQLUI packaged runtime provider startup smoke succeeded.
```

## Packaged Runtime Provider Subsystem Smoke

`-RunPackagedProviderSubsystemSmoke` proves the intended passive subsystem startup surface for `USQLUILayoutRepositoryRuntimeSubsystem`. It runs only when the packaged executable is launched with:

```text
-SQLUIRuntimeProviderSubsystemSmoke
```

The script also passes explicit auto-init and repository command-line settings:

```text
-SQLUILayoutRepositoryProviderAutoInit
-SQLUILayoutRepositoryBackend=SQLite
-SQLUISQLiteLayoutRepositoryPath="PackagedRuntimeSmoke/RuntimeProviderSubsystem/RuntimeProviderSubsystem.db"
-SQLUISQLiteLayoutRepositoryInitializeSchema
-SQLUISQLiteLayoutRepositoryCreateDatabase
```

The relative SQLite path is resolved by `FSQLUILayoutRepositoryRuntimeConfigResolver` under the packaged runtime saved directory:

```text
<ProjectSavedDir>/SQLUI/LayoutRepositories/PackagedRuntimeSmoke/RuntimeProviderSubsystem/RuntimeProviderSubsystem.db
```

The subsystem is passive when `-SQLUILayoutRepositoryProviderAutoInit` is absent. It does not select SQLite, create databases, copy seed files, attach widgets, or touch the viewport by default.

The provider subsystem smoke waits briefly for the packaged runtime `GameInstance`, finds `USQLUILayoutRepositoryRuntimeSubsystem`, verifies explicit auto-initialization selected SQLite, saves and loads one probe layout through the base repository callback methods, verifies SQLite metadata/tags listing in smoke-only code, resets the subsystem/provider, removes the smoke database and sidecars, and requests process exit.

Success requires the packaged process to exit cleanly and the runtime log to contain:

```text
SQLUI packaged runtime provider subsystem smoke succeeded.
```

## Latest Local Result

The earlier unresolved `__std_*` linker failure was observed when UBT selected non-preferred Visual Studio 2022 MSVC toolchains for the local UE 5.7 package run.

The local machine now has these Visual Studio 2022 MSVC toolsets installed:

```text
14.29.30133
14.38.33130
14.44.35207
```

Installing the UE 5.7-preferred Visual Studio 2022 MSVC `14.44.x` toolchain resolved the local packaged validation failure. The latest local validation command was:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput
```

That run completed build, cook, stage, package, and archive, then exited with:

```text
AutomationTool exited with ExitCode=0.
SQLUI packaged-build validation exit code: 0.
```

This records local BuildCookRun package compatibility for the installed UE 5.7 toolchain and Win64 Development validation path. Package build validation without `-RunPackagedSQLiteSmoke`, `-RunPackagedProviderStartupSmoke`, or `-RunPackagedProviderSubsystemSmoke` still does not prove packaged runtime SQLite lifecycle, direct provider startup, or subsystem provider startup execution inside the packaged executable.

## Troubleshooting Local Linker Failures

AutomationTool and UnrealBuildTool logs are the source of truth for the selected compiler, linker, Windows SDK, and preferred-toolchain warnings. The script prints shell-visible `cl.exe` and `link.exe` versions when they are available on `PATH`, but UBT may still select a different Visual Studio toolchain.

Useful log locations:

```text
%APPDATA%\Unreal Engine\AutomationTool\Logs\<SanitizedEnginePath>\Log.txt
%APPDATA%\Unreal Engine\AutomationTool\Logs\<SanitizedEnginePath>\UBA-JerryRiggedEditor-Win64-Development.txt
```

For the local UE 5.7 install path used by these docs, the sanitized log directory is typically:

```text
%APPDATA%\Unreal Engine\AutomationTool\Logs\C+Program+Files+Epic+Games+UE_5.7
```

Link response files can also be useful when a link command fails:

```text
Intermediate/Build/Win64/x64/JerryRigged/Development/JerryRigged.exe.rsp
```

In the first local packaged validation run for this scaffold, UAT reached the native `JerryRigged.exe` link and failed with unresolved MSVC STL helper symbols such as:

```text
__std_search_1
__std_search_2
__std_find_first_of_trivial_pos_1
__std_mismatch_4
__std_minmax_element_d
__std_minmax_element_f
__std_max_element_f
```

The same UBT log showed:

- `JerryRigged` and `JerryRiggedEditor` using Visual Studio 2022 `14.38.33141` from `VC\Tools\MSVC\14.38.33130`.
- Windows SDK `10.0.22621.0`.
- `Warning: Visual Studio 2022 compiler is not a preferred version`.
- The failing link command using that toolchain's `link.exe` with `JerryRigged.exe.rsp`.
- The first unresolved references coming from Unreal/engine or engine-plugin objects such as `Module.LiveCoding.cpp.obj`, `Module.TraceAnalysis.cpp.obj`, `Module.GeometryAlgorithms.*.cpp.obj`, `reverb_onset_compensator.cc.obj`, and `reverb_node.cc.obj`.

That evidence indicated a local Visual Studio/MSVC toolchain or runtime-library mismatch in that run, not a SQLUI SQLite repository runtime failure. Installing the UE 5.7-preferred Visual Studio 2022 MSVC `14.44.x` toolchain resolved the local failure, and the latest local packaged validation passed with UAT exit code `0`.

These troubleshooting notes remain useful for future toolchain mismatch cases. Do not treat this symbol pattern as a SQLUI or SQLiteCore bug unless the UBT log points to SQLUI or SQLiteCore objects.

Recommended local follow-up if unresolved `__std_*` linker symbols appear again:

1. Install or select the UE 5.7 preferred MSVC toolchain reported by UBT for the machine.
2. Ensure the matching MSVC libraries and Windows SDK components are installed through Visual Studio Installer.
3. Clean generated package/build outputs only through normal Unreal workflows or targeted generated folders; do not commit generated outputs.
4. Re-run `RunSQLUIPackagedBuildValidation.ps1`.

## Remaining Work

Packaged-build validation is only one step toward packaged SQLite readiness.

Future work still includes:

- Target-platform coverage beyond local Win64 Development.
- CI automation if Unreal-capable build agents become available.
- Broader packaged runtime database path coverage beyond the first `Saved/SQLUI/PackagedRuntimeSmoke` lifecycle proof.
- User-facing runtime settings and product startup policy beyond the passive provider subsystem and explicit packaged smoke flags.
- Production async database service, queue, cancellation, and shutdown hardening.
- Migration upgrade/versioning validation beyond the initial schema.
