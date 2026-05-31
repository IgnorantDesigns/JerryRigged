[CmdletBinding()]
param(
	[string]$EngineRoot,

	[string]$RunUATPath,

	[string]$ProjectPath,

	[string]$Platform = 'Win64',

	[string]$Configuration = 'Development',

	[string]$ArchiveDirectory,

	[string]$StageDirectory,

	[switch]$NoBuild,

	[switch]$NoCook,

	[switch]$SkipPackage,

	[switch]$CleanPackageOutput,

	[Alias('?')]
	[switch]$Help
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Show-SQLUIPackagedBuildValidationHelp
{
	$HelpText = @'
Run a local SQLUI packaged-build validation through Unreal AutomationTool.

Usage:
  powershell -NoProfile -ExecutionPolicy Bypass -File .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"

Parameters:
  -EngineRoot
      Unreal Engine install root. The script derives:
      <EngineRoot>\Engine\Build\BatchFiles\RunUAT.bat

  -RunUATPath
      Optional direct path to RunUAT.bat. This overrides -EngineRoot.

  -ProjectPath
      Optional path to JerryRigged.uproject. Defaults to ..\JerryRigged.uproject
      relative to this script.

  -Platform
      Target platform for BuildCookRun. Defaults to Win64.

  -Configuration
      Client configuration for BuildCookRun. Defaults to Development.

  -ArchiveDirectory
      Optional archive output directory. Defaults to:
      Saved\SQLUI\PackagedValidation\<Platform>\<Configuration>\Archive
      The resolved directory must stay under Saved\SQLUI\PackagedValidation.

  -StageDirectory
      Optional staging directory. Defaults to:
      Saved\SQLUI\PackagedValidation\<Platform>\<Configuration>\Stage
      The resolved directory must stay under Saved\SQLUI\PackagedValidation.

  -NoBuild
      Omit the BuildCookRun -build flag. Use only when build outputs already exist.

  -NoCook
      Omit the BuildCookRun -cook flag. Use only when cooked content already exists.

  -SkipPackage
      Omit -pak, -package, -archive, and -archivedirectory. The command still stages.

  -CleanPackageOutput
      Remove only the resolved archive/stage directories before running. Cleanup is
      refused unless those directories are under Saved\SQLUI\PackagedValidation.

  -Help, -?
      Show this help.

Examples:
  .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7"
  .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput
  .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -RunUATPath "C:\UE\Engine\Build\BatchFiles\RunUAT.bat" -Platform Win64 -Configuration Development
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

function Resolve-FullPath
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path,

		[Parameter(Mandatory = $true)]
		[string]$BasePath
	)

	$PathToResolve = $Path
	if (-not [System.IO.Path]::IsPathRooted($PathToResolve))
	{
		$PathToResolve = Join-Path -Path $BasePath -ChildPath $PathToResolve
	}

	return [System.IO.Path]::GetFullPath($PathToResolve)
}

function Test-IsPathUnder
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path,

		[Parameter(Mandatory = $true)]
		[string]$RootPath
	)

	$FullPath = [System.IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
	$FullRootPath = [System.IO.Path]::GetFullPath($RootPath).TrimEnd('\', '/')

	return $FullPath.Equals($FullRootPath, [System.StringComparison]::OrdinalIgnoreCase) -or
		$FullPath.StartsWith($FullRootPath + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)
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

function Clear-ValidationDirectory
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Directory,

		[Parameter(Mandatory = $true)]
		[string]$AllowedRoot
	)

	if (-not (Test-IsPathUnder -Path $Directory -RootPath $AllowedRoot))
	{
		Stop-WithUsageError "Refusing to clean outside Saved\SQLUI\PackagedValidation: $Directory"
	}

	if (Test-Path -LiteralPath $Directory)
	{
		Write-Host "Removing packaged validation output: $Directory"
		Remove-Item -LiteralPath $Directory -Recurse -Force
	}
}

function Write-DetectedToolVersion
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$ToolName
	)

	$Command = Get-Command $ToolName -ErrorAction SilentlyContinue | Select-Object -First 1
	if ($null -eq $Command)
	{
		Write-Host "SQLUI packaged-build validation preflight $ToolName not found on PATH. UBT will choose the build toolchain."
		return
	}

	Write-Host "SQLUI packaged-build validation preflight $ToolName path: $($Command.Source)"
	$VersionOutput = & $Command.Source 2>&1 | Select-Object -First 4
	foreach ($Line in $VersionOutput)
	{
		Write-Host "  $Line"
	}
}

if ($Help)
{
	Show-SQLUIPackagedBuildValidationHelp
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
$ProjectRoot = Split-Path -Parent $ResolvedProjectPath
$PackagedValidationRoot = [System.IO.Path]::GetFullPath((Join-Path -Path $ProjectRoot -ChildPath 'Saved\SQLUI\PackagedValidation'))
$DefaultOutputRoot = Join-Path -Path $PackagedValidationRoot -ChildPath (Join-Path -Path $Platform -ChildPath $Configuration)

if ([string]::IsNullOrWhiteSpace($ArchiveDirectory))
{
	$ArchiveDirectory = Join-Path -Path $DefaultOutputRoot -ChildPath 'Archive'
}

if ([string]::IsNullOrWhiteSpace($StageDirectory))
{
	$StageDirectory = Join-Path -Path $DefaultOutputRoot -ChildPath 'Stage'
}

$ResolvedArchiveDirectory = Resolve-FullPath -Path $ArchiveDirectory -BasePath $ProjectRoot
$ResolvedStageDirectory = Resolve-FullPath -Path $StageDirectory -BasePath $ProjectRoot

if (-not (Test-IsPathUnder -Path $ResolvedArchiveDirectory -RootPath $PackagedValidationRoot))
{
	Stop-WithUsageError "ArchiveDirectory must be under Saved\SQLUI\PackagedValidation: $ResolvedArchiveDirectory"
}

if (-not (Test-IsPathUnder -Path $ResolvedStageDirectory -RootPath $PackagedValidationRoot))
{
	Stop-WithUsageError "StageDirectory must be under Saved\SQLUI\PackagedValidation: $ResolvedStageDirectory"
}

if ($CleanPackageOutput)
{
	Clear-ValidationDirectory -Directory $ResolvedArchiveDirectory -AllowedRoot $PackagedValidationRoot
	Clear-ValidationDirectory -Directory $ResolvedStageDirectory -AllowedRoot $PackagedValidationRoot
}

New-Item -ItemType Directory -Force -Path $ResolvedArchiveDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $ResolvedStageDirectory | Out-Null

if ([string]::IsNullOrWhiteSpace($RunUATPath))
{
	if ([string]::IsNullOrWhiteSpace($EngineRoot))
	{
		Stop-WithUsageError 'Pass -EngineRoot or -RunUATPath.'
	}

	$RunUATPath = Join-Path `
		-Path $EngineRoot `
		-ChildPath 'Engine\Build\BatchFiles\RunUAT.bat'
}

$ResolvedRunUATPath = Resolve-ExistingFile -Path $RunUATPath -Description 'RunUAT.bat'

Write-Host "SQLUI packaged-build validation RunUAT path: $ResolvedRunUATPath"
Write-Host "SQLUI packaged-build validation project path: $ResolvedProjectPath"
Write-Host "SQLUI packaged-build validation platform: $Platform"
Write-Host "SQLUI packaged-build validation configuration: $Configuration"
Write-Host 'SQLUI packaged-build validation note: UnrealBuildTool logs are the source of truth for the selected compiler, linker, Windows SDK, and preferred-toolchain warnings.'
Write-DetectedToolVersion -ToolName 'cl.exe'
Write-DetectedToolVersion -ToolName 'link.exe'

$BuildCookRunArgs = @(
	'BuildCookRun',
	"-project=$ResolvedProjectPath",
	'-nop4',
	'-utf8output',
	"-platform=$Platform",
	"-clientconfig=$Configuration",
	'-stage',
	"-stagingdirectory=$ResolvedStageDirectory"
)

if (-not $NoBuild)
{
	$BuildCookRunArgs += '-build'
}

if (-not $NoCook)
{
	$BuildCookRunArgs += '-cook'
}

if (-not $SkipPackage)
{
	$BuildCookRunArgs += @(
		'-pak',
		'-package',
		'-archive',
		"-archivedirectory=$ResolvedArchiveDirectory"
	)
}

$PrintableCommandParts = @($ResolvedRunUATPath) + $BuildCookRunArgs
$PrintableCommand = '& ' + (($PrintableCommandParts | ForEach-Object { Format-PowerShellCommandPart -Value $_ }) -join ' ')

Write-Host 'Running SQLUI packaged-build validation command:'
Write-Host $PrintableCommand
Write-Host "SQLUI packaged-build validation archive directory: $ResolvedArchiveDirectory"
Write-Host "SQLUI packaged-build validation stage directory: $ResolvedStageDirectory"

& $ResolvedRunUATPath @BuildCookRunArgs
$ExitCode = $LASTEXITCODE

if ($null -eq $ExitCode)
{
	$ExitCode = 0
}

Write-Host "SQLUI packaged-build validation exit code: $ExitCode"
exit $ExitCode
