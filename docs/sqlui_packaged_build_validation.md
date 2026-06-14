# SQLUI Packaged Build Validation

This document describes the local packaged-build validation scaffold for JerryRigged and SQLUI SQLite readiness checks.

For the concise SQLite phase status and roadmap, see [`sqlui_sqlite_phase_status_roadmap.md`](sqlui_sqlite_phase_status_roadmap.md). For the future persistence/settings UX policy that should sit above the packaged persistence workflow, see [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md). For the production config target strategy gate before real Apply writes, see [`sqlui_persistence_config_target_strategy.md`](sqlui_persistence_config_target_strategy.md). For the planned mutating settings editing/reset phase, see [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md). For the focused read-only UMG binding recipe, see [`sqlui_persistence_status_umg_usage.md`](sqlui_persistence_status_umg_usage.md). For the validation-only draft settings UMG binding recipe, see [`sqlui_persistence_settings_draft_umg_usage.md`](sqlui_persistence_settings_draft_umg_usage.md). For the dry-run apply-preview UMG binding recipe, see [`sqlui_persistence_settings_apply_preview_umg_usage.md`](sqlui_persistence_settings_apply_preview_umg_usage.md). For the apply/cancel contract UMG binding recipe, see [`sqlui_persistence_settings_apply_contract_umg_usage.md`](sqlui_persistence_settings_apply_contract_umg_usage.md). For the apply-result UMG binding recipe, see [`sqlui_persistence_settings_apply_result_umg_usage.md`](sqlui_persistence_settings_apply_result_umg_usage.md).

## Purpose

`Scripts/RunSQLUIPackagedBuildValidation.ps1` runs Unreal AutomationTool `BuildCookRun` for the JerryRigged project. The goal is to verify that the project can build, cook, stage, package, and archive locally with the SQLUI plugin and engine `SQLiteCore` wiring enabled.

When `-RunPackagedSQLiteSmoke` is passed, the script also launches the packaged executable with `-SQLUIPackagedRuntimeSQLiteSmoke` and verifies the runtime log contains the packaged SQLite lifecycle success line.

When `-RunPackagedProviderStartupSmoke` is passed, the script launches the packaged executable with `-SQLUIRuntimeProviderStartupSmoke` plus explicit command-line repository settings. That smoke proves packaged startup/runtime code can intentionally create `USQLUILayoutRepositoryRuntimeProvider`, initialize it through command-line config, use the active repository through the repository contract for save/load, verify SQLite list readback in smoke-only code, then reset the provider and remove the smoke database.

When `-RunPackagedProviderSubsystemSmoke` is passed, the script launches the packaged executable with `-SQLUIRuntimeProviderSubsystemSmoke`, `-SQLUILayoutRepositoryProviderAutoInit`, and explicit SQLite repository command-line settings. That smoke proves the passive SQLUICore `UGameInstanceSubsystem` can act as the app-level provider holder, opt into command-line initialization, use the active repository through the repository contract, reset the provider, and remove the smoke database.

When `-RunPackagedPersistenceWorkflowSmoke` is passed, the script launches the packaged executable three times with `-SQLUIRuntimePersistenceWorkflowSmoke`: Save, Verify, then Cleanup. The Save phase uses `USQLUILayoutRepositoryRuntimeSubsystem` plus `FSQLUILayoutPersistenceWorkflow` to save/list/load one layout and intentionally leaves the SQLite database on disk. The Verify phase starts a separate packaged process, list/loads the same persisted layout without saving first, and leaves the database on disk. The Cleanup phase starts one more packaged process and removes the database plus SQLite sidecar files.

SQLUICore also has config-backed runtime repository settings, but they remain default-off. Packaged provider subsystem smoke intentionally uses explicit command-line auto-init instead of requiring a committed runtime config file.

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
- Run packaged runtime persistence workflow smoke unless `-RunPackagedPersistenceWorkflowSmoke` is explicitly passed.
- Add maps, Content assets, packaged outputs, database files, or generated files to source control.
- Change normal game startup behavior unless an explicit packaged smoke flag such as `-SQLUIPackagedRuntimeSQLiteSmoke`, `-SQLUIRuntimeProviderStartupSmoke`, `-SQLUIRuntimeProviderSubsystemSmoke`, or `-SQLUIRuntimePersistenceWorkflowSmoke` is present.

The read-only persistence status UMG foundation, docs-only settings editing/reset planning, validation-only persistence settings draft model, dry-run apply-intent preview, non-mutating apply/cancel contract, default unavailable/non-mutating apply behavior, UI-safe draft validation display rows, UI-safe apply-preview display rows, UI-safe apply/cancel contract display rows, UI-safe apply-result display rows, SQLUISamples draft validation/apply-preview/apply-contract/apply-result sample adapters, optional validation/apply-preview/apply-contract/apply-result C++ UMG widget shells, docs-only draft/apply-preview/apply-contract/apply-result UMG usage guides including #116, #123, and #130, #111 docs-only draft validation foundation checkpoint, #117 docs-only apply-preview UI foundation checkpoint, the #124 docs-only apply/cancel contract UI foundation checkpoint, the #125 docs-only actual apply implementation gate, the docs-only #105-#131 apply entrypoint/result UI foundation checkpoint, the #132 explicit smoke-owned apply config target scaffold, the #134 apply config target policy/resolver skeleton, docs-only checkpoint updates for that scaffold, and the production config target strategy/resolution/design/descriptor gate do not add packaged startup behavior. Docs-only checkpoint/planning/usage/strategy/design PRs and validation/preview/contract/result-display-only PRs do not require a full packaged build when they do not touch maps, config, startup wiring, provider lifecycle, viewport attachment, or packaged lifecycle behavior. The #132 scaffold writes only an explicit temporary `Saved/SQLUI/SmokeTests` ini target during editor commandlet smoke coverage and cleans it up; the production target descriptor represents `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`; and the guarded backend-only production apply path can write only `Backend=<value>` to that target during explicit smoke coverage, then cleans up the probe-created artifact. None of those changes packaged startup/config/provider behavior by themselves. Future PRs that expand selected-target writes beyond backend-only, add runtime config loading, change startup behavior, or wire persistence status UI or mutating settings/reset UI into startup, default maps, config, viewport attachment, provider lifecycle, or packaged runtime flows should include packaged validation appropriate to that behavior.

The production config target resolution, PR #139 guarded enablement request, descriptor checkpoint, and backend-only apply-write slice are in that same non-packaged category as long as they stay editor-smoke-only and do not change startup/default-map/provider lifecycle behavior. They document that SQLUICore can represent the selected target, use `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, and write only the backend value when the guarded backend-only request is explicitly made. They do not write SQLite paths, provider auto-init, migration/seed/reset/delete settings, create databases, initialize providers, or change packaged startup, default maps, runtime config loading, provider lifecycle, or packaged DB path behavior. Future broader config-write work or startup/default-map/provider lifecycle behavior changes should run packaged validation and preserve the MSVC toolchain guidance below.

The #143 backend-only write did not require packaged validation by itself because it did not make packaged startup consume `RuntimeSettings.ini`, did not wire UI into startup/maps/config, did not change provider lifecycle, and did not change packaging behavior. Future PRs that make packaged/runtime startup read or apply `RuntimeSettings.ini`, enable provider auto-init from that file, switch backends at startup, wire settings UI into maps or viewport startup, or persist SQLite path/provider-auto-init values should run packaged validation and keep the MSVC toolchain caveat below visible.

Actual apply/save/config-write implementations should treat packaged validation as required when they can affect startup, config-backed provider lifecycle, default maps, viewport wiring, packaged runtime settings, or persisted runtime behavior. If a slice remains SQLUICore-only, explicit, editor-smoke-owned, and has no startup/provider lifecycle changes, focused editor smoke coverage is usually sufficient.

Run this validation before treating SQLite as packaged-runtime-ready, but do not treat a successful package alone as proof of packaged SQLite lifecycle behavior. Use `-RunPackagedSQLiteSmoke` for the packaged runtime SQLite repository lifecycle proof, `-RunPackagedProviderStartupSmoke` for the direct provider startup proof, `-RunPackagedProviderSubsystemSmoke` for the passive subsystem startup proof, and `-RunPackagedPersistenceWorkflowSmoke` for the first packaged persistence-across-launches workflow proof.

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

The default packaged runtime persistence workflow smoke log directory is:

```text
Saved/SQLUI/PackagedValidation/Win64/Development/RuntimeSmoke/PersistenceWorkflow
```

That directory contains separate phase logs:

```text
SQLUIPackagedRuntimePersistenceWorkflowSave.log
SQLUIPackagedRuntimePersistenceWorkflowVerify.log
SQLUIPackagedRuntimePersistenceWorkflowCleanup.log
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

To build/package and then run the packaged runtime persistence workflow smoke across three separate packaged launches:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput -RunPackagedPersistenceWorkflowSmoke
```

To run all packaged runtime smoke paths after one BuildCookRun:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput -RunPackagedSQLiteSmoke -RunPackagedProviderStartupSmoke -RunPackagedProviderSubsystemSmoke -RunPackagedPersistenceWorkflowSmoke
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
- When `-RunPackagedPersistenceWorkflowSmoke` is passed, locates the packaged executable, launches Save, Verify, and Cleanup phases with `-SQLUIRuntimePersistenceWorkflowSmoke`, verifies each phase log, and checks that Cleanup removed the resolved database plus SQLite sidecar files.
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
- `-RunPackagedPersistenceWorkflowSmoke`: launches the packaged executable after BuildCookRun succeeds for Save, Verify, and Cleanup persistence workflow phases.
- `-PackagedPersistenceWorkflowSmokeTimeoutSeconds`: overrides each packaged runtime persistence workflow phase timeout. The default is `120`.
- `-PackagedPersistenceWorkflowSmokeLogDirectory`: overrides the packaged runtime persistence workflow phase log directory. The resolved directory must stay under `Saved/SQLUI/PackagedValidation`.

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

With `-RunPackagedPersistenceWorkflowSmoke`, failure can also mean the packaged executable could not be found, one phase timed out, one phase returned a non-zero exit code, a phase log was missing, a phase log contained `SQLUI packaged runtime persistence workflow smoke failed:`, a phase success line was absent, Cleanup failed, or the resolved persistence database/sidecar files still existed after Cleanup. Cleanup is run even when Verify fails so local state is removed before the script reports the failure.

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

The subsystem is passive when config-backed auto-init is off and `-SQLUILayoutRepositoryProviderAutoInit` is absent. It does not select SQLite, create databases, copy seed files, attach widgets, or touch the viewport by default.

The provider subsystem smoke waits briefly for the packaged runtime `GameInstance`, finds `USQLUILayoutRepositoryRuntimeSubsystem`, verifies explicit auto-initialization selected SQLite, saves and loads one probe layout through the base repository callback methods, verifies SQLite metadata/tags listing in smoke-only code, resets the subsystem/provider, removes the smoke database and sidecars, and requests process exit.

Success requires the packaged process to exit cleanly and the runtime log to contain:

```text
SQLUI packaged runtime provider subsystem smoke succeeded.
```

## Packaged Runtime Persistence Workflow Smoke

`-RunPackagedPersistenceWorkflowSmoke` proves that the storage-agnostic runtime persistence workflow can persist one SQLite-backed layout across separate packaged process launches. It runs only when the packaged executable is launched with:

```text
-SQLUIRuntimePersistenceWorkflowSmoke
```

The script runs three explicit phases:

```text
-SQLUIRuntimePersistenceWorkflowSmokePhase=Save
-SQLUIRuntimePersistenceWorkflowSmokePhase=Verify
-SQLUIRuntimePersistenceWorkflowSmokePhase=Cleanup
```

Each phase also uses explicit subsystem auto-init and repository command-line settings:

```text
-SQLUILayoutRepositoryProviderAutoInit
-SQLUILayoutRepositoryBackend=SQLite
-SQLUISQLiteLayoutRepositoryPath="PackagedRuntimeSmoke/PersistenceWorkflow/PersistenceWorkflow.db"
-SQLUISQLiteLayoutRepositoryInitializeSchema
-SQLUISQLiteLayoutRepositoryCreateDatabase
```

The relative SQLite path is resolved by `FSQLUILayoutRepositoryRuntimeConfigResolver` under the packaged runtime saved directory:

```text
<ProjectSavedDir>/SQLUI/LayoutRepositories/PackagedRuntimeSmoke/PersistenceWorkflow/PersistenceWorkflow.db
```

The Save phase deletes any existing workflow smoke database before starting, waits for `USQLUILayoutRepositoryRuntimeSubsystem`, verifies explicit SQLite auto-init, saves one probe layout through `FSQLUILayoutPersistenceWorkflow`, lists and loads it through the same workflow helper, resets the provider/subsystem, and verifies the database remains on disk.

The Verify phase starts a separate packaged process, waits for the same subsystem auto-init path, lists and loads the persisted layout through `FSQLUILayoutPersistenceWorkflow` without saving first, resets the provider/subsystem, and verifies the database still remains on disk.

The Cleanup phase starts a final packaged process and removes the database plus SQLite sidecar files. The validation script also parses the cleanup log's resolved database path and verifies the database and sidecar files no longer exist. Cleanup runs even when Verify fails, and the script still returns a non-zero exit code for the failing Save/Verify/Cleanup phase.

Success requires all three packaged phase processes to exit cleanly and their logs to contain phase-specific success lines:

```text
SQLUI packaged runtime persistence workflow smoke Save phase succeeded.
SQLUI packaged runtime persistence workflow smoke Verify phase succeeded.
SQLUI packaged runtime persistence workflow smoke Cleanup phase succeeded.
```

The phase logs also include:

```text
SQLUI packaged runtime persistence workflow smoke selected phase: Save
SQLUI packaged runtime persistence workflow smoke workflow save succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow list succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow listed metadata found: true
SQLUI packaged runtime persistence workflow smoke workflow load succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow loaded document valid: true
SQLUI packaged runtime persistence workflow smoke provider reset: true
SQLUI packaged runtime persistence workflow smoke database exists after phase: true

SQLUI packaged runtime persistence workflow smoke selected phase: Verify
SQLUI packaged runtime persistence workflow smoke workflow list succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow listed persisted metadata found: true
SQLUI packaged runtime persistence workflow smoke workflow load succeeded: true
SQLUI packaged runtime persistence workflow smoke workflow loaded persisted document valid: true
SQLUI packaged runtime persistence workflow smoke provider reset: true
SQLUI packaged runtime persistence workflow smoke database exists after phase: true

SQLUI packaged runtime persistence workflow smoke selected phase: Cleanup
SQLUI packaged runtime persistence workflow smoke database removed: true
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

This records local BuildCookRun package compatibility for the installed UE 5.7 toolchain and Win64 Development validation path. Package build validation without `-RunPackagedSQLiteSmoke`, `-RunPackagedProviderStartupSmoke`, `-RunPackagedProviderSubsystemSmoke`, or `-RunPackagedPersistenceWorkflowSmoke` still does not prove packaged runtime SQLite lifecycle, direct provider startup, subsystem provider startup, or persistence-across-launches execution inside the packaged executable.

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

If a later local run selects MSVC `14.38.x` or a Visual Studio 2026 Preview toolchain and fails with the same unresolved `__std_*` symbols, treat that as a local toolchain selection issue unless the UBT log points to SQLUI or SQLiteCore objects.

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
- More packaged persistence-across-launches scenarios beyond the first explicit workflow smoke.
- Implementing the user-facing persistence settings/DB path UX documented in [`sqlui_persistence_settings_ux_design.md`](sqlui_persistence_settings_ux_design.md), gated by [`sqlui_persistence_config_target_strategy.md`](sqlui_persistence_config_target_strategy.md), and planned in [`sqlui_persistence_settings_editing_reset_plan.md`](sqlui_persistence_settings_editing_reset_plan.md), plus product startup policy beyond the safe config-backed settings object, passive provider subsystem, explicit smoke-owned apply config target scaffold, and explicit packaged smoke flags.
- Packaged validation for any future real runtime/user config-write Apply behavior that changes startup/config/provider lifecycle semantics. The current apply config target scaffold, target policy/resolver skeleton, production config target strategy resolution/descriptor, guarded enablement request, and backend-only selected-target write are smoke-owned, non-mutating, policy-only, or explicitly backend-only commandlet-smoke work that do not require packaged validation by themselves because they do not change startup, default maps, provider lifecycle, runtime config loading, or packaged DB path behavior.
- Production async database service, queue, cancellation, and shutdown hardening.
- Migration upgrade/versioning validation beyond the initial schema.
