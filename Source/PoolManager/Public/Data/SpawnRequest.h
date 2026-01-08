// Copyright (c) Yevhenii Selivanov

#pragma once

#include "UObject/Object.h"

// Pool Manager
#include "Data/PoolObjectHandle.h"
#include "Data/SpawnCallbacks.h"
#include "Data/SpawnRequestPriority.h"

// UE
#include "Templates/NonNullSubclassOf.h"

#include "SpawnRequest.generated.h"

/**
 * Define a structure to hold the necessary information for spawning an object.
 */
USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/PoolManager.PoolManagerUtils.MakeSpawnRequest"))
struct POOLMANAGER_API FSpawnRequest
{
	GENERATED_BODY()

	/** Default constructor. */
	FSpawnRequest() = default;

	/** Parameterized constructor that takes class of the object, generates handle automatically. */
	explicit FSpawnRequest(const UClass* InClass);

	/** Returns array of spawn requests by specified class and their amount. */
	static void MakeRequests(TArray<FSpawnRequest>& OutRequests, const UClass* InClass, int32 Amount, ESpawnRequestPriority Priority);

	/** Leave only those requests that are not in the list of free objects. */
	static void FilterRequests(TArray<FSpawnRequest>& InOutRequests, const TArray<struct FPoolObjectData>& FreeObjectsData, int32 ExpectedAmount = INDEX_NONE);

	/** Transform of the object to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	FTransform Transform = FTransform::Identity;

	/** Priority of the spawn request in the queue, higher priority object is spawned first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	ESpawnRequestPriority Priority = ESpawnRequestPriority::Normal;

	/** The handle associated with spawning pool object for management within the Pool Manager system.
	 * Is generated automatically if not set. */
	UPROPERTY(BlueprintReadOnly, Transient)
	FPoolObjectHandle Handle = FPoolObjectHandle::EmptyHandle;

	/** Contains the functions that are called when the object is spawned. */
	FSpawnCallbacks Callbacks;

	/** Returns true if this spawn request can be processed. */
	FORCEINLINE bool IsValid() const { return Handle.IsValid(); }

	/** Class of the object to spawn. */
	const FORCEINLINE UClass* GetClass() const { return Handle.GetObjectClass(); }

	/** Returns requested class or crashes if it is not set or can't be casted to the specified type. */
	template <typename T = UObject>
	FORCEINLINE TNonNullSubclassOf<T> GetClassChecked() const { return TNonNullSubclassOf<T>(const_cast<UClass*>(Handle.GetObjectClass())); }
};
