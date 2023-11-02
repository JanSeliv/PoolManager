// Copyright (c) Yevhenii Selivanov

#include "Factories/PoolFactory_UObject.h"
//---
#include "Data/PoolManagerSettings.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolFactory_UObject)

// Method to queue object spawn requests
void UPoolFactory_UObject::RequestSpawn_Implementation(const FSpawnRequest& Request)
{
	// Add request to queue
	SpawnQueueInternal.Enqueue(Request);

	// If this is the first object in the queue, schedule the OnNextTickProcessSpawn to be called on the next frame
	// Creating UObjects on separate threads is not thread-safe and leads to problems with garbage collection,
	// so we will create them on the game thread, but defer to next frame to avoid hitches
	if (++SpawnQueueSize == 1)
	{
		const UWorld* World = GetWorld();
		checkf(World, TEXT("ERROR: [%i] %s:\n'World' is null!"), __LINE__, *FString(__FUNCTION__));

		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::OnNextTickProcessSpawn);
	}
}

// Method to immediately spawn requested object
UObject* UPoolFactory_UObject::SpawnNow_Implementation(const FSpawnRequest& Request)
{
	UObject* CreatedObject = NewObject<UObject>(GetOuter(), Request.Class);

	if (Request.Callbacks.OnPreConstructed != nullptr)
	{
		Request.Callbacks.OnPreConstructed(CreatedObject);
	}

	if (Request.Callbacks.OnPostSpawned != nullptr)
	{
		Request.Callbacks.OnPostSpawned(CreatedObject);
	}

	return CreatedObject;
}

// Is called on next frame to process a chunk of the spawn queue
void UPoolFactory_UObject::OnNextTickProcessSpawn_Implementation()
{
	int32 ObjectsPerFrame = UPoolManagerSettings::Get().GetSpawnObjectsPerFrame();
	if (!ensureMsgf(ObjectsPerFrame >= 1, TEXT("ASSERT: [%i] %s:\n'ObjectsPerFrame' is less than 1, set the config!"), __LINE__, *FString(__FUNCTION__)))
	{
		ObjectsPerFrame = 1;
	}

	for (int32 Index = 0; Index < FMath::Min(ObjectsPerFrame, SpawnQueueSize); ++Index)
	{
		FSpawnRequest OutRequest;
		DequeueSpawnRequest(OutRequest);
		SpawnNow(OutRequest);
		--SpawnQueueSize;
	}

	// If there are more actors to spawn, schedule this function to be called again on the next frame
	// Is deferred to next frame instead of doing it on other threads since spawning actors is not thread-safe operation
	if (!SpawnQueueInternal.IsEmpty())
	{
		const UWorld* World = GetWorld();
		checkf(World, TEXT("ERROR: [%i] %s:\n'World' is null!"), __LINE__, *FString(__FUNCTION__));
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::OnNextTickProcessSpawn);
	}
}

// Method to destroy given object
void UPoolFactory_UObject::Destroy_Implementation(UObject* Object)
{
	checkf(IsValid(Object), TEXT("ERROR: [%i] %s:\n'IsValid(Object)' is not valid!"), __LINE__, *FString(__FUNCTION__));
	Object->ConditionalBeginDestroy();
}
