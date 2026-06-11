# SQLUI Persistence Editor Validation Checkpoint

This checkpoint records a local Unreal Editor validation pass for the current SQLUI persistence/settings foundation after PR #119 merged to `main`.

It is a documentation checkpoint only. It does not add runtime code, settings controls, apply/save/reset behavior, provider lifecycle behavior, maps, assets, widget Blueprints, config changes, startup wiring, or packaged validation behavior.

## Source State

- Base branch: `main`
- Work branch: `codex/sqlui-persistence-editor-validation-checkpoint`
- Confirmed `origin/main` included PR #119:
  - Merge commit: `33005612e18f9f045c739c641f15508deda0b266`
  - Subject: `Merge pull request #119 from IgnorantDesigns/codex/sqlui-apply-cancel-contract-display-rows`
- Validation date: June 11, 2026
- Engine used: Unreal Engine 5.7 at `C:\Program Files\Epic Games\UE_5.7`

## Validated Commands

```powershell
git status --short --branch --untracked-files=all
```

Result: clean before the docs-only checkpoint edit.

```powershell
git diff --check
git diff --check main...HEAD
```

Result: both passed with no whitespace errors before the docs-only checkpoint edit.

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" JerryRiggedEditor Win64 Development "-Project=C:\Users\Dayton Brown\Documents\Unreal Projects\JerryRigged-fresh\JerryRigged.uproject" -WaitMutex -NoHotReloadFromIDE
```

Result: exit code `0`; target was up to date and the build reported `Result: Succeeded`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -Help
```

Result: exit code `0`; help output printed successfully.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"
```

Result: exit code `0`; the default SQLUI sample smoke commandlet succeeded.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceStatusSampleSurfaceProbe
```

Result: exit code `0`; the persistence status sample surface probe succeeded. The probe confirmed the sample/dev presenter, adapter, and widget-shell reflection path remains read-only and that smoke-owned probe database files were removed.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceSettingsDraftProbe
```

Result: exit code `0`; the persistence settings draft probe succeeded. The probe confirmed draft validation, dry-run apply preview, apply/cancel contract display rows, and optional sample surfaces remain non-mutating.

```powershell
$paths = @(
  "Saved\SQLUI\SmokeTests\PersistenceStatusSampleSurface",
  "Saved\SQLUI\SmokeTests\PersistenceStatusDisplayRows",
  "Saved\SQLUI\SmokeTests\PersistenceStatusSurface",
  "Saved\SQLUI\SmokeTests\PersistenceSettingsDraft"
)
$found = @()
foreach ($path in $paths) {
  if (Test-Path $path) {
    $found += Get-ChildItem -Path $path -File -Recurse -Include *.db,*.db-journal,*.db-wal,*.db-shm | Select-Object -ExpandProperty FullName
  }
}
if ($found.Count -eq 0) {
  Write-Output "No persistence status/settings probe DB or sidecar files found."
} else {
  $found | ForEach-Object { Write-Output $_ }
  exit 1
}
```

Result: exit code `0`; no persistence status/settings probe database or SQLite sidecar files were found.

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\Dayton Brown\Documents\Unreal Projects\JerryRigged-fresh\JerryRigged.uproject" -unattended -nop4 -nosplash -nullrhi -NoSound -ExecCmds="QUIT_EDITOR"
```

Result: exit code `0`; the Unreal Editor executable opened the project and exited cleanly.

This editor launch is intentionally narrow. It validates that the current project/editor/plugin state can open under UE 5.7 in an unattended local checkpoint, but it is not a manual UI review, packaged runtime test, map validation, viewport validation, or packaged SQLite lifecycle test.

## Confirmed Boundaries

- SQLUICore was not modified by this checkpoint.
- SQLUICore did not gain UMG, Slate, SlateCore, or editor-only dependencies.
- No default config enables SQLite.
- No default config enables provider auto-init.
- `InMemory` remains the default backend.
- The sample/status display paths remain optional and dev-facing.
- The sample/status display paths are not wired into startup, maps, viewport attachment, or default config.
- The sample/status display paths remain read-only:
  - no database creation
  - no migrations
  - no seed copying
  - no provider initialization
  - no repository initialization
  - no settings writes
  - no reset/delete behavior
  - no destructive actions
- No maps, Content assets, widget Blueprint assets, packaged outputs, database files, or generated files were intentionally changed.

## Packaged Validation

Packaged validation was not rerun for this checkpoint because startup behavior, packaging settings, runtime code, maps, Content assets, and default config were not changed.

Packaged-build validation remains documented separately in [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md).
