// Copyright (c) Yevhenii Selivanov

#include "Data/SpawnRequest.h"

// Pool Manager
#include "Data/PoolObjectData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpawnRequest)

// Parameterized constructor that takes class of the object to spawn, generates handle automatically
FSpawnRequest::FSpawnRequest(const UClass* InClass)
    : Handle(InClass)
{
}

// Returns array of spawn requests by specified class and their amount
void FSpawnRequest::MakeRequests(TArray<FSpawnRequest>& OutRequests, const UClass* InClass, int32 Amount, ESpawnRequestPriority Priority)
{
	if (!OutRequests.IsEmpty())
	{
		OutRequests.Empty();
	}

	for (int32 Index = 0; Index < Amount; ++Index)
	{
		FSpawnRequest Request(InClass);
		Request.Priority = Priority;
		OutRequests.Emplace(MoveTemp(Request));
	}
}

// Leave only those requests that are not in the list of free objects
void FSpawnRequest::FilterRequests(TArray<FSpawnRequest>& InOutRequests, const TArray<FPoolObjectData>& FreeObjectsData, int32 ExpectedAmount /* = INDEX_NONE*/)
{
	if (!ensureMsgf(!InOutRequests.IsEmpty(), TEXT("ASSERT: [%i] %hs:\n'InOutRequests' is empty!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	InOutRequests.RemoveAll([&FreeObjectsData](const FSpawnRequest& Request)
	{
		return FreeObjectsData.ContainsByPredicate([&Request](const FPoolObjectData& It)
		{
			return It.Handle == Request.Handle;
		});
	});

	ensureMsgf(ExpectedAmount == INDEX_NONE || InOutRequests.Num() == ExpectedAmount,
	    TEXT("ASSERT: [%i] %hs:\nInRequests %i and Expected Amount %i don't equal, something went wrong!"),
	    __LINE__, __FUNCTION__, InOutRequests.Num(), ExpectedAmount);
}
