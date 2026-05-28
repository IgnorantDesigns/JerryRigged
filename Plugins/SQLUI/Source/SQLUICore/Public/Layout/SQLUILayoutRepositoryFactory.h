#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutRepository.h"
#include "UObject/Object.h"

#include "SQLUILayoutRepositoryFactory.generated.h"

UENUM(BlueprintType)
enum class ESQLUILayoutRepositoryBackend : uint8
{
	Unavailable UMETA(DisplayName = "Unavailable"),
	InMemory UMETA(DisplayName = "In Memory"),
	JsonFile UMETA(DisplayName = "JSON File")
};

USTRUCT(BlueprintType)
struct SQLUICORE_API FSQLUILayoutRepositoryFactorySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	ESQLUILayoutRepositoryBackend Backend = ESQLUILayoutRepositoryBackend::Unavailable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SQLUI|Layout Repository")
	FString JsonFileBaseDirectory;
};

UCLASS(BlueprintType)
class SQLUICORE_API USQLUILayoutRepositoryFactory : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SQLUI|Layout Repository")
	static USQLUILayoutRepository* CreateLayoutRepository(
		UObject* Outer,
		const FSQLUILayoutRepositoryFactorySettings& Settings);
};
