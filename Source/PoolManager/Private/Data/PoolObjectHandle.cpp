// Copyright (c) Yevhenii Selivanov

#include "Data/PoolObjectHandle.h"

// Pool Manager
#include "Data/PoolObjectData.h"
#include "Data/SpawnRequest.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolObjectHandle)

// Empty pool object handle
const FPoolObjectHandle FPoolObjectHandle::EmptyHandle = FPoolObjectHandle();

// Parameterized constructor that takes class of the object, generates handle automatically
FPoolObjectHandle::FPoolObjectHandle(const UClass* InClass)
{
	*this = NewHandle(InClass);
}

// Generates a new handle for the specified object class
FPoolObjectHandle FPoolObjectHandle::NewHandle(const UClass* InObjectClass)
{
	if (!ensureMsgf(InObjectClass, TEXT("ASSERT: [%i] %hs:\n'InObjectClass' is null, can't generate new handle!"), __LINE__, __FUNCTION__))
	{
		return EmptyHandle;
	}

	FPoolObjectHandle Handle;
	Handle.ObjectClass = InObjectClass;
	Handle.Hash = FGuid::NewGuid();
	return Handle;
}

// Converts from array of spawn requests to array of handles
void FPoolObjectHandle::Conv_RequestsToHandles(TArray<FPoolObjectHandle>& OutHandles, const TArray<FSpawnRequest>& InRequests)
{
	if (!OutHandles.IsEmpty())
	{
		OutHandles.Empty();
	}

	OutHandles.Reserve(InRequests.Num());
	for (const FSpawnRequest& Request : InRequests)
	{
		OutHandles.Emplace(Request.Handle);
	}
}

// Converts from array of pool objects to array of handles
void FPoolObjectHandle::Conv_ObjectsToHandles(TArray<FPoolObjectHandle>& OutHandles, const TArray<FPoolObjectData>& InRequests)
{
	if (!OutHandles.IsEmpty())
	{
		OutHandles.Empty();
	}

	OutHandles.Reserve(InRequests.Num());
	for (const FPoolObjectData& It : InRequests)
	{
		OutHandles.Emplace(It.Handle);
	}
}
