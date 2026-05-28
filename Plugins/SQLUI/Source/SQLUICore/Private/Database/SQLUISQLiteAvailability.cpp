#include "Database/SQLUISQLiteAvailability.h"

#include "SQLiteDatabase.h"

bool FSQLUISQLiteAvailability::IsSQLiteCoreCompiledIn()
{
	FSQLiteDatabase Database;
	(void)Database.IsValid();
	return true;
}

FString FSQLUISQLiteAvailability::GetSQLiteCoreAvailabilitySummary()
{
	FSQLiteDatabase Database;
	const ESQLiteDatabaseOpenMode DefaultOpenMode = ESQLiteDatabaseOpenMode::ReadWriteCreate;
	const bool bDefaultDatabaseStartsClosed = !Database.IsValid();
	const bool bOpenModeEnumAvailable = DefaultOpenMode == ESQLiteDatabaseOpenMode::ReadWriteCreate;

	return FString::Printf(
		TEXT("SQLiteCore is compiled into SQLUICore. FSQLiteDatabase is available; default wrapper state is %s; ReadWriteCreate open mode is %s; no database was opened."),
		bDefaultDatabaseStartsClosed ? TEXT("closed") : TEXT("open"),
		bOpenModeEnumAvailable ? TEXT("available") : TEXT("unavailable"));
}
