#include "Layout/SQLUILayoutWidgetFactory.h"

#include "Layout/SQLUILayoutJson.h"

namespace
{
void AddSQLUIWidgetBuildCatalogError(FSQLUIWidgetBuildResult& Result, const FString& ErrorMessage)
{
	Result.CatalogValidation.bIsValid = false;
	Result.CatalogValidation.Errors.Add(ErrorMessage);
	Result.ErrorMessage = ErrorMessage;
}

UClass* ResolveSQLUIWidgetClass(const FString& WidgetClassPath)
{
	UClass* WidgetClass = FindObject<UClass>(nullptr, *WidgetClassPath);
	if (!WidgetClass)
	{
		WidgetClass = LoadObject<UClass>(nullptr, *WidgetClassPath);
	}

	return WidgetClass;
}

FSQLUIWidgetBuildResult PrepareSQLUILayoutWidgetBuild(const FSQLUIWidgetBuildRequest& Request)
{
	FSQLUIWidgetBuildResult Result;
	Result.RootWidgetId = Request.Document.RootWidgetId;

	if (!Request.RuntimeContext)
	{
		Result.ErrorMessage = TEXT("SQLUI layout widget factory requires a runtime context before preparing a widget build.");
		return Result;
	}

	if (!Request.RuntimeContext->IsInitialized())
	{
		Result.ErrorMessage = TEXT("SQLUI layout widget factory requires an initialized runtime context before preparing a widget build.");
		return Result;
	}

	USQLUIWidgetCatalog* WidgetCatalog = Request.WidgetCatalogOverride
		? Request.WidgetCatalogOverride.Get()
		: Request.RuntimeContext->GetWidgetCatalog();

	if (!WidgetCatalog)
	{
		AddSQLUIWidgetBuildCatalogError(
			Result,
			TEXT("SQLUI layout widget factory requires a widget catalog before preparing a widget build."));
		return Result;
	}

	Result.LayoutValidation = FSQLUILayoutJson::ValidateDocument(Request.Document);
	if (!Result.LayoutValidation.bIsValid)
	{
		Result.ErrorMessage = TEXT("SQLUI layout widget factory could not prepare an invalid layout document.");
		return Result;
	}

	Result.CatalogValidation = WidgetCatalog->ValidateLayoutDocument(Request.Document);
	if (!Result.CatalogValidation.bIsValid)
	{
		Result.ErrorMessage = TEXT("SQLUI layout widget factory could not prepare a layout that failed widget catalog validation.");
		return Result;
	}

	TArray<FSQLUIPreparedWidgetBuildNode> PreparedNodes;
	PreparedNodes.Reserve(Request.Document.Nodes.Num());

	for (const FSQLUILayoutNode& LayoutNode : Request.Document.Nodes)
	{
		const FSQLUIWidgetCatalogEntry* CatalogEntry = WidgetCatalog->FindEntry(LayoutNode.WidgetTypeKey);
		if (!CatalogEntry)
		{
			AddSQLUIWidgetBuildCatalogError(
				Result,
				FString::Printf(
					TEXT("SQLUI layout node '%s' references unknown widget type key '%s'."),
					*LayoutNode.WidgetId,
					*LayoutNode.WidgetTypeKey));
			return Result;
		}

		if (CatalogEntry->WidgetClassPath.IsEmpty())
		{
			AddSQLUIWidgetBuildCatalogError(
				Result,
				FString::Printf(
					TEXT("SQLUI widget type '%s' does not include a widget class path."),
					*CatalogEntry->WidgetTypeKey.Value));
			return Result;
		}

		UClass* WidgetClass = ResolveSQLUIWidgetClass(CatalogEntry->WidgetClassPath);
		if (!WidgetClass)
		{
			AddSQLUIWidgetBuildCatalogError(
				Result,
				FString::Printf(
					TEXT("SQLUI widget type '%s' could not resolve widget class path '%s'."),
					*CatalogEntry->WidgetTypeKey.Value,
					*CatalogEntry->WidgetClassPath));
			return Result;
		}

		if (!WidgetClass->IsChildOf(USQLUIBaseWidget::StaticClass()))
		{
			AddSQLUIWidgetBuildCatalogError(
				Result,
				FString::Printf(
					TEXT("SQLUI widget type '%s' resolved class '%s' is not a USQLUIBaseWidget."),
					*CatalogEntry->WidgetTypeKey.Value,
					*WidgetClass->GetPathName()));
			return Result;
		}

		FSQLUIPreparedWidgetBuildNode PreparedNode;
		PreparedNode.LayoutNode = LayoutNode;
		PreparedNode.CatalogEntry = *CatalogEntry;
		PreparedNode.WidgetClass = WidgetClass;
		PreparedNodes.Add(PreparedNode);
	}

	Result.PreparedNodes = MoveTemp(PreparedNodes);
	Result.bSucceeded = true;
	return Result;
}
}

FSQLUIWidgetBuildResult USQLUILayoutWidgetBuilder::PrepareBuild(const FSQLUIWidgetBuildRequest& Request) const
{
	return PrepareSQLUILayoutWidgetBuild(Request);
}

FSQLUIWidgetBuildResult USQLUILayoutWidgetFactory::PrepareBuild(const FSQLUIWidgetBuildRequest& Request) const
{
	return PrepareSQLUILayoutWidgetBuild(Request);
}
