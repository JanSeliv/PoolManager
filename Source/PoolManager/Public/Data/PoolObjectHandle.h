// Copyright (c) Yevhenii Selivanov

#pragma once

#include "UObject/Object.h"

// UE
#include "Misc/Guid.h"

#include "PoolObjectHandle.generated.h"

/**
 * A handle for managing pool object indirectly.
 * - Provides a unique identifier, Hash, with associated object in the pool.
 * - Enables tracking and control of objects within the Pool Manager system.
 * - Useful in scenarios where object is requested from the pool and the handle is obtained immediately,
 *   even if the object spawning is delayed to a later frame or different thread.
 */
USTRUCT(BlueprintType)
struct POOLMANAGER_API FPoolObjectHandle
{
	GENERATED_BODY()

	/* Default constructor of empty handle. */
	FPoolObjectHandle() = default;

	/** Parameterized constructor that takes class of the object, generates handle automatically. */
	explicit FPoolObjectHandle(const UClass* InClass);

	/*********************************************************************************************
	 * Static Helpers
	 ********************************************************************************************* */

	/** Empty pool object handle. */
	static const FPoolObjectHandle EmptyHandle;

	/** Generates a new handle for the specified object class. */
	static FPoolObjectHandle NewHandle(const UClass* InObjectClass);

	/** Converts from array of spawn requests to array of handles. */
	static void Conv_RequestsToHandles(TArray<FPoolObjectHandle>& OutHandles, const TArray<struct FSpawnRequest>& InRequests);

	/** Converts from array of pool objects to array of handles. */
	static void Conv_ObjectsToHandles(TArray<FPoolObjectHandle>& OutHandles, const TArray<struct FPoolObjectData>& InRequests);

	/*********************************************************************************************
	 * Getters and operators
	 ********************************************************************************************* */

	/** Returns true if Hash is generated. */
	FORCEINLINE bool IsValid() const { return ObjectClass && Hash.IsValid(); }

	/** Empties the handle. */
	void Invalidate() { *this = EmptyHandle; }

	friend POOLMANAGER_API uint32 GetTypeHash(const FPoolObjectHandle& InHandle) { return GetTypeHash(InHandle.Hash); }
	friend POOLMANAGER_API bool operator==(const FPoolObjectHandle& A, const FPoolObjectHandle& B) { return A.Hash == B.Hash; }

	/*********************************************************************************************
	 * Fields
	 * Is private to prevent direct access to the fields, use NewHandle() instead.
	 ********************************************************************************************* */
	const UClass* GetObjectClass() const { return ObjectClass; }
	const FGuid& GetHash() const { return Hash; }

private:
	/** Class of the object in the pool. */
	UPROPERTY(Transient)
	const UClass* ObjectClass = nullptr;

	/** Generated hash for the object. */
	FGuid Hash;
};
