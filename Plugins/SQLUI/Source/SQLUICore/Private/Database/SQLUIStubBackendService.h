#pragma once

#include "CoreMinimal.h"
#include "Database/ISQLUIBackendService.h"

class FSQLUIStubBackendService final : public ISQLUIBackendService
{
public:
	explicit FSQLUIStubBackendService(FString InUnavailableReason);
	virtual ~FSQLUIStubBackendService() override = default;

	virtual bool Open(const FString& DatabasePath, FString& OutErrorMessage) override;
	virtual void Close() override;
	virtual bool IsAvailable() const override;
	virtual FSQLUIQueryResult Execute(const FSQLUIQueryRequest& Request) override;

private:
	FString UnavailableReason;
};
