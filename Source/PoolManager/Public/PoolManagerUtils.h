// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

// Pool Manager
#include "PoolManagerTypes.h"

#include "PoolManagerUtils.generated.h"

enum class ESpawnRequestPriority : uint8;

/**
 * Blueprint utility class for Pool Manager.
 * Provides helper functions for blueprints.
 */
UCLASS()
class POOLMANAGER_API UPoolManagerUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Creates a spawn request for pool objects.
	 * This function properly initializes the FSpawnRequest with a valid handle.
	 * @param ObjectClass The class of object to spawn from the pool.
	 * @param Transform The transform for the spawned object.
	 * @param Priority The priority of this spawn request in the queue.
	 * @return A properly initialized spawn request ready to use. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Pool Manager]", meta = (NativeMakeFunc, AutoCreateRefTerm = "Transform"))
	static struct FSpawnRequest MakeSpawnRequest(TSubclassOf<UObject> ObjectClass, const FTransform& Transform, ESpawnRequestPriority Priority = ESpawnRequestPriority::Normal);
};
