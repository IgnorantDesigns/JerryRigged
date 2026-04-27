#pragma once

#include "CoreMinimal.h"

#include "SQLUILayoutTypes.generated.h"

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutVersion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	int32 SchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	int32 Revision = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString Label;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString LayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString CreatedBy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString CreatedAtUtc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString UpdatedAtUtc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TArray<FString> Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TMap<FString, FString> SearchMetadata;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString BindingId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString TargetProperty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString SourceKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString SourcePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString TransformKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TMap<FString, FString> Options;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString ActionTypeKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TMap<FString, FString> Parameters;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString WidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString ParentWidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString WidgetTypeKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TArray<FString> ChildWidgetIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TMap<FString, FString> Properties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TArray<FSQLUILayoutBinding> Bindings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TArray<FSQLUILayoutAction> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TArray<FString> Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TMap<FString, FString> SearchMetadata;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutDocument
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FSQLUILayoutVersion Version;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FSQLUILayoutMetadata Metadata;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString RootWidgetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TArray<FSQLUILayoutNode> Nodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TMap<FString, FString> Properties;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutValidationResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	bool bIsValid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TArray<FString> Errors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	TArray<FString> Warnings;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutLoadResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	bool bBackendUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FSQLUILayoutDocument Document;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FSQLUILayoutValidationResult Validation;
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutSaveResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	bool bBackendUnavailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString ErrorMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FString SavedLayoutId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout")
	FSQLUILayoutValidationResult Validation;
};
