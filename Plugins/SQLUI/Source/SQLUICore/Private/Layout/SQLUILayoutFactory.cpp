#include "Layout/SQLUILayoutFactory.h"

#include "Layout/SQLUILayoutJson.h"

namespace
{
void AddSQLUILayoutFactoryCatalogError(FSQLUIWidgetCatalogValidationResult& Result, const FString& ErrorMessage)
{
	Result.bIsValid = false;
	Result.Errors.Add(ErrorMessage);
}
}

FSQLUILayoutFactoryResult USQLUILayoutFactory::PrepareLayout(const FSQLUILayoutFactoryRequest& Request) const
{
	FSQLUILayoutFactoryResult Result;
	Result.PreparedDocument = Request.Document;
	Result.LayoutValidation = FSQLUILayoutJson::ValidateDocument(Request.Document);

	if (!Result.LayoutValidation.bIsValid)
	{
		Result.ErrorMessage = TEXT("SQLUI layout factory could not prepare an invalid layout document.");
		return Result;
	}

	if (!Request.WidgetCatalog)
	{
		AddSQLUILayoutFactoryCatalogError(
			Result.CatalogValidation,
			TEXT("SQLUI layout factory requires a widget catalog before preparing a layout."));
		Result.ErrorMessage = TEXT("SQLUI layout factory could not prepare a layout without a widget catalog.");
		return Result;
	}

	Result.CatalogValidation = Request.WidgetCatalog->ValidateLayoutDocument(Request.Document);
	if (!Result.CatalogValidation.bIsValid)
	{
		Result.ErrorMessage = TEXT("SQLUI layout factory could not prepare a layout that failed widget catalog validation.");
		return Result;
	}

	Result.bSucceeded = true;
	return Result;
}
