#include "Actions/SQLUIActionRegistry.h"

namespace
{
FSQLUIActionResult MakeSQLUIUnavailableActionResult(const FString& ActionName, const FString& Reason)
{
	FSQLUIActionResult Result;
	Result.bSucceeded = false;
	Result.bActionUnavailable = true;
	Result.bNotImplemented = true;
	Result.ErrorMessage = ActionName.IsEmpty()
		? Reason
		: FString::Printf(TEXT("SQLUI action '%s' is unavailable: %s"), *ActionName, *Reason);
	return Result;
}
}

bool USQLUIActionRegistry::RegisterAction(const FSQLUIActionKey& Key, FSQLUIActionHandler Handler)
{
	if (Key.Name.IsEmpty() || !Handler.IsBound())
	{
		return false;
	}

	HandlersByName.Add(Key.Name, MoveTemp(Handler));
	return true;
}

bool USQLUIActionRegistry::UnregisterAction(const FSQLUIActionKey& Key)
{
	if (Key.Name.IsEmpty())
	{
		return false;
	}

	return HandlersByName.Remove(Key.Name) > 0;
}

bool USQLUIActionRegistry::IsActionRegistered(const FSQLUIActionKey& Key) const
{
	const FSQLUIActionHandler* Handler = HandlersByName.Find(Key.Name);
	return Handler && Handler->IsBound();
}

FSQLUIActionResult USQLUIActionRegistry::ExecuteAction(const FSQLUIActionRequest& Request) const
{
	if (Request.ActionKey.Name.IsEmpty())
	{
		return MakeSQLUIUnavailableActionResult(
			Request.ActionKey.Name,
			TEXT("no action key was provided."));
	}

	const FSQLUIActionHandler* Handler = HandlersByName.Find(Request.ActionKey.Name);
	if (!Handler)
	{
		return MakeSQLUIUnavailableActionResult(
			Request.ActionKey.Name,
			TEXT("no handler is registered yet."));
	}

	if (!Handler->IsBound())
	{
		return MakeSQLUIUnavailableActionResult(
			Request.ActionKey.Name,
			TEXT("the registered handler is not bound."));
	}

	return Handler->Execute(Request);
}

void USQLUIActionRegistry::ClearActions()
{
	HandlersByName.Empty();
}

