#include "Layout/SQLUILayoutRepositoryRuntimeConfig.h"

#include "Misc/Parse.h"
#include "Misc/Paths.h"

const TCHAR* FSQLUILayoutRepositoryRuntimeConfigResolver::DefaultSQLiteDatabaseToken =
	TEXT("Default");
const TCHAR* FSQLUILayoutRepositoryRuntimeConfigResolver::DefaultSQLiteDatabaseFilename =
	TEXT("LayoutRepository.sqlite");

namespace
{
FString NormalizeSQLUILayoutRepositoryRuntimePath(FString Path)
{
	FPaths::NormalizeFilename(Path);
	return FPaths::ConvertRelativePathToFull(Path);
}

FString MakeSQLUILayoutRepositoryRuntimeSQLiteBaseDirectory()
{
	FString Directory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("LayoutRepositories"));
	FPaths::NormalizeFilename(Directory);
	return FPaths::ConvertRelativePathToFull(Directory);
}

bool ParseSQLUILayoutRepositoryRuntimeCommandLineValue(
	const TCHAR* CommandLine,
	const TCHAR* KeyWithoutDash,
	FString& OutValue)
{
	if (FParse::Value(CommandLine, KeyWithoutDash, OutValue))
	{
		return true;
	}

	const FString KeyWithDash = FString::Printf(TEXT("-%s"), KeyWithoutDash);
	return FParse::Value(CommandLine, *KeyWithDash, OutValue);
}
}

FSQLUILayoutRepositoryRuntimeConfig
FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault()
{
	FSQLUILayoutRepositoryRuntimeConfig Config;
	Config.Backend = ESQLUILayoutRepositoryBackend::InMemory;
	return Config;
}

FSQLUILayoutRepositoryFactorySettings
FSQLUILayoutRepositoryRuntimeConfigResolver::ToFactorySettings(
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	FSQLUILayoutRepositoryFactorySettings Settings;
	Settings.Backend = RuntimeConfig.Backend;
	Settings.JsonFileBaseDirectory = RuntimeConfig.JsonFileBaseDirectory;
	Settings.SQLiteSettings.DatabasePath =
		ResolveSQLiteDatabasePath(RuntimeConfig.SQLiteDatabasePath);
	Settings.SQLiteSettings.bReadOnly = RuntimeConfig.bSQLiteReadOnly;
	Settings.SQLiteSettings.bInitializeSchemaIfMissing =
		RuntimeConfig.bSQLiteInitializeSchemaIfMissing;
	Settings.SQLiteSettings.bCreateDatabaseIfMissing =
		RuntimeConfig.bSQLiteCreateDatabaseIfMissing;
	Settings.SQLiteSettings.bRunCallbackOperationsAsync =
		RuntimeConfig.bSQLiteRunCallbackOperationsAsync;
	return Settings;
}

FSQLUISQLiteSeedDatabaseCopyRequest
FSQLUILayoutRepositoryRuntimeConfigResolver::ToSeedDatabaseCopyRequest(
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	FSQLUISQLiteSeedDatabaseCopyRequest Request;
	Request.SeedDatabasePath =
		ResolveSQLiteSeedDatabasePath(RuntimeConfig.SQLiteSeedDatabasePath);
	Request.TargetDatabasePath =
		ResolveSQLiteDatabasePath(RuntimeConfig.SQLiteDatabasePath);
	Request.bCopyIfTargetMissing = RuntimeConfig.bSQLiteCopySeedIfMissing;
	Request.bOverwriteTarget = RuntimeConfig.bSQLiteOverwriteDatabaseFromSeed;
	Request.bCreateTargetDirectory = true;
	return Request;
}

FSQLUILayoutRepositoryRuntimeConfig
FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
	const TCHAR* CommandLine,
	const FSQLUILayoutRepositoryRuntimeConfig& Defaults)
{
	FSQLUILayoutRepositoryRuntimeConfig Config = Defaults;
	const TCHAR* SafeCommandLine = CommandLine ? CommandLine : TEXT("");

	FString BackendText;
	if (ParseSQLUILayoutRepositoryRuntimeCommandLineValue(
		SafeCommandLine,
		TEXT("SQLUILayoutRepositoryBackend="),
		BackendText))
	{
		ESQLUILayoutRepositoryBackend ParsedBackend = Config.Backend;
		if (TryParseBackend(BackendText, ParsedBackend))
		{
			Config.Backend = ParsedBackend;
		}
	}

	FString JsonFileBaseDirectory;
	if (ParseSQLUILayoutRepositoryRuntimeCommandLineValue(
		SafeCommandLine,
		TEXT("SQLUIJsonFileLayoutRepositoryDir="),
		JsonFileBaseDirectory))
	{
		Config.JsonFileBaseDirectory = JsonFileBaseDirectory;
	}

	FString SQLiteDatabasePath;
	if (ParseSQLUILayoutRepositoryRuntimeCommandLineValue(
		SafeCommandLine,
		TEXT("SQLUISQLiteLayoutRepositoryPath="),
		SQLiteDatabasePath))
	{
		Config.SQLiteDatabasePath = SQLiteDatabasePath;
	}

	FString SQLiteSeedDatabasePath;
	if (ParseSQLUILayoutRepositoryRuntimeCommandLineValue(
		SafeCommandLine,
		TEXT("SQLUISQLiteLayoutRepositorySeedPath="),
		SQLiteSeedDatabasePath))
	{
		Config.SQLiteSeedDatabasePath = SQLiteSeedDatabasePath;
	}

	if (FParse::Param(SafeCommandLine, TEXT("SQLUISQLiteLayoutRepositoryReadOnly")))
	{
		Config.bSQLiteReadOnly = true;
	}

	if (FParse::Param(SafeCommandLine, TEXT("SQLUISQLiteLayoutRepositoryInitializeSchema")))
	{
		Config.bSQLiteInitializeSchemaIfMissing = true;
	}

	if (FParse::Param(SafeCommandLine, TEXT("SQLUISQLiteLayoutRepositoryCreateDatabase")))
	{
		Config.bSQLiteCreateDatabaseIfMissing = true;
	}

	if (FParse::Param(SafeCommandLine, TEXT("SQLUISQLiteLayoutRepositoryAsyncCallbacks")))
	{
		Config.bSQLiteRunCallbackOperationsAsync = true;
	}

	if (FParse::Param(SafeCommandLine, TEXT("SQLUISQLiteLayoutRepositoryCopySeedIfMissing")))
	{
		Config.bSQLiteCopySeedIfMissing = true;
	}

	if (FParse::Param(SafeCommandLine, TEXT("SQLUISQLiteLayoutRepositoryOverwriteFromSeed")))
	{
		Config.bSQLiteOverwriteDatabaseFromSeed = true;
	}

	return Config;
}

FString FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteDatabasePath(
	const FString& InputPath)
{
	FString TrimmedPath = InputPath;
	TrimmedPath.TrimStartAndEndInline();
	if (TrimmedPath.IsEmpty())
	{
		return FString();
	}

	if (TrimmedPath.Equals(DefaultSQLiteDatabaseToken, ESearchCase::IgnoreCase))
	{
		TrimmedPath = DefaultSQLiteDatabaseFilename;
	}

	if (FPaths::IsRelative(TrimmedPath))
	{
		return NormalizeSQLUILayoutRepositoryRuntimePath(
			FPaths::Combine(
				MakeSQLUILayoutRepositoryRuntimeSQLiteBaseDirectory(),
				TrimmedPath));
	}

	return NormalizeSQLUILayoutRepositoryRuntimePath(TrimmedPath);
}

FString FSQLUILayoutRepositoryRuntimeConfigResolver::ResolveSQLiteSeedDatabasePath(
	const FString& InputPath)
{
	FString TrimmedPath = InputPath;
	TrimmedPath.TrimStartAndEndInline();
	if (TrimmedPath.IsEmpty())
	{
		return FString();
	}

	return NormalizeSQLUILayoutRepositoryRuntimePath(TrimmedPath);
}

bool FSQLUILayoutRepositoryRuntimeConfigResolver::TryParseBackend(
	const FString& BackendText,
	ESQLUILayoutRepositoryBackend& OutBackend)
{
	FString NormalizedBackend = BackendText;
	NormalizedBackend.TrimStartAndEndInline();
	NormalizedBackend.ReplaceInline(TEXT("-"), TEXT(""));
	NormalizedBackend.ReplaceInline(TEXT("_"), TEXT(""));
	NormalizedBackend.ReplaceInline(TEXT(" "), TEXT(""));

	if (NormalizedBackend.Equals(TEXT("Unavailable"), ESearchCase::IgnoreCase))
	{
		OutBackend = ESQLUILayoutRepositoryBackend::Unavailable;
		return true;
	}

	if (NormalizedBackend.Equals(TEXT("InMemory"), ESearchCase::IgnoreCase)
		|| NormalizedBackend.Equals(TEXT("Memory"), ESearchCase::IgnoreCase))
	{
		OutBackend = ESQLUILayoutRepositoryBackend::InMemory;
		return true;
	}

	if (NormalizedBackend.Equals(TEXT("JsonFile"), ESearchCase::IgnoreCase)
		|| NormalizedBackend.Equals(TEXT("JSONFile"), ESearchCase::IgnoreCase)
		|| NormalizedBackend.Equals(TEXT("Json"), ESearchCase::IgnoreCase))
	{
		OutBackend = ESQLUILayoutRepositoryBackend::JsonFile;
		return true;
	}

	if (NormalizedBackend.Equals(TEXT("SQLite"), ESearchCase::IgnoreCase)
		|| NormalizedBackend.Equals(TEXT("Sqlite"), ESearchCase::IgnoreCase))
	{
		OutBackend = ESQLUILayoutRepositoryBackend::SQLite;
		return true;
	}

	return false;
}
