// Copyright (c) Lim Young

#pragma once

#include "CoreMinimal.h"
#include "PoolManagerTypes.h"
#include "UObject/Interface.h"
#include "PoolObjectCallback.generated.h"

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
	 * @param IsNewSpawned If true, the object is newly spawned.
	 * @param Transform The transform of the object.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "PoolManager")
	void OnTakeFromPool(bool IsNewSpawned, const FTransform& Transform);

	/**
	 * Called when the object is returned to the pool.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "PoolManager")
	void OnReturnToPool();

	/**
	 * Called when the object is activated or deactivated.
	 * @param NewState The new state of the object.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "PoolManager")
	void OnChangedStateInPool(EPoolObjectState NewState);
};
