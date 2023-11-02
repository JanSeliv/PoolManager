// Copyright (c) Yevhenii Selivanov

#include "PoolManagerTypes.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolManagerTypes)

// Empty pool object data
const FPoolObjectData FPoolObjectData::EmptyObject = FPoolObjectData();

// Empty pool data container
const FPoolContainer FPoolContainer::EmptyPool = FPoolContainer();

// Parameterized constructor that takes object to keep
FPoolObjectData::FPoolObjectData(UObject* InPoolObject)
{
	PoolObject = InPoolObject;
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

// Returns factory or crashes as critical error if it is not set
UPoolFactory_UObject& FPoolContainer::GetFactoryChecked() const
{
	checkf(Factory, TEXT("ERROR: [%i] %s:\n'Factory' is null!"), __LINE__, *FString(__FUNCTION__));
	return *Factory;
}
