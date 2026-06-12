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

Result: exit code `0`; the persistence settings draft probe succeeded. The probe confirmed draft validation, dry-run apply preview, apply/cancel contract display rows, apply-result display rows, the apply-result sample adapter, the apply-result C++ UMG widget shell contract, and optional sample surfaces remain non-mutating.

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

Result: exit code `0`; the Unreal Editor executable opened the project and exited cleanly. This confirmed that the project and loaded SQLUI/SQLUISamples modules were sufficient for the editor process to start, mount plugins, load the project, execute the quit command, and shut down.

This editor launch is intentionally narrow. It validates that the current project/editor/plugin state can open under UE 5.7 in an unattended local checkpoint, but it is not a manual UI review, PIE test, packaged runtime test, map validation, viewport validation, widget Blueprint subclass test, settings apply/reset flow test, or packaged SQLite lifecycle test.

## Confirmed Boundaries

- SQLUICore was not modified by this checkpoint.
- SQLUICore did not gain UMG, Slate, SlateCore, or editor-only dependencies.
- No default config enables SQLite.
- No default config enables provider auto-init.
- `InMemory` remains the default backend.
- The sample/status display paths remain optional and dev-facing.
- The sample/status display paths are not wired into startup, maps, viewport attachment, or default config.
- No maps were saved.
- No assets were saved.
- No widget Blueprint assets were created or committed.
- No generated editor/session/build files were committed.
- No backend selector UI, SQLite path editor UI, provider auto-init control, settings apply/save/config-write behavior, reset/delete UX, polling, ticking, timer, or auto-refresh behavior was added.
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

## Future Validation Guidance

This checkpoint is stronger than build/commandlet-only validation because the full Unreal Editor executable launched and exited successfully. It is still not a replacement for future interactive editor, PIE, or packaged runtime validation when later PRs add real UI assets or runtime wiring.

Future slices should add stronger validation when they introduce stronger behavior:

- Interactive editor/manual validation when widget Blueprint assets, visible UI controls, or real settings screens are introduced.
- Manual editor inspection from clean `main` before the first real config-write/apply PR merges, if practical. That inspection should not commit assets, maps, config, widget Blueprints, generated logs, database files, or packaged outputs; local throwaway widget Blueprints are acceptable only when they are not committed.
- The smoke-owned apply config target scaffold is covered by the commandlet draft probe only. It is not a real editor Apply flow and does not replace future manual editor validation for user/runtime config writes.
- PR #133 is a docs-only checkpoint update for the smoke-owned apply config target scaffold. It does not claim additional manual editor, PIE, or packaged validation; it only records that #132 remains smoke-owned and non-production.
- PR #134's apply config target policy/resolver skeleton is covered by the same commandlet draft probe only. It makes default/runtime, smoke-owned, and future project/user target decisions explicit, but it does not add a manual editor flow, a real Apply button, runtime settings application, production/user config writes, or provider/database lifecycle work.
- PIE validation when runtime UI behavior is wired into maps, viewport flow, startup, provider lifecycle, or game execution.
- Packaged validation when startup behavior, default maps, config wiring, provider lifecycle, packaged runtime behavior, or actual config writes are introduced.
- Reset/delete validation when reset UX or destructive maintenance actions are introduced.

The local packaging/toolchain caveat remains separate from SQLUI persistence correctness. Packaged-build validation depends on Unreal selecting a compatible Visual Studio/MSVC toolchain; known local toolchain issues are documented in [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md).

## Packaged Validation

Packaged validation was not rerun for this checkpoint because startup behavior, packaging settings, runtime code, maps, Content assets, and default config were not changed.

Packaged-build validation remains documented separately in [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md).
