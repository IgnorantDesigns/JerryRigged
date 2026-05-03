#include "Bindings/SQLUIWidgetBindingApplier.h"

#include "Bindings/SQLUIBindingTypes.h"
#include "Variables/SQLUIVariableTypes.h"
#include "Widgets/SQLUIBaseWidget.h"

namespace
{
FString GetSQLUIWidgetBindingLabel(const FSQLUILayoutBinding& Binding)
{
	if (!Binding.BindingId.IsEmpty())
	{
		return Binding.BindingId;
	}

	if (!Binding.TargetProperty.IsEmpty())
	{
		return Binding.TargetProperty;
	}

	if (!Binding.SourcePath.IsEmpty())
	{
		return Binding.SourcePath;
	}

	return TEXT("<unnamed>");
}

FString MakeSQLUIWidgetBindingMessagePrefix(
	const FString& WidgetId,
	const FSQLUILayoutBinding& Binding)
{
	return FString::Printf(
		TEXT("SQLUI widget binding '%s' on widget '%s'"),
		*GetSQLUIWidgetBindingLabel(Binding),
		*WidgetId);
}

FSQLUIWidgetBindingApplyError MakeSQLUIWidgetBindingApplyError(
	const FString& WidgetId,
	const FSQLUILayoutBinding& Binding,
	const FString& ErrorMessage,
	const bool bUnsupportedBinding = false,
	const bool bPropertyApplyFailure = false,
	const bool bUnsupportedProperty = false)
{
	FSQLUIWidgetBindingApplyError Error;
	Error.WidgetId = WidgetId;
	Error.BindingId = Binding.BindingId;
	Error.TargetProperty = Binding.TargetProperty;
	Error.SourceKey = Binding.SourceKey;
	Error.SourcePath = Binding.SourcePath;
	Error.ErrorMessage = ErrorMessage;
	Error.bUnsupportedBinding = bUnsupportedBinding;
	Error.bPropertyApplyFailure = bPropertyApplyFailure;
	Error.bUnsupportedProperty = bUnsupportedProperty;
	return Error;
}

FSQLUIWidgetBindingApplyWarning MakeSQLUIWidgetBindingApplyWarning(
	const FString& WidgetId,
	const FSQLUILayoutBinding& Binding,
	const FString& WarningMessage,
	const bool bUnsupportedBinding = false,
	const bool bPropertyApplyFailure = false,
	const bool bUnsupportedProperty = false)
{
	FSQLUIWidgetBindingApplyWarning Warning;
	Warning.WidgetId = WidgetId;
	Warning.BindingId = Binding.BindingId;
	Warning.TargetProperty = Binding.TargetProperty;
	Warning.SourceKey = Binding.SourceKey;
	Warning.SourcePath = Binding.SourcePath;
	Warning.WarningMessage = WarningMessage;
	Warning.bUnsupportedBinding = bUnsupportedBinding;
	Warning.bPropertyApplyFailure = bPropertyApplyFailure;
	Warning.bUnsupportedProperty = bUnsupportedProperty;
	return Warning;
}

void AddSQLUIWidgetBindingApplyError(
	FSQLUIWidgetBindingApplyResult& Result,
	const FString& WidgetId,
	const FSQLUILayoutBinding& Binding,
	const FString& ErrorMessage,
	const bool bUnsupportedBinding = false,
	const bool bPropertyApplyFailure = false,
	const bool bUnsupportedProperty = false)
{
	Result.Errors.Add(MakeSQLUIWidgetBindingApplyError(
		WidgetId,
		Binding,
		ErrorMessage,
		bUnsupportedBinding,
		bPropertyApplyFailure,
		bUnsupportedProperty));

	if (Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = ErrorMessage;
	}
}

void AddSQLUIWidgetBindingApplyWarning(
	FSQLUIWidgetBindingApplyResult& Result,
	const FString& WidgetId,
	const FSQLUILayoutBinding& Binding,
	const FString& WarningMessage,
	const bool bUnsupportedBinding = false,
	const bool bPropertyApplyFailure = false,
	const bool bUnsupportedProperty = false)
{
	Result.Warnings.Add(MakeSQLUIWidgetBindingApplyWarning(
		WidgetId,
		Binding,
		WarningMessage,
		bUnsupportedBinding,
		bPropertyApplyFailure,
		bUnsupportedProperty));
}

void AddSQLUIWidgetBindingApplyProblem(
	FSQLUIWidgetBindingApplyResult& Result,
	const bool bShouldFail,
	const FString& WidgetId,
	const FSQLUILayoutBinding& Binding,
	const FString& Message,
	const bool bUnsupportedBinding = false,
	const bool bPropertyApplyFailure = false,
	const bool bUnsupportedProperty = false)
{
	if (bShouldFail)
	{
		AddSQLUIWidgetBindingApplyError(
			Result,
			WidgetId,
			Binding,
			Message,
			bUnsupportedBinding,
			bPropertyApplyFailure,
			bUnsupportedProperty);
		return;
	}

	AddSQLUIWidgetBindingApplyWarning(
		Result,
		WidgetId,
		Binding,
		Message,
		bUnsupportedBinding,
		bPropertyApplyFailure,
		bUnsupportedProperty);
}

USQLUIBaseWidget* FindSQLUIBindingApplyWidget(
	const FSQLUIWidgetBindingApplyRequest& Request,
	const FString& WidgetId)
{
	USQLUIBaseWidget* const* CreatedWidget = Request.CreatedWidgetMap.Find(WidgetId);
	return CreatedWidget ? *CreatedWidget : nullptr;
}

bool TryConvertSQLUIResolvedValueToPropertyString(
	const FSQLUIVariableValue& ResolvedValue,
	FString& OutPropertyValue,
	FString& OutFailureMessage)
{
	switch (ResolvedValue.Type)
	{
	case ESQLUIVariableValueType::String:
		OutPropertyValue = ResolvedValue.StringValue;
		return true;

	case ESQLUIVariableValueType::Number:
		OutPropertyValue = FString::SanitizeFloat(ResolvedValue.NumberValue);
		return true;

	case ESQLUIVariableValueType::Boolean:
		OutPropertyValue = ResolvedValue.bBooleanValue ? TEXT("true") : TEXT("false");
		return true;

	default:
		OutPropertyValue.Reset();
		OutFailureMessage = TEXT("resolved binding value type is not supported by widget binding application.");
		return false;
	}
}

FSQLUIAppliedWidgetBinding MakeSQLUIAppliedWidgetBinding(
	const FString& WidgetId,
	const FSQLUILayoutBinding& Binding,
	const FString& ResolvedValueString,
	const bool bAppliedToWidget)
{
	FSQLUIAppliedWidgetBinding AppliedBinding;
	AppliedBinding.WidgetId = WidgetId;
	AppliedBinding.BindingId = Binding.BindingId;
	AppliedBinding.TargetProperty = Binding.TargetProperty;
	AppliedBinding.SourceKey = Binding.SourceKey;
	AppliedBinding.SourcePath = Binding.SourcePath;
	AppliedBinding.ResolvedValueString = ResolvedValueString;
	AppliedBinding.bAppliedToWidget = bAppliedToWidget;
	return AppliedBinding;
}
}

FSQLUIWidgetBindingApplyResult USQLUIWidgetBindingApplier::ApplyBindings(
	const FSQLUIWidgetBindingApplyRequest& Request) const
{
	FSQLUIWidgetBindingApplyResult Result;

	if (!IsValid(Request.RuntimeContext.Get()))
	{
		AddSQLUIWidgetBindingApplyError(
			Result,
			FString(),
			FSQLUILayoutBinding(),
			TEXT("SQLUI widget binding application requires a runtime context."));
		Result.bSucceeded = false;
		return Result;
	}

	USQLUIBindingResolver* BindingResolver = Request.BindingResolverOverride.Get();
	if (Request.BindingResolverOverride && !IsValid(BindingResolver))
	{
		AddSQLUIWidgetBindingApplyError(
			Result,
			FString(),
			FSQLUILayoutBinding(),
			TEXT("SQLUI widget binding application received an invalid binding resolver override."));
		Result.bSucceeded = false;
		return Result;
	}

	if (!BindingResolver)
	{
		BindingResolver = NewObject<USQLUIBindingResolver>();
		if (!IsValid(BindingResolver))
		{
			AddSQLUIWidgetBindingApplyError(
				Result,
				FString(),
				FSQLUILayoutBinding(),
				TEXT("SQLUI widget binding application could not create a binding resolver."));
			Result.bSucceeded = false;
			return Result;
		}
	}

	for (int32 NodeIndex = 0; NodeIndex < Request.LayoutNodes.Num(); ++NodeIndex)
	{
		const FSQLUILayoutNode& LayoutNode = Request.LayoutNodes[NodeIndex];
		if (LayoutNode.WidgetId.IsEmpty())
		{
			AddSQLUIWidgetBindingApplyError(
				Result,
				FString(),
				FSQLUILayoutBinding(),
				FString::Printf(TEXT("SQLUI widget binding application found a layout node without a widget id at index %d."), NodeIndex));
			continue;
		}

		USQLUIBaseWidget* Widget = FindSQLUIBindingApplyWidget(Request, LayoutNode.WidgetId);
		if (!IsValid(Widget))
		{
			AddSQLUIWidgetBindingApplyError(
				Result,
				LayoutNode.WidgetId,
				FSQLUILayoutBinding(),
				FString::Printf(
					TEXT("SQLUI widget binding application layout node '%s' does not have a valid created widget."),
					*LayoutNode.WidgetId));
			continue;
		}

		for (const FSQLUILayoutBinding& Binding : LayoutNode.Bindings)
		{
			if (Binding.TargetProperty.IsEmpty())
			{
				AddSQLUIWidgetBindingApplyError(
					Result,
					LayoutNode.WidgetId,
					Binding,
					FString::Printf(
						TEXT("%s does not include a target property."),
						*MakeSQLUIWidgetBindingMessagePrefix(LayoutNode.WidgetId, Binding)));
				continue;
			}

			const FSQLUIBindingResolveResult ResolveResult =
				BindingResolver->ResolveLayoutBinding(Binding, Request.RuntimeContext.Get());
			if (!ResolveResult.bSucceeded)
			{
				const FString ResolveMessage = ResolveResult.ErrorMessage.IsEmpty()
					? FString::Printf(
						TEXT("%s could not be resolved."),
						*MakeSQLUIWidgetBindingMessagePrefix(LayoutNode.WidgetId, Binding))
					: ResolveResult.ErrorMessage;

				AddSQLUIWidgetBindingApplyProblem(
					Result,
					Request.bFailOnUnsupportedBindings,
					LayoutNode.WidgetId,
					Binding,
					ResolveMessage,
					true);
				continue;
			}

			FString ResolvedValueString;
			FString ConversionFailureMessage;
			if (!TryConvertSQLUIResolvedValueToPropertyString(
				ResolveResult.ResolvedValue,
				ResolvedValueString,
				ConversionFailureMessage))
			{
				AddSQLUIWidgetBindingApplyProblem(
					Result,
					Request.bFailOnUnsupportedBindings,
					LayoutNode.WidgetId,
					Binding,
					FString::Printf(
						TEXT("%s resolved but could not be converted for property application: %s"),
						*MakeSQLUIWidgetBindingMessagePrefix(LayoutNode.WidgetId, Binding),
						*ConversionFailureMessage),
					true);
				continue;
			}

			if (!Binding.TransformKey.IsEmpty())
			{
				Result.AppliedBindings.Add(MakeSQLUIAppliedWidgetBinding(
					LayoutNode.WidgetId,
					Binding,
					ResolvedValueString,
					false));

				AddSQLUIWidgetBindingApplyProblem(
					Result,
					Request.bFailOnUnsupportedBindings,
					LayoutNode.WidgetId,
					Binding,
					FString::Printf(
						TEXT("%s resolved value for target property '%s', but transform key '%s' is not implemented yet."),
						*MakeSQLUIWidgetBindingMessagePrefix(LayoutNode.WidgetId, Binding),
						*Binding.TargetProperty,
						*Binding.TransformKey),
					true);
				continue;
			}

			FString ApplyMessage;
			bool bUnsupportedProperty = false;
			const bool bApplied = Widget->ApplySQLUIWidgetProperty(
				Binding.TargetProperty,
				ResolvedValueString,
				ApplyMessage,
				bUnsupportedProperty);

			Result.AppliedBindings.Add(MakeSQLUIAppliedWidgetBinding(
				LayoutNode.WidgetId,
				Binding,
				ResolvedValueString,
				bApplied));

			if (bApplied)
			{
				continue;
			}

			if (ApplyMessage.IsEmpty())
			{
				ApplyMessage = FString::Printf(
					TEXT("SQLUI widget binding application could not apply target property '%s'."),
					*Binding.TargetProperty);
			}

			AddSQLUIWidgetBindingApplyProblem(
				Result,
				Request.bFailOnPropertyApplyFailure,
				LayoutNode.WidgetId,
				Binding,
				ApplyMessage,
				false,
				true,
				bUnsupportedProperty);
		}
	}

	Result.bSucceeded = Result.Errors.IsEmpty();
	return Result;
}
