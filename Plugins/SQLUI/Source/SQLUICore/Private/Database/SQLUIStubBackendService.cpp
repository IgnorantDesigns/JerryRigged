#include "Database/SQLUIStubBackendService.h"

FSQLUIStubBackendService::FSQLUIStubBackendService(FString InUnavailableReason)
	: UnavailableReason(MoveTemp(InUnavailableReason))
{
	if (UnavailableReason.IsEmpty())
	{
		UnavailableReason = TEXT("SQLUI database backend is unavailable or not implemented.");
	}
}

bool FSQLUIStubBackendService::Open(const FString& DatabasePath, FString& OutErrorMessage)
{
	OutErrorMessage = UnavailableReason;
	return false;
}

void FSQLUIStubBackendService::Close()
{
}

bool FSQLUIStubBackendService::IsAvailable() const
{
	return false;
}

FSQLUIQueryResult FSQLUIStubBackendService::Execute(const FSQLUIQueryRequest& Request)
{
	FSQLUIQueryResult Result;
	Result.bSucceeded = false;
	Result.bBackendUnavailable = true;
	Result.ErrorMessage = UnavailableReason;
	return Result;
}
