// Copyright (c) Yevhenii Selivanov

#pragma once

#include "UObject/Object.h"

// Pool Manager
#include "Data/PoolObjectHandle.h"

#include "PoolObjectData.generated.h"

enum class EPoolObjectState : uint8;

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

	/** Equal operator to find the pool object. */
	friend POOLMANAGER_API bool operator==(const FPoolObjectData& A, const FPoolObjectData& B) { return A.Handle == B.Handle; }
	friend POOLMANAGER_API bool operator==(const FPoolObjectData& A, const FPoolObjectHandle& B) { return A.Handle == B; }
	friend POOLMANAGER_API bool operator==(const FPoolObjectData& A, const UObject* B) { return A.PoolObject == B; }
};
