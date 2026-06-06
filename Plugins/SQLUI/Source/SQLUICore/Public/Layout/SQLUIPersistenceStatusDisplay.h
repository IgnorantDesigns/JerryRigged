#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Layout/SQLUIPersistenceStatus.h"

#include "SQLUIPersistenceStatusDisplay.generated.h"

UENUM(BlueprintType)
enum class ESQLUIPersistenceStatusDisplayState : uint8
{
	Normal,
	Good,
	Warning,
	Unavailable
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUIPersistenceStatusDisplayRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FText Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	ESQLUIPersistenceStatusDisplayState State = ESQLUIPersistenceStatusDisplayState::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Persistence")
	FText DetailText;
};

/**
 * UI-safe, read-only formatter for SQLUI persistence status.
 *
 * This helper converts the existing status snapshot into display rows. It does
 * not probe files directly, initialize providers, create repositories, create
 * databases, run migrations, copy seed databases, reset databases, or delete
 * files.
 */
UCLASS(BlueprintType)
class SQLUICORE_API USQLUIPersistenceStatusDisplayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence", meta = (WorldContext = "WorldContextObject"))
	static TArray<FSQLUIPersistenceStatusDisplayRow> GetPersistenceStatusDisplayRows(
		UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence", meta = (WorldContext = "WorldContextObject"))
	static TArray<FSQLUIPersistenceStatusDisplayRow> GetPersistenceStatusDisplayRowsFromRuntimeConfig(
		UObject* WorldContextObject,
		const FSQLUILayoutRepositoryRuntimeConfig& RuntimeConfig);

	UFUNCTION(BlueprintCallable, Category = "SQLUI|Persistence")
	static TArray<FSQLUIPersistenceStatusDisplayRow> MakePersistenceStatusDisplayRows(
		const FSQLUIPersistenceStatusSnapshot& Snapshot);
};
