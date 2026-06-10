[CmdletBinding()]
param(
	[string]$EngineRoot,

	[string]$UnrealEditorCmdPath,

	[string]$ProjectPath,

	[switch]$UseJsonLayoutFixture,

	[switch]$UseInMemoryLayoutRepository,

	[switch]$UseJsonFileLayoutRepository,

	[switch]$UseSQLiteCoreProbe,

	[switch]$UseDatabaseAsyncProbe,

	[switch]$UseDatabaseAsyncQueueShutdownProbe,

	[switch]$UseLayoutRepositoryRuntimeConfigProbe,

	[switch]$UseLayoutRepositoryRuntimeIntegrationProbe,

	[switch]$UseLayoutRepositoryRuntimeProviderProbe,

	[switch]$UseLayoutRepositoryRuntimeSettingsProbe,

	[switch]$UseLayoutPersistenceWorkflowProbe,

	[switch]$UseLayoutRepositoryDatabaseManagementProbe,

	[switch]$UsePersistenceStatusSurfaceProbe,

	[switch]$UsePersistenceStatusDisplayRowsProbe,

	[switch]$UsePersistenceStatusSampleSurfaceProbe,

	[switch]$UsePersistenceSettingsDraftProbe,

	[switch]$UseSQLiteMigrationProbe,

	[switch]$UseSQLiteLayoutSchemaMigrationProbe,

	[switch]$UseSQLiteLayoutReadProbe,

	[switch]$UseSQLiteReadOnlyLayoutRepository,

	[switch]$UseSQLiteSaveLayoutRepository,

	[switch]$UseSQLiteRemoveLayoutRepository,

	[switch]$UseSQLiteClearLayoutsRepository,

	[switch]$UseSQLiteFullLifecycleRepository,

	[switch]$UseSQLiteAsyncCallbackRepository,

	[switch]$UseSQLiteSerializedAsyncCallbackRepository,

	[switch]$UseSQLiteFactoryLayoutRepository,

	[switch]$UseSQLiteFactorySchemaInitRepository,

	[switch]$UseSQLiteSchemaInitHardening,

	[switch]$UseSQLiteSeedDatabaseCopyPolicyProbe,

	[switch]$UseSQLiteMigrationVersioningPolicyProbe,

	[Alias('?')]
	[switch]$Help
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Show-SQLUISmokeTestHelp
{
	$HelpText = @'
Run the SQLUI sample smoke test commandlet.

Usage:
  powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"

Parameters:
  -EngineRoot
      Unreal Engine install root. The script derives:
      <EngineRoot>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe

  -UnrealEditorCmdPath
      Optional direct path to UnrealEditor-Cmd.exe. This overrides -EngineRoot.

  -ProjectPath
      Optional path to JerryRigged.uproject. Defaults to ..\JerryRigged.uproject
      relative to this script.

  -UseJsonLayoutFixture
      Run the smoke test with the built-in SQLUISamples JSON layout fixture.

  -UseInMemoryLayoutRepository
      Run the JSON fixture through the SQLUISamples in-memory layout repository
      before running the runtime widget pipeline.

  -UseJsonFileLayoutRepository
      Run the JSON fixture through the SQLUICore JSON file layout repository
      before running the runtime widget pipeline.

  -UseSQLiteCoreProbe
      Run the optional SQLiteCore open/close probe under Saved\SQLUI\SmokeTests.

  -UseDatabaseAsyncProbe
      Run the optional SQLUICore database async-boundary probe.

  -UseDatabaseAsyncQueueShutdownProbe
      Run the optional SQLUICore database async queue shutdown/stale-callback probe.

  -UseLayoutRepositoryRuntimeConfigProbe
      Run the optional SQLUICore layout repository runtime config policy probe.

  -UseLayoutRepositoryRuntimeIntegrationProbe
      Run the optional SQLUICore layout repository runtime integration probe.

  -UseLayoutRepositoryRuntimeProviderProbe
      Run the optional SQLUICore layout repository runtime provider probe.

  -UseLayoutRepositoryRuntimeSettingsProbe
      Run the optional SQLUICore layout repository runtime settings policy probe.

  -UseLayoutPersistenceWorkflowProbe
      Run the optional SQLUICore layout persistence workflow probe.

  -UseLayoutRepositoryDatabaseManagementProbe
      Run the optional SQLUICore layout repository database management policy probe.

  -UsePersistenceStatusSurfaceProbe
      Run the optional SQLUICore read-only persistence status surface probe.

  -UsePersistenceStatusDisplayRowsProbe
      Run the optional SQLUICore read-only persistence status display-row probe.

  -UsePersistenceStatusSampleSurfaceProbe
      Run the optional SQLUISamples read-only persistence status sample surface and
      Blueprint-facing presenter/panel adapter/widget shell hook probe.

  -UsePersistenceSettingsDraftProbe
      Run the optional SQLUICore validation/preview-only persistence settings draft
      and validation display-row probe, including the dry-run apply-intent preview,
      SQLUISamples sample adapter, and C++ UMG widget shell contract.

  -UseSQLiteMigrationProbe
      Run the optional SQLUICore SQLite migration-runner probe.

  -UseSQLiteLayoutSchemaMigrationProbe
      Run the optional SQLUICore SQLite layout schema migration probe.

  -UseSQLiteLayoutReadProbe
      Run the optional SQLUICore SQLite layout read probe.

  -UseSQLiteReadOnlyLayoutRepository
      Run the optional SQLUICore SQLite read-only layout repository smoke path.

  -UseSQLiteSaveLayoutRepository
      Run the optional SQLUICore SQLite SaveLayout repository smoke path.

  -UseSQLiteRemoveLayoutRepository
      Run the optional SQLUICore SQLite RemoveLayout repository smoke path.

  -UseSQLiteClearLayoutsRepository
      Run the optional SQLUICore SQLite ClearLayouts repository smoke path.

  -UseSQLiteFullLifecycleRepository
      Run the optional SQLUICore SQLite full lifecycle repository smoke path.

  -UseSQLiteAsyncCallbackRepository
      Run the optional SQLUICore SQLite async callback repository smoke path.

  -UseSQLiteSerializedAsyncCallbackRepository
      Run the optional SQLUICore SQLite serialized async callback repository smoke path.

  -UseSQLiteFactoryLayoutRepository
      Run the optional SQLUICore SQLite factory layout repository smoke path.

  -UseSQLiteFactorySchemaInitRepository
      Run the optional SQLUICore SQLite factory schema-init repository smoke path.

  -UseSQLiteSchemaInitHardening
      Run the optional SQLUICore SQLite schema-init hardening smoke path.

  -UseSQLiteSeedDatabaseCopyPolicyProbe
      Run the optional SQLUICore SQLite seed database copy policy probe.

  -UseSQLiteMigrationVersioningPolicyProbe
      Run the optional SQLUICore SQLite migration versioning policy probe.

  -Help, -?
      Show this help.

Examples:
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseJsonLayoutFixture
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseInMemoryLayoutRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseJsonFileLayoutRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteCoreProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseDatabaseAsyncProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseDatabaseAsyncQueueShutdownProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryRuntimeConfigProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryRuntimeIntegrationProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryRuntimeProviderProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryRuntimeSettingsProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutPersistenceWorkflowProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseLayoutRepositoryDatabaseManagementProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceStatusSurfaceProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceStatusDisplayRowsProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceStatusSampleSurfaceProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UsePersistenceSettingsDraftProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteMigrationProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteLayoutSchemaMigrationProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteLayoutReadProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteReadOnlyLayoutRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteSaveLayoutRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteRemoveLayoutRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteClearLayoutsRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteFullLifecycleRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteAsyncCallbackRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteSerializedAsyncCallbackRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteFactoryLayoutRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteFactorySchemaInitRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteSchemaInitHardening
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteSeedDatabaseCopyPolicyProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteMigrationVersioningPolicyProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -UnrealEditorCmdPath "C:\UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -ProjectPath ".\JerryRigged.uproject"
'@

	Write-Host $HelpText
}

function Stop-WithUsageError
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Message
	)

	Write-Error $Message
	exit 2
}

function Resolve-ExistingFile
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path,

		[Parameter(Mandatory = $true)]
		[string]$Description
	)

	if ([string]::IsNullOrWhiteSpace($Path))
	{
		Stop-WithUsageError "$Description path is required."
	}

	if (-not (Test-Path -LiteralPath $Path -PathType Leaf))
	{
		Stop-WithUsageError "$Description was not found: $Path"
	}

	return (Resolve-Path -LiteralPath $Path).ProviderPath
}

function Format-PowerShellCommandPart
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Value
	)

	if ($Value -match '^[A-Za-z0-9_./:=+\-\\]+$')
	{
		return $Value
	}

	return "'" + ($Value -replace "'", "''") + "'"
}

if ($Help)
{
	Show-SQLUISmokeTestHelp
	exit 0
}

$ScriptRoot = if (-not [string]::IsNullOrWhiteSpace($PSScriptRoot))
{
	$PSScriptRoot
}
else
{
	Split-Path -Parent $MyInvocation.MyCommand.Path
}

if ([string]::IsNullOrWhiteSpace($ProjectPath))
{
	$ProjectPath = Join-Path -Path $ScriptRoot -ChildPath '..\JerryRigged.uproject'
}

$ResolvedProjectPath = Resolve-ExistingFile -Path $ProjectPath -Description 'Project file'

if ([string]::IsNullOrWhiteSpace($UnrealEditorCmdPath))
{
	if ([string]::IsNullOrWhiteSpace($EngineRoot))
	{
		Stop-WithUsageError 'Pass -EngineRoot or -UnrealEditorCmdPath.'
	}

	$UnrealEditorCmdPath = Join-Path `
		-Path $EngineRoot `
		-ChildPath 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
}

$ResolvedUnrealEditorCmdPath = Resolve-ExistingFile `
	-Path $UnrealEditorCmdPath `
	-Description 'UnrealEditor-Cmd.exe'

$CommandletArgs = @(
	$ResolvedProjectPath,
	'-run=SQLUISampleSmokeTest',
	'-unattended',
	'-nop4',
	'-nosplash',
	'-nullrhi',
	'-stdout',
	'-FullStdOutLogOutput',
	'-DDC-AllowNoActiveStores'
)

if ($UseJsonLayoutFixture)
{
	$CommandletArgs += '-UseJsonLayoutFixture'
}

if ($UseInMemoryLayoutRepository)
{
	$CommandletArgs += '-UseInMemoryLayoutRepository'
}

if ($UseJsonFileLayoutRepository)
{
	$CommandletArgs += '-UseJsonFileLayoutRepository'
}

if ($UseSQLiteCoreProbe)
{
	$CommandletArgs += '-SQLiteCoreProbe'
}

if ($UseDatabaseAsyncProbe)
{
	$CommandletArgs += '-DatabaseAsyncProbe'
}

if ($UseDatabaseAsyncQueueShutdownProbe)
{
	$CommandletArgs += '-DatabaseAsyncQueueShutdownProbe'
}

if ($UseLayoutRepositoryRuntimeConfigProbe)
{
	$CommandletArgs += '-LayoutRepositoryRuntimeConfigProbe'
}

if ($UseLayoutRepositoryRuntimeIntegrationProbe)
{
	$CommandletArgs += '-LayoutRepositoryRuntimeIntegrationProbe'
}

if ($UseLayoutRepositoryRuntimeProviderProbe)
{
	$CommandletArgs += '-LayoutRepositoryRuntimeProviderProbe'
}

if ($UseLayoutRepositoryRuntimeSettingsProbe)
{
	$CommandletArgs += '-LayoutRepositoryRuntimeSettingsProbe'
}

if ($UseLayoutPersistenceWorkflowProbe)
{
	$CommandletArgs += '-LayoutPersistenceWorkflowProbe'
}

if ($UseLayoutRepositoryDatabaseManagementProbe)
{
	$CommandletArgs += '-LayoutRepositoryDatabaseManagementProbe'
}

if ($UsePersistenceStatusSurfaceProbe)
{
	$CommandletArgs += '-PersistenceStatusSurfaceProbe'
}

if ($UsePersistenceStatusDisplayRowsProbe)
{
	$CommandletArgs += '-PersistenceStatusDisplayRowsProbe'
}

if ($UsePersistenceStatusSampleSurfaceProbe)
{
	$CommandletArgs += '-PersistenceStatusSampleSurfaceProbe'
}

if ($UsePersistenceSettingsDraftProbe)
{
	$CommandletArgs += '-PersistenceSettingsDraftProbe'
}

if ($UseSQLiteMigrationProbe)
{
	$CommandletArgs += '-SQLiteMigrationProbe'
}

if ($UseSQLiteLayoutSchemaMigrationProbe)
{
	$CommandletArgs += '-SQLiteLayoutSchemaMigrationProbe'
}

if ($UseSQLiteLayoutReadProbe)
{
	$CommandletArgs += '-SQLiteLayoutReadProbe'
}

if ($UseSQLiteReadOnlyLayoutRepository)
{
	$CommandletArgs += '-SQLiteReadOnlyLayoutRepository'
}

if ($UseSQLiteSaveLayoutRepository)
{
	$CommandletArgs += '-SQLiteSaveLayoutRepository'
}

if ($UseSQLiteRemoveLayoutRepository)
{
	$CommandletArgs += '-SQLiteRemoveLayoutRepository'
}

if ($UseSQLiteClearLayoutsRepository)
{
	$CommandletArgs += '-SQLiteClearLayoutsRepository'
}

if ($UseSQLiteFullLifecycleRepository)
{
	$CommandletArgs += '-SQLiteFullLifecycleRepository'
}

if ($UseSQLiteAsyncCallbackRepository)
{
	$CommandletArgs += '-SQLiteAsyncCallbackRepository'
}

if ($UseSQLiteSerializedAsyncCallbackRepository)
{
	$CommandletArgs += '-SQLiteSerializedAsyncCallbackRepository'
}

if ($UseSQLiteFactoryLayoutRepository)
{
	$CommandletArgs += '-SQLiteFactoryLayoutRepository'
}

if ($UseSQLiteFactorySchemaInitRepository)
{
	$CommandletArgs += '-SQLiteFactorySchemaInitRepository'
}

if ($UseSQLiteSchemaInitHardening)
{
	$CommandletArgs += '-SQLiteSchemaInitHardening'
}

if ($UseSQLiteSeedDatabaseCopyPolicyProbe)
{
	$CommandletArgs += '-SQLiteSeedDatabaseCopyPolicyProbe'
}

if ($UseSQLiteMigrationVersioningPolicyProbe)
{
	$CommandletArgs += '-SQLiteMigrationVersioningPolicyProbe'
}

$PrintableCommandParts = @($ResolvedUnrealEditorCmdPath) + $CommandletArgs
$PrintableCommand = '& ' + (($PrintableCommandParts | ForEach-Object { Format-PowerShellCommandPart -Value $_ }) -join ' ')

Write-Host 'Running SQLUI smoke test commandlet:'
Write-Host $PrintableCommand

& $ResolvedUnrealEditorCmdPath @CommandletArgs
$ExitCode = $LASTEXITCODE

if ($null -eq $ExitCode)
{
	$ExitCode = 0
}

Write-Host "SQLUI smoke test commandlet exit code: $ExitCode"
exit $ExitCode
