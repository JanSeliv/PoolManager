// Copyright (c) Yevhenii Selivanov

#pragma once

#include "UObject/Object.h"
//---
#include "PoolManagerTypes.h"
//---
#include "PoolFactory_UObject.generated.h"

/**
 * Each factory implements specific logic of creating and managing objects of its class and its children.
 * Factories are designed to handle such differences as:
 * Creation: UObjects call NewObject; Actors call SpawnActor, Components call NewObject+RegisterComponent, Widgets call CreateWidget etc.  
 * Destruction: UObjects call ConditionalBeginDestroy; Actors call DestroyActor, Components call DestroyComponent, Widgets call RemoveFromParent etc.
 * Pool: Actors and Scene Components are changing visibility, collision, ticking, etc. UObjects and Widgets are not.
 *
 * To create new factory:
 * 1. Inherit from this/child class
 * 2. Add it to the 'Project Settings' -> "Plugins" -> "Pool Manager" -> "Pool Factories"
 * 3. Override GetObjectClass() method.
 */
UCLASS(Blueprintable, BlueprintType)
class POOLMANAGER_API UPoolFactory_UObject : public UObject
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Setup overrides
	 ********************************************************************************************* */
public:
	/** OVERRIDE by child class to return the class of an object that this factory will create and manage.
	 * E.g: UObject for UPoolFactory_UObject, AActor for UPoolFactory_Actor, UWidget for UPoolFactory_Widget etc. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Factory")
	const UClass* GetObjectClass() const;
	virtual FORCEINLINE const UClass* GetObjectClass_Implementation() const { return UObject::StaticClass(); }

	/*********************************************************************************************
	 * Creation
	 * RequestSpawn -> DequeueSpawnRequest -> SpawnNow -> OnPreRegistered -> OnPostSpawned
	 ********************************************************************************************* */
public:
	/** Method to queue object spawn requests.
	 * Is called from UPoolManagerSubsystem::CreateNewObjectInPool. */
	UFUNCTION(BlueprintNativeEvent, Blueprintable, Category = "Pool Factory", meta = (AutoCreateRefTerm = "Request"))
	void RequestSpawn(const FSpawnRequest& Request);
	virtual void RequestSpawn_Implementation(const FSpawnRequest& Request);

	/** Removes the first spawn request from the queue and returns it.
	 * Is called after 'RequestSpawn'. */
	UFUNCTION(BlueprintCallable, Category = "Pool Factory")
	virtual bool DequeueSpawnRequest(FSpawnRequest& OutRequest);

	/** Method to immediately spawn requested object.
	 * Is called after 'DequeueSpawnRequest'. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Factory", meta = (AutoCreateRefTerm = "Request"))
	UObject* SpawnNow(const FSpawnRequest& Request);
	virtual UObject* SpawnNow_Implementation(const FSpawnRequest& Request);

	/** Alternative method to remove specific spawn request from the queue and returns it. */
	UFUNCTION(BlueprintCallable, Category = "Pool Factory")
	virtual bool DequeueSpawnRequestByHandle(const FPoolObjectHandle& Handle, FSpawnRequest& OutRequest);

	/** Returns true if the spawn queue is empty, so there are no spawn request at current moment. */
	UFUNCTION(BlueprintPure, Category = "Pool Factory")
	virtual FORCEINLINE bool IsSpawnQueueEmpty() const { return SpawnQueueInternal.IsEmpty(); }

	/** Is called right after object is spawned and before it is registered in the Pool.
	 * Is called after 'SpawnNow'. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	virtual void OnPreRegistered(const FSpawnRequest& Request, const FPoolObjectData& ObjectData);

	/** Is called right after object is spawned and registered in the Pool.
	 * Is called after 'OnPreRegistered'. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	virtual void OnPostSpawned(const FSpawnRequest& Request, const FPoolObjectData& ObjectData);

protected:
	/** Is called on next frame to process a chunk of the spawn queue. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Factory", meta = (BlueprintProtected))
	void OnNextTickProcessSpawn();
	virtual void OnNextTickProcessSpawn_Implementation();

	/*********************************************************************************************
	 * Destruction
	 ********************************************************************************************* */
public:
	/** Method to destroy given object. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Factory")
	void Destroy(UObject* Object);
	virtual void Destroy_Implementation(UObject* Object);

	/*********************************************************************************************
	 * Pool
	 ********************************************************************************************* */
public:
	/** Is called right before taking the object from its pool. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Factory", meta = (AutoCreateRefTerm = "Transform"))
	void OnTakeFromPool(UObject* Object, const FTransform& Transform);
	virtual void OnTakeFromPool_Implementation(UObject* Object, const FTransform& Transform) {}

	/** Is called right before returning the object back to its pool. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Factory")
	void OnReturnToPool(UObject* Object);
	virtual void OnReturnToPool_Implementation(UObject* Object) {}

	/** Is called when activates the object to take it from pool or deactivate when is returned back. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Factory")
	void OnChangedStateInPool(EPoolObjectState NewState, UObject* InObject);
	virtual void OnChangedStateInPool_Implementation(EPoolObjectState NewState, UObject* InObject) {}

	/*********************************************************************************************
	 * Data
	 ********************************************************************************************* */
protected:
	/** All request to spawn. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "Pool Factory", meta = (BlueprintProtected, DisplayName = "Spawn Queue"))
	TArray<FSpawnRequest> SpawnQueueInternal;
};
