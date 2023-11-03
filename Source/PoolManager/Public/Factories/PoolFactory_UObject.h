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
 * To create new factory just inherit from this/child class and override GetObjectClass() method.
 * Pool Manager will automatically create and use your factory for objects of your class and its children.
*/
UCLASS(Blueprintable, BlueprintType)
class POOLMANAGER_API UPoolFactory_UObject : public UObject
{
	GENERATED_BODY()

public:
	/** Returns the class of object that this factory will create and manage.
	 * Has to be overridden by child classes if it wants to handle logic for specific class and its children.
	 * E.g: UObject for UPoolFactory_UObject, AActor for UPoolFactory_Actor, UWidget for UPoolFactory_Widget etc. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Factory")
	const UClass* GetObjectClass() const;
	virtual FORCEINLINE const UClass* GetObjectClass_Implementation() const { return UObject::StaticClass(); }

	/*********************************************************************************************
	 * Creation
	 ********************************************************************************************* */
public:
	/** Method to queue object spawn requests. */
	UFUNCTION(BlueprintNativeEvent, Blueprintable, Category = "Pool Factory", meta = (AutoCreateRefTerm = "Request"))
	void RequestSpawn(const FSpawnRequest& Request);
	virtual void RequestSpawn_Implementation(const FSpawnRequest& Request);

	/** Method to immediately spawn requested object. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Factory", meta = (AutoCreateRefTerm = "Request"))
	UObject* SpawnNow(const FSpawnRequest& Request);
	virtual UObject* SpawnNow_Implementation(const FSpawnRequest& Request);

	/** Removes the first spawn request from the queue and returns it. */
	UFUNCTION(BlueprintCallable, Category = "Pool Factory")
	virtual bool DequeueSpawnRequest(FSpawnRequest& OutRequest);

	/** Alternative method to remove specific spawn request from the queue and returns it. */
	UFUNCTION(BlueprintCallable, Category = "Pool Factory")
	virtual bool DequeueSpawnRequestByHandle(const FPoolObjectHandle& Handle, FSpawnRequest& OutRequest);

	/** Returns true if the spawn queue is empty, so there are no spawn request at current moment. */
	UFUNCTION(BlueprintPure, Category = "Pool Factory")
	virtual FORCEINLINE bool IsSpawnQueueEmpty() const { return SpawnQueueInternal.IsEmpty(); }

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
