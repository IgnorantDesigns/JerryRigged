#include "Bindings/SQLUIBindingResolver.h"

#include "Runtime/SQLUIRuntimeContext.h"
#include "Variables/SQLUIVariableStore.h"

namespace
{
bool IsSQLUIVariableBindingSource(const FString& SourceKey)
{
	return SourceKey.Equals(TEXT("Variable"), ESearchCase::IgnoreCase) ||
		SourceKey.Equals(TEXT("VariableStore"), ESearchCase::IgnoreCase);
}

FString GetSQLUIBindingLabel(const FSQLUILayoutBinding& Binding)
{
	if (!Binding.BindingId.IsEmpty())
	{
		return Binding.BindingId;
	}

	return Binding.TargetProperty;
}

FSQLUIBindingResolveResult MakeSQLUIBindingUnavailableResult(
	const FSQLUILayoutBinding& Binding,
	const FString& Reason,
	const bool bNotImplemented)
{
	FSQLUIBindingResolveResult Result;
	Result.bSucceeded = false;
	Result.bBindingUnavailable = true;
	Result.bNotImplemented = bNotImplemented;

	const FString BindingLabel = GetSQLUIBindingLabel(Binding);
	Result.ErrorMessage = BindingLabel.IsEmpty()
		? Reason
		: FString::Printf(TEXT("SQLUI binding '%s' is unavailable: %s"), *BindingLabel, *Reason);

	return Result;
}
}

FSQLUIBindingResolveResult USQLUIBindingResolver::ResolveBinding(
	const FSQLUIBindingResolveRequest& Request,
	const USQLUIRuntimeContext* RuntimeContext) const
{
	const FSQLUILayoutBinding& Binding = Request.Binding;

	if (!RuntimeContext)
	{
		return MakeSQLUIBindingUnavailableResult(
			Binding,
			TEXT("runtime context was not provided."),
			false);
	}

	if (Binding.SourceKey.IsEmpty())
	{
		return MakeSQLUIBindingUnavailableResult(
			Binding,
			TEXT("no binding source key was provided."),
			false);
	}

	if (!IsSQLUIVariableBindingSource(Binding.SourceKey))
	{
		return MakeSQLUIBindingUnavailableResult(
			Binding,
			FString::Printf(TEXT("binding source key '%s' is not implemented yet."), *Binding.SourceKey),
			true);
	}

	if (Binding.SourcePath.IsEmpty())
	{
		return MakeSQLUIBindingUnavailableResult(
			Binding,
			TEXT("variable bindings require SourcePath to name a variable."),
			false);
	}

	const USQLUIVariableStore* VariableStore = RuntimeContext->GetVariableStore();
	if (!VariableStore)
	{
		return MakeSQLUIBindingUnavailableResult(
			Binding,
			TEXT("runtime context has no variable store."),
			false);
	}

	FSQLUIVariableKey VariableKey;
	VariableKey.Name = Binding.SourcePath;

	FSQLUIVariableValue ResolvedValue;
	if (!VariableStore->GetVariable(VariableKey, ResolvedValue))
	{
		return MakeSQLUIBindingUnavailableResult(
			Binding,
			FString::Printf(TEXT("variable '%s' is not available."), *VariableKey.Name),
			false);
	}

	FSQLUIBindingResolveResult Result;
	Result.bSucceeded = true;
	Result.ResolvedValue = ResolvedValue;
	return Result;
}

FSQLUIBindingResolveResult USQLUIBindingResolver::ResolveLayoutBinding(
	const FSQLUILayoutBinding& Binding,
	const USQLUIRuntimeContext* RuntimeContext) const
{
	FSQLUIBindingResolveRequest Request;
	Request.Binding = Binding;
	return ResolveBinding(Request, RuntimeContext);
}
