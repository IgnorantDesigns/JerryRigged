#include "Layout/SQLUIInMemoryLayoutRepository.h"

#include "Layout/SQLUILayoutJson.h"

namespace
{
const TCHAR* SQLUIInMemoryLayoutRepositoryEmptyLayoutIdMessage = TEXT("SQLUI in-memory layout repository requires a non-empty layout id.");
const TCHAR* SQLUIInMemoryLayoutRepositoryInvalidDocumentMessage = TEXT("SQLUI in-memory layout repository could not save an invalid layout document.");

void AddSQLUILayoutRepositoryValidationError(
	FSQLUILayoutValidationResult& Validation,
	const FString& ErrorMessage)
{
	Validation.bIsValid = false;
	Validation.Errors.Add(ErrorMessage);
}

FSQLUILayoutLoadResult MakeSQLUIInMemoryLayoutLoadFailure(
	const FString& LayoutId,
	const FString& ErrorMessage)
{
	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = false;
	Result.ErrorMessage = ErrorMessage;
	Result.Document.Metadata.LayoutId = LayoutId;
	return Result;
}

FSQLUILayoutSaveResult MakeSQLUIInMemoryLayoutSaveFailure(
	const FString& LayoutId,
	const FString& ErrorMessage,
	const FSQLUILayoutValidationResult& Validation)
{
	FSQLUILayoutSaveResult Result;
	Result.bSucceeded = false;
	Result.ErrorMessage = ErrorMessage;
	Result.SavedLayoutId = LayoutId;
	Result.Validation = Validation;
	return Result;
}

FSQLUILayoutRepositoryRemoveResult MakeSQLUIInMemoryLayoutRemoveFailure(
	const FString& LayoutId,
	const FString& ErrorMessage)
{
	FSQLUILayoutRepositoryRemoveResult Result;
	Result.bSucceeded = false;
	Result.ErrorMessage = ErrorMessage;
	Result.RemovedLayoutId = LayoutId;
	return Result;
}
}

void USQLUIInMemoryLayoutRepository::LoadLayout(
	const FString& LayoutId,
	FSQLUILayoutLoadCompleteDelegate Callback)
{
	Callback.ExecuteIfBound(LoadLayoutById(LayoutId));
}

void USQLUIInMemoryLayoutRepository::SaveLayout(
	const FSQLUILayoutDocument& Document,
	FSQLUILayoutSaveCompleteDelegate Callback)
{
	const FString& LayoutId = Document.Metadata.LayoutId;
	FSQLUILayoutValidationResult Validation = FSQLUILayoutJson::ValidateDocument(Document);

	if (LayoutId.IsEmpty())
	{
		AddSQLUILayoutRepositoryValidationError(
			Validation,
			SQLUIInMemoryLayoutRepositoryEmptyLayoutIdMessage);
		Callback.ExecuteIfBound(MakeSQLUIInMemoryLayoutSaveFailure(
			LayoutId,
			SQLUIInMemoryLayoutRepositoryEmptyLayoutIdMessage,
			Validation));
		return;
	}

	if (!Validation.bIsValid)
	{
		Callback.ExecuteIfBound(MakeSQLUIInMemoryLayoutSaveFailure(
			LayoutId,
			SQLUIInMemoryLayoutRepositoryInvalidDocumentMessage,
			Validation));
		return;
	}

	LayoutsById.Add(LayoutId, Document);

	FSQLUILayoutSaveResult Result;
	Result.bSucceeded = true;
	Result.SavedLayoutId = LayoutId;
	Result.Validation = Validation;
	Callback.ExecuteIfBound(Result);
}

FSQLUILayoutLoadResult USQLUIInMemoryLayoutRepository::LoadLayoutById(const FString& LayoutId) const
{
	if (LayoutId.IsEmpty())
	{
		return MakeSQLUIInMemoryLayoutLoadFailure(
			LayoutId,
			SQLUIInMemoryLayoutRepositoryEmptyLayoutIdMessage);
	}

	const FSQLUILayoutDocument* StoredDocument = LayoutsById.Find(LayoutId);
	if (!StoredDocument)
	{
		return MakeSQLUIInMemoryLayoutLoadFailure(
			LayoutId,
			FString::Printf(TEXT("SQLUI in-memory layout repository could not find layout id '%s'."), *LayoutId));
	}

	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = true;
	Result.Document = *StoredDocument;
	Result.Validation = FSQLUILayoutJson::ValidateDocument(Result.Document);
	return Result;
}

FSQLUILayoutLoadResult USQLUIInMemoryLayoutRepository::LoadLayoutByDisplayName(const FString& DisplayName) const
{
	if (DisplayName.IsEmpty())
	{
		return MakeSQLUIInMemoryLayoutLoadFailure(
			FString(),
			TEXT("SQLUI in-memory layout repository requires a non-empty display name."));
	}

	const FSQLUILayoutDocument* MatchingDocument = nullptr;
	for (const TPair<FString, FSQLUILayoutDocument>& LayoutPair : LayoutsById)
	{
		if (LayoutPair.Value.Metadata.DisplayName != DisplayName)
		{
			continue;
		}

		if (!MatchingDocument ||
			LayoutPair.Value.Metadata.LayoutId.Compare(MatchingDocument->Metadata.LayoutId, ESearchCase::IgnoreCase) < 0)
		{
			MatchingDocument = &LayoutPair.Value;
		}
	}

	if (MatchingDocument)
	{
		FSQLUILayoutLoadResult Result;
		Result.bSucceeded = true;
		Result.Document = *MatchingDocument;
		Result.Validation = FSQLUILayoutJson::ValidateDocument(Result.Document);
		return Result;
	}

	return MakeSQLUIInMemoryLayoutLoadFailure(
		FString(),
		FString::Printf(TEXT("SQLUI in-memory layout repository could not find display name '%s'."), *DisplayName));
}

FSQLUILayoutRepositoryRemoveResult USQLUIInMemoryLayoutRepository::RemoveLayout(const FString& LayoutId)
{
	if (LayoutId.IsEmpty())
	{
		return MakeSQLUIInMemoryLayoutRemoveFailure(
			LayoutId,
			SQLUIInMemoryLayoutRepositoryEmptyLayoutIdMessage);
	}

	FSQLUILayoutRepositoryRemoveResult Result;
	Result.RemovedLayoutId = LayoutId;
	Result.bRemoved = LayoutsById.Remove(LayoutId) > 0;
	Result.bSucceeded = true;

	if (!Result.bRemoved)
	{
		Result.ErrorMessage = FString::Printf(
			TEXT("SQLUI in-memory layout repository did not contain layout id '%s'."),
			*LayoutId);
	}

	return Result;
}

FSQLUILayoutRepositoryListResult USQLUIInMemoryLayoutRepository::ListLayouts() const
{
	FSQLUILayoutRepositoryListResult Result;
	Result.bSucceeded = true;
	Result.Layouts.Reserve(LayoutsById.Num());

	for (const TPair<FString, FSQLUILayoutDocument>& LayoutPair : LayoutsById)
	{
		Result.Layouts.Add(LayoutPair.Value.Metadata);
	}

	Result.Layouts.Sort([](const FSQLUILayoutMetadata& Left, const FSQLUILayoutMetadata& Right)
	{
		const int32 DisplayNameCompare = Left.DisplayName.Compare(Right.DisplayName, ESearchCase::IgnoreCase);
		if (DisplayNameCompare != 0)
		{
			return DisplayNameCompare < 0;
		}

		return Left.LayoutId.Compare(Right.LayoutId, ESearchCase::IgnoreCase) < 0;
	});

	return Result;
}

FSQLUILayoutRepositoryClearResult USQLUIInMemoryLayoutRepository::ClearLayouts()
{
	FSQLUILayoutRepositoryClearResult Result;
	Result.bSucceeded = true;
	Result.RemovedCount = LayoutsById.Num();
	LayoutsById.Empty();
	return Result;
}
