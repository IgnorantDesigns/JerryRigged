#include "Layout/SQLUILayoutRepository.h"

namespace
{
const TCHAR* SQLUILayoutRepositoryUnavailableMessage = TEXT("No concrete SQLUI layout repository backend is implemented yet.");
}

void USQLUILayoutRepository::LoadLayout(const FString& LayoutId, FSQLUILayoutLoadCompleteDelegate Callback)
{
	Callback.ExecuteIfBound(MakeBackendUnavailableLoadResult(LayoutId));
}

void USQLUILayoutRepository::SaveLayout(const FSQLUILayoutDocument& Document, FSQLUILayoutSaveCompleteDelegate Callback)
{
	Callback.ExecuteIfBound(MakeBackendUnavailableSaveResult(Document));
}

FSQLUILayoutRepositoryListResult USQLUILayoutRepository::ListLayouts() const
{
	FSQLUILayoutRepositoryListResult Result;
	Result.bSucceeded = false;
	Result.ErrorMessage = SQLUILayoutRepositoryUnavailableMessage;
	return Result;
}

FSQLUILayoutLoadResult USQLUILayoutRepository::MakeBackendUnavailableLoadResult(const FString& LayoutId)
{
	FSQLUILayoutLoadResult Result;
	Result.bSucceeded = false;
	Result.bBackendUnavailable = true;
	Result.ErrorMessage = SQLUILayoutRepositoryUnavailableMessage;
	Result.Document.Metadata.LayoutId = LayoutId;
	return Result;
}

FSQLUILayoutSaveResult USQLUILayoutRepository::MakeBackendUnavailableSaveResult(const FSQLUILayoutDocument& Document)
{
	FSQLUILayoutSaveResult Result;
	Result.bSucceeded = false;
	Result.bBackendUnavailable = true;
	Result.ErrorMessage = SQLUILayoutRepositoryUnavailableMessage;
	Result.SavedLayoutId = Document.Metadata.LayoutId;
	return Result;
}
