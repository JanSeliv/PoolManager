// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Subsystems/WorldSubsystem.h"
//---
#include "PoolManagerTypes.h"
//---
#include "PoolManagerSubsystem.generated.h"

/**
 * The Pool Manager helps reuse objects that show up often instead of creating and destroying them each time.
 *
 * Issue:
 *     Unreal Engine can be slow to create and destroy objects like projectiles or explosions.
 *     Doing this a lot can cause problems, like making the game slow or laggy.
 *
 * How the Pool Manager helps:
 *     Instead of creating and destroying objects all the time, the Pool Manager keeps a pool of objects to use.
 *     This way, objects are reused, which makes the game run smoother.
 *
 * How it works:
 *     Objects are taken from and returned to the Pool Manager when not in use, which makes them 'inactive'.
 *     In case of actors, they are moved outside the game level, hidden, and they don't interact with anything or use up resources.
 *
 * Code architecture:
 *		 - Pool Manager is a subsystem that is created automatically including all children.
 *		 - It stores and manages only the data (Pools and objects).
 *		 - It does not manage any specific logic for handling objects, only base pooling logic related to data.
 *		 - Pool Factories are used to handle specific logic about objects behavior (creation, destruction, visibility etc).
 *		 - Prefer overriding Pool Factories to implement custom logic instead of Pool Manager.
 *
 * Optional features:
 *     - Pool Manager is available even before starting the game that allows to preconstruct your actors on the level.
 *     - You can implement your own Pool Manager by inheriting from this class and overriding the functions.
 */
UCLASS(BlueprintType, Blueprintable)
class POOLMANAGER_API UPoolManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Static Getters
	 ********************************************************************************************* */
public:
	/** Returns the Pool Manager, is checked and wil crash if can't be obtained.
	 * Is useful in code, in most cases can be used with no parameters to obtain default Pool Manager.
	 * E.g:
	 * - UPoolManagerSubsystem::Get().TakeFromPool...
	 * - UPoolManagerSubsystem::Get<UCustomCodePoolManager>().TakeFromPool...
	 * - UPoolManagerSubsystem::Get(CustomBlueprintPoolManager).TakeFromPool...
	 * @tparam T is optional, put your child class if you implemented your own Pull Manager in code.
	 * @param OptionalClass is optional, specify the class if you implemented your own Pool Manager in blueprints.
	 * @param OptionalWorldContext is optional, can be null in most cases, could be useful to avoid obtaining the automatically. */
	template <typename T = ThisClass>
	static FORCEINLINE T& Get(TSubclassOf<UPoolManagerSubsystem> OptionalClass = T::StaticClass(), const UObject* OptionalWorldContext = nullptr) { return *CastChecked<T>(GetPoolManagerByClass(OptionalClass, OptionalWorldContext)); }

	/** Returns the pointer to the Pool Manager.
	 * Is useful for blueprints to obtain **default** Pool Manager.
	 * @param OptionalWorldContext is optional parameter and hidden in blueprints, can be null in most cases, could be useful to avoid obtaining the world automatically. */
	UFUNCTION(BlueprintPure, meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static UPoolManagerSubsystem* GetPoolManager(const UObject* OptionalWorldContext = nullptr) { return GetPoolManagerByClass(StaticClass(), OptionalWorldContext); }

	/** Returns the pointer to custom Pool Manager by given class.
	 * Is useful for blueprints to obtain your **custom** Pool Manager. 
	 * @param OptionalClass is optional, specify the class if you implemented your own Pool Manager.
	 * @param OptionalWorldContext is optional parameter and hidden in blueprints, can be null in most cases, could be useful to avoid obtaining the world automatically. */
	UFUNCTION(BlueprintPure, meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext, DeterminesOutputType = "OptionalClass", BlueprintAutocast))
	static UPoolManagerSubsystem* GetPoolManagerByClass(TSubclassOf<UPoolManagerSubsystem> OptionalClass = nullptr, const UObject* OptionalWorldContext = nullptr);

	/*********************************************************************************************
	 * Take From Pool (single object)
	 * Use it to get an object instead of creating it manually by your own.
	 ********************************************************************************************* */
public:
	DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTakenFromPool, UObject*, Object);

	/** Get signle object from a pool by specified class, where output is async that returns the object when is ready.
	 *  It creates new object if there no free objects contained in pool or does not exist any.
	 *  @param ObjectClass The class of object to get from the pool.
	 *  @param Transform The transform to set for the object (if actor).
	 *  @param Completed The callback output that is called when the object is ready.
	 *  @return if any is found and free, activates and returns object from the pool, otherwise async spawns new one next frames and register in the pool.
	 *  @warning BP-ONLY: in code, use TakeFromPool() instead. 
	 *  - 'SpawnObjectsPerFrame' affects how fast new objects are created, it can be changed in 'Project Settings' -> "Plugins" -> "Pool Manager".
	 *  - Is custom blueprint node implemented in K2Node_TakeFromPool.h, so can't be overridden and accessible on graph (not inside functions).
	 *  - use BPTakeFromPoolArray instead of requesting one by one in for/while: 'Completed' output does not work in loops. */
	UFUNCTION(BlueprintCallable, Category = "Pool Manager", DisplayName = "Take From Pool", meta = (BlueprintInternalUseOnly = "true", AutoCreateRefTerm = "Transform"))
	void BPTakeFromPool(const UClass* ObjectClass, const FTransform& Transform, const FOnTakenFromPool& Completed);

	/** Is code-overridable alternative version of BPTakeFromPool() that calls callback functions when the object is ready.
	 * Can be overridden by child code classes.
	 * Is useful in code with blueprint classes, e.g: TakeFromPool(SomeBlueprintClass);
	 * @return Handle to the object with the Hash associated with the object, is indirect since the object could be not ready yet. */
	virtual FPoolObjectHandle TakeFromPool(const UClass* ObjectClass, const FTransform& Transform = FTransform::Identity, const FOnSpawnCallback& Completed = nullptr);

	/** A templated alternative to get the object from a pool by class in template.
	 * Is useful in code with code classes, e.g: TakeFromPool<AProjectile>(); */
	template <typename T>
	FPoolObjectHandle TakeFromPool(const FTransform& Transform = FTransform::Identity, const FOnSpawnCallback& Completed = nullptr) { return TakeFromPool(T::StaticClass(), Transform, Completed); }

	/** Is alternative version of TakeFromPool() to find object in pool or return null. */
	virtual const FPoolObjectData* TakeFromPoolOrNull(const UClass* ObjectClass, const FTransform& Transform = FTransform::Identity);

	/*********************************************************************************************
	 * Take From Pool (multiple objects)
	 * Use it instead of single-object version when you need to get multiple objects at once.
	 ********************************************************************************************* */
public:
	DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTakenFromPoolArray, const TArray<UObject*>&, Objects);

	/** Is the same as BPTakeFromPool() but for multiple objects.
	 * @param ObjectClass The class of object to get from the pool.
	 * @param Amount The amount of objects to get from the pool.
	 * @param Completed The callback output that is called when all objects are ready.
	 * @warning BP-ONLY: in code, use TakeFromPoolArray() instead. */
	UFUNCTION(BlueprintCallable, Category = "Pool Manager", DisplayName = "Take From Pool Array", meta = (BlueprintInternalUseOnly = "true"))
	void BPTakeFromPoolArray(const UClass* ObjectClass, int32 Amount, const FOnTakenFromPoolArray& Completed);

	/** Is code-overridable alternative version of BPTakeFromPoolArray() that calls callback functions when all objects of the same class are ready. */
	virtual void TakeFromPoolArray(TArray<FPoolObjectHandle>& OutHandles, const UClass* ObjectClass, int32 Amount, const FOnSpawnAllCallback& Completed = nullptr);

	/** Is alternative version of TakeFromPoolArray() that can process multiple requests of different classes and different transforms at once.
	 * @param OutHandles Returns the handles associated with objects to be spawned next frames.
	 * @param InRequests Takes the classes and Transforms.
	 * @param Completed The callback function that is called once when all objects are ready. */
	virtual void TakeFromPoolArray(TArray<FPoolObjectHandle>& OutHandles, TArray<FSpawnRequest>& InRequests, const FOnSpawnAllCallback& Completed = nullptr);

	/** Is alternative version of TakeFromPoolArrayOrNull() to find multiple object in pool or return null.
	 * @param OutObjects All found and taken objects, or empty array if no one is ready yet
	 * @param InRequests Takes the classes and Transforms. */
	virtual void TakeFromPoolArrayOrNull(TArray<FPoolObjectData>& OutObjects, TArray<FSpawnRequest>& InRequests);

	/*********************************************************************************************
	 * Return To Pool (single object)
	 * Returns an object back to the pool instead of destroying by your own.
	 ********************************************************************************************* */
public:
	/** Returns the specified object to the pool and deactivates it if the object was taken from the pool before.
	 * @param Object The object to return to the pool.
	 * @return true if pool was found and returned successfully, otherwise false. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool ReturnToPool(UObject* Object);
	virtual bool ReturnToPool_Implementation(UObject* Object);

	/** Alternative to ReturnToPool() to return object to the pool by its handle.
	 * Return by handle is more reliable that by objects since object could be not ready yet (in spawning queue).
	 * Is useful in case when you don't have a reference to the object but have its handle.
	 * @param Handle The handle associated with the object to return to the pool.
	 * @return true if handle was found and return successfully, otherwise false. */
	virtual bool ReturnToPool(const FPoolObjectHandle& Handle);

	/*********************************************************************************************
	 * Return To Pool (multiple objects)
	 * Use it instead of single-object version when you need to return multiple objects at once.
	 ********************************************************************************************* */
public:
	/** Is the same as ReturnToPool() but for multiple objects.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Manager")
	bool ReturnToPoolArray(const TArray<UObject*>& Objects);
	virtual bool ReturnToPoolArray_Implementation(const TArray<UObject*>& Objects);

	/** Is the same as ReturnToPool() but for multiple handle. */
	virtual bool ReturnToPoolArray(const TArray<FPoolObjectHandle>& Handles);

	/*********************************************************************************************
	 * Advanced
	 * In most cases, you don't need to use this section.
	 ********************************************************************************************* */
public:
	/** Adds specified object as is to the pool by its class to be handled by the Pool Manager.
	 * Should not be used directly in most cases since is called automatically.
	 * Could be useful to add already existed objects (spawned by outer code) to the pool.
	 * It's designed to be used only on already existed objects unknown for the Pool Manager.
	 * @param InData The data with the object to register in the pool.
	 * @return true if registered successfully, otherwise false. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Manager", meta = (AutoCreateRefTerm = "InData"))
	bool RegisterObjectInPool(const FPoolObjectData& InData);
	virtual bool RegisterObjectInPool_Implementation(const FPoolObjectData& InData);

	/** Always creates new object and adds it to the pool by its class.
	 * Use carefully if only there is no free objects contained in pool.
	 * @param InRequest The request to spawn new object.
	 * @return Handle to the object with the hash associated with object to be spawned next frames. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Manager", meta = (AutoCreateRefTerm = "InRequest"))
	FPoolObjectHandle CreateNewObjectInPool(const FSpawnRequest& InRequest);
	virtual FPoolObjectHandle CreateNewObjectInPool_Implementation(const FSpawnRequest& InRequest);

	/** Is the same as CreateNewObjectInPool() but for multiple objects. */
	virtual void CreateNewObjectsArrayInPool(TArray<FSpawnRequest>& InRequests, TArray<FPoolObjectHandle>& OutAllHandles, const FOnSpawnAllCallback& Completed = nullptr);

	/*********************************************************************************************
	 * Advanced - Factories
	 ********************************************************************************************* */
public:
	/** Registers new factory to be used by the Pool Manager when dealing with objects of specific class and its children.
	 * In most cases, you don't need to use this function since factories are registered automatically.
	 * Could be useful to register factory from different plugin or module. */
	UFUNCTION(BlueprintCallable, Category = "Pool Manager")
	virtual void AddFactory(TSubclassOf<UPoolFactory_UObject> FactoryClass);

	/** Removes factory from the Pool Manager by its class.
	 * In most cases, you don't need to use this function since factories are removed with destroying the Pool Manager.
	 * Could be useful to remove factory in runtime when some class is not needed anymore. */
	UFUNCTION(BlueprintCallable, Category = "Pool Manager")
	virtual void RemoveFactory(TSubclassOf<UPoolFactory_UObject> FactoryClass);

	/** Traverses the class hierarchy to find the closest registered factory for a given object type or its ancestors. */
	UFUNCTION(BlueprintPure, Category = "Pool Manager")
	UPoolFactory_UObject* FindPoolFactoryChecked(const UClass* ObjectClass) const;

	/** Returns default class of object that is handled by given factory. */
	UFUNCTION(BlueprintPure, Category = "Pool Manager")
	static const UClass* GetObjectClassByFactory(const TSubclassOf<UPoolFactory_UObject>& FactoryClass);

protected:
	/** Creates all possible Pool Factories to be used by the Pool Manager when dealing with objects. */
	virtual void InitializeAllFactories();

	/** Destroys all Pool Factories that are used by the Pool Manager when dealing with objects. */
	virtual void ClearAllFactories();

	/*********************************************************************************************
	 * Empty Pool
	 ********************************************************************************************* */
public:
	/** Destroy all object of a pool by a given class. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void EmptyPool(const UClass* ObjectClass);
	virtual void EmptyPool_Implementation(const UClass* ObjectClass);

	/** Destroy all objects in all pools that are handled by the Pool Manager. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void EmptyAllPools();
	virtual void EmptyAllPools_Implementation();

	/** Destroy all objects in Pool Manager based on a predicate functor. */
	virtual void EmptyAllByPredicate(const TFunctionRef<bool(const UObject* PoolObject)> Predicate);

	/*********************************************************************************************
	 * Getters
	 ********************************************************************************************* */
public:
	/** Returns current state of specified object. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	EPoolObjectState GetPoolObjectState(const UObject* Object) const;
	virtual EPoolObjectState GetPoolObjectState_Implementation(const UObject* Object) const;

	/** Returns true is specified object is handled by Pool Manager. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool ContainsObjectInPool(const UObject* Object) const;
	virtual bool ContainsObjectInPool_Implementation(const UObject* Object) const;

	/** Returns true is specified class is handled by Pool Manager. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager")
	bool ContainsClassInPool(const UClass* ObjectClass) const;
	virtual bool ContainsClassInPool_Implementation(const UClass* ObjectClass) const;

	/** Returns true if specified object is handled by the Pool Manager and was taken from its pool. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool IsActive(const UObject* Object) const;
	virtual bool IsActive_Implementation(const UObject* Object) const;

	/** Returns true if handled object is inactive and ready to be taken from pool. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool IsFreeObjectInPool(const UObject* Object) const;
	virtual bool IsFreeObjectInPool_Implementation(const UObject* Object) const;

	/** Returns number of free objects in pool by specified class. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager")
	int32 GetFreeObjectsNum(const UClass* ObjectClass) const;
	virtual int32 GetFreeObjectsNum_Implementation(const UClass* ObjectClass) const;

	/** Returns true if object is known by Pool Manager. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool IsRegistered(const UObject* Object) const;
	virtual bool IsRegistered_Implementation(const UObject* Object) const;

	/** Returns number of registered objects in pool by specified class. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager")
	int32 GetRegisteredObjectsNum(const UClass* ObjectClass) const;
	virtual int32 GetRegisteredObjectsNum_Implementation(const UClass* ObjectClass) const;

	/** Returns true if object is valid and registered in pool. */
	UFUNCTION(BlueprintPure, Category = "Pool Manager", meta = (AutoCreateRefTerm = "InPoolObject"))
	static bool IsPoolObjectValid(const FPoolObjectData& InPoolObject) { return InPoolObject.IsValid(); }

	/** Returns the object associated with given handle.
	 * Can return invalid PoolObject  if not found or object is in spawning queue. */
	UFUNCTION(BlueprintPure, Category = "Pool Manager")
	const FPoolObjectData& FindPoolObjectByHandle(const FPoolObjectHandle& Handle) const;

	/** Returns handle associated with given object.
	 * Can be invalid (FPoolObjectHandle::EmptyHandle) if not found. */
	UFUNCTION(BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	const FPoolObjectHandle& FindPoolHandleByObject(const UObject* Object) const;

	/** Returns from all given handles only valid ones. */
	UFUNCTION(BlueprintPure, Category = "Pool Manager")
	void FindPoolObjectsByHandles(TArray<FPoolObjectData>& OutObjects, const TArray<FPoolObjectHandle>& InHandles) const;

	/*********************************************************************************************
	 * Protected properties
	 ********************************************************************************************* */
protected:
	/** Contains all pools that are handled by the Pool Manger. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Pool Manager", meta = (BlueprintProtected, DisplayName = "Pools"))
	TArray<FPoolContainer> PoolsInternal;

	/** Map to store registered factories against the class types they handle.
	 * @see UPoolFactory_UObject's description. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Pool Manager", meta = (BlueprintProtected, DisplayName = "All Factories"))
	TMap<TObjectPtr<const UClass>, TObjectPtr<UPoolFactory_UObject>> AllFactoriesInternal;

	/*********************************************************************************************
	 * Protected methods
	 ********************************************************************************************* */
protected:
	/** Is called on initialization of the Pool Manager instance. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Is called on deinitialization of the Pool Manager instance. */
	virtual void Deinitialize() override;

	/** Returns the pointer to found pool by specified class. */
	virtual FPoolContainer& FindPoolOrAdd(const UClass* ObjectClass);
	virtual FPoolContainer* FindPool(const UClass* ObjectClass);
	const FORCEINLINE FPoolContainer* FindPool(const UClass* ObjectClass) const { return const_cast<UPoolManagerSubsystem*>(this)->FindPool(ObjectClass); }

	/** Activates or deactivates the object if such object is handled by the Pool Manager.
	 * Is called when the object is taken from, registered or returned to the pool.
	 * @param NewState If true, the object will be activated, otherwise deactivated.
	 * @param InObject The object to activate or deactivate.
	 * @param InPool The pool that contains the object.
	 * @warning Do not call it directly, use TakeFromPool() or ReturnToPool() instead. */
	virtual void SetObjectStateInPool(EPoolObjectState NewState, UObject& InObject, UPARAM(ref) FPoolContainer& InPool);
};
