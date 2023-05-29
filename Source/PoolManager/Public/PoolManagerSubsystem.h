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
 * Tips:
 *     - You can use the Pool Manager in the Editor before you start the game.
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
	* UPoolManagerSubsystem::Get(). with no parameters can be used in most cases if there is no specific set up.
	* @tparam T is optional, put your child class if you implemented your own Pull Manager in code.
	* @param OptionalClass is optional, specify the class if you implemented your own Pool Manager in blueprints.
	* @param OptionalWorldContext is optional, can be null in most cases, could be useful to avoid obtaining the automatically. */
	template <typename T = ThisClass>
	static FORCEINLINE T& Get(TSubclassOf<UPoolManagerSubsystem> OptionalClass = T::StaticClass(), const UObject* OptionalWorldContext = nullptr) { return *CastChecked<T>(GetPoolManagerByClass(OptionalClass, OptionalWorldContext)); }

	/** Returns the pointer to the Pool Manager.
	 * @param OptionalWorldContext is optional parameter and hidden in blueprints, can be null in most cases, could be useful to avoid obtaining the world automatically. */
	UFUNCTION(BlueprintPure, meta = (WorldContext = "OptionalWorldContext"))
	static UPoolManagerSubsystem* GetPoolManager(const UObject* OptionalWorldContext = nullptr) { return GetPoolManagerByClass(StaticClass(), OptionalWorldContext); }

	/** Returns the pointer to custom Pool Manager by given class.
	* @param OptionalClass is optional, specify the class if you implemented your own Pool Manager.
	* @param OptionalWorldContext is optional parameter and hidden in blueprints, can be null in most cases, could be useful to avoid obtaining the world automatically. */
	UFUNCTION(BlueprintPure, meta = (WorldContext = "OptionalWorldContext"))
	static UPoolManagerSubsystem* GetPoolManagerByClass(TSubclassOf<UPoolManagerSubsystem> OptionalClass = nullptr, const UObject* OptionalWorldContext = nullptr);

	/*********************************************************************************************
	 * Main
	 *
	 * Use TakeFromPool() to get it instead of creating by your own.
	 * Use ReturnToPool() to return it back to the pool instead of destroying by your own.
	 ********************************************************************************************* */
public:
	/** Get the object from a pool by specified class.
	 *  It creates new object if there no free objects contained in pool or does not exist any.
	 *  @return Activated object requested from the pool. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Manager", meta = (AutoCreateRefTerm = "Transform"))
	UObject* TakeFromPool(const UClass* ClassInPool, const FTransform& Transform);
	virtual UObject* TakeFromPool_Implementation(const UClass* ClassInPool, const FTransform& Transform);

	/** The templated alternative to get the object from a pool by specified class. */
	template <typename T>
	FORCEINLINE T* TakeFromPool(const UClass* ClassInPool = T::StaticClass(), const FTransform& Transform = FTransform::Identity) { return Cast<T>(TakeFromPool(ClassInPool, Transform)); }

	/** Returns the specified object to the pool and deactivates it if the object was taken from the pool before. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	void ReturnToPool(UObject* Object);
	virtual void ReturnToPool_Implementation(UObject* Object);

	/*********************************************************************************************
	 * Advanced
	 *
	 * In most cases, you don't need to use this section.
	 ********************************************************************************************* */
public:
	/** Adds specified object as is to the pool by its class to be handled by the Pool Manager.
	 * It's designed to be used only on already existed objects unknown for the Pool Manager. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool RegisterObjectInPool(UObject* Object = nullptr, EPoolObjectState PoolObjectState = EPoolObjectState::Inactive);
	virtual bool RegisterObjectInPool_Implementation(UObject* Object = nullptr, EPoolObjectState PoolObjectState = EPoolObjectState::Inactive);

	/** Always creates new object and adds it to the pool by its class.
	 * Use carefully if only there is no free objects contained in pool. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	UObject* CreateNewObjectInPool(const UClass* ObjectClass, const FTransform& Transform, EPoolObjectState PoolObjectState = EPoolObjectState::Active);
	virtual UObject* CreateNewObjectInPool_Implementation(const UClass* ObjectClass, const FTransform& Transform, EPoolObjectState PoolObjectState = EPoolObjectState::Active);

	/** Destroy all object of a pool by a given class. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void EmptyPool(const UClass* ClassInPool);
	virtual void EmptyPool_Implementation(const UClass* ClassInPool);

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
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool ContainsClassInPool(const UClass* ClassInPool) const;
	virtual bool ContainsClassInPool_Implementation(const UClass* ClassInPool) const;

	/** Returns true if specified object is handled by the Pool Manager and was taken from its pool. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool IsActive(const UObject* Object) const;
	virtual bool IsActive_Implementation(const UObject* Object) const;

	/** Returns true if handled object is inactive and ready to be taken from pool. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool IsFreeObjectInPool(const UObject* Object) const;
	virtual bool IsFreeObjectInPool_Implementation(const UObject* Object) const;

	/** Returns first object contained in the Pool by its class that is inactive and ready to be taken from pool.
	 * Return null if there no free objects contained in pool or does not exist any.
	 * Consider to use TakeFromPool() instead to create new object if there no free objects. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	UObject* GetFreeObjectInPool(const UClass* ObjectClass) const;
	virtual UObject* GetFreeObjectInPool_Implementation(const UClass* ObjectClass) const;

	/** Returns true if object is known by Pool Manager. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Pool Manager", meta = (DefaultToSelf = "Object"))
	bool IsRegistered(const UObject* Object) const;
	virtual bool IsRegistered_Implementation(const UObject* Object) const;

	/*********************************************************************************************
	 * Protected properties
	 ********************************************************************************************* */
protected:
	/** Contains all pools that are handled by the Pool Manger. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Pool Manager", meta = (BlueprintProtected, DisplayName = "Pools"))
	TArray<FPoolContainer> PoolsInternal;

	/*********************************************************************************************
	 * Protected methods
	 ********************************************************************************************* */
protected:
	/** Is called on initialization of the Pool Manager instance. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Returns the pointer to found pool by specified class. */
	virtual FPoolContainer* FindPool(const UClass* ClassInPool);
	const FORCEINLINE FPoolContainer* FindPool(const UClass* ClassInPool) const { return const_cast<UPoolManagerSubsystem*>(this)->FindPool(ClassInPool); }

	/** Activates or deactivates the object if such object is handled by the Pool Manager. */
	UFUNCTION(BlueprintCallable, Category = "Pool Manager", meta = (BlueprintProtected, DefaultToSelf = "Object"))
	virtual void SetActive(bool bShouldActivate, UObject* Object);
};
