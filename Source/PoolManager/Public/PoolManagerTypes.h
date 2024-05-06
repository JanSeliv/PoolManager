// Copyright (c) Yevhenii Selivanov

#pragma once

#include "UObject/Object.h"
//---
#include "Misc/Guid.h"
#include "Templates/NonNullSubclassOf.h"
//---
#include "PoolManagerTypes.generated.h"

/**
 * States of the object in Pool
 */
UENUM(BlueprintType)
enum class EPoolObjectState : uint8
{
	///< Is not handled by Pool Manager
	None,
	///< Contains in pool, is free and ready to be taken
	Inactive,
	///< Was taken from pool and can be returned back.
	Active
};

struct FSpawnRequest;
struct FPoolObjectData;

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
	static void Conv_RequestsToHandles(TArray<FPoolObjectHandle>& OutHandles, const TArray<FSpawnRequest>& InRequests);

	/** Converts from array of pool objects to array of handles. */
	static void Conv_ObjectsToHandles(TArray<FPoolObjectHandle>& OutHandles, const TArray<FPoolObjectData>& InRequests);

	/*********************************************************************************************
	 * Getters and operators
	 ********************************************************************************************* */

	/** Returns true if Hash is generated. */
	FORCEINLINE bool IsValid() const { return ObjectClass && Hash.IsValid(); }

	/** Empties the handle. */
	void Invalidate() { *this = EmptyHandle; }

	friend POOLMANAGER_API uint32 GetTypeHash(const FPoolObjectHandle& InHandle) { return GetTypeHash(InHandle.Hash); }
	friend POOLMANAGER_API bool operator==(const FPoolObjectHandle& Lhs, const FPoolObjectHandle& Rhs) { return Lhs.Hash == Rhs.Hash; }

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

/**
 * Contains the data that describe specific object in a pool.
 */
USTRUCT(BlueprintType)
struct POOLMANAGER_API FPoolObjectData
{
	GENERATED_BODY()

	/* Default constructor. */
	FPoolObjectData() = default;

	/* Parameterized constructor that takes object to keep. */
	explicit FPoolObjectData(UObject* InPoolObject);

	/*********************************************************************************************
	 * Static Helpers
	 ********************************************************************************************* */

	/** Empty pool object data. */
	static const FPoolObjectData EmptyObject;

	/** Converts to array of objects. */
	static void Conv_PoolDataToObjects(TArray<UObject*>& OutObjects, const TArray<FPoolObjectData>& InPoolData);

	/*********************************************************************************************
	 * Fields
	 ********************************************************************************************* */

	/** Is true whenever the object is taken from the pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	bool bIsActive = false;

	/** The object that is handled by the pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	TObjectPtr<UObject> PoolObject = nullptr;

	/** The handle associated with this pool object for management within the Pool Manager system. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient)
	FPoolObjectHandle Handle = FPoolObjectHandle::EmptyHandle;

	/*********************************************************************************************
	 * Getters and operators
	 ********************************************************************************************* */

	/** Returns true if the object is taken from the pool. */
	FORCEINLINE bool IsActive() const { return bIsActive && IsValid(); }

	/** Returns the state of the object in the pool. */
	EPoolObjectState GetState() const;

	/** Returns true if handled object is inactive and ready to be taken from pool. */
	FORCEINLINE bool IsFree() const { return !bIsActive && IsValid(); }

	/** Returns true if the object is created. */
	FORCEINLINE bool IsValid() const { return PoolObject && Handle.IsValid(); }

	/** conversion to "bool" returning true if pool object is valid. */
	FORCEINLINE operator bool() const { return IsValid(); }

	/** Element access. */
	template <typename T = UObject>
	FORCEINLINE T* Get() const { return Cast<T>(PoolObject.Get()); }

	/** Element access, crash if object is not valid. */
	template <typename T = UObject>
	FORCEINLINE T& GetChecked() const { return *CastChecked<T>(PoolObject.Get()); }

	/** Element access. */
	FORCEINLINE UObject* operator->() const { return PoolObject.Get(); }
};

/**
 * Keeps the objects by class to be handled by the Pool Manager.
 */
USTRUCT(BlueprintType)
struct POOLMANAGER_API FPoolContainer
{
	GENERATED_BODY()

	/** Empty pool data container. */
	static const FPoolContainer EmptyPool;

	/* Default constructor. */
	FPoolContainer() = default;

	/* Parameterized constructor that takes a class of the pool. */
	explicit FPoolContainer(const UClass* InClass);

	/** Class of all objects in this pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	TObjectPtr<const UClass> ObjectClass = nullptr;

	/** Factory that manages objects for this pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	TObjectPtr<class UPoolFactory_UObject> Factory = nullptr;

	/** All objects in this pool that are handled by the Pool Manager. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	TArray<FPoolObjectData> PoolObjects;

	/** Returns the pointer to the Pool element by specified object. */
	FPoolObjectData* FindInPool(const UObject& Object);
	const FORCEINLINE FPoolObjectData* FindInPool(const UObject& Object) const { return const_cast<FPoolContainer*>(this)->FindInPool(Object); }

	/** Returns the pointer to the Pool element by specified handle. */
	FPoolObjectData* FindInPool(const FPoolObjectHandle& Handle);
	const FORCEINLINE FPoolObjectData* FindInPool(const FPoolObjectHandle& Handle) const { return const_cast<FPoolContainer*>(this)->FindInPool(Handle); }

	/** Returns factory or crashes as critical error if it is not set. */
	UPoolFactory_UObject& GetFactoryChecked() const;

	/** Returns true if the class is set for the Pool. */
	FORCEINLINE bool IsValid() const { return ObjectClass != nullptr; }
};

typedef TFunction<void(const FPoolObjectData&)> FOnSpawnCallback;
typedef TFunction<void(const TArray<FPoolObjectData>&)> FOnSpawnAllCallback;

/**
 * Contains the functions that are called when the object is spawned.
 */
struct POOLMANAGER_API FSpawnCallbacks
{
	/** Returns complete object data before registration in the Pool. */
	FOnSpawnCallback OnPreRegistered = nullptr;

	/** Returns already spawned and registered object. */
	FOnSpawnCallback OnPostSpawned = nullptr;
};

/**
 * Define a structure to hold the necessary information for spawning an object.
 */
USTRUCT(BlueprintType)
struct POOLMANAGER_API FSpawnRequest
{
	GENERATED_BODY()

	/** Default constructor. */
	FSpawnRequest() = default;

	/** Parameterized constructor that takes class of the object, generates handle automatically. */
	explicit FSpawnRequest(const UClass* InClass);

	/** Returns array of spawn requests by specified class and their amount. */
	static void MakeRequests(TArray<FSpawnRequest>& OutRequests, const UClass* InClass, int32 Amount);

	/** Leave only those requests that are not in the list of free objects. */
	static void FilterRequests(TArray<FSpawnRequest>& InOutRequests, const TArray<FPoolObjectData>& FreeObjectsData, int32 ExpectedAmount = INDEX_NONE);

	/** Transform of the object to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	FTransform Transform = FTransform::Identity;

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
