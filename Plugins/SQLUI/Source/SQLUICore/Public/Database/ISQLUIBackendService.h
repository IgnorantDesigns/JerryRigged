#pragma once

#include "CoreMinimal.h"
#include "Database/SQLUIQueryTypes.h"

class SQLUICORE_API ISQLUIBackendService
{
public:
	virtual ~ISQLUIBackendService() = default;

	virtual bool Open(const FString& DatabasePath, FString& OutErrorMessage) = 0;
	virtual void Close() = 0;
	virtual bool IsAvailable() const = 0;
	virtual FSQLUIQueryResult Execute(const FSQLUIQueryRequest& Request) = 0;
};
