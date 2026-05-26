#include "WidgetCatalog/SQLUIWidgetCatalog.h"

namespace
{
void AddSQLUIWidgetCatalogValidationError(FSQLUIWidgetCatalogValidationResult& Result, const FString& ErrorMessage)
{
	Result.bIsValid = false;
	Result.Errors.Add(ErrorMessage);
}

void AddSQLUIWidgetCatalogValidationWarning(FSQLUIWidgetCatalogValidationResult& Result, const FString& WarningMessage)
{
	Result.Warnings.Add(WarningMessage);
}

void ValidateSQLUIWidgetCatalogSupportedKeys(
	FSQLUIWidgetCatalogValidationResult& Result,
	const FString& EntryLabel,
	const TCHAR* SupportedKeyLabel,
	const TArray<FString>& SupportedKeys)
{
	TSet<FString> SeenKeys;
	for (const FString& SupportedKey : SupportedKeys)
	{
		if (SupportedKey.IsEmpty())
		{
			AddSQLUIWidgetCatalogValidationError(
				Result,
				FString::Printf(TEXT("SQLUI widget catalog entry '%s' includes an empty supported %s key."), *EntryLabel, SupportedKeyLabel));
			continue;
		}

		if (SeenKeys.Contains(SupportedKey))
		{
			AddSQLUIWidgetCatalogValidationWarning(
				Result,
				FString::Printf(TEXT("SQLUI widget catalog entry '%s' lists supported %s key '%s' more than once."), *EntryLabel, SupportedKeyLabel, *SupportedKey));
		}
		else
		{
			SeenKeys.Add(SupportedKey);
		}
	}
}

void ValidateSQLUIWidgetCatalogEntry(
	FSQLUIWidgetCatalogValidationResult& Result,
	const FSQLUIWidgetCatalogEntry& Entry,
	const FString& EntryLabel)
{
	if (Entry.WidgetTypeKey.Value.IsEmpty())
	{
		AddSQLUIWidgetCatalogValidationError(
			Result,
			FString::Printf(TEXT("SQLUI widget catalog entry '%s' must include a widget type key."), *EntryLabel));
	}

	ValidateSQLUIWidgetCatalogSupportedKeys(Result, EntryLabel, TEXT("property"), Entry.SupportedPropertyKeys);
	ValidateSQLUIWidgetCatalogSupportedKeys(Result, EntryLabel, TEXT("binding"), Entry.SupportedBindingKeys);
	ValidateSQLUIWidgetCatalogSupportedKeys(Result, EntryLabel, TEXT("action"), Entry.SupportedActionKeys);
}

void ValidateSQLUIWidgetCatalogKey(
	FSQLUIWidgetCatalogValidationResult& Result,
	const FString& WidgetId,
	const FString& WidgetTypeKey,
	const FString& KeyType,
	const FString& Key,
	const TArray<FString>& SupportedKeys)
{
	if (Key.IsEmpty())
	{
		return;
	}

	if (SupportedKeys.IsEmpty())
	{
		return;
	}

	if (!SupportedKeys.Contains(Key))
	{
		AddSQLUIWidgetCatalogValidationError(
			Result,
			FString::Printf(
				TEXT("SQLUI layout node '%s' of widget type '%s' uses unsupported %s key '%s'."),
				*WidgetId,
				*WidgetTypeKey,
				*KeyType,
				*Key));
	}
}
}

void USQLUIWidgetCatalog::SetEntries(const TArray<FSQLUIWidgetCatalogEntry>& InEntries)
{
	Entries = InEntries;
}

void USQLUIWidgetCatalog::ClearEntries()
{
	Entries.Empty();
}

bool USQLUIWidgetCatalog::RegisterEntry(const FSQLUIWidgetCatalogEntry& Entry, FSQLUIWidgetCatalogValidationResult* OutValidation)
{
	FSQLUIWidgetCatalogValidationResult Validation;
	const FString EntryLabel = Entry.WidgetTypeKey.Value.IsEmpty()
		? TEXT("new entry")
		: Entry.WidgetTypeKey.Value;

	ValidateSQLUIWidgetCatalogEntry(Validation, Entry, EntryLabel);

	if (!Entry.WidgetTypeKey.Value.IsEmpty() && FindEntry(Entry.WidgetTypeKey))
	{
		AddSQLUIWidgetCatalogValidationError(
			Validation,
			FString::Printf(TEXT("SQLUI widget catalog already contains widget type key '%s'."), *Entry.WidgetTypeKey.Value));
	}

	if (OutValidation)
	{
		*OutValidation = Validation;
	}

	if (!Validation.bIsValid)
	{
		return false;
	}

	Entries.Add(Entry);
	return true;
}

const FSQLUIWidgetCatalogEntry* USQLUIWidgetCatalog::FindEntry(const FString& WidgetTypeKey) const
{
	if (WidgetTypeKey.IsEmpty())
	{
		return nullptr;
	}

	for (const FSQLUIWidgetCatalogEntry& Entry : Entries)
	{
		if (Entry.WidgetTypeKey.Value == WidgetTypeKey)
		{
			return &Entry;
		}
	}

	return nullptr;
}

const FSQLUIWidgetCatalogEntry* USQLUIWidgetCatalog::FindEntry(const FSQLUIWidgetTypeKey& WidgetTypeKey) const
{
	return FindEntry(WidgetTypeKey.Value);
}

FSQLUIWidgetCatalogValidationResult USQLUIWidgetCatalog::ValidateCatalog() const
{
	FSQLUIWidgetCatalogValidationResult Result;

	TSet<FString> SeenWidgetTypeKeys;
	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
	{
		const FSQLUIWidgetCatalogEntry& Entry = Entries[EntryIndex];
		const FString EntryLabel = Entry.WidgetTypeKey.Value.IsEmpty()
			? FString::Printf(TEXT("index %d"), EntryIndex)
			: Entry.WidgetTypeKey.Value;

		ValidateSQLUIWidgetCatalogEntry(Result, Entry, EntryLabel);

		if (Entry.WidgetTypeKey.Value.IsEmpty())
		{
			continue;
		}

		if (SeenWidgetTypeKeys.Contains(Entry.WidgetTypeKey.Value))
		{
			AddSQLUIWidgetCatalogValidationError(
				Result,
				FString::Printf(TEXT("SQLUI widget catalog contains duplicate widget type key '%s'."), *Entry.WidgetTypeKey.Value));
		}
		else
		{
			SeenWidgetTypeKeys.Add(Entry.WidgetTypeKey.Value);
		}
	}

	return Result;
}

FSQLUIWidgetCatalogValidationResult USQLUIWidgetCatalog::ValidateLayoutDocument(const FSQLUILayoutDocument& Document) const
{
	FSQLUIWidgetCatalogValidationResult Result = ValidateCatalog();

	for (const FSQLUILayoutNode& Node : Document.Nodes)
	{
		if (Node.WidgetTypeKey.IsEmpty())
		{
			continue;
		}

		const FSQLUIWidgetCatalogEntry* Entry = FindEntry(Node.WidgetTypeKey);
		if (!Entry)
		{
			AddSQLUIWidgetCatalogValidationError(
				Result,
				FString::Printf(TEXT("SQLUI layout node '%s' references unknown widget type key '%s'."), *Node.WidgetId, *Node.WidgetTypeKey));
			continue;
		}

		for (const TPair<FString, FString>& PropertyPair : Node.Properties)
		{
			ValidateSQLUIWidgetCatalogKey(
				Result,
				Node.WidgetId,
				Node.WidgetTypeKey,
				TEXT("property"),
				PropertyPair.Key,
				Entry->SupportedPropertyKeys);
		}

		for (const FSQLUILayoutBinding& Binding : Node.Bindings)
		{
			ValidateSQLUIWidgetCatalogKey(
				Result,
				Node.WidgetId,
				Node.WidgetTypeKey,
				TEXT("binding target property"),
				Binding.TargetProperty,
				Entry->SupportedPropertyKeys);

			ValidateSQLUIWidgetCatalogKey(
				Result,
				Node.WidgetId,
				Node.WidgetTypeKey,
				TEXT("binding"),
				Binding.SourceKey,
				Entry->SupportedBindingKeys);
		}

		for (const FSQLUILayoutAction& Action : Node.Actions)
		{
			ValidateSQLUIWidgetCatalogKey(
				Result,
				Node.WidgetId,
				Node.WidgetTypeKey,
				TEXT("action"),
				Action.ActionTypeKey,
				Entry->SupportedActionKeys);
		}
	}

	return Result;
}
