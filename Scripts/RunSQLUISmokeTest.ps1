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

	[switch]$UseSQLiteMigrationProbe,

	[switch]$UseSQLiteLayoutSchemaMigrationProbe,

	[switch]$UseSQLiteLayoutReadProbe,

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

  -UseSQLiteMigrationProbe
      Run the optional SQLUICore SQLite migration-runner probe.

  -UseSQLiteLayoutSchemaMigrationProbe
      Run the optional SQLUICore SQLite layout schema migration probe.

  -UseSQLiteLayoutReadProbe
      Run the optional SQLUICore SQLite layout read probe.

  -Help, -?
      Show this help.

Examples:
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseJsonLayoutFixture
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseInMemoryLayoutRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseJsonFileLayoutRepository
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteCoreProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseDatabaseAsyncProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteMigrationProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteLayoutSchemaMigrationProbe
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -UseSQLiteLayoutReadProbe
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
