[CmdletBinding()]
param(
	[string]$EngineRoot,

	[string]$RunUATPath,

	[string]$ProjectPath,

	[string]$Platform = 'Win64',

	[string]$Configuration = 'Development',

	[string]$ArchiveDirectory,

	[string]$StageDirectory,

	[switch]$RunPackagedSQLiteSmoke,

	[int]$PackagedSmokeTimeoutSeconds = 120,

	[string]$PackagedSmokeLogPath,

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

  -RunPackagedSQLiteSmoke
      After BuildCookRun succeeds, launch the packaged executable with
      -SQLUIPackagedRuntimeSQLiteSmoke and verify the runtime smoke log.

  -PackagedSmokeTimeoutSeconds
      Timeout for the packaged runtime smoke process. Defaults to 120.

  -PackagedSmokeLogPath
      Optional packaged runtime smoke log path. Defaults to:
      Saved\SQLUI\PackagedValidation\<Platform>\<Configuration>\RuntimeSmoke\SQLUIPackagedRuntimeSQLiteSmoke.log
      The resolved path must stay under Saved\SQLUI\PackagedValidation.

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
  .\Scripts\RunSQLUIPackagedBuildValidation.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.7" -CleanPackageOutput -RunPackagedSQLiteSmoke
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

function Get-SQLUIPackagedRuntimePlatformFolders
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$PlatformName
	)

	$Folders = New-Object System.Collections.Generic.List[string]
	if ($PlatformName.Equals('Win64', [System.StringComparison]::OrdinalIgnoreCase))
	{
		$Folders.Add('Windows')
	}

	$Folders.Add($PlatformName)
	return $Folders | Select-Object -Unique
}

function Find-SQLUIPackagedExecutable
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$ArchiveRoot,

		[Parameter(Mandatory = $true)]
		[string]$StageRoot,

		[Parameter(Mandatory = $true)]
		[string]$PlatformName
	)

	$PlatformFolders = @(Get-SQLUIPackagedRuntimePlatformFolders -PlatformName $PlatformName)
	$CandidatePaths = New-Object System.Collections.Generic.List[string]

	foreach ($Root in @($ArchiveRoot, $StageRoot))
	{
		$CandidatePaths.Add((Join-Path -Path $Root -ChildPath 'JerryRigged.exe'))

		foreach ($Folder in $PlatformFolders)
		{
			$CandidatePaths.Add((Join-Path -Path $Root -ChildPath (Join-Path -Path $Folder -ChildPath 'JerryRigged.exe')))
			$CandidatePaths.Add((Join-Path -Path $Root -ChildPath (Join-Path -Path $Folder -ChildPath 'JerryRigged\JerryRigged.exe')))
			$CandidatePaths.Add((Join-Path -Path $Root -ChildPath (Join-Path -Path $Folder -ChildPath 'JerryRigged\Binaries\Win64\JerryRigged.exe')))
		}
	}

	foreach ($CandidatePath in ($CandidatePaths | Select-Object -Unique))
	{
		if (Test-Path -LiteralPath $CandidatePath -PathType Leaf)
		{
			return (Resolve-Path -LiteralPath $CandidatePath).ProviderPath
		}
	}

	Write-Host 'SQLUI packaged runtime SQLite smoke executable candidates checked:'
	foreach ($CandidatePath in ($CandidatePaths | Select-Object -Unique))
	{
		Write-Host "  $CandidatePath"
	}

	return $null
}

function Invoke-SQLUIPackagedRuntimeSQLiteSmoke
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$ExecutablePath,

		[Parameter(Mandatory = $true)]
		[string]$LogPath,

		[Parameter(Mandatory = $true)]
		[int]$TimeoutSeconds
	)

	if ($TimeoutSeconds -lt 1)
	{
		Stop-WithUsageError 'PackagedSmokeTimeoutSeconds must be 1 or greater.'
	}

	$LogDirectory = Split-Path -Parent $LogPath
	New-Item -ItemType Directory -Force -Path $LogDirectory | Out-Null

	if (Test-Path -LiteralPath $LogPath)
	{
		Remove-Item -LiteralPath $LogPath -Force
	}

	$SmokeArgs = @(
		'-SQLUIPackagedRuntimeSQLiteSmoke',
		'-unattended',
		'-nosound',
		'-NullRHI',
		"-abslog=`"$LogPath`""
	)

	$PrintableCommandParts = @($ExecutablePath) + $SmokeArgs
	$PrintableCommand = '& ' + (($PrintableCommandParts | ForEach-Object { Format-PowerShellCommandPart -Value $_ }) -join ' ')

	Write-Host 'Running SQLUI packaged runtime SQLite smoke command:'
	Write-Host $PrintableCommand
	Write-Host "SQLUI packaged runtime SQLite smoke log path: $LogPath"

	$Process = Start-Process -FilePath $ExecutablePath -ArgumentList $SmokeArgs -WindowStyle Hidden -PassThru
	$TimeoutMilliseconds = [Math]::Max(1, $TimeoutSeconds) * 1000
	if (-not $Process.WaitForExit($TimeoutMilliseconds))
	{
		Write-Error "SQLUI packaged runtime SQLite smoke timed out after $TimeoutSeconds second(s)."
		try
		{
			$Process.Kill()
		}
		catch
		{
			Write-Warning "SQLUI packaged runtime SQLite smoke could not kill timed-out process: $($_.Exception.Message)"
		}
		return 4
	}

	$RuntimeExitCode = $Process.ExitCode
	Write-Host "SQLUI packaged runtime SQLite smoke process exit code: $RuntimeExitCode"

	if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf))
	{
		Write-Error "SQLUI packaged runtime SQLite smoke log was not created: $LogPath"
		return 5
	}

	$LogText = Get-Content -LiteralPath $LogPath -Raw
	if ($LogText.Contains('SQLUI packaged runtime SQLite smoke failed:'))
	{
		Write-Error 'SQLUI packaged runtime SQLite smoke log contains a failure line.'
		return 6
	}

	if (-not $LogText.Contains('SQLUI packaged runtime SQLite smoke succeeded.'))
	{
		Write-Error 'SQLUI packaged runtime SQLite smoke success line was not found in the runtime log.'
		return 7
	}

	if ($RuntimeExitCode -ne 0)
	{
		Write-Error "SQLUI packaged runtime SQLite smoke process did not exit cleanly. ExitCode=$RuntimeExitCode"
		return $RuntimeExitCode
	}

	Write-Host 'SQLUI packaged runtime SQLite smoke succeeded.'
	return 0
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

if ([string]::IsNullOrWhiteSpace($PackagedSmokeLogPath))
{
	$PackagedSmokeLogPath = Join-Path `
		-Path $DefaultOutputRoot `
		-ChildPath 'RuntimeSmoke\SQLUIPackagedRuntimeSQLiteSmoke.log'
}

$ResolvedArchiveDirectory = Resolve-FullPath -Path $ArchiveDirectory -BasePath $ProjectRoot
$ResolvedStageDirectory = Resolve-FullPath -Path $StageDirectory -BasePath $ProjectRoot
$ResolvedPackagedSmokeLogPath = Resolve-FullPath -Path $PackagedSmokeLogPath -BasePath $ProjectRoot

if (-not (Test-IsPathUnder -Path $ResolvedArchiveDirectory -RootPath $PackagedValidationRoot))
{
	Stop-WithUsageError "ArchiveDirectory must be under Saved\SQLUI\PackagedValidation: $ResolvedArchiveDirectory"
}

if (-not (Test-IsPathUnder -Path $ResolvedStageDirectory -RootPath $PackagedValidationRoot))
{
	Stop-WithUsageError "StageDirectory must be under Saved\SQLUI\PackagedValidation: $ResolvedStageDirectory"
}

if (-not (Test-IsPathUnder -Path $ResolvedPackagedSmokeLogPath -RootPath $PackagedValidationRoot))
{
	Stop-WithUsageError "PackagedSmokeLogPath must be under Saved\SQLUI\PackagedValidation: $ResolvedPackagedSmokeLogPath"
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
if ($RunPackagedSQLiteSmoke)
{
	Write-Host "SQLUI packaged runtime SQLite smoke requested: true"
	Write-Host "SQLUI packaged runtime SQLite smoke timeout seconds: $PackagedSmokeTimeoutSeconds"
	Write-Host "SQLUI packaged runtime SQLite smoke log path: $ResolvedPackagedSmokeLogPath"
}

& $ResolvedRunUATPath @BuildCookRunArgs
$ExitCode = $LASTEXITCODE

if ($null -eq $ExitCode)
{
	$ExitCode = 0
}

Write-Host "SQLUI packaged-build validation exit code: $ExitCode"
if ($ExitCode -ne 0 -or -not $RunPackagedSQLiteSmoke)
{
	exit $ExitCode
}

$PackagedExecutablePath = Find-SQLUIPackagedExecutable `
	-ArchiveRoot $ResolvedArchiveDirectory `
	-StageRoot $ResolvedStageDirectory `
	-PlatformName $Platform

if ([string]::IsNullOrWhiteSpace($PackagedExecutablePath))
{
	Write-Error 'SQLUI packaged runtime SQLite smoke failed: could not locate packaged executable.'
	exit 3
}

Write-Host "SQLUI packaged runtime SQLite smoke executable path: $PackagedExecutablePath"
$SmokeExitCode = Invoke-SQLUIPackagedRuntimeSQLiteSmoke `
	-ExecutablePath $PackagedExecutablePath `
	-LogPath $ResolvedPackagedSmokeLogPath `
	-TimeoutSeconds $PackagedSmokeTimeoutSeconds

Write-Host "SQLUI packaged runtime SQLite smoke exit code: $SmokeExitCode"
exit $SmokeExitCode
