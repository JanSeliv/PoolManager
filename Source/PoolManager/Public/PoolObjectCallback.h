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
	UFUNCTION(BlueprintNativeEvent, Category = "PoolManager")
	void OnTakeFromPool(const FTransform& Transform);

	UFUNCTION(BlueprintNativeEvent, Category = "PoolManager")
	void OnReturnToPool();

	UFUNCTION(BlueprintNativeEvent, Category = "PoolManager")
	void OnChangedStateInPool(EPoolObjectState NewState);
};
