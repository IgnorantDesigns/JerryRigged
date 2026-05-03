#include "Properties/SQLUIWidgetPropertyApplier.h"

#include "Widgets/SQLUIBaseWidget.h"

namespace
{
void AddSQLUIWidgetPropertyApplyError(
	FSQLUIWidgetPropertyApplyResult& Result,
	const FString& WidgetId,
	const FString& PropertyName,
	const FString& ErrorMessage,
	const bool bUnsupportedProperty = false)
{
	FSQLUIWidgetPropertyApplyError Error;
	Error.WidgetId = WidgetId;
	Error.PropertyName = PropertyName;
	Error.ErrorMessage = ErrorMessage;
	Error.bUnsupportedProperty = bUnsupportedProperty;

	Result.bSucceeded = false;
	Result.Errors.Add(Error);

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = ErrorMessage;
	}
}

void AddSQLUIWidgetPropertyApplyWarning(
	FSQLUIWidgetPropertyApplyResult& Result,
	const FString& WidgetId,
	const FString& PropertyName,
	const FString& WarningMessage)
{
	Result.Warnings.Add(FString::Printf(
		TEXT("Widget '%s' property '%s': %s"),
		*WidgetId,
		*PropertyName,
		*WarningMessage));
}

USQLUIBaseWidget* FindSQLUIPropertyApplyWidget(
	const FSQLUIWidgetPropertyApplyRequest& Request,
	const FString& WidgetId)
{
	USQLUIBaseWidget* const* CreatedWidget = Request.CreatedWidgetMap.Find(WidgetId);
	return CreatedWidget ? *CreatedWidget : nullptr;
}

FString MakeSQLUIAppliedPropertyName(const FString& WidgetId, const FString& PropertyName)
{
	return FString::Printf(TEXT("%s.%s"), *WidgetId, *PropertyName);
}
}

FSQLUIWidgetPropertyApplyResult USQLUIWidgetPropertyApplier::ApplyProperties(
	const FSQLUIWidgetPropertyApplyRequest& Request) const
{
	FSQLUIWidgetPropertyApplyResult Result;

	for (int32 NodeIndex = 0; NodeIndex < Request.LayoutNodes.Num(); ++NodeIndex)
	{
		const FSQLUILayoutNode& LayoutNode = Request.LayoutNodes[NodeIndex];
		if (LayoutNode.WidgetId.IsEmpty())
		{
			AddSQLUIWidgetPropertyApplyError(
				Result,
				FString(),
				FString(),
				FString::Printf(TEXT("SQLUI widget property application found a layout node without a widget id at index %d."), NodeIndex));
			continue;
		}

		USQLUIBaseWidget* Widget = FindSQLUIPropertyApplyWidget(Request, LayoutNode.WidgetId);
		if (!IsValid(Widget))
		{
			AddSQLUIWidgetPropertyApplyError(
				Result,
				LayoutNode.WidgetId,
				FString(),
				FString::Printf(
					TEXT("SQLUI widget property application layout node '%s' does not have a valid created widget."),
					*LayoutNode.WidgetId));
			continue;
		}

		for (const TPair<FString, FString>& PropertyPair : LayoutNode.Properties)
		{
			FString ApplyMessage;
			bool bUnsupportedProperty = false;
			const bool bApplied = Widget->ApplySQLUIWidgetProperty(
				PropertyPair.Key,
				PropertyPair.Value,
				ApplyMessage,
				bUnsupportedProperty);

			if (bApplied)
			{
				Result.AppliedPropertyNames.Add(MakeSQLUIAppliedPropertyName(LayoutNode.WidgetId, PropertyPair.Key));
				continue;
			}

			if (ApplyMessage.IsEmpty())
			{
				ApplyMessage = FString::Printf(
					TEXT("SQLUI widget property '%s' could not be applied."),
					*PropertyPair.Key);
			}

			if (bUnsupportedProperty && !Request.bFailOnUnsupportedProperties)
			{
				AddSQLUIWidgetPropertyApplyWarning(
					Result,
					LayoutNode.WidgetId,
					PropertyPair.Key,
					ApplyMessage);
				continue;
			}

			AddSQLUIWidgetPropertyApplyError(
				Result,
				LayoutNode.WidgetId,
				PropertyPair.Key,
				ApplyMessage,
				bUnsupportedProperty);
		}
	}

	Result.bSucceeded = Result.Errors.IsEmpty();
	return Result;
}
