#include "Layout/SQLUIPersistenceStatusDisplay.h"

namespace
{
FText SQLUIPersistenceStatusDisplayText(const TCHAR* Text)
{
	return FText::FromString(Text);
}

FText SQLUIPersistenceStatusDisplayString(const FString& Text)
{
	return Text.IsEmpty()
		? SQLUIPersistenceStatusDisplayText(TEXT("Unavailable"))
		: FText::FromString(Text);
}

FText SQLUIPersistenceStatusDisplayYesNo(const bool bValue)
{
	return SQLUIPersistenceStatusDisplayText(bValue ? TEXT("Yes") : TEXT("No"));
}

FText SQLUIPersistenceStatusDisplayByteCount(const int64 ByteCount)
{
	if (ByteCount < 0)
	{
		return SQLUIPersistenceStatusDisplayText(TEXT("Unavailable"));
	}

	if (ByteCount < 1024)
	{
		return FText::FromString(
			FString::Printf(TEXT("%lld bytes"), ByteCount));
	}

	const double Kilobytes = static_cast<double>(ByteCount) / 1024.0;
	if (Kilobytes < 1024.0)
	{
		return FText::FromString(
			FString::Printf(TEXT("%.1f KB"), Kilobytes));
	}

	const double Megabytes = Kilobytes / 1024.0;
	if (Megabytes < 1024.0)
	{
		return FText::FromString(
			FString::Printf(TEXT("%.1f MB"), Megabytes));
	}

	const double Gigabytes = Megabytes / 1024.0;
	return FText::FromString(
		FString::Printf(TEXT("%.1f GB"), Gigabytes));
}

void AddSQLUIPersistenceStatusDisplayRow(
	TArray<FSQLUIPersistenceStatusDisplayRow>& Rows,
	const TCHAR* Label,
	const FText& Value,
	const ESQLUIPersistenceStatusDisplayState State =
		ESQLUIPersistenceStatusDisplayState::Normal,
	const TCHAR* DetailText = TEXT(""))
{
	FSQLUIPersistenceStatusDisplayRow Row;
	Row.Label = SQLUIPersistenceStatusDisplayText(Label);
	Row.Value = Value;
	Row.State = State;
	Row.DetailText = SQLUIPersistenceStatusDisplayText(DetailText);
	Rows.Add(Row);
}

bool IsSQLUIPersistenceStatusDisplaySQLite(
	const FSQLUIPersistenceStatusSnapshot& Snapshot)
{
	return Snapshot.ConfiguredBackend == ESQLUILayoutRepositoryBackend::SQLite;
}

FText MakeSQLUIPersistenceStatusSchemaValue(
	const FSQLUIPersistenceStatusSnapshot& Snapshot)
{
	if (!IsSQLUIPersistenceStatusDisplaySQLite(Snapshot))
	{
		return SQLUIPersistenceStatusDisplayText(TEXT("Not applicable"));
	}

	if (!Snapshot.bMigrationStatusChecked)
	{
		return SQLUIPersistenceStatusDisplayText(TEXT("Not checked"));
	}

	if (!Snapshot.bMigrationStatusSucceeded)
	{
		return SQLUIPersistenceStatusDisplayText(TEXT("Warning"));
	}

	if (Snapshot.bSQLiteHasPendingMigrations)
	{
		return SQLUIPersistenceStatusDisplayText(TEXT("Pending migrations"));
	}

	return Snapshot.bSQLiteSchemaObjectsReady
		? SQLUIPersistenceStatusDisplayText(TEXT("Ready"))
		: SQLUIPersistenceStatusDisplayText(TEXT("Unavailable"));
}

ESQLUIPersistenceStatusDisplayState MakeSQLUIPersistenceStatusSchemaState(
	const FSQLUIPersistenceStatusSnapshot& Snapshot)
{
	if (!IsSQLUIPersistenceStatusDisplaySQLite(Snapshot)
		|| !Snapshot.bMigrationStatusChecked)
	{
		return ESQLUIPersistenceStatusDisplayState::Normal;
	}

	if (!Snapshot.bMigrationStatusSucceeded
		|| Snapshot.bSQLiteHasPendingMigrations
		|| !Snapshot.bSQLiteSchemaObjectsReady)
	{
		return ESQLUIPersistenceStatusDisplayState::Warning;
	}

	return ESQLUIPersistenceStatusDisplayState::Good;
}
}

TArray<FSQLUIPersistenceStatusDisplayRow>
USQLUIPersistenceStatusDisplayLibrary::GetPersistenceStatusDisplayRows(
	UObject* WorldContextObject)
{
	return MakePersistenceStatusDisplayRows(
		USQLUIPersistenceStatusLibrary::GetPersistenceStatus(WorldContextObject));
}

TArray<FSQLUIPersistenceStatusDisplayRow>
USQLUIPersistenceStatusDisplayLibrary::GetPersistenceStatusDisplayRowsFromRuntimeConfig(
	UObject* WorldContextObject,
	const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig)
{
	return MakePersistenceStatusDisplayRows(
		USQLUIPersistenceStatusLibrary::GetPersistenceStatusFromRuntimeConfig(
			WorldContextObject,
			RuntimeConfig));
}

TArray<FSQLUIPersistenceStatusDisplayRow>
USQLUIPersistenceStatusDisplayLibrary::MakePersistenceStatusDisplayRows(
	const FSQLUIPersistenceStatusSnapshot& Snapshot)
{
	TArray<FSQLUIPersistenceStatusDisplayRow> Rows;
	const bool bSQLite = IsSQLUIPersistenceStatusDisplaySQLite(Snapshot);

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("Backend"),
		SQLUIPersistenceStatusDisplayString(Snapshot.ConfiguredBackendName),
		ESQLUIPersistenceStatusDisplayState::Normal,
		TEXT("Configured runtime repository backend."));

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("Provider initialized"),
		SQLUIPersistenceStatusDisplayYesNo(Snapshot.bProviderInitialized),
		Snapshot.bProviderInitialized
			? ESQLUIPersistenceStatusDisplayState::Good
			: ESQLUIPersistenceStatusDisplayState::Normal,
		TEXT("No is expected until provider initialization is explicitly requested."));

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("Repository active"),
		SQLUIPersistenceStatusDisplayYesNo(Snapshot.bRepositoryActive),
		Snapshot.bRepositoryActive
			? ESQLUIPersistenceStatusDisplayState::Good
			: ESQLUIPersistenceStatusDisplayState::Normal,
		TEXT("No is expected until a repository is explicitly initialized."));

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("Active backend"),
		SQLUIPersistenceStatusDisplayString(Snapshot.ActiveBackendName),
		Snapshot.bRepositoryActive
			? ESQLUIPersistenceStatusDisplayState::Good
			: ESQLUIPersistenceStatusDisplayState::Unavailable,
		TEXT("Active backend is unavailable until a repository is initialized."));

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("SQLite database path"),
		bSQLite
			? (Snapshot.bSQLiteDatabasePathResolved
				? FText::FromString(Snapshot.ResolvedSQLiteDatabasePath)
				: SQLUIPersistenceStatusDisplayText(TEXT("Not configured")))
			: SQLUIPersistenceStatusDisplayText(TEXT("Not applicable")),
		bSQLite && !Snapshot.bSQLiteDatabasePathResolved
			? ESQLUIPersistenceStatusDisplayState::Warning
			: ESQLUIPersistenceStatusDisplayState::Normal,
		TEXT("Path is shown only from the SQLUICore status snapshot."));

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("SQLite database exists"),
		bSQLite
			? SQLUIPersistenceStatusDisplayYesNo(Snapshot.bSQLiteDatabaseExists)
			: SQLUIPersistenceStatusDisplayText(TEXT("Not applicable")),
		bSQLite
			? (Snapshot.bSQLiteDatabaseExists
				? ESQLUIPersistenceStatusDisplayState::Good
				: ESQLUIPersistenceStatusDisplayState::Warning)
			: ESQLUIPersistenceStatusDisplayState::Normal,
		TEXT("Missing is normal when SQLite is not the configured backend."));

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("SQLite database size"),
		bSQLite && Snapshot.bSQLiteDatabaseExists
			? SQLUIPersistenceStatusDisplayByteCount(Snapshot.SQLiteDatabaseSizeBytes)
			: SQLUIPersistenceStatusDisplayText(TEXT("Not applicable")),
		ESQLUIPersistenceStatusDisplayState::Normal,
		TEXT("File size is reported by the read-only status snapshot."));

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("SQLite sidecars"),
		bSQLite
			? (Snapshot.bSQLiteSidecarsPresent
				? FText::FromString(
					FString::Printf(
						TEXT("Present: %s"),
						*Snapshot.SQLiteSidecarSummary))
				: SQLUIPersistenceStatusDisplayText(TEXT("None")))
			: SQLUIPersistenceStatusDisplayText(TEXT("Not applicable")),
		ESQLUIPersistenceStatusDisplayState::Normal,
		TEXT("Sidecar status is informational and not an ownership or cleanup action."));

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("Schema status"),
		MakeSQLUIPersistenceStatusSchemaValue(Snapshot),
		MakeSQLUIPersistenceStatusSchemaState(Snapshot),
		TEXT("Schema status is read only and never applies migrations."));

	AddSQLUIPersistenceStatusDisplayRow(
		Rows,
		TEXT("Status"),
		SQLUIPersistenceStatusDisplayString(Snapshot.StatusText),
		Snapshot.WarningText.IsEmpty() && Snapshot.ErrorMessage.IsEmpty()
			? ESQLUIPersistenceStatusDisplayState::Normal
			: ESQLUIPersistenceStatusDisplayState::Warning,
		TEXT("Summary from the SQLUICore persistence status snapshot."));

	if (!Snapshot.WarningText.IsEmpty())
	{
		AddSQLUIPersistenceStatusDisplayRow(
			Rows,
			TEXT("Warnings"),
			FText::FromString(Snapshot.WarningText),
			ESQLUIPersistenceStatusDisplayState::Warning,
			TEXT("Read-only warning text from SQLUICore status."));
	}

	if (!Snapshot.ErrorMessage.IsEmpty())
	{
		AddSQLUIPersistenceStatusDisplayRow(
			Rows,
			TEXT("Errors"),
			FText::FromString(Snapshot.ErrorMessage),
			ESQLUIPersistenceStatusDisplayState::Warning,
			TEXT("Read-only error text from SQLUICore status."));
	}

	return Rows;
}
