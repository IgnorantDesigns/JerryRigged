#include "Layout/SQLUILayoutRepositoryRuntimeSettings.h"

#include "Misc/Parse.h"

namespace
{
const TCHAR* GetSQLUILayoutRepositoryRuntimeSettingsCommandLine(const TCHAR* CommandLine)
{
	return CommandLine ? CommandLine : TEXT("");
}

FSQLUILayoutRepositoryRuntimeConfig GetSQLUILayoutRepositoryRuntimeSettingsDefaults(
	const USQLUILayoutRepositoryRuntimeSettings* Settings)
{
	return Settings
		? Settings->RuntimeConfig
		: FSQLUILayoutRepositoryRuntimeConfigResolver::MakeDefault();
}
}

bool FSQLUILayoutRepositoryRuntimeSettingsPolicy::ShouldAutoInitializeProvider(
	const USQLUILayoutRepositoryRuntimeSettings* Settings,
	const TCHAR* CommandLine)
{
	const bool bSettingsRequestedAutoInit =
		Settings && Settings->bAutoInitializeProvider;
	const bool bCommandLineRequestedAutoInit =
		FParse::Param(
			GetSQLUILayoutRepositoryRuntimeSettingsCommandLine(CommandLine),
			TEXT("SQLUILayoutRepositoryProviderAutoInit"));

	return bSettingsRequestedAutoInit || bCommandLineRequestedAutoInit;
}

FSQLUILayoutRepositoryRuntimeConfig
FSQLUILayoutRepositoryRuntimeSettingsPolicy::MakeRuntimeConfigFromSettingsAndCommandLine(
	const USQLUILayoutRepositoryRuntimeSettings* Settings,
	const TCHAR* CommandLine)
{
	const FSQLUILayoutRepositoryRuntimeConfig Defaults =
		GetSQLUILayoutRepositoryRuntimeSettingsDefaults(Settings);
	if (Settings && !Settings->bAllowCommandLineOverrides)
	{
		return Defaults;
	}

	return FSQLUILayoutRepositoryRuntimeConfigResolver::FromCommandLine(
		GetSQLUILayoutRepositoryRuntimeSettingsCommandLine(CommandLine),
		Defaults);
}

FSQLUILayoutRepositoryRuntimeIntegrationRequest
FSQLUILayoutRepositoryRuntimeSettingsPolicy::MakeRuntimeIntegrationRequestFromSettingsAndCommandLine(
	const USQLUILayoutRepositoryRuntimeSettings* Settings,
	const TCHAR* CommandLine)
{
	FSQLUILayoutRepositoryRuntimeIntegrationRequest Request;
	Request.RuntimeConfig =
		MakeRuntimeConfigFromSettingsAndCommandLine(Settings, CommandLine);
	Request.bRunSQLiteSeedCopyPolicy =
		Settings ? Settings->bRunSQLiteSeedCopyPolicy : true;
	Request.bTreatSeedCopyFailureAsFatal =
		Settings ? Settings->bTreatSeedCopyFailureAsFatal : true;
	return Request;
}
