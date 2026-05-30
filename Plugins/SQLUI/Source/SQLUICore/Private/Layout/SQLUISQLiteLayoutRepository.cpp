#include "Layout/SQLUISQLiteLayoutRepository.h"

#include "HAL/FileManager.h"
#include "Layout/SQLUILayoutJson.h"
#include "Misc/Paths.h"
#include "SQLiteDatabase.h"

namespace
{
const TCHAR* SQLUISQLiteLayoutRepositoryEmptyLayoutIdMessage = TEXT("SQLUI SQLite layout repository requires a non-empty layout id.");
const TCHAR* SQLUISQLiteLayoutRepositoryInvalidDocumentMessage = TEXT("SQLUI SQLite layout repository could not save an invalid layout document.");
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

FSQLUILayoutSaveResult MakeSQLUISQLiteLayoutSaveFailure(
	const FString& LayoutId,
	const FString& ErrorMessage,
	const FSQLUILayoutValidationResult& Validation)
{
	FSQLUILayoutSaveResult Result;
	Result.bSucceeded = false;
	Result.SavedLayoutId = LayoutId;
	Result.ErrorMessage = ErrorMessage;
	Result.Validation = Validation;
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

bool TryOpenSQLUISQLiteLayoutWriteDatabase(
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

	if (!Database.Open(*DatabasePath, ESQLiteDatabaseOpenMode::ReadWrite))
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not %s: failed to open database '%s' for writing. SQLiteCore error: %s"),
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

bool TryExecuteSQLUISQLiteLayoutStatement(
	FSQLiteDatabase& Database,
	const TCHAR* Statement,
	const TCHAR* OperationName,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();
	if (Database.Execute(Statement))
	{
		return true;
	}

	OutErrorMessage = FString::Printf(
		TEXT("SQLUI SQLite layout repository could not %s. SQLiteCore error: %s"),
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

FString MakeSQLUISQLiteLayoutRepositorySearchText(
	const FSQLUILayoutDocument& Document)
{
	TArray<FString> SearchParts;
	SearchParts.Add(Document.Metadata.LayoutId);
	SearchParts.Add(Document.Metadata.DisplayName);
	SearchParts.Add(Document.Metadata.Description);
	SearchParts.Add(Document.Metadata.CreatedBy);
	SearchParts.Append(Document.Metadata.Tags);

	for (const TPair<FString, FString>& SearchPair : Document.Metadata.SearchMetadata)
	{
		SearchParts.Add(SearchPair.Key);
		SearchParts.Add(SearchPair.Value);
	}

	return FString::Join(SearchParts, TEXT(" "));
}

FString ResolveSQLUISQLiteLayoutRepositoryRevisionTimestamp(
	const FSQLUILayoutDocument& Document)
{
	if (!Document.Metadata.UpdatedAtUtc.IsEmpty())
	{
		return Document.Metadata.UpdatedAtUtc;
	}

	return Document.Metadata.CreatedAtUtc;
}

bool TryQuerySQLUISQLiteLayoutNextRevision(
	FSQLiteDatabase& Database,
	const FString& LayoutId,
	int32& OutNextRevision,
	FString& OutErrorMessage)
{
	OutNextRevision = 1;
	OutErrorMessage.Empty();

	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("SELECT COALESCE(MAX(revision), 0) ")
		TEXT("FROM layout_revisions ")
		TEXT("WHERE layout_id = ?;"));

	if (!Statement.IsValid())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not prepare next revision query for layout id '%s'. SQLiteCore error: %s"),
			*LayoutId,
			*Database.GetLastError());
		return false;
	}

	if (!Statement.SetBindingValueByIndex(1, LayoutId))
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not bind next revision query for layout id '%s'. SQLiteCore error: %s"),
			*LayoutId,
			*Database.GetLastError());
		return false;
	}

	int32 MaxRevision = 0;
	const int64 RowCount = Statement.Execute(
		[&MaxRevision](const FSQLitePreparedStatement& Row)
		{
			return Row.GetColumnValueByIndex(0, MaxRevision)
				? ESQLitePreparedStatementExecuteRowResult::Continue
				: ESQLitePreparedStatementExecuteRowResult::Error;
		});

	if (RowCount != 1)
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository next revision query failed for layout id '%s'. RowCount=%lld SQLiteCore error: %s"),
			*LayoutId,
			RowCount,
			*Database.GetLastError());
		return false;
	}

	OutNextRevision = MaxRevision + 1;
	return true;
}

bool TryUpsertSQLUISQLiteLayoutRow(
	FSQLiteDatabase& Database,
	const FSQLUILayoutDocument& Document,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("INSERT INTO layouts (")
		TEXT("layout_id, display_name, description, created_by, created_at_utc, updated_at_utc, ")
		TEXT("current_revision, schema_version, search_text, b_deleted")
		TEXT(") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 0) ")
		TEXT("ON CONFLICT(layout_id) DO UPDATE SET ")
		TEXT("display_name = excluded.display_name, ")
		TEXT("description = excluded.description, ")
		TEXT("created_by = excluded.created_by, ")
		TEXT("created_at_utc = excluded.created_at_utc, ")
		TEXT("updated_at_utc = excluded.updated_at_utc, ")
		TEXT("current_revision = excluded.current_revision, ")
		TEXT("schema_version = excluded.schema_version, ")
		TEXT("search_text = excluded.search_text, ")
		TEXT("b_deleted = 0;"));

	if (!Statement.IsValid())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not prepare layout save statement for layout id '%s'. SQLiteCore error: %s"),
			*Document.Metadata.LayoutId,
			*Database.GetLastError());
		return false;
	}

	const FString SearchText = MakeSQLUISQLiteLayoutRepositorySearchText(Document);
	if (!Statement.SetBindingValueByIndex(1, Document.Metadata.LayoutId)
		|| !Statement.SetBindingValueByIndex(2, Document.Metadata.DisplayName)
		|| !Statement.SetBindingValueByIndex(3, Document.Metadata.Description)
		|| !Statement.SetBindingValueByIndex(4, Document.Metadata.CreatedBy)
		|| !Statement.SetBindingValueByIndex(5, Document.Metadata.CreatedAtUtc)
		|| !Statement.SetBindingValueByIndex(6, Document.Metadata.UpdatedAtUtc)
		|| !Statement.SetBindingValueByIndex(7, Document.Version.Revision)
		|| !Statement.SetBindingValueByIndex(8, Document.Version.SchemaVersion)
		|| !Statement.SetBindingValueByIndex(9, SearchText)
		|| !Statement.Execute())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not save layout row for layout id '%s'. SQLiteCore error: %s"),
			*Document.Metadata.LayoutId,
			*Database.GetLastError());
		return false;
	}

	return true;
}

bool TryInsertSQLUISQLiteLayoutRevisionRow(
	FSQLiteDatabase& Database,
	const FSQLUILayoutDocument& Document,
	const FString& DocumentJson,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("INSERT INTO layout_revisions (")
		TEXT("layout_id, revision, label, document_json, document_hash, created_by, created_at_utc, change_note")
		TEXT(") VALUES (?, ?, ?, ?, ?, ?, ?, ?);"));

	if (!Statement.IsValid())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not prepare revision save statement for layout id '%s'. SQLiteCore error: %s"),
			*Document.Metadata.LayoutId,
			*Database.GetLastError());
		return false;
	}

	const FString RevisionCreatedAtUtc = ResolveSQLUISQLiteLayoutRepositoryRevisionTimestamp(Document);
	if (!Statement.SetBindingValueByIndex(1, Document.Metadata.LayoutId)
		|| !Statement.SetBindingValueByIndex(2, Document.Version.Revision)
		|| !Statement.SetBindingValueByIndex(3, Document.Version.Label)
		|| !Statement.SetBindingValueByIndex(4, DocumentJson)
		|| !Statement.SetBindingValueByIndex(5, FString())
		|| !Statement.SetBindingValueByIndex(6, Document.Metadata.CreatedBy)
		|| !Statement.SetBindingValueByIndex(7, RevisionCreatedAtUtc)
		|| !Statement.SetBindingValueByIndex(8, FString())
		|| !Statement.Execute())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not insert revision %d for layout id '%s'. SQLiteCore error: %s"),
			Document.Version.Revision,
			*Document.Metadata.LayoutId,
			*Database.GetLastError());
		return false;
	}

	return true;
}

bool TryReplaceSQLUISQLiteLayoutTags(
	FSQLiteDatabase& Database,
	const FSQLUILayoutDocument& Document,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	FSQLitePreparedStatement DeleteStatement = Database.PrepareStatement(
		TEXT("DELETE FROM layout_tags WHERE layout_id = ?;"));
	if (!DeleteStatement.IsValid())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not prepare tag replacement delete for layout id '%s'. SQLiteCore error: %s"),
			*Document.Metadata.LayoutId,
			*Database.GetLastError());
		return false;
	}

	if (!DeleteStatement.SetBindingValueByIndex(1, Document.Metadata.LayoutId)
		|| !DeleteStatement.Execute())
	{
		OutErrorMessage = FString::Printf(
			TEXT("SQLUI SQLite layout repository could not delete existing tags for layout id '%s'. SQLiteCore error: %s"),
			*Document.Metadata.LayoutId,
			*Database.GetLastError());
		return false;
	}

	for (const FString& Tag : Document.Metadata.Tags)
	{
		FSQLitePreparedStatement InsertStatement = Database.PrepareStatement(
			TEXT("INSERT OR IGNORE INTO layout_tags (layout_id, tag) VALUES (?, ?);"));

		if (!InsertStatement.IsValid())
		{
			OutErrorMessage = FString::Printf(
				TEXT("SQLUI SQLite layout repository could not prepare tag insert for layout id '%s'. SQLiteCore error: %s"),
				*Document.Metadata.LayoutId,
				*Database.GetLastError());
			return false;
		}

		if (!InsertStatement.SetBindingValueByIndex(1, Document.Metadata.LayoutId)
			|| !InsertStatement.SetBindingValueByIndex(2, Tag)
			|| !InsertStatement.Execute())
		{
			OutErrorMessage = FString::Printf(
				TEXT("SQLUI SQLite layout repository could not insert tag '%s' for layout id '%s'. SQLiteCore error: %s"),
				*Tag,
				*Document.Metadata.LayoutId,
				*Database.GetLastError());
			return false;
		}
	}

	return true;
}

bool TrySaveSQLUISQLiteLayoutDocument(
	FSQLiteDatabase& Database,
	const FSQLUILayoutDocument& Document,
	FSQLUILayoutValidationResult& OutValidation,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	if (!TryExecuteSQLUISQLiteLayoutStatement(
		Database,
		TEXT("BEGIN TRANSACTION;"),
		TEXT("begin SaveLayout transaction"),
		OutErrorMessage))
	{
		return false;
	}

	int32 NextRevision = 1;
	FSQLUILayoutDocument StoredDocument = Document;
	FString DocumentJson;
	if (!TryQuerySQLUISQLiteLayoutNextRevision(
			Database,
			Document.Metadata.LayoutId,
			NextRevision,
			OutErrorMessage))
	{
		Database.Execute(TEXT("ROLLBACK;"));
		return false;
	}

	StoredDocument.Version.Revision = NextRevision;
	if (!FSQLUILayoutJson::ToJsonString(StoredDocument, DocumentJson, OutValidation))
	{
		OutErrorMessage = SQLUISQLiteLayoutRepositoryInvalidDocumentMessage;
		Database.Execute(TEXT("ROLLBACK;"));
		return false;
	}

	if (!TryUpsertSQLUISQLiteLayoutRow(Database, StoredDocument, OutErrorMessage)
		|| !TryInsertSQLUISQLiteLayoutRevisionRow(Database, StoredDocument, DocumentJson, OutErrorMessage)
		|| !TryReplaceSQLUISQLiteLayoutTags(Database, StoredDocument, OutErrorMessage))
	{
		Database.Execute(TEXT("ROLLBACK;"));
		return false;
	}

	if (!TryExecuteSQLUISQLiteLayoutStatement(
		Database,
		TEXT("COMMIT;"),
		TEXT("commit SaveLayout transaction"),
		OutErrorMessage))
	{
		Database.Execute(TEXT("ROLLBACK;"));
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
	const FString& LayoutId = Document.Metadata.LayoutId;
	if (Settings.bReadOnly)
	{
		Callback.ExecuteIfBound(MakeSQLUISQLiteLayoutReadOnlySaveFailure(LayoutId));
		return;
	}

	FSQLUILayoutValidationResult Validation = FSQLUILayoutJson::ValidateDocument(Document);
	if (LayoutId.IsEmpty())
	{
		AddSQLUISQLiteLayoutRepositoryValidationError(
			Validation,
			SQLUISQLiteLayoutRepositoryEmptyLayoutIdMessage);
		Callback.ExecuteIfBound(MakeSQLUISQLiteLayoutSaveFailure(
			LayoutId,
			SQLUISQLiteLayoutRepositoryEmptyLayoutIdMessage,
			Validation));
		return;
	}

	if (!Validation.bIsValid)
	{
		Callback.ExecuteIfBound(MakeSQLUISQLiteLayoutSaveFailure(
			LayoutId,
			SQLUISQLiteLayoutRepositoryInvalidDocumentMessage,
			Validation));
		return;
	}

	const FString DatabasePath = GetResolvedDatabasePath();
	FString ErrorMessage;
	FSQLiteDatabase Database;
	if (!TryOpenSQLUISQLiteLayoutWriteDatabase(
		DatabasePath,
		TEXT("save layout"),
		Database,
		ErrorMessage))
	{
		Callback.ExecuteIfBound(MakeSQLUISQLiteLayoutSaveFailure(
			LayoutId,
			ErrorMessage,
			Validation));
		return;
	}

	const bool bSaved = TrySaveSQLUISQLiteLayoutDocument(
		Database,
		Document,
		Validation,
		ErrorMessage);

	if (!TryCloseSQLUISQLiteLayoutDatabase(Database, TEXT("save layout"), ErrorMessage))
	{
		Callback.ExecuteIfBound(MakeSQLUISQLiteLayoutSaveFailure(
			LayoutId,
			ErrorMessage,
			Validation));
		return;
	}

	if (!bSaved)
	{
		Callback.ExecuteIfBound(MakeSQLUISQLiteLayoutSaveFailure(
			LayoutId,
			ErrorMessage,
			Validation));
		return;
	}

	FSQLUILayoutSaveResult Result;
	Result.bSucceeded = true;
	Result.SavedLayoutId = LayoutId;
	Result.Validation = Validation;
	Callback.ExecuteIfBound(Result);
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
