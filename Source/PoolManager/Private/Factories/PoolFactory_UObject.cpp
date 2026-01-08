// Copyright (c) Yevhenii Selivanov

#include "Factories/PoolFactory_UObject.h"

// Pool Manager
#include "PoolObjectCallback.h"
#include "Data/PoolManagerSettings.h"

// UE
#include "TimerManager.h"
#include "Engine/World.h"

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

	// Lambda to find the correct insertion index based on priority
	auto FindInsertionIndex = [&](ESpawnRequestPriority Priority)
	{
		int32 InsertIdx = 0;
		for (int32 Index = 0; Index < SpawnQueue.Num(); ++Index)
		{
			if (SpawnQueue[Index].Priority < Priority)
			{
				break;
			}
			InsertIdx = Index + 1;
		}
		return InsertIdx;
	};

	// Insert request based on priority
	switch (Request.Priority)
	{
	case ESpawnRequestPriority::Critical:
		{
			// Immediate processing for Critical priority requests
			ProcessRequestNow(Request);
			// Exit since we don't add Critical requests to the queue
			return;
		}

	case ESpawnRequestPriority::High: // Fall-through
	case ESpawnRequestPriority::Medium:
		{
			// Use lambda to find the correct insertion index based on the priority
			const int32 InsertIdx = FindInsertionIndex(Request.Priority);
			SpawnQueue.Insert(Request, InsertIdx);
		}
		break;

	case ESpawnRequestPriority::Normal:
		// Normal, add to the end of the queue
		SpawnQueue.Emplace(Request);
		break;

	default:
		ensureAlwaysMsgf(false, TEXT("ASSERT: [%i] %hs:\n'Priority' is not valid: %d"), __LINE__, __FUNCTION__, static_cast<int32>(Request.Priority));
	}

	// If this is the first object in the queue, schedule the OnNextTickProcessSpawn to be called on the next frame
	// Creating UObjects on separate threads is not thread-safe and leads to problems with garbage collection,
	// so we will create them on the game thread, but defer to next frame to avoid hitches
	if (SpawnQueue.Num() == 1)
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
	if (SpawnQueue.IsValidIndex(0))
	{
		// Copy and remove first request from the queue (without Swap to keep order)
		OutRequest = SpawnQueue[0];
		SpawnQueue.RemoveAt(0);

		bResult = OutRequest.IsValid();
	}
	return ensureAlwaysMsgf(bResult, TEXT("ASSERT: [%i] %hs:\nFailed to dequeue the spawn request, handle is '%s'!"), __LINE__, __FUNCTION__, *OutRequest.Handle.GetHash().ToString());
}

// Calls SpawnNow with the given request and process the callbacks
void UPoolFactory_UObject::ProcessRequestNow(const FSpawnRequest& Request)
{
	UObject* CreatedObject = SpawnNow(Request);
	checkf(CreatedObject, TEXT("ERROR: [%i] %hs:\n'CreatedObject' failed to spawn!"), __LINE__, __FUNCTION__);

	FPoolObjectData ObjectData;
	ObjectData.bIsActive = true;
	ObjectData.PoolObject = CreatedObject;
	ObjectData.Handle = Request.Handle;

	OnPreRegistered(Request, ObjectData);
	OnPostSpawned(Request, ObjectData);
}

// Alternative method to remove specific spawn request from the queue and returns it.
bool UPoolFactory_UObject::DequeueSpawnRequestByHandle(const FPoolObjectHandle& Handle, FSpawnRequest& OutRequest)
{
	const int32 Idx = SpawnQueue.IndexOfByPredicate([&Handle](const FSpawnRequest& Request)
	{
		return Request.Handle == Handle;
	});

	if (!ensureMsgf(SpawnQueue.IsValidIndex(Idx), TEXT("ASSERT: [%i] %hs:\nHandle is not found within Spawn Requests, can't dequeue it: %s"), __LINE__, __FUNCTION__, *Handle.GetHash().ToString()))
	{
		return false;
	}

	// Copy and remove first request from the queue (without Swap to keep order)
	OutRequest = SpawnQueue[Idx];
	SpawnQueue.RemoveAt(Idx);

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

	FTakeFromPoolPayload Payload;
	Payload.bIsNewSpawned = true;
	Payload.Transform = Request.Transform;
	OnTakeFromPool(ObjectData.Get(), Payload);
}

// Is called on next frame to process a chunk of the spawn queue
void UPoolFactory_UObject::OnNextTickProcessSpawn_Implementation()
{
	int32 ObjectsPerFrame = UPoolManagerSettings::Get().GetSpawnObjectsPerFrame();
	if (!ensureMsgf(ObjectsPerFrame >= 1, TEXT("ASSERT: [%i] %hs:\n'ObjectsPerFrame' is less than 1, set the config!"), __LINE__, __FUNCTION__))
	{
		ObjectsPerFrame = 1;
	}

	const int32 NumToSpawn = FMath::Min(ObjectsPerFrame, SpawnQueue.Num());
	for (int32 Index = 0; Index < NumToSpawn; ++Index)
	{
		FSpawnRequest OutRequest;
		if (DequeueSpawnRequest(OutRequest))
		{
			ProcessRequestNow(OutRequest);
		}
	}

	// If there are more actors to spawn, schedule this function to be called again on the next frame
	// Is deferred to next frame instead of doing it on other threads since spawning actors is not thread-safe operation
	if (!SpawnQueue.IsEmpty())
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

/*********************************************************************************************
 * Pool
 ********************************************************************************************* */

// Is called right before taking the object from its pool
void UPoolFactory_UObject::OnTakeFromPool_Implementation(UObject* Object, const FTakeFromPoolPayload& Payload)
{
	// Is optional callback if object implements interface
	if (Object && Object->Implements<UPoolObjectCallback>())
	{
		IPoolObjectCallback::Execute_OnTakeFromPool(Object, Payload);
	}
}

// Is called right before returning the object back to its pool
void UPoolFactory_UObject::OnReturnToPool_Implementation(UObject* Object)
{
	// Is optional callback if object implements interface
	if (Object && Object->Implements<UPoolObjectCallback>())
	{
		IPoolObjectCallback::Execute_OnReturnToPool(Object);
	}
}

// Is called when activates the object to take it from pool or deactivate when is returned back
void UPoolFactory_UObject::OnChangedStateInPool_Implementation(EPoolObjectState NewState, UObject* InObject)
{
	// Is optional callback if object implements interface
	if (InObject && InObject->Implements<UPoolObjectCallback>())
	{
		IPoolObjectCallback::Execute_OnChangedStateInPool(InObject, NewState);
	}
}
