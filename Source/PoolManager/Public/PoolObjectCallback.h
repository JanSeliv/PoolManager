// Copyright (c) Lim Young & Yevhenii Selivanov

#pragma once

#include "UObject/Interface.h"
#include "PoolObjectCallback.generated.h"

enum class EPoolObjectState : uint8;

UINTERFACE()
class UPoolObjectCallback : public UInterface
{
	GENERATED_BODY()
};

/**
 * Provides for the reception of pool manager events from pool objects.
 * Implement this interface in your pool object class to receive pool manager events.
 */
class POOLMANAGER_API IPoolObjectCallback
{
	GENERATED_BODY()

public:
	/**
	 * Called when the object is taken from the pool.If IsNewSpawned is true, the object is newly spawned.
	 * @param bIsNewSpawned If true, the object is newly spawned.
	 * @param Transform The transform of the object.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "PoolManager", meta = (AutoCreateRefTerm = "Transform"))
	void OnTakeFromPool(bool bIsNewSpawned, const FTransform& Transform);
	virtual void OnTakeFromPool_Implementation(bool bIsNewSpawned, const FTransform& Transform) {}

	/**
	 * Called when the object is returned to the pool.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "PoolManager")
	void OnReturnToPool();
	virtual void OnReturnToPool_Implementation() {}

	/**
	 * Called when the object is activated or deactivated.
	 * @param NewState The new state of the object.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "PoolManager")
	void OnChangedStateInPool(EPoolObjectState NewState);
	virtual void OnChangedStateInPool_Implementation(EPoolObjectState NewState) {}
};
