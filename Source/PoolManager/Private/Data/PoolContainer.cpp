// Copyright (c) Yevhenii Selivanov

#include "Data/PoolContainer.h"

// Pool Manager
#include "Data/PoolObjectHandle.h"
#include "Factories/PoolFactory_UObject.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolContainer)

// Empty pool data container
const FPoolContainer FPoolContainer::EmptyPool = FPoolContainer();

// Parameterized constructor that takes a class of the pool
FPoolContainer::FPoolContainer(const UClass* InClass)
    : ObjectClass(InClass)
{
}

// Returns the pointer to the Pool element by specified object
FPoolObjectData* FPoolContainer::FindInPool(const UObject& Object)
{
	return PoolObjects.FindByKey(&Object);
}

// Returns the pointer to the Pool element by specified handle
FPoolObjectData* FPoolContainer::FindInPool(const FPoolObjectHandle& Handle)
{
	if (!ensureMsgf(Handle.IsValid(), TEXT("ASSERT: [%i] %hs:\n'Handle' is not valid!"), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}

	return PoolObjects.FindByKey(Handle);
}

// Returns factory or crashes as critical error if it is not set
UPoolFactory_UObject& FPoolContainer::GetFactoryChecked() const
{
	checkf(Factory, TEXT("ERROR: [%i] %hs:\n'Factory' is null!"), __LINE__, __FUNCTION__);
	return *Factory;
}
