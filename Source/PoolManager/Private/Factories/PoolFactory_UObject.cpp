// Copyright (c) Yevhenii Selivanov

#include "Factories/PoolFactory_UObject.h"
//---
#include "Data/PoolManagerSettings.h"
//---
#include "TimerManager.h"
#include "Engine/World.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolFactory_UObject)

/*********************************************************************************************
 * Creation
 ********************************************************************************************* */

// Method to queue object spawn requests
void UPoolFactory_UObject::RequestSpawn_Implementation(const FSpawnRequest& Request)
{
	if (!ensureMsgf(Request.IsValid(), TEXT("ASSERT: [%i] %hs:\n'Request' is not valid and can't be processed!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// Add request to queue
	SpawnQueueInternal.Emplace(Request);

	// If this is the first object in the queue, schedule the OnNextTickProcessSpawn to be called on the next frame
	// Creating UObjects on separate threads is not thread-safe and leads to problems with garbage collection,
	// so we will create them on the game thread, but defer to next frame to avoid hitches
	if (SpawnQueueInternal.Num() == 1)
	{
		const UWorld* World = GetWorld();
		checkf(World, TEXT("ERROR: [%i] %hs:\n'World' is null!"), __LINE__, __FUNCTION__);

		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::OnNextTickProcessSpawn);
	}
}

// Removes the first spawn request from the queue and returns it
bool UPoolFactory_UObject::DequeueSpawnRequest(FSpawnRequest& OutRequest)
{
	bool bResult = false;
	if (SpawnQueueInternal.IsValidIndex(0))
	{
		// Copy and remove first request from the queue (without Swap to keep order)
		OutRequest = SpawnQueueInternal[0];
		SpawnQueueInternal.RemoveAt(0);

		bResult = OutRequest.IsValid();
	}
	return ensureAlwaysMsgf(bResult, TEXT("ASSERT: [%i] %hs:\nFailed to dequeue the spawn request, handle is '%s'!"), __LINE__, __FUNCTION__, *OutRequest.Handle.GetHash().ToString());
}

// Alternative method to remove specific spawn request from the queue and returns it.
bool UPoolFactory_UObject::DequeueSpawnRequestByHandle(const FPoolObjectHandle& Handle, FSpawnRequest& OutRequest)
{
	const int32 Idx = SpawnQueueInternal.IndexOfByPredicate([&Handle](const FSpawnRequest& Request)
	{
		return Request.Handle == Handle;
	});

	if (!ensureMsgf(SpawnQueueInternal.IsValidIndex(Idx), TEXT("ASSERT: [%i] %hs:\nHandle is not found within Spawn Requests, can't dequeue it: %s"), __LINE__, __FUNCTION__, *Handle.GetHash().ToString()))
	{
		return false;
	}

	// Copy and remove first request from the queue (without Swap to keep order)
	OutRequest = SpawnQueueInternal[Idx];
	SpawnQueueInternal.RemoveAt(Idx);

	return OutRequest.IsValid();
}

// Method to immediately spawn requested object
UObject* UPoolFactory_UObject::SpawnNow_Implementation(const FSpawnRequest& Request)
{
	return NewObject<UObject>(GetOuter(), Request.GetClassChecked());
}

// Notifies all listeners that the object is about to be spawned
void UPoolFactory_UObject::OnPreRegistered(const FSpawnRequest& Request, const FPoolObjectData& ObjectData)
{
	if (Request.Callbacks.OnPreRegistered != nullptr)
	{
		Request.Callbacks.OnPreRegistered(ObjectData);
	}
}

// Notifies all listeners that the object is spawned
void UPoolFactory_UObject::OnPostSpawned(const FSpawnRequest& Request, const FPoolObjectData& ObjectData)
{
	if (Request.Callbacks.OnPostSpawned != nullptr)
	{
		Request.Callbacks.OnPostSpawned(ObjectData);
	}
}

// Is called on next frame to process a chunk of the spawn queue
void UPoolFactory_UObject::OnNextTickProcessSpawn_Implementation()
{
	int32 ObjectsPerFrame = UPoolManagerSettings::Get().GetSpawnObjectsPerFrame();
	if (!ensureMsgf(ObjectsPerFrame >= 1, TEXT("ASSERT: [%i] %hs:\n'ObjectsPerFrame' is less than 1, set the config!"), __LINE__, __FUNCTION__))
	{
		ObjectsPerFrame = 1;
	}

	const int32 NumToSpawn = FMath::Min(ObjectsPerFrame, SpawnQueueInternal.Num());
	for (int32 Index = 0; Index < NumToSpawn; ++Index)
	{
		FSpawnRequest OutRequest;
		if (DequeueSpawnRequest(OutRequest))
		{
			UObject* CreatedObject = SpawnNow(OutRequest);
			checkf(CreatedObject, TEXT("ERROR: [%i] %hs:\n'CreatedObject' is failed to spawn!"), __LINE__, __FUNCTION__);

			FPoolObjectData ObjectData;
			ObjectData.bIsActive = true;
			ObjectData.PoolObject = CreatedObject;
			ObjectData.Handle = OutRequest.Handle;

			OnPreRegistered(OutRequest, ObjectData);
			OnPostSpawned(OutRequest, ObjectData);
		}
	}

	// If there are more actors to spawn, schedule this function to be called again on the next frame
	// Is deferred to next frame instead of doing it on other threads since spawning actors is not thread-safe operation
	if (!SpawnQueueInternal.IsEmpty())
	{
		const UWorld* World = GetWorld();
		checkf(World, TEXT("ERROR: [%i] %hs:\n'World' is null!"), __LINE__, __FUNCTION__);
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::OnNextTickProcessSpawn);
	}
}

/*********************************************************************************************
 * Destruction
 ********************************************************************************************* */

// Method to destroy given object
void UPoolFactory_UObject::Destroy_Implementation(UObject* Object)
{
	checkf(IsValid(Object), TEXT("ERROR: [%i] %hs:\n'IsValid(Object)' is not valid!"), __LINE__, __FUNCTION__);
	Object->ConditionalBeginDestroy();
}
