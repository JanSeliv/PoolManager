// Copyright (c) Yevhenii Selivanov

#pragma once

#include "PoolFactory_UObject.h"
//---
#include "PoolFactory_Actor.generated.h"

/**
 * Is responsible for managing actors, it handles such differences in actors as:
 * Creation: call SpawnActor.  
 * Destruction: call DestroyActor.
 * Pool: change visibility, collision, ticking, etc.
 */
UCLASS()
class POOLMANAGER_API UPoolFactory_Actor : public UPoolFactory_UObject
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Setup overrides
	 ********************************************************************************************* */
public:
	/** Is overridden to handle Actors-inherited classes. */
	virtual const UClass* GetObjectClass_Implementation() const override;

	/*********************************************************************************************
	 * Creation
	 ********************************************************************************************* */
public:
	/** Is overridden to spawn actors using its engine's Spawn Actor method. */
	virtual UObject* SpawnNow_Implementation(const FSpawnRequest& Request) override;

	/** Is overridden to finish spawning the actor since it was deferred. */
	virtual void OnPreRegistered(const FSpawnRequest& Request, const FPoolObjectData& ObjectData) override;

	/*********************************************************************************************
	 * Destruction
	 ********************************************************************************************* */
public:
	/** Is overridden to destroy given actor using its engine's Destroy Actor method. */
	virtual void Destroy_Implementation(UObject* Object) override;

	/*********************************************************************************************
	 * Pool
	 ********************************************************************************************* */
public:
	/** Is overridden to set transform to the actor before taking the object from its pool. */
	virtual void OnTakeFromPool_Implementation(UObject* Object, const FTransform& Transform) override;

	/** Is overridden to reset transform to the actor before returning the object to its pool. */
	virtual void OnReturnToPool_Implementation(UObject* Object) override;

	/** Is overridden to change visibility, collision, ticking, etc. according new state. */
	virtual void OnChangedStateInPool_Implementation(EPoolObjectState NewState, UObject* InObject) override;
};
