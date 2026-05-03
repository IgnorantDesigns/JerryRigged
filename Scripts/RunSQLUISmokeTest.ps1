[CmdletBinding()]
param(
	[string]$EngineRoot,

	[string]$UnrealEditorCmdPath,

	[string]$ProjectPath,

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

  -Help, -?
      Show this help.

Examples:
  .\Scripts\RunSQLUISmokeTest.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"
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
	'-FullStdOutLogOutput'
)

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
