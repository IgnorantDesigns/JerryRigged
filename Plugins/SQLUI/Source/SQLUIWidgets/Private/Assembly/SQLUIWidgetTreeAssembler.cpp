#include "Assembly/SQLUIWidgetTreeAssembler.h"

namespace
{
void AddSQLUIWidgetTreeAssemblyError(FSQLUIWidgetTreeAssemblyResult& Result, const FString& ErrorMessage)
{
	Result.bSucceeded = false;
	Result.Errors.Add(ErrorMessage);

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = ErrorMessage;
	}
}

void AddSQLUIWidgetTreeAssemblyWarning(FSQLUIWidgetTreeAssemblyResult& Result, const FString& WarningMessage)
{
	Result.Warnings.Add(WarningMessage);
}

USQLUIBaseWidget* FindSQLUICreatedWidget(
	const FSQLUIWidgetTreeAssemblyRequest& Request,
	const FString& WidgetId)
{
	const TObjectPtr<USQLUIBaseWidget>* CreatedWidget = Request.CreatedWidgetMap.Find(WidgetId);
	return CreatedWidget ? CreatedWidget->Get() : nullptr;
}

void AddUniqueSQLUIAssemblyChild(
	TMap<FString, TArray<FString>>& ChildrenByParentId,
	const FString& ParentWidgetId,
	const FString& ChildWidgetId)
{
	TArray<FString>& ChildWidgetIds = ChildrenByParentId.FindOrAdd(ParentWidgetId);
	ChildWidgetIds.AddUnique(ChildWidgetId);
}

void ValidateSQLUIWidgetTreeAssemblyLayoutNodes(
	const FSQLUIWidgetTreeAssemblyRequest& Request,
	FSQLUIWidgetTreeAssemblyResult& Result,
	TMap<FString, int32>& OutNodeIndicesById)
{
	if (Request.LayoutNodes.IsEmpty())
	{
		AddSQLUIWidgetTreeAssemblyError(
			Result,
			TEXT("SQLUI widget tree assembly requires at least one layout node."));
		return;
	}

	for (int32 NodeIndex = 0; NodeIndex < Request.LayoutNodes.Num(); ++NodeIndex)
	{
		const FSQLUILayoutNode& LayoutNode = Request.LayoutNodes[NodeIndex];
		if (LayoutNode.WidgetId.IsEmpty())
		{
			AddSQLUIWidgetTreeAssemblyError(
				Result,
				FString::Printf(TEXT("SQLUI widget tree assembly found a layout node without a widget id at index %d."), NodeIndex));
			continue;
		}

		if (OutNodeIndicesById.Contains(LayoutNode.WidgetId))
		{
			AddSQLUIWidgetTreeAssemblyError(
				Result,
				FString::Printf(TEXT("SQLUI widget tree assembly found duplicate widget id '%s'."), *LayoutNode.WidgetId));
			continue;
		}

		OutNodeIndicesById.Add(LayoutNode.WidgetId, NodeIndex);
	}
}

FString ResolveSQLUIWidgetTreeAssemblyRootWidgetId(
	const FSQLUIWidgetTreeAssemblyRequest& Request,
	FSQLUIWidgetTreeAssemblyResult& Result,
	const TMap<FString, int32>& NodeIndicesById)
{
	if (!Request.RootWidgetId.IsEmpty())
	{
		return Request.RootWidgetId;
	}

	TArray<FString> ParentlessWidgetIds;
	for (const FSQLUILayoutNode& LayoutNode : Request.LayoutNodes)
	{
		if (!LayoutNode.WidgetId.IsEmpty() && NodeIndicesById.Contains(LayoutNode.WidgetId) && LayoutNode.ParentWidgetId.IsEmpty())
		{
			ParentlessWidgetIds.Add(LayoutNode.WidgetId);
		}
	}

	if (ParentlessWidgetIds.Num() == 1)
	{
		return ParentlessWidgetIds[0];
	}

	if (ParentlessWidgetIds.IsEmpty())
	{
		AddSQLUIWidgetTreeAssemblyError(
			Result,
			TEXT("SQLUI widget tree assembly could not infer a root widget because no parentless layout node exists."));
	}
	else
	{
		AddSQLUIWidgetTreeAssemblyError(
			Result,
			FString::Printf(
				TEXT("SQLUI widget tree assembly could not infer a root widget because %d parentless layout nodes exist."),
				ParentlessWidgetIds.Num()));
	}

	return FString();
}

void ValidateSQLUIWidgetTreeAssemblyCreatedWidgets(
	const FSQLUIWidgetTreeAssemblyRequest& Request,
	FSQLUIWidgetTreeAssemblyResult& Result,
	const TMap<FString, int32>& NodeIndicesById,
	const FString& ResolvedRootWidgetId)
{
	for (const TPair<FString, int32>& NodePair : NodeIndicesById)
	{
		if (!IsValid(FindSQLUICreatedWidget(Request, NodePair.Key)))
		{
			AddSQLUIWidgetTreeAssemblyError(
				Result,
				FString::Printf(TEXT("SQLUI widget tree assembly layout node '%s' does not have a valid created widget."), *NodePair.Key));
		}
	}

	if (ResolvedRootWidgetId.IsEmpty())
	{
		return;
	}

	if (!NodeIndicesById.Contains(ResolvedRootWidgetId))
	{
		AddSQLUIWidgetTreeAssemblyError(
			Result,
			FString::Printf(TEXT("SQLUI widget tree assembly root widget id '%s' does not match a layout node."), *ResolvedRootWidgetId));
	}

	if (!IsValid(FindSQLUICreatedWidget(Request, ResolvedRootWidgetId)))
	{
		AddSQLUIWidgetTreeAssemblyError(
			Result,
			FString::Printf(TEXT("SQLUI widget tree assembly root widget id '%s' does not have a valid created widget."), *ResolvedRootWidgetId));
	}
}

void ValidateSQLUIWidgetTreeAssemblyRelationships(
	const FSQLUIWidgetTreeAssemblyRequest& Request,
	FSQLUIWidgetTreeAssemblyResult& Result,
	const TMap<FString, int32>& NodeIndicesById,
	const FString& ResolvedRootWidgetId,
	TMap<FString, TArray<FString>>& OutChildrenByParentId)
{
	TMap<FString, FString> ListedParentByChildId;

	if (!ResolvedRootWidgetId.IsEmpty())
	{
		const int32* RootNodeIndex = NodeIndicesById.Find(ResolvedRootWidgetId);
		if (RootNodeIndex)
		{
			const FSQLUILayoutNode& RootNode = Request.LayoutNodes[*RootNodeIndex];
			if (!RootNode.ParentWidgetId.IsEmpty())
			{
				AddSQLUIWidgetTreeAssemblyError(
					Result,
					FString::Printf(
						TEXT("SQLUI widget tree assembly resolved root '%s' cannot also reference parent '%s'."),
						*ResolvedRootWidgetId,
						*RootNode.ParentWidgetId));
			}
		}
	}

	for (const FSQLUILayoutNode& LayoutNode : Request.LayoutNodes)
	{
		if (LayoutNode.WidgetId.IsEmpty() || !NodeIndicesById.Contains(LayoutNode.WidgetId))
		{
			continue;
		}

		if (!LayoutNode.ParentWidgetId.IsEmpty())
		{
			if (LayoutNode.ParentWidgetId == LayoutNode.WidgetId)
			{
				AddSQLUIWidgetTreeAssemblyError(
					Result,
					FString::Printf(TEXT("SQLUI widget tree assembly node '%s' cannot reference itself as its parent."), *LayoutNode.WidgetId));
			}
			else if (!NodeIndicesById.Contains(LayoutNode.ParentWidgetId))
			{
				AddSQLUIWidgetTreeAssemblyError(
					Result,
					FString::Printf(
						TEXT("SQLUI widget tree assembly node '%s' references missing parent '%s'."),
						*LayoutNode.WidgetId,
						*LayoutNode.ParentWidgetId));
			}
			else
			{
				AddUniqueSQLUIAssemblyChild(OutChildrenByParentId, LayoutNode.ParentWidgetId, LayoutNode.WidgetId);

				const FSQLUILayoutNode& ParentNode = Request.LayoutNodes[NodeIndicesById[LayoutNode.ParentWidgetId]];
				if (!ParentNode.ChildWidgetIds.Contains(LayoutNode.WidgetId))
				{
					AddSQLUIWidgetTreeAssemblyWarning(
						Result,
						FString::Printf(
							TEXT("SQLUI widget tree assembly node '%s' references parent '%s', but the parent does not list it as a child."),
							*LayoutNode.WidgetId,
							*LayoutNode.ParentWidgetId));
				}
			}
		}

		TSet<FString> SeenChildWidgetIds;
		for (const FString& ChildWidgetId : LayoutNode.ChildWidgetIds)
		{
			if (ChildWidgetId.IsEmpty())
			{
				AddSQLUIWidgetTreeAssemblyError(
					Result,
					FString::Printf(TEXT("SQLUI widget tree assembly node '%s' contains an empty child widget id."), *LayoutNode.WidgetId));
				continue;
			}

			if (ChildWidgetId == LayoutNode.WidgetId)
			{
				AddSQLUIWidgetTreeAssemblyError(
					Result,
					FString::Printf(TEXT("SQLUI widget tree assembly node '%s' cannot list itself as a child."), *LayoutNode.WidgetId));
			}

			if (SeenChildWidgetIds.Contains(ChildWidgetId))
			{
				AddSQLUIWidgetTreeAssemblyError(
					Result,
					FString::Printf(
						TEXT("SQLUI widget tree assembly node '%s' lists child '%s' more than once."),
						*LayoutNode.WidgetId,
						*ChildWidgetId));
			}
			else
			{
				SeenChildWidgetIds.Add(ChildWidgetId);
			}

			if (!NodeIndicesById.Contains(ChildWidgetId))
			{
				AddSQLUIWidgetTreeAssemblyError(
					Result,
					FString::Printf(
						TEXT("SQLUI widget tree assembly node '%s' references missing child '%s'."),
						*LayoutNode.WidgetId,
						*ChildWidgetId));
				continue;
			}

			AddUniqueSQLUIAssemblyChild(OutChildrenByParentId, LayoutNode.WidgetId, ChildWidgetId);

			const FString* ExistingListedParentId = ListedParentByChildId.Find(ChildWidgetId);
			if (ExistingListedParentId && *ExistingListedParentId != LayoutNode.WidgetId)
			{
				AddSQLUIWidgetTreeAssemblyError(
					Result,
					FString::Printf(
						TEXT("SQLUI widget tree assembly child '%s' is listed by multiple parents: '%s' and '%s'."),
						*ChildWidgetId,
						**ExistingListedParentId,
						*LayoutNode.WidgetId));
			}
			else if (!ExistingListedParentId)
			{
				ListedParentByChildId.Add(ChildWidgetId, LayoutNode.WidgetId);
			}

			const FSQLUILayoutNode& ChildNode = Request.LayoutNodes[NodeIndicesById[ChildWidgetId]];
			if (!ChildNode.ParentWidgetId.IsEmpty() && ChildNode.ParentWidgetId != LayoutNode.WidgetId)
			{
				AddSQLUIWidgetTreeAssemblyError(
					Result,
					FString::Printf(
						TEXT("SQLUI widget tree assembly node '%s' lists child '%s', but that child references parent '%s'."),
						*LayoutNode.WidgetId,
						*ChildWidgetId,
						*ChildNode.ParentWidgetId));
			}
			else if (ChildNode.ParentWidgetId.IsEmpty())
			{
				AddSQLUIWidgetTreeAssemblyWarning(
					Result,
					FString::Printf(
						TEXT("SQLUI widget tree assembly node '%s' lists child '%s', but the child does not reference a parent."),
						*LayoutNode.WidgetId,
						*ChildWidgetId));
			}
		}
	}
}

void VisitSQLUIAssemblyNode(
	const FString& WidgetId,
	const TMap<FString, TArray<FString>>& ChildrenByParentId,
	TSet<FString>& VisitingWidgetIds,
	TSet<FString>& VisitedWidgetIds,
	FSQLUIWidgetTreeAssemblyResult& Result)
{
	if (VisitingWidgetIds.Contains(WidgetId))
	{
		AddSQLUIWidgetTreeAssemblyError(
			Result,
			FString::Printf(TEXT("SQLUI widget tree assembly detected a cycle at widget '%s'."), *WidgetId));
		return;
	}

	if (VisitedWidgetIds.Contains(WidgetId))
	{
		return;
	}

	VisitingWidgetIds.Add(WidgetId);
	VisitedWidgetIds.Add(WidgetId);

	const TArray<FString>* ChildWidgetIds = ChildrenByParentId.Find(WidgetId);
	if (ChildWidgetIds)
	{
		for (const FString& ChildWidgetId : *ChildWidgetIds)
		{
			VisitSQLUIAssemblyNode(ChildWidgetId, ChildrenByParentId, VisitingWidgetIds, VisitedWidgetIds, Result);
		}
	}

	VisitingWidgetIds.Remove(WidgetId);
}

void ValidateSQLUIWidgetTreeAssemblyReachability(
	FSQLUIWidgetTreeAssemblyResult& Result,
	const TMap<FString, int32>& NodeIndicesById,
	const TMap<FString, TArray<FString>>& ChildrenByParentId,
	const FString& ResolvedRootWidgetId)
{
	if (ResolvedRootWidgetId.IsEmpty() || !NodeIndicesById.Contains(ResolvedRootWidgetId))
	{
		return;
	}

	TSet<FString> VisitingWidgetIds;
	TSet<FString> VisitedWidgetIds;
	VisitSQLUIAssemblyNode(ResolvedRootWidgetId, ChildrenByParentId, VisitingWidgetIds, VisitedWidgetIds, Result);

	for (const TPair<FString, int32>& NodePair : NodeIndicesById)
	{
		if (!VisitedWidgetIds.Contains(NodePair.Key))
		{
			AddSQLUIWidgetTreeAssemblyError(
				Result,
				FString::Printf(
					TEXT("SQLUI widget tree assembly node '%s' is unreachable from root '%s'."),
					*NodePair.Key,
					*ResolvedRootWidgetId));
		}
	}
}

FString ResolveSQLUIAssembledParentWidgetId(
	const FSQLUILayoutNode& LayoutNode,
	const TMap<FString, TArray<FString>>& ChildrenByParentId)
{
	if (!LayoutNode.ParentWidgetId.IsEmpty())
	{
		return LayoutNode.ParentWidgetId;
	}

	for (const TPair<FString, TArray<FString>>& ChildrenPair : ChildrenByParentId)
	{
		if (ChildrenPair.Value.Contains(LayoutNode.WidgetId))
		{
			return ChildrenPair.Key;
		}
	}

	return FString();
}

FSQLUIAssembledWidgetNode MakeSQLUIAssembledWidgetNode(
	const FSQLUIWidgetTreeAssemblyRequest& Request,
	FSQLUIWidgetTreeAssemblyResult& Result,
	const TMap<FString, int32>& NodeIndicesById,
	const TMap<FString, TArray<FString>>& ChildrenByParentId,
	const FSQLUILayoutNode& LayoutNode)
{
	FSQLUIAssembledWidgetNode AssembledNode;
	const FString ResolvedParentWidgetId = ResolveSQLUIAssembledParentWidgetId(LayoutNode, ChildrenByParentId);

	AssembledNode.WidgetId = LayoutNode.WidgetId;
	AssembledNode.ParentWidgetId = ResolvedParentWidgetId;
	AssembledNode.LayoutNode = LayoutNode;
	AssembledNode.Widget = FindSQLUICreatedWidget(Request, LayoutNode.WidgetId);
	AssembledNode.ParentWidget = ResolvedParentWidgetId.IsEmpty()
		? nullptr
		: FindSQLUICreatedWidget(Request, ResolvedParentWidgetId);

	if (const TArray<FString>* ChildWidgetIds = ChildrenByParentId.Find(LayoutNode.WidgetId))
	{
		AssembledNode.ChildWidgetIds = *ChildWidgetIds;
	}

	AssembledNode.bHasChildren = !AssembledNode.ChildWidgetIds.IsEmpty();

	for (const FString& ChildWidgetId : AssembledNode.ChildWidgetIds)
	{
		USQLUIBaseWidget* ChildWidget = FindSQLUICreatedWidget(Request, ChildWidgetId);
		AssembledNode.ChildWidgets.Add(ChildWidget);

		if (!IsValid(AssembledNode.Widget.Get()) || !IsValid(ChildWidget))
		{
			continue;
		}

		const int32* ChildNodeIndex = NodeIndicesById.Find(ChildWidgetId);
		if (!ChildNodeIndex)
		{
			continue;
		}

		const FSQLUILayoutNode& ChildLayoutNode = Request.LayoutNodes[*ChildNodeIndex];
		if (!AssembledNode.Widget->CanAcceptSQLUIChildWidget(ChildWidget, ChildLayoutNode))
		{
			AssembledNode.bCanAcceptAllChildren = false;
			AssembledNode.UnsupportedChildWidgetIds.Add(ChildWidgetId);

			const FString UnsupportedContainerMessage = FString::Printf(
				TEXT("SQLUI widget tree assembly parent '%s' does not currently support accepting child '%s'."),
				*LayoutNode.WidgetId,
				*ChildWidgetId);

			if (Request.bFailOnUnsupportedContainers)
			{
				AddSQLUIWidgetTreeAssemblyError(Result, UnsupportedContainerMessage);
			}
			else
			{
				AddSQLUIWidgetTreeAssemblyWarning(Result, UnsupportedContainerMessage);
			}
		}
	}

	return AssembledNode;
}
}

FSQLUIWidgetTreeAssemblyResult USQLUIWidgetTreeAssembler::AssembleWidgetTree(const FSQLUIWidgetTreeAssemblyRequest& Request) const
{
	FSQLUIWidgetTreeAssemblyResult Result;

	TMap<FString, int32> NodeIndicesById;
	ValidateSQLUIWidgetTreeAssemblyLayoutNodes(Request, Result, NodeIndicesById);

	const FString ResolvedRootWidgetId = ResolveSQLUIWidgetTreeAssemblyRootWidgetId(Request, Result, NodeIndicesById);
	Result.RootWidgetId = ResolvedRootWidgetId;
	Result.RootWidget = ResolvedRootWidgetId.IsEmpty()
		? nullptr
		: FindSQLUICreatedWidget(Request, ResolvedRootWidgetId);

	ValidateSQLUIWidgetTreeAssemblyCreatedWidgets(Request, Result, NodeIndicesById, ResolvedRootWidgetId);

	TMap<FString, TArray<FString>> ChildrenByParentId;
	ValidateSQLUIWidgetTreeAssemblyRelationships(Request, Result, NodeIndicesById, ResolvedRootWidgetId, ChildrenByParentId);
	ValidateSQLUIWidgetTreeAssemblyReachability(Result, NodeIndicesById, ChildrenByParentId, ResolvedRootWidgetId);

	if (!Result.Errors.IsEmpty())
	{
		return Result;
	}

	Result.AssembledNodes.Reserve(Request.LayoutNodes.Num());
	for (const FSQLUILayoutNode& LayoutNode : Request.LayoutNodes)
	{
		Result.AssembledNodes.Add(MakeSQLUIAssembledWidgetNode(
			Request,
			Result,
			NodeIndicesById,
			ChildrenByParentId,
			LayoutNode));
	}

	Result.bSucceeded = Result.Errors.IsEmpty();
	return Result;
}
