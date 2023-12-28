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

// Generates a new handle for the specified object class
FPoolObjectHandle FPoolObjectHandle::NewHandle(const UClass* InObjectClass)
{
	if (!ensureMsgf(InObjectClass, TEXT("ASSERT: [%i] %s:\n'InObjectClass' is null, can't generate new handle!"), __LINE__, *FString(__FUNCTION__)))
	{
		return EmptyHandle;
	}

	FPoolObjectHandle Handle;
	Handle.ObjectClass = InObjectClass;
	Handle.Hash = FGuid::NewGuid();
	return Handle;
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
	if (!ensureMsgf(Handle.IsValid(), TEXT("ASSERT: [%i] %s:\n'Handle' is not valid!"), __LINE__, *FString(__FUNCTION__)))
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
	checkf(Factory, TEXT("ERROR: [%i] %s:\n'Factory' is null!"), __LINE__, *FString(__FUNCTION__));
	return *Factory;
}
