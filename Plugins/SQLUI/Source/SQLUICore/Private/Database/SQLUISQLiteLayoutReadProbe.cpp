#include "Database/SQLUISQLiteLayoutReadProbe.h"

#include "Database/SQLUISQLiteLayoutSchemaMigration.h"
#include "HAL/FileManager.h"
#include "Layout/SQLUILayoutJson.h"
#include "Misc/Paths.h"
#include "SQLiteDatabase.h"

namespace
{
const TCHAR* SQLUILayoutReadProbeLayoutId = TEXT("sqlui.smoke.sqlite-layout-read-probe");
const TCHAR* SQLUILayoutReadProbeDisplayName = TEXT("SQLUI SQLite Layout Read Probe");
const TCHAR* SQLUILayoutReadProbeDescription = TEXT("Smoke/probe layout for SQLite read mapping.");
const TCHAR* SQLUILayoutReadProbeCreatedBy = TEXT("SQLUICore");

void AppendSQLUILayoutReadProbeError(
	FSQLUISQLiteLayoutReadProbeResult& Result,
	const FString& ErrorMessage)
{
	if (ErrorMessage.IsEmpty())
	{
		return;
	}

	if (!Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage += TEXT(" ");
	}

	Result.ErrorMessage += ErrorMessage;
}

FString NormalizeSQLUILayoutReadProbePath(FString DatabasePath)
{
	FPaths::NormalizeFilename(DatabasePath);
	return FPaths::ConvertRelativePathToFull(DatabasePath);
}

bool DeleteSQLUILayoutReadProbeFiles(
	const FString& DatabasePath,
	FSQLUISQLiteLayoutReadProbeResult& Result)
{
	const TArray<FString> PathsToRemove = {
		DatabasePath,
		DatabasePath + TEXT("-journal"),
		DatabasePath + TEXT("-wal"),
		DatabasePath + TEXT("-shm")
	};

	bool bRemoved = true;
	for (const FString& PathToRemove : PathsToRemove)
	{
		if (FPaths::FileExists(PathToRemove)
			&& !IFileManager::Get().Delete(*PathToRemove, false, true, true))
		{
			AppendSQLUILayoutReadProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite layout read probe failed: could not remove '%s'."),
					*PathToRemove));
			bRemoved = false;
		}
	}

	return bRemoved;
}

bool ExecuteSQLUILayoutReadProbeStatement(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutReadProbeResult& Result,
	const TCHAR* Statement,
	const TCHAR* OperationName)
{
	if (Database.Execute(Statement))
	{
		return true;
	}

	AppendSQLUILayoutReadProbeError(
		Result,
		FString::Printf(
			TEXT("SQLUI SQLite layout read probe failed: %s failed. SQLiteCore error: %s"),
			OperationName,
			*Database.GetLastError()));
	return false;
}

FSQLUILayoutDocument MakeSQLUILayoutReadProbeDocument()
{
	FSQLUILayoutDocument Document;
	Document.Version.SchemaVersion = 1;
	Document.Version.Revision = 1;
	Document.Version.Label = TEXT("SQLite Layout Read Probe");
	Document.Metadata.LayoutId = SQLUILayoutReadProbeLayoutId;
	Document.Metadata.DisplayName = SQLUILayoutReadProbeDisplayName;
	Document.Metadata.Description = SQLUILayoutReadProbeDescription;
	Document.Metadata.CreatedBy = SQLUILayoutReadProbeCreatedBy;
	Document.Metadata.CreatedAtUtc = TEXT("2026-05-29T00:00:00Z");
	Document.Metadata.UpdatedAtUtc = Document.Metadata.CreatedAtUtc;
	Document.Metadata.Tags.Add(TEXT("sqlite"));
	Document.Metadata.Tags.Add(TEXT("smoke"));
	Document.Metadata.Tags.Add(TEXT("read-probe"));
	Document.Metadata.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteLayoutRead"));
	Document.RootWidgetId = TEXT("SQLUI.SQLite.LayoutReadProbe.Root");

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = Document.RootWidgetId;
	RootNode.WidgetTypeKey = TEXT("SQLUI.ProbeRoot");
	RootNode.Properties.Add(TEXT("Text"), SQLUILayoutReadProbeDisplayName);
	RootNode.Tags.Add(TEXT("sqlite"));
	RootNode.Tags.Add(TEXT("read-probe"));
	RootNode.SearchMetadata.Add(TEXT("Probe"), TEXT("SQLiteLayoutRead"));

	Document.Nodes.Add(RootNode);
	return Document;
}

FString MakeSQLUILayoutReadProbeSearchText(const FSQLUILayoutDocument& Document)
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

bool InsertSQLUILayoutReadProbeLayoutRow(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutReadProbeResult& Result,
	const FSQLUILayoutDocument& Document)
{
	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("INSERT INTO layouts (")
		TEXT("layout_id, display_name, description, created_by, created_at_utc, updated_at_utc, ")
		TEXT("current_revision, schema_version, search_text, b_deleted")
		TEXT(") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 0);"));

	if (!Statement.IsValid())
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: could not prepare layout seed insert. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	const FString SearchText = MakeSQLUILayoutReadProbeSearchText(Document);
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
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: could not insert layout seed row. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	return true;
}

bool InsertSQLUILayoutReadProbeRevisionRow(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutReadProbeResult& Result,
	const FSQLUILayoutDocument& Document,
	const FString& DocumentJson)
{
	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("INSERT INTO layout_revisions (")
		TEXT("layout_id, revision, label, document_json, document_hash, created_by, created_at_utc, change_note")
		TEXT(") VALUES (?, ?, ?, ?, ?, ?, ?, ?);"));

	if (!Statement.IsValid())
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: could not prepare revision seed insert. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	if (!Statement.SetBindingValueByIndex(1, Document.Metadata.LayoutId)
		|| !Statement.SetBindingValueByIndex(2, Document.Version.Revision)
		|| !Statement.SetBindingValueByIndex(3, Document.Version.Label)
		|| !Statement.SetBindingValueByIndex(4, DocumentJson)
		|| !Statement.SetBindingValueByIndex(5, FString())
		|| !Statement.SetBindingValueByIndex(6, Document.Metadata.CreatedBy)
		|| !Statement.SetBindingValueByIndex(7, Document.Metadata.CreatedAtUtc)
		|| !Statement.SetBindingValueByIndex(8, FString(TEXT("Smoke/probe seed for SQLite read mapping.")))
		|| !Statement.Execute())
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: could not insert revision seed row. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	return true;
}

bool InsertSQLUILayoutReadProbeTagRows(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutReadProbeResult& Result,
	const FSQLUILayoutDocument& Document)
{
	for (const FString& Tag : Document.Metadata.Tags)
	{
		FSQLitePreparedStatement Statement = Database.PrepareStatement(
			TEXT("INSERT INTO layout_tags (layout_id, tag) VALUES (?, ?);"));

		if (!Statement.IsValid())
		{
			AppendSQLUILayoutReadProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite layout read probe failed: could not prepare tag seed insert. SQLiteCore error: %s"),
					*Database.GetLastError()));
			return false;
		}

		if (!Statement.SetBindingValueByIndex(1, Document.Metadata.LayoutId)
			|| !Statement.SetBindingValueByIndex(2, Tag)
			|| !Statement.Execute())
		{
			AppendSQLUILayoutReadProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite layout read probe failed: could not insert tag seed row '%s'. SQLiteCore error: %s"),
					*Tag,
					*Database.GetLastError()));
			return false;
		}
	}

	return true;
}

bool SeedSQLUILayoutReadProbeDocument(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutReadProbeResult& Result,
	const FSQLUILayoutDocument& Document,
	const FString& DocumentJson)
{
	if (!ExecuteSQLUILayoutReadProbeStatement(Database, Result, TEXT("BEGIN TRANSACTION;"), TEXT("begin seed transaction")))
	{
		return false;
	}

	if (!InsertSQLUILayoutReadProbeLayoutRow(Database, Result, Document)
		|| !InsertSQLUILayoutReadProbeRevisionRow(Database, Result, Document, DocumentJson)
		|| !InsertSQLUILayoutReadProbeTagRows(Database, Result, Document))
	{
		Database.Execute(TEXT("ROLLBACK;"));
		return false;
	}

	if (!ExecuteSQLUILayoutReadProbeStatement(Database, Result, TEXT("COMMIT;"), TEXT("commit seed transaction")))
	{
		Database.Execute(TEXT("ROLLBACK;"));
		return false;
	}

	Result.bSeedInserted = true;
	return true;
}

bool DoesSQLUILayoutReadProbeMetadataMatch(
	const FSQLUILayoutMetadata& Candidate,
	const FSQLUILayoutMetadata& Expected)
{
	TSet<FString> CandidateTags;
	for (const FString& Tag : Candidate.Tags)
	{
		CandidateTags.Add(Tag);
	}

	TSet<FString> ExpectedTags;
	for (const FString& Tag : Expected.Tags)
	{
		ExpectedTags.Add(Tag);
	}

	bool bTagsMatch = CandidateTags.Num() == ExpectedTags.Num();
	if (bTagsMatch)
	{
		for (const FString& Tag : ExpectedTags)
		{
			if (!CandidateTags.Contains(Tag))
			{
				bTagsMatch = false;
				break;
			}
		}
	}

	return Candidate.LayoutId == Expected.LayoutId
		&& Candidate.DisplayName == Expected.DisplayName
		&& Candidate.Description == Expected.Description
		&& Candidate.CreatedBy == Expected.CreatedBy
		&& bTagsMatch;
}

bool ListSQLUILayoutReadProbeMetadata(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutReadProbeResult& Result,
	const FSQLUILayoutDocument& SeedDocument)
{
	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("SELECT layout_id, display_name, description, created_by, created_at_utc, updated_at_utc, ")
		TEXT("COALESCE((SELECT group_concat(tag, char(31)) FROM layout_tags WHERE layout_tags.layout_id = layouts.layout_id), '') ")
		TEXT("FROM layouts ")
		TEXT("WHERE b_deleted = 0 ")
		TEXT("ORDER BY display_name COLLATE NOCASE ASC, layout_id COLLATE NOCASE ASC;"));

	if (!Statement.IsValid())
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: could not prepare metadata list query. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	const int64 RowCount = Statement.Execute(
		[&Result, &SeedDocument](const FSQLitePreparedStatement& Row)
		{
			FSQLUILayoutMetadata Metadata;
			FString JoinedTags;
			const bool bReadRow =
				Row.GetColumnValueByIndex(0, Metadata.LayoutId)
				&& Row.GetColumnValueByIndex(1, Metadata.DisplayName)
				&& Row.GetColumnValueByIndex(2, Metadata.Description)
				&& Row.GetColumnValueByIndex(3, Metadata.CreatedBy)
				&& Row.GetColumnValueByIndex(4, Metadata.CreatedAtUtc)
				&& Row.GetColumnValueByIndex(5, Metadata.UpdatedAtUtc)
				&& Row.GetColumnValueByIndex(6, JoinedTags);

			if (!bReadRow)
			{
				return ESQLitePreparedStatementExecuteRowResult::Error;
			}

			if (!JoinedTags.IsEmpty())
			{
				FString TagSeparator;
				TagSeparator.AppendChar(static_cast<TCHAR>(31));
				JoinedTags.ParseIntoArray(Metadata.Tags, *TagSeparator, true);
			}

			Result.ListedLayoutCount += 1;
			if (DoesSQLUILayoutReadProbeMetadataMatch(Metadata, SeedDocument.Metadata))
			{
				Result.bListedMetadataFound = true;
			}

			return ESQLitePreparedStatementExecuteRowResult::Continue;
		});

	Result.bListSucceeded = RowCount != INDEX_NONE;
	if (!Result.bListSucceeded)
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: metadata list query failed. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	if (!Result.bListedMetadataFound)
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: list query did not include seeded metadata and tags for layout id '%s'."),
				*SeedDocument.Metadata.LayoutId));
		return false;
	}

	return true;
}

bool LoadSQLUILayoutReadProbeDocument(
	FSQLiteDatabase& Database,
	FSQLUISQLiteLayoutReadProbeResult& Result,
	const FSQLUILayoutDocument& SeedDocument)
{
	FSQLitePreparedStatement Statement = Database.PrepareStatement(
		TEXT("SELECT layout_revisions.document_json ")
		TEXT("FROM layouts ")
		TEXT("INNER JOIN layout_revisions ")
		TEXT("ON layout_revisions.layout_id = layouts.layout_id ")
		TEXT("AND layout_revisions.revision = layouts.current_revision ")
		TEXT("WHERE layouts.layout_id = ? AND layouts.b_deleted = 0;"));

	if (!Statement.IsValid())
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: could not prepare layout load query. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	if (!Statement.SetBindingValueByIndex(1, SeedDocument.Metadata.LayoutId))
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: could not bind layout load query. SQLiteCore error: %s"),
				*Database.GetLastError()));
		return false;
	}

	FString LoadedDocumentJson;
	const int64 RowCount = Statement.Execute(
		[&LoadedDocumentJson](const FSQLitePreparedStatement& Row)
		{
			return Row.GetColumnValueByIndex(0, LoadedDocumentJson)
				? ESQLitePreparedStatementExecuteRowResult::Continue
				: ESQLitePreparedStatementExecuteRowResult::Error;
		});

	Result.bLoadSucceeded = RowCount == 1 && !LoadedDocumentJson.IsEmpty();
	if (!Result.bLoadSucceeded)
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: layout load query returned %lld row(s). SQLiteCore error: %s"),
				RowCount,
				*Database.GetLastError()));
		return false;
	}

	FSQLUILayoutDocument LoadedDocument;
	FSQLUILayoutValidationResult Validation;
	Result.bLoadedDocumentValid =
		FSQLUILayoutJson::FromJsonString(LoadedDocumentJson, LoadedDocument, Validation);
	Result.LoadedLayoutId = LoadedDocument.Metadata.LayoutId;

	if (!Result.bLoadedDocumentValid)
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: loaded document is invalid. ValidationErrors=%s"),
				*FString::Join(Validation.Errors, TEXT(", "))));
		return false;
	}

	if (Result.LoadedLayoutId != SeedDocument.Metadata.LayoutId)
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: loaded layout id '%s' did not match seeded layout id '%s'."),
				*Result.LoadedLayoutId,
				*SeedDocument.Metadata.LayoutId));
		Result.bLoadedDocumentValid = false;
		return false;
	}

	return true;
}
}

FString FSQLUISQLiteLayoutReadProbe::GetDefaultProbeDatabasePath()
{
	return NormalizeSQLUILayoutReadProbePath(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("SmokeTests"),
		TEXT("LayoutReadProbe"),
		TEXT("LayoutReadProbe.db")));
}

FSQLUISQLiteLayoutReadProbeResult FSQLUISQLiteLayoutReadProbe::RunProbe(
	const FString& DatabasePath,
	const bool bRemoveDatabaseAfterClose)
{
	FSQLUISQLiteLayoutReadProbeResult Result;
	Result.DatabasePath = DatabasePath.IsEmpty()
		? GetDefaultProbeDatabasePath()
		: NormalizeSQLUILayoutReadProbePath(DatabasePath);

	const FSQLUILayoutDocument SeedDocument = MakeSQLUILayoutReadProbeDocument();
	Result.SeedLayoutId = SeedDocument.Metadata.LayoutId;

	FSQLUILayoutValidationResult SeedValidation;
	FString SeedDocumentJson;
	if (!FSQLUILayoutJson::ToJsonString(SeedDocument, SeedDocumentJson, SeedValidation))
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: seed document is invalid. ValidationErrors=%s"),
				*FString::Join(SeedValidation.Errors, TEXT(", "))));
		return Result;
	}

	if (!DeleteSQLUILayoutReadProbeFiles(Result.DatabasePath, Result))
	{
		return Result;
	}

	const FSQLUISQLiteLayoutSchemaMigrationProbeResult SchemaResult =
		FSQLUISQLiteLayoutSchemaMigration::RunProbe(Result.DatabasePath, false);
	Result.bSchemaMigrationSucceeded = SchemaResult.bSucceeded;
	if (!SchemaResult.ErrorMessage.IsEmpty())
	{
		AppendSQLUILayoutReadProbeError(Result, SchemaResult.ErrorMessage);
	}

	if (!Result.bSchemaMigrationSucceeded)
	{
		if (bRemoveDatabaseAfterClose)
		{
			Result.bDatabaseRemoved = DeleteSQLUILayoutReadProbeFiles(Result.DatabasePath, Result);
		}
		return Result;
	}

	FSQLiteDatabase Database;
	if (!Database.Open(*Result.DatabasePath, ESQLiteDatabaseOpenMode::ReadWrite))
	{
		AppendSQLUILayoutReadProbeError(
			Result,
			FString::Printf(
				TEXT("SQLUI SQLite layout read probe failed: could not reopen database '%s'. SQLiteCore error: %s"),
				*Result.DatabasePath,
				*Database.GetLastError()));
	}
	else
	{
		const bool bSeeded = SeedSQLUILayoutReadProbeDocument(
			Database,
			Result,
			SeedDocument,
			SeedDocumentJson);
		const bool bListed = bSeeded && ListSQLUILayoutReadProbeMetadata(
			Database,
			Result,
			SeedDocument);
		const bool bLoaded = bListed && LoadSQLUILayoutReadProbeDocument(
			Database,
			Result,
			SeedDocument);

		if (!Database.Close())
		{
			AppendSQLUILayoutReadProbeError(
				Result,
				FString::Printf(
					TEXT("SQLUI SQLite layout read probe failed: could not close database '%s'. SQLiteCore error: %s"),
					*Result.DatabasePath,
					*Database.GetLastError()));
		}

		Result.bSucceeded =
			bSeeded
			&& bListed
			&& bLoaded
			&& Result.bSchemaMigrationSucceeded
			&& Result.bSeedInserted
			&& Result.bListSucceeded
			&& Result.bListedMetadataFound
			&& Result.bLoadSucceeded
			&& Result.bLoadedDocumentValid
			&& Result.LoadedLayoutId == Result.SeedLayoutId;
	}

	if (bRemoveDatabaseAfterClose)
	{
		Result.bDatabaseRemoved = DeleteSQLUILayoutReadProbeFiles(Result.DatabasePath, Result);
		Result.bSucceeded = Result.bSucceeded && Result.bDatabaseRemoved;
	}

	return Result;
}
