#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"

class SQLUICORE_API FSQLUILayoutJson
{
public:
	static bool ToJsonString(
		const FSQLUILayoutDocument& Document,
		FString& OutJson,
		FSQLUILayoutValidationResult& OutValidation);

	static bool FromJsonString(
		const FString& Json,
		FSQLUILayoutDocument& OutDocument,
		FSQLUILayoutValidationResult& OutValidation);

	static FSQLUILayoutValidationResult ValidateDocument(const FSQLUILayoutDocument& Document);
};
