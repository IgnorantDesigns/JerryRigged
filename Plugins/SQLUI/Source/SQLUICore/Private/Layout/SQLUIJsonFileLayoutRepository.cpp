#include "Layout/SQLUIJsonFileLayoutRepository.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Layout/SQLUILayoutJson.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
const TCHAR* SQLUIJsonFileLayoutRepositoryEmptyLayoutIdMessage = TEXT("SQLUI JSON file layout repository requires a non-empty layout id.");
const TCHAR* SQLUIJsonFileLayoutRepositoryInvalidDocumentMessage = TEXT("SQLUI JSON file layout repository could not save an invalid layout document.");

void AddSQLUIJsonFileLayoutRepositoryValidationError(
	FSQLUILayoutValidationResult& Validation,
	const FString& ErrorMessage)
{
	Validation.bIsValid = false;
	Validation.Errors.Add(ErrorMessage);
}

FSQLUILayoutLoadResult MakeSQLUIJsonFileLayoutLoadFailure(
	const FString& LayoutId,
	const FString& ErrorMessage)
{
	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = false;
	Result.ErrorMessage = ErrorMessage;
	Result.Document.Metadata.LayoutId = LayoutId;
	return Result;
}

FSQLUILayoutLoadResult MakeSQLUIJsonFileLayoutLoadFailure(
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

FSQLUILayoutSaveResult MakeSQLUIJsonFileLayoutSaveFailure(
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

FSQLUILayoutRepositoryRemoveResult MakeSQLUIJsonFileLayoutRemoveFailure(
	const FString& LayoutId,
	const FString& ErrorMessage)
{
	FSQLUILayoutRepositoryRemoveResult Result;
	Result.bSucceeded = false;
	Result.ErrorMessage = ErrorMessage;
	Result.RemovedLayoutId = LayoutId;
	return Result;
}

FString ResolveSQLUIJsonFileLayoutRepositoryDefaultBaseDirectory()
{
	FString BaseDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("Layouts"));
	FPaths::NormalizeDirectoryName(BaseDirectory);
	return BaseDirectory;
}

FString NormalizeSQLUIJsonFileLayoutRepositoryDirectory(const FString& Directory)
{
	FString NormalizedDirectory = Directory;
	if (FPaths::IsRelative(NormalizedDirectory))
	{
		NormalizedDirectory = FPaths::ConvertRelativePathToFull(NormalizedDirectory);
	}

	FPaths::NormalizeDirectoryName(NormalizedDirectory);
	return NormalizedDirectory;
}

bool IsSQLUIJsonFileLayoutRepositorySafeLayoutId(const FString& LayoutId)
{
	if (LayoutId.IsEmpty())
	{
		return false;
	}

	if (LayoutId == TEXT(".") || LayoutId == TEXT(".."))
	{
		return false;
	}

	if (LayoutId.Contains(TEXT("/")) || LayoutId.Contains(TEXT("\\")))
	{
		return false;
	}

	return FPaths::MakeValidFileName(LayoutId) == LayoutId;
}

FString MakeSQLUIJsonFileLayoutRepositoryInvalidLayoutIdMessage(const FString& LayoutId)
{
	return FString::Printf(
		TEXT("SQLUI JSON file layout repository cannot use layout id '%s' as a file name."),
		*LayoutId);
}

FString MakeSQLUIJsonFileLayoutRepositoryFilePath(
	const FString& BaseDirectory,
	const FString& LayoutId)
{
	return FPaths::Combine(BaseDirectory, LayoutId + TEXT(".json"));
}

bool EnsureSQLUIJsonFileLayoutRepositoryDirectory(
	const FString& BaseDirectory,
	FString& OutErrorMessage)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.DirectoryExists(*BaseDirectory))
	{
		return true;
	}

	if (PlatformFile.CreateDirectoryTree(*BaseDirectory))
	{
		return true;
	}

	OutErrorMessage = FString::Printf(
		TEXT("SQLUI JSON file layout repository could not create directory '%s'."),
		*BaseDirectory);
	return false;
}

bool TryReadSQLUIJsonFileLayoutDocument(
	const FString& FilePath,
	FString& OutJson,
	FString& OutErrorMessage)
{
	OutJson.Empty();
	if (FFileHelper::LoadFileToString(OutJson, *FilePath))
	{
		return true;
	}

	OutErrorMessage = FString::Printf(
		TEXT("SQLUI JSON file layout repository could not read '%s'."),
		*FilePath);
	return false;
}

void SortSQLUILayoutMetadataByDisplayName(TArray<FSQLUILayoutMetadata>& Layouts)
{
	Layouts.Sort([](const FSQLUILayoutMetadata& Left, const FSQLUILayoutMetadata& Right)
	{
		const int32 DisplayNameCompare = Left.DisplayName.Compare(Right.DisplayName, ESearchCase::IgnoreCase);
		if (DisplayNameCompare != 0)
		{
			return DisplayNameCompare < 0;
		}

		return Left.LayoutId.Compare(Right.LayoutId, ESearchCase::IgnoreCase) < 0;
	});
}
}

void USQLUIJsonFileLayoutRepository::LoadLayout(
	const FString& LayoutId,
	FSQLUILayoutLoadCompleteDelegate Callback)
{
	Callback.ExecuteIfBound(LoadLayoutById(LayoutId));
}

void USQLUIJsonFileLayoutRepository::SaveLayout(
	const FSQLUILayoutDocument& Document,
	FSQLUILayoutSaveCompleteDelegate Callback)
{
	const FString& LayoutId = Document.Metadata.LayoutId;
	FString Json;
	FSQLUILayoutValidationResult Validation;

	if (!FSQLUILayoutJson::ToJsonString(Document, Json, Validation))
	{
		Callback.ExecuteIfBound(MakeSQLUIJsonFileLayoutSaveFailure(
			LayoutId,
			SQLUIJsonFileLayoutRepositoryInvalidDocumentMessage,
			Validation));
		return;
	}

	FString FilePath;
	FString ErrorMessage;
	if (!TryResolveLayoutFilePath(LayoutId, FilePath, ErrorMessage))
	{
		AddSQLUIJsonFileLayoutRepositoryValidationError(Validation, ErrorMessage);
		Callback.ExecuteIfBound(MakeSQLUIJsonFileLayoutSaveFailure(
			LayoutId,
			ErrorMessage,
			Validation));
		return;
	}

	const FString BaseDirectory = ResolveBaseDirectory();
	if (!EnsureSQLUIJsonFileLayoutRepositoryDirectory(BaseDirectory, ErrorMessage))
	{
		Callback.ExecuteIfBound(MakeSQLUIJsonFileLayoutSaveFailure(
			LayoutId,
			ErrorMessage,
			Validation));
		return;
	}

	if (!FFileHelper::SaveStringToFile(Json, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		ErrorMessage = FString::Printf(
			TEXT("SQLUI JSON file layout repository could not write '%s'."),
			*FilePath);
		Callback.ExecuteIfBound(MakeSQLUIJsonFileLayoutSaveFailure(
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

void USQLUIJsonFileLayoutRepository::Configure(
	const FSQLUIJsonFileLayoutRepositorySettings& InSettings)
{
	Settings = InSettings;
}

FSQLUIJsonFileLayoutRepositorySettings USQLUIJsonFileLayoutRepository::GetSettings() const
{
	return Settings;
}

FString USQLUIJsonFileLayoutRepository::GetResolvedBaseDirectory() const
{
	return ResolveBaseDirectory();
}

FSQLUILayoutLoadResult USQLUIJsonFileLayoutRepository::LoadLayoutById(const FString& LayoutId) const
{
	FString FilePath;
	FString ErrorMessage;
	if (!TryResolveLayoutFilePath(LayoutId, FilePath, ErrorMessage))
	{
		return MakeSQLUIJsonFileLayoutLoadFailure(LayoutId, ErrorMessage);
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*FilePath))
	{
		return MakeSQLUIJsonFileLayoutLoadFailure(
			LayoutId,
			FString::Printf(TEXT("SQLUI JSON file layout repository could not find layout id '%s'."), *LayoutId));
	}

	FString Json;
	if (!TryReadSQLUIJsonFileLayoutDocument(FilePath, Json, ErrorMessage))
	{
		return MakeSQLUIJsonFileLayoutLoadFailure(LayoutId, ErrorMessage);
	}

	FSQLUILayoutDocument Document;
	FSQLUILayoutValidationResult Validation;
	if (!FSQLUILayoutJson::FromJsonString(Json, Document, Validation))
	{
		return MakeSQLUIJsonFileLayoutLoadFailure(
			LayoutId,
			TEXT("SQLUI JSON file layout repository could not load an invalid layout document."),
			Document,
			Validation);
	}

	if (Document.Metadata.LayoutId != LayoutId)
	{
		ErrorMessage = FString::Printf(
			TEXT("SQLUI JSON file layout repository loaded layout id '%s' from file for requested id '%s'."),
			*Document.Metadata.LayoutId,
			*LayoutId);
		AddSQLUIJsonFileLayoutRepositoryValidationError(Validation, ErrorMessage);
		return MakeSQLUIJsonFileLayoutLoadFailure(LayoutId, ErrorMessage, Document, Validation);
	}

	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = true;
	Result.Document = Document;
	Result.Validation = Validation;
	return Result;
}

FSQLUILayoutRepositoryRemoveResult USQLUIJsonFileLayoutRepository::RemoveLayout(const FString& LayoutId)
{
	FString FilePath;
	FString ErrorMessage;
	if (!TryResolveLayoutFilePath(LayoutId, FilePath, ErrorMessage))
	{
		return MakeSQLUIJsonFileLayoutRemoveFailure(LayoutId, ErrorMessage);
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	FSQLUILayoutRepositoryRemoveResult Result;
	Result.RemovedLayoutId = LayoutId;

	if (!PlatformFile.FileExists(*FilePath))
	{
		Result.bSucceeded = true;
		Result.bRemoved = false;
		Result.ErrorMessage = FString::Printf(
			TEXT("SQLUI JSON file layout repository did not contain layout id '%s'."),
			*LayoutId);
		return Result;
	}

	if (!PlatformFile.DeleteFile(*FilePath))
	{
		Result.bSucceeded = false;
		Result.bRemoved = false;
		Result.ErrorMessage = FString::Printf(
			TEXT("SQLUI JSON file layout repository could not delete '%s'."),
			*FilePath);
		return Result;
	}

	Result.bSucceeded = true;
	Result.bRemoved = true;
	return Result;
}

FSQLUILayoutRepositoryListResult USQLUIJsonFileLayoutRepository::ListLayouts() const
{
	FSQLUILayoutRepositoryListResult Result;
	Result.bSucceeded = true;

	const FString BaseDirectory = ResolveBaseDirectory();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*BaseDirectory))
	{
		return Result;
	}

	TArray<FString> FileNames;
	IFileManager::Get().FindFiles(
		FileNames,
		*FPaths::Combine(BaseDirectory, TEXT("*.json")),
		true,
		false);

	Result.Layouts.Reserve(FileNames.Num());
	for (const FString& FileName : FileNames)
	{
		FString LayoutId = FPaths::GetCleanFilename(FileName);
		LayoutId.RemoveFromEnd(TEXT(".json"), ESearchCase::IgnoreCase);

		const FSQLUILayoutLoadResult LoadResult = LoadLayoutById(LayoutId);
		if (!LoadResult.bSucceeded)
		{
			Result.bSucceeded = false;
			Result.ErrorMessage = LoadResult.ErrorMessage;
			return Result;
		}

		Result.Layouts.Add(LoadResult.Document.Metadata);
	}

	SortSQLUILayoutMetadataByDisplayName(Result.Layouts);
	return Result;
}

FSQLUILayoutRepositoryClearResult USQLUIJsonFileLayoutRepository::ClearLayouts()
{
	FSQLUILayoutRepositoryClearResult Result;

	const FString BaseDirectory = ResolveBaseDirectory();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*BaseDirectory))
	{
		Result.bSucceeded = true;
		return Result;
	}

	TArray<FString> FileNames;
	IFileManager::Get().FindFiles(
		FileNames,
		*FPaths::Combine(BaseDirectory, TEXT("*.json")),
		true,
		false);

	for (const FString& FileName : FileNames)
	{
		const FString FilePath = FPaths::Combine(BaseDirectory, FPaths::GetCleanFilename(FileName));
		if (!PlatformFile.DeleteFile(*FilePath))
		{
			Result.bSucceeded = false;
			Result.ErrorMessage = FString::Printf(
				TEXT("SQLUI JSON file layout repository could not delete '%s'."),
				*FilePath);
			return Result;
		}

		++Result.RemovedCount;
	}

	Result.bSucceeded = true;
	return Result;
}

FString USQLUIJsonFileLayoutRepository::ResolveBaseDirectory() const
{
	if (Settings.BaseDirectory.IsEmpty())
	{
		return ResolveSQLUIJsonFileLayoutRepositoryDefaultBaseDirectory();
	}

	return NormalizeSQLUIJsonFileLayoutRepositoryDirectory(Settings.BaseDirectory);
}

bool USQLUIJsonFileLayoutRepository::TryResolveLayoutFilePath(
	const FString& LayoutId,
	FString& OutFilePath,
	FString& OutErrorMessage) const
{
	OutFilePath.Empty();
	OutErrorMessage.Empty();

	if (LayoutId.IsEmpty())
	{
		OutErrorMessage = SQLUIJsonFileLayoutRepositoryEmptyLayoutIdMessage;
		return false;
	}

	if (!IsSQLUIJsonFileLayoutRepositorySafeLayoutId(LayoutId))
	{
		OutErrorMessage = MakeSQLUIJsonFileLayoutRepositoryInvalidLayoutIdMessage(LayoutId);
		return false;
	}

	OutFilePath = MakeSQLUIJsonFileLayoutRepositoryFilePath(ResolveBaseDirectory(), LayoutId);
	return true;
}
