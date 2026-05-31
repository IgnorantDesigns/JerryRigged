# SQLUI Packaged Build Validation

This document describes the local packaged-build validation scaffold for JerryRigged and SQLUI SQLite readiness checks.

## Purpose

`Scripts/RunSQLUIPackagedBuildValidation.ps1` runs Unreal AutomationTool `BuildCookRun` for the JerryRigged project. The goal is to verify that the project can build, cook, stage, package, and archive locally with the SQLUI plugin and engine `SQLiteCore` wiring enabled.

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
- Add packaged runtime smoke automation.
- Launch the packaged executable.
- Prove SQLite repository lifecycle behavior inside a packaged runtime.
- Verify packaged runtime database paths under `Saved/SQLUI`.
- Add maps, Content assets, packaged outputs, database files, or generated files to source control.
- Change SQLUI runtime behavior.

Run this validation before treating SQLite as packaged-runtime-ready, but do not treat a successful package alone as proof of packaged SQLite lifecycle behavior.

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
- Returns the AutomationTool exit code.
- Refuses `-CleanPackageOutput` outside `Saved/SQLUI/PackagedValidation`.

Optional switches:

- `-NoBuild`: omits `-build`; use only when build outputs already exist.
- `-NoCook`: omits `-cook`; use only when cooked content already exists.
- `-SkipPackage`: omits `-pak`, `-package`, `-archive`, and `-archivedirectory`; the command still stages.
- `-CleanPackageOutput`: removes only resolved validation stage/archive directories before running.

## Interpreting Results

Success means AutomationTool returned exit code `0` for the requested BuildCookRun command. That proves the selected local machine, engine install, project checkout, SQLUI modules, and SQLiteCore wiring survived the requested package path.

Failure means AutomationTool returned a non-zero exit code. Check the command output and Unreal logs for the first meaningful build, cook, staging, package, plugin, or dependency error. Do not treat script failure as a SQLUI runtime failure unless the logs point to SQLUI or SQLiteCore specifically.

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

That evidence indicated a local Visual Studio/MSVC toolchain or runtime-library mismatch in this run, not a SQLUI SQLite repository runtime failure. Do not treat this symbol pattern as a SQLUI or SQLiteCore bug unless the UBT log points to SQLUI or SQLiteCore objects.

Recommended local follow-up:

1. Install or select the UE 5.7 preferred MSVC toolchain reported by UBT for the machine.
2. Ensure the matching MSVC libraries and Windows SDK components are installed through Visual Studio Installer.
3. Clean generated package/build outputs only through normal Unreal workflows or targeted generated folders; do not commit generated outputs.
4. Re-run `RunSQLUIPackagedBuildValidation.ps1`.

## Remaining Work

Packaged-build validation is only one step toward packaged SQLite readiness.

Future work still includes:

- Packaged runtime SQLite lifecycle smoke automation.
- Target-platform coverage beyond local Win64 Development.
- CI automation if Unreal-capable build agents become available.
- Packaged runtime database path verification under `Saved/SQLUI`.
- Production async database service, queue, cancellation, and shutdown hardening.
- Migration upgrade/versioning validation beyond the initial schema.
