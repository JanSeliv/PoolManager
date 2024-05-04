// Copyright (c) Yevhenii Selivanov

#include "PoolManagerTypes.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolManagerTypes)

// Empty pool object handle
const FPoolObjectHandle FPoolObjectHandle::EmptyHandle = FPoolObjectHandle();

// Empty pool object data
const FPoolObjectData FPoolObjectData::EmptyObject = FPoolObjectData();

// Empty pool data container
const FPoolContainer FPoolContainer::EmptyPool = FPoolContainer();

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

// Converts to array of objects
void FPoolObjectData::Conv_PoolDataToObjects(TArray<UObject*>& OutObjects, const TArray<FPoolObjectData>& InPoolData)
{
	if (!OutObjects.IsEmpty())
	{
		OutObjects.Empty();
	}

	OutObjects.Reserve(InPoolData.Num());
	for (const FPoolObjectData& It : InPoolData)
	{
		OutObjects.Add(It.PoolObject);
	}
}

// Parameterized constructor that takes object to keep
FPoolObjectData::FPoolObjectData(UObject* InPoolObject)
{
	PoolObject = InPoolObject;
}

// Returns the state of the object in the pool
EPoolObjectState FPoolObjectData::GetState() const
{
	return bIsActive ? EPoolObjectState::Active : EPoolObjectState::Inactive;
}

// Parameterized constructor that takes a class of the pool
FPoolContainer::FPoolContainer(const UClass* InClass)
{
	ObjectClass = InClass;
}

// Returns the pointer to the Pool element by specified object
FPoolObjectData* FPoolContainer::FindInPool(const UObject& Object)
{
	return PoolObjects.FindByPredicate([&Object](const FPoolObjectData& It)
	{
		return It.PoolObject == &Object;
	});
}

// Returns the pointer to the Pool element by specified handle
FPoolObjectData* FPoolContainer::FindInPool(const FPoolObjectHandle& Handle)
{
	if (!ensureMsgf(Handle.IsValid(), TEXT("ASSERT: [%i] %hs:\n'Handle' is not valid!"), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}

	return PoolObjects.FindByPredicate([&Handle](const FPoolObjectData& PoolObjectIt)
	{
		return PoolObjectIt.Handle == Handle;
	});
}

// Returns factory or crashes as critical error if it is not set
UPoolFactory_UObject& FPoolContainer::GetFactoryChecked() const
{
	checkf(Factory, TEXT("ERROR: [%i] %hs:\n'Factory' is null!"), __LINE__, __FUNCTION__);
	return *Factory;
}

// Parameterized constructor that takes class of the object to spawn, generates handle automatically
FSpawnRequest::FSpawnRequest(const UClass* InClass)
	: Handle(InClass) {}

// Returns array of spawn requests by specified class and their amount
void FSpawnRequest::MakeRequests(TArray<FSpawnRequest>& OutRequests, const UClass* InClass, int32 Amount)
{
	if (!OutRequests.IsEmpty())
	{
		OutRequests.Empty();
	}

	for (int32 Index = 0; Index < Amount; ++Index)
	{
		OutRequests.Emplace(InClass);
	}
}

// Leave only those requests that are not in the list of free objects
void FSpawnRequest::FilterRequests(TArray<FSpawnRequest>& InOutRequests, const TArray<FPoolObjectData>& FreeObjectsData, int32 ExpectedAmount/* = INDEX_NONE*/)
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
	           TEXT("ASSERT: [%i] %hs:\nInRequests %s and Expected Amount %s don't equal, something went wrong!"),
	           __LINE__, __FUNCTION__, *FString::FromInt(InOutRequests.Num()), *FString::FromInt(ExpectedAmount));
}
