#include "Layout/SQLUISQLiteLayoutRepository.h"

#include "HAL/FileManager.h"
#include "Layout/SQLUILayoutJson.h"
#include "Misc/Paths.h"
#include "SQLiteDatabase.h"

namespace
{
const TCHAR* SQLUISQLiteLayoutRepositoryEmptyLayoutIdMessage = TEXT("SQLUI SQLite layout repository requires a non-empty layout id.");
const TCHAR* SQLUISQLiteLayoutRepositoryReadOnlyMessage = TEXT("SQLUI SQLite layout repository is read-only in this implementation slice.");

void AddSQLUISQLiteLayoutRepositoryValidationError(
	FSQLUILayoutValidationResult& Validation,
	const FString& ErrorMessage)
{
	Validation.bIsValid = false;
	Validation.Errors.Add(ErrorMessage);
}

FString NormalizeSQLUISQLiteLayoutRepositoryPath(FString DatabasePath)
{
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

FSQLUILayoutLoadResult MakeSQLUISQLiteLayoutLoadFailure(
	const FString& LayoutId,
	const FString& ErrorMessage)
{
	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = false;
	Result.ErrorMessage = ErrorMessage;
	Result.Document.Metadata.LayoutId = LayoutId;
	return Result;
}

FSQLUILayoutLoadResult MakeSQLUISQLiteLayoutLoadFailure(
	const FString& LayoutId,
	const FString& ErrorMessage,
	const FSQLUILayoutDocument& Document,
	const FSQLUILayoutValidationResult& Validation)
{
	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = false;
	Result.ErrorMessage = ErrorMessage;
	Result.Document = Document;
	Result.Validation = Validation;
	if (Result.Document.Metadata.LayoutId.IsEmpty())
	{
		Result.Document.Metadata.LayoutId = LayoutId;
	}
	return Result;
}

FSQLUILayoutSaveResult MakeSQLUISQLiteLayoutReadOnlySaveFailure(
	const FString& LayoutId)
{
	FSQLUILayoutSaveResult Result;
	Result.bSucceeded = false;
	Result.SavedLayoutId = LayoutId;
	Result.ErrorMessage = SQLUISQLiteLayoutRepositoryReadOnlyMessage;
	return Result;
}

FSQLUILayoutRepositoryRemoveResult MakeSQLUISQLiteLayoutReadOnlyRemoveFailure(
	const FString& LayoutId)
{
	FSQLUILayoutRepositoryRemoveResult Result;
	Result.bSucceeded = false;
	Result.RemovedLayoutId = LayoutId;
	Result.bRemoved = false;
	Result.ErrorMessage = SQLUISQLiteLayoutRepositoryReadOnlyMessage;
	return Result;
}

FSQLUILayoutRepositoryClearResult MakeSQLUISQLiteLayoutReadOnlyClearFailure()
{
	FSQLUILayoutRepositoryClearResult Result;
	Result.bSucceeded = false;
	Result.RemovedCount = 0;
	Result.ErrorMessage = SQLUISQLiteLayoutRepositoryReadOnlyMessage;
	return Result;
}

bool TryOpenSQLUISQLiteLayoutReadOnlyDatabase(
	const FString& DatabasePath,
	const TCHAR* OperationName,
	FSQLiteDatabase& Database,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	if (DatabasePath.IsEmpty())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not %s: database path is empty."),
			OperationName);
		return false;
	}

	if (!FPaths::FileExists(DatabasePath))
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not %s: database '%s' does not exist."),
			OperationName,
			*DatabasePath);
		return false;
	}

	if (!Database.Open(*DatabasePath, ESQLiteDatabaseOpenMode::ReadOnly))
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not %s: failed to open database '%s' read-only. SQLiteCore error: %s"),
			OperationName,
			*DatabasePath,
			*Database.GetLastError());
		return false;
	}

	return true;
}

bool TryCloseSQLUISQLiteLayoutDatabase(
	FSQLiteDatabase& Database,
	const TCHAR* OperationName,
	FString& OutErrorMessage)
{
	if (Database.Close())
	{
		return true;
	}

	OutErrorMessage = FString::Printf(
		TEXT("SQLUI SQLite layout repository could not %s: failed to close database. SQLiteCore error: %s"),
		OperationName,
		*Database.GetLastError());
	return false;
}

bool TryReadSQLUISQLiteLayoutTags(
	FSQLiteDatabase& Database,
	const FString& LayoutId,
	TArray<FString>& OutTags,
	FString& OutErrorMessage)
{
	OutTags.Empty();
	OutErrorMessage.Empty();

	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("SELECT tag ")
		TEXT("FROM layout_tags ")
		TEXT("WHERE layout_id = ? ")
		TEXT("ORDER BY tag COLLATE NOCASE ASC;"));

	if (!Statement.IsValid())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not prepare tag query for layout id '%s'. SQLiteCore error: %s"),
			*LayoutId,
			*Database.GetLastError());
		return false;
	}

	if (!Statement.SetBindingValueByIndex(1, LayoutId))
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not bind tag query for layout id '%s'. SQLiteCore error: %s"),
			*LayoutId,
			*Database.GetLastError());
		return false;
	}

	const int64 RowCount = Statement.Execute(
		[&OutTags](const FSQLitePreparedStatement& Row)
		{
			FString Tag;
			if (!Row.GetColumnValueByIndex(0, Tag))
			{
				return ESQLitePreparedStatementExecuteRowResult::Error;
			}

			OutTags.Add(Tag);
			return ESQLitePreparedStatementExecuteRowResult::Continue;
		});

	if (RowCount == INDEX_NONE)
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository tag query failed for layout id '%s'. SQLiteCore error: %s"),
			*LayoutId,
			*Database.GetLastError());
		return false;
	}

	return true;
}

bool TryReadSQLUISQLiteLayoutMetadataRow(
	FSQLiteDatabase& Database,
	const FSQLitePreparedStatement& Row,
	FSQLUILayoutMetadata& OutMetadata,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	const bool bReadRow =
		Row.GetColumnValueByIndex(0, OutMetadata.LayoutId)
		&& Row.GetColumnValueByIndex(1, OutMetadata.DisplayName)
		&& Row.GetColumnValueByIndex(2, OutMetadata.Description)
		&& Row.GetColumnValueByIndex(3, OutMetadata.CreatedBy)
		&& Row.GetColumnValueByIndex(4, OutMetadata.CreatedAtUtc)
		&& Row.GetColumnValueByIndex(5, OutMetadata.UpdatedAtUtc);

	if (!bReadRow)
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not read layout metadata row. SQLiteCore error: %s"),
			*Database.GetLastError());
		return false;
	}

	return TryReadSQLUISQLiteLayoutTags(
		Database,
		OutMetadata.LayoutId,
		OutMetadata.Tags,
		OutErrorMessage);
}

bool TryQuerySQLUISQLiteLayoutDocumentJson(
	FSQLiteDatabase& Database,
	const FString& LayoutId,
	FString& OutDocumentJson,
	int64& OutRowCount,
	FString& OutErrorMessage)
{
	OutDocumentJson.Empty();
	OutRowCount = INDEX_NONE;
	OutErrorMessage.Empty();

	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("SELECT layout_revisions.document_json ")
		TEXT("FROM layouts ")
		TEXT("INNER JOIN layout_revisions ")
		TEXT("ON layout_revisions.layout_id = layouts.layout_id ")
		TEXT("AND layout_revisions.revision = layouts.current_revision ")
		TEXT("WHERE layouts.layout_id = ? AND layouts.b_deleted = 0;"));

	if (!Statement.IsValid())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not prepare load query for layout id '%s'. SQLiteCore error: %s"),
			*LayoutId,
			*Database.GetLastError());
		return false;
	}

	if (!Statement.SetBindingValueByIndex(1, LayoutId))
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not bind load query for layout id '%s'. SQLiteCore error: %s"),
			*LayoutId,
			*Database.GetLastError());
		return false;
	}

	OutRowCount = Statement.Execute(
		[&OutDocumentJson](const FSQLitePreparedStatement& Row)
		{
			return Row.GetColumnValueByIndex(0, OutDocumentJson)
				? ESQLitePreparedStatementExecuteRowResult::Continue
				: ESQLitePreparedStatementExecuteRowResult::Error;
		});

	if (OutRowCount == INDEX_NONE)
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository load query failed for layout id '%s'. SQLiteCore error: %s"),
			*LayoutId,
			*Database.GetLastError());
		return false;
	}

	return true;
}

bool TryQuerySQLUISQLiteLayoutMetadata(
	FSQLiteDatabase& Database,
	FSQLUILayoutRepositoryListResult& Result,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("SELECT layout_id, display_name, description, created_by, created_at_utc, updated_at_utc ")
		TEXT("FROM layouts ")
		TEXT("WHERE b_deleted = 0 ")
		TEXT("ORDER BY display_name COLLATE NOCASE ASC, layout_id COLLATE NOCASE ASC;"));

	if (!Statement.IsValid())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not prepare metadata list query. SQLiteCore error: %s"),
			*Database.GetLastError());
		return false;
	}

	const int64 RowCount = Statement.Execute(
		[&Database, &Result](const FSQLitePreparedStatement& Row)
		{
			FSQLUILayoutMetadata Metadata;
			FString ErrorMessage;
			if (!TryReadSQLUISQLiteLayoutMetadataRow(
				Database,
				Row,
				Metadata,
				ErrorMessage))
			{
				Result.ErrorMessage = ErrorMessage;
				return ESQLitePreparedStatementExecuteRowResult::Error;
			}

			Result.Layouts.Add(Metadata);
			return ESQLitePreparedStatementExecuteRowResult::Continue;
		});

	if (RowCount == INDEX_NONE)
	{
		OutErrorMessage = Result.ErrorMessage.IsEmpty()
			? FString::Printf(
				TEXT("SQLUI SQLite layout repository metadata list query failed. SQLiteCore error: %s"),
				*Database.GetLastError())
			: Result.ErrorMessage;
		return false;
	}

	return true;
}
}

void USQLUISQLiteLayoutRepository::LoadLayout(
	const FString& LayoutId,
	FSQLUILayoutLoadCompleteDelegate Callback)
{
	Callback.ExecuteIfBound(LoadLayoutById(LayoutId));
}

void USQLUISQLiteLayoutRepository::SaveLayout(
	const FSQLUILayoutDocument& Document,
	FSQLUILayoutSaveCompleteDelegate Callback)
{
	Callback.ExecuteIfBound(MakeSQLUISQLiteLayoutReadOnlySaveFailure(
		Document.Metadata.LayoutId));
}

void USQLUISQLiteLayoutRepository::Configure(
	const FSQLUISQLiteLayoutRepositorySettings& InSettings)
{
	Settings = InSettings;
}

FSQLUISQLiteLayoutRepositorySettings USQLUISQLiteLayoutRepository::GetSettings() const
{
	return Settings;
}

FString USQLUISQLiteLayoutRepository::GetResolvedDatabasePath() const
{
	if (Settings.DatabasePath.IsEmpty())
	{
		return FString();
	}

	return NormalizeSQLUISQLiteLayoutRepositoryPath(Settings.DatabasePath);
}

FSQLUILayoutLoadResult USQLUISQLiteLayoutRepository::LoadLayoutById(
	const FString& LayoutId) const
{
	if (LayoutId.IsEmpty())
	{
		return MakeSQLUISQLiteLayoutLoadFailure(
			LayoutId,
			SQLUISQLiteLayoutRepositoryEmptyLayoutIdMessage);
	}

	const FString DatabasePath = GetResolvedDatabasePath();
	FString ErrorMessage;
	FSQLiteDatabase Database;
	if (!TryOpenSQLUISQLiteLayoutReadOnlyDatabase(
		DatabasePath,
		TEXT("load layout"),
		Database,
		ErrorMessage))
	{
		return MakeSQLUISQLiteLayoutLoadFailure(LayoutId, ErrorMessage);
	}

	FString DocumentJson;
	int64 RowCount = INDEX_NONE;
	const bool bQueriedDocument = TryQuerySQLUISQLiteLayoutDocumentJson(
		Database,
		LayoutId,
		DocumentJson,
		RowCount,
		ErrorMessage);

	if (!TryCloseSQLUISQLiteLayoutDatabase(Database, TEXT("load layout"), ErrorMessage))
	{
		return MakeSQLUISQLiteLayoutLoadFailure(LayoutId, ErrorMessage);
	}

	if (!bQueriedDocument)
	{
		return MakeSQLUISQLiteLayoutLoadFailure(LayoutId, ErrorMessage);
	}

	if (RowCount != 1 || DocumentJson.IsEmpty())
	{
		return MakeSQLUISQLiteLayoutLoadFailure(
			LayoutId,
			FString::Printf(
				TEXT("SQLUI SQLite layout repository could not find active layout id '%s'."),
				*LayoutId));
	}

	FSQLUILayoutDocument Document;
	FSQLUILayoutValidationResult Validation;
	if (!FSQLUILayoutJson::FromJsonString(DocumentJson, Document, Validation))
	{
		return MakeSQLUISQLiteLayoutLoadFailure(
			LayoutId,
			TEXT("SQLUI SQLite layout repository loaded an invalid layout document."),
			Document,
			Validation);
	}

	if (Document.Metadata.LayoutId != LayoutId)
	{
		ErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository loaded layout id '%s' for requested id '%s'."),
			*Document.Metadata.LayoutId,
			*LayoutId);
		AddSQLUISQLiteLayoutRepositoryValidationError(Validation, ErrorMessage);
		return MakeSQLUISQLiteLayoutLoadFailure(LayoutId, ErrorMessage, Document, Validation);
	}

	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = true;
	Result.Document = Document;
	Result.Validation = Validation;
	return Result;
}

FSQLUILayoutRepositoryListResult USQLUISQLiteLayoutRepository::ListLayouts() const
{
	FSQLUILayoutRepositoryListResult Result;

	const FString DatabasePath = GetResolvedDatabasePath();
	FString ErrorMessage;
	FSQLiteDatabase Database;
	if (!TryOpenSQLUISQLiteLayoutReadOnlyDatabase(
		DatabasePath,
		TEXT("list layouts"),
		Database,
		ErrorMessage))
	{
		Result.bSucceeded = false;
		Result.ErrorMessage = ErrorMessage;
		return Result;
	}

	const bool bQueriedMetadata = TryQuerySQLUISQLiteLayoutMetadata(
		Database,
		Result,
		ErrorMessage);

	if (!TryCloseSQLUISQLiteLayoutDatabase(Database, TEXT("list layouts"), ErrorMessage))
	{
		Result.bSucceeded = false;
		Result.ErrorMessage = ErrorMessage;
		return Result;
	}

	if (!bQueriedMetadata)
	{
		Result.bSucceeded = false;
		Result.ErrorMessage = ErrorMessage;
		return Result;
	}

	Result.bSucceeded = true;
	return Result;
}

FSQLUILayoutRepositoryRemoveResult USQLUISQLiteLayoutRepository::RemoveLayout(
	const FString& LayoutId)
{
	return MakeSQLUISQLiteLayoutReadOnlyRemoveFailure(LayoutId);
}

FSQLUILayoutRepositoryClearResult USQLUISQLiteLayoutRepository::ClearLayouts()
{
	return MakeSQLUISQLiteLayoutReadOnlyClearFailure();
}
