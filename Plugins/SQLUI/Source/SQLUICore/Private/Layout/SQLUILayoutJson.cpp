#include "Layout/SQLUILayoutJson.h"

#include "JsonObjectConverter.h"

namespace
{
void AddSQLUILayoutValidationError(FSQLUILayoutValidationResult& Result, const FString& ErrorMessage)
{
	Result.bIsValid = false;
	Result.Errors.Add(ErrorMessage);
}

void AddSQLUILayoutValidationWarning(FSQLUILayoutValidationResult& Result, const FString& WarningMessage)
{
	Result.Warnings.Add(WarningMessage);
}
}

bool FSQLUILayoutJson::ToJsonString(
	const FSQLUILayoutDocument& Document,
	FString& OutJson,
	FSQLUILayoutValidationResult& OutValidation)
{
	OutJson.Empty();
	OutValidation = ValidateDocument(Document);
	if (!OutValidation.bIsValid)
	{
		return false;
	}

	const bool bSerialized = FJsonObjectConverter::UStructToJsonObjectString(
		FSQLUILayoutDocument::StaticStruct(),
		&Document,
		OutJson,
		0,
		0);

	if (!bSerialized)
	{
		AddSQLUILayoutValidationError(OutValidation, TEXT("Failed to serialize SQLUI layout document to JSON."));
	}

	return bSerialized && OutValidation.bIsValid;
}

bool FSQLUILayoutJson::FromJsonString(
	const FString& Json,
	FSQLUILayoutDocument& OutDocument,
	FSQLUILayoutValidationResult& OutValidation)
{
	OutDocument = FSQLUILayoutDocument();
	OutValidation = FSQLUILayoutValidationResult();

	if (Json.IsEmpty())
	{
		AddSQLUILayoutValidationError(OutValidation, TEXT("Cannot deserialize an empty SQLUI layout JSON string."));
		return false;
	}

	const bool bDeserialized = FJsonObjectConverter::JsonObjectStringToUStruct(
		Json,
		&OutDocument,
		0,
		0);

	if (!bDeserialized)
	{
		AddSQLUILayoutValidationError(OutValidation, TEXT("Failed to deserialize SQLUI layout JSON string."));
		return false;
	}

	OutValidation = ValidateDocument(OutDocument);
	return OutValidation.bIsValid;
}

FSQLUILayoutValidationResult FSQLUILayoutJson::ValidateDocument(const FSQLUILayoutDocument& Document)
{
	FSQLUILayoutValidationResult Result;

	if (Document.Version.SchemaVersion <= 0)
	{
		AddSQLUILayoutValidationError(Result, TEXT("SQLUI layout schema version must be greater than zero."));
	}

	if (Document.Version.Revision <= 0)
	{
		AddSQLUILayoutValidationError(Result, TEXT("SQLUI layout revision must be greater than zero."));
	}

	if (Document.Metadata.LayoutId.IsEmpty())
	{
		AddSQLUILayoutValidationError(Result, TEXT("SQLUI layout metadata must include a layout id."));
	}

	if (Document.RootWidgetId.IsEmpty())
	{
		AddSQLUILayoutValidationError(Result, TEXT("SQLUI layout document must include a root widget id."));
	}

	if (Document.Nodes.IsEmpty())
	{
		AddSQLUILayoutValidationError(Result, TEXT("SQLUI layout document must include at least one widget node."));
	}

	TMap<FString, const FSQLUILayoutNode*> NodesById;
	for (const FSQLUILayoutNode& Node : Document.Nodes)
	{
		if (Node.WidgetId.IsEmpty())
		{
			AddSQLUILayoutValidationError(Result, TEXT("SQLUI layout node must include a widget id."));
			continue;
		}

		if (NodesById.Contains(Node.WidgetId))
		{
			AddSQLUILayoutValidationError(Result, FString::Printf(TEXT("SQLUI layout node id '%s' is duplicated."), *Node.WidgetId));
		}
		else
		{
			NodesById.Add(Node.WidgetId, &Node);
		}

		if (Node.WidgetTypeKey.IsEmpty())
		{
			AddSQLUILayoutValidationError(Result, FString::Printf(TEXT("SQLUI layout node '%s' must include a widget type key."), *Node.WidgetId));
		}
	}

	if (!Document.RootWidgetId.IsEmpty() && !NodesById.Contains(Document.RootWidgetId))
	{
		AddSQLUILayoutValidationError(Result, FString::Printf(TEXT("SQLUI layout root widget id '%s' does not match any node."), *Document.RootWidgetId));
	}

	for (const FSQLUILayoutNode& Node : Document.Nodes)
	{
		if (Node.WidgetId.IsEmpty())
		{
			continue;
		}

		if (!Node.ParentWidgetId.IsEmpty())
		{
			const FSQLUILayoutNode* const* ParentNode = NodesById.Find(Node.ParentWidgetId);
			if (!ParentNode)
			{
				AddSQLUILayoutValidationError(Result, FString::Printf(TEXT("SQLUI layout node '%s' references missing parent '%s'."), *Node.WidgetId, *Node.ParentWidgetId));
			}
			else if (!(*ParentNode)->ChildWidgetIds.Contains(Node.WidgetId))
			{
				AddSQLUILayoutValidationWarning(Result, FString::Printf(TEXT("SQLUI layout parent '%s' does not list child '%s'."), *Node.ParentWidgetId, *Node.WidgetId));
			}
		}

		TSet<FString> SeenChildIds;
		for (const FString& ChildWidgetId : Node.ChildWidgetIds)
		{
			if (ChildWidgetId.IsEmpty())
			{
				AddSQLUILayoutValidationError(Result, FString::Printf(TEXT("SQLUI layout node '%s' contains an empty child widget id."), *Node.WidgetId));
				continue;
			}

			if (ChildWidgetId == Node.WidgetId)
			{
				AddSQLUILayoutValidationError(Result, FString::Printf(TEXT("SQLUI layout node '%s' cannot be its own child."), *Node.WidgetId));
			}

			if (SeenChildIds.Contains(ChildWidgetId))
			{
				AddSQLUILayoutValidationError(Result, FString::Printf(TEXT("SQLUI layout node '%s' lists child '%s' more than once."), *Node.WidgetId, *ChildWidgetId));
			}
			else
			{
				SeenChildIds.Add(ChildWidgetId);
			}

			const FSQLUILayoutNode* const* ChildNode = NodesById.Find(ChildWidgetId);
			if (!ChildNode)
			{
				AddSQLUILayoutValidationError(Result, FString::Printf(TEXT("SQLUI layout node '%s' references missing child '%s'."), *Node.WidgetId, *ChildWidgetId));
			}
			else if (!(*ChildNode)->ParentWidgetId.IsEmpty() && (*ChildNode)->ParentWidgetId != Node.WidgetId)
			{
				AddSQLUILayoutValidationError(Result, FString::Printf(TEXT("SQLUI layout node '%s' lists child '%s', but that child references parent '%s'."), *Node.WidgetId, *ChildWidgetId, *(*ChildNode)->ParentWidgetId));
			}
			else if ((*ChildNode)->ParentWidgetId.IsEmpty())
			{
				AddSQLUILayoutValidationWarning(Result, FString::Printf(TEXT("SQLUI layout child '%s' does not reference parent '%s'."), *ChildWidgetId, *Node.WidgetId));
			}
		}
	}

	return Result;
}
