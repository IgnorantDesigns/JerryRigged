#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"
#include "UObject/Object.h"

#include "SQLUIWidgetCatalog.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIWidgetTypeKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	FString Value;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIWidgetCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	FSQLUIWidgetTypeKey WidgetTypeKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	FString WidgetClassPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	TArray<FString> SupportedPropertyKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	TArray<FString> SupportedBindingKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	TArray<FString> SupportedActionKeys;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIWidgetCatalogValidationResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	bool bIsValid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	TArray<FString> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	TArray<FString> Warnings;
};

UCLASS(BlueprintType)
class SQLUICORE_API USQLUIWidgetCatalog : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Widget Catalog")
	TArray<FSQLUIWidgetCatalogEntry> Entries;

	void SetEntries(const TArray<FSQLUIWidgetCatalogEntry>& InEntries);
	void ClearEntries();
	bool RegisterEntry(const FSQLUIWidgetCatalogEntry& Entry, FSQLUIWidgetCatalogValidationResult* OutValidation = nullptr);

	const FSQLUIWidgetCatalogEntry* FindEntry(const FString& WidgetTypeKey) const;
	const FSQLUIWidgetCatalogEntry* FindEntry(const FSQLUIWidgetTypeKey& WidgetTypeKey) const;

	FSQLUIWidgetCatalogValidationResult ValidateCatalog() const;
	FSQLUIWidgetCatalogValidationResult ValidateLayoutDocument(const FSQLUILayoutDocument& Document) const;
};
