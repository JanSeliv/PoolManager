// Copyright (c) Yevhenii Selivanov

#include "Data/PoolObjectData.h"

// Pool Manager
#include "Data/PoolObjectState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolObjectData)

// Empty pool object data
const FPoolObjectData FPoolObjectData::EmptyObject = FPoolObjectData();

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
    : PoolObject(InPoolObject)
{
}

// Returns the state of the object in the pool
EPoolObjectState FPoolObjectData::GetState() const
{
	return bIsActive ? EPoolObjectState::Active : EPoolObjectState::Inactive;
}
