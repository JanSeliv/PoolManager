// Copyright (c) Yevhenii Selivanov

#pragma once

#include "UObject/Object.h"

// Pool Manager
#include "Data/PoolObjectData.h"

#include "PoolContainer.generated.h"

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
	FPoolObjectData* FindInPool(const struct FPoolObjectHandle& Handle);
	const FORCEINLINE FPoolObjectData* FindInPool(const struct FPoolObjectHandle& Handle) const { return const_cast<FPoolContainer*>(this)->FindInPool(Handle); }

	/** Returns factory or crashes as critical error if it is not set. */
	class UPoolFactory_UObject& GetFactoryChecked() const;

	/** Returns true if the class is set for the Pool. */
	FORCEINLINE bool IsValid() const { return ObjectClass != nullptr; }

	/** Equal operator to find the pool */
	friend POOLMANAGER_API bool operator==(const FPoolContainer& A, const FPoolContainer& B) { return A.ObjectClass == B.ObjectClass; }
	friend POOLMANAGER_API bool operator==(const FPoolContainer& A, const UClass* B) { return A.ObjectClass == B; }
};
