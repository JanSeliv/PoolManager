// Copyright (c) Yevhenii Selivanov

#include "PoolManagerSubsystem.h"
//---
#include "Factories/PoolFactory_UObject.h"
//---
#include "Engine/World.h"
//---
#if WITH_EDITOR
#include "Editor.h"
#endif // WITH_EDITOR
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolManagerSubsystem)

/*********************************************************************************************
 * Static Getters
 ********************************************************************************************* */

// Returns the pointer to your Pool Manager
UPoolManagerSubsystem* UPoolManagerSubsystem::GetPoolManagerByClass(TSubclassOf<UPoolManagerSubsystem> OptionalClass/* = nullptr*/, const UObject* OptionalWorldContext/* = nullptr*/)
{
	if (!OptionalClass)
	{
		OptionalClass = StaticClass();
	}

	const UWorld* FoundWorld = OptionalWorldContext
		                           ? GEngine->GetWorldFromContextObject(OptionalWorldContext, EGetWorldErrorMode::Assert)
		                           : GEngine->GetCurrentPlayWorld();

#if WITH_EDITOR
	if (!FoundWorld && GEditor)
	{
		// If world is not found, most likely a game did not start yet and we are in editor
		const FWorldContext& WorldContext = GEditor->IsPlaySessionInProgress() ? *GEditor->GetPIEWorldContext() : GEditor->GetEditorWorldContext();
		FoundWorld = WorldContext.World();
	}
#endif // WITH_EDITOR

	if (!ensureMsgf(FoundWorld, TEXT("%s: Can not obtain current world"), *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	UPoolManagerSubsystem* FoundPoolManager = Cast<UPoolManagerSubsystem>(FoundWorld->GetSubsystemBase(OptionalClass));
	if (!ensureMsgf(FoundPoolManager, TEXT("%s: 'Can not find Pool Manager for %s class in %s world"), *FString(__FUNCTION__), *OptionalClass->GetName(), *FoundWorld->GetName()))
	{
		return nullptr;
	}

	return FoundPoolManager;
}

/*********************************************************************************************
 * Main API
 ********************************************************************************************* */

// Async version of TakeFromPool() that returns the object by specified class
void UPoolManagerSubsystem::BPTakeFromPool(const UClass* ObjectClass, const FTransform& Transform, const FOnTakenFromPool& Completed)
{
	UObject* PoolObject = TakeFromPoolOrNull(ObjectClass, Transform);
	if (PoolObject)
	{
		// Found in pool
		Completed.ExecuteIfBound(PoolObject);
		return;
	}

	FSpawnRequest Request;
	Request.Class = ObjectClass;
	Request.Transform = Transform;
	Request.Callbacks.OnPostSpawned = [Completed](UObject* SpawnedObject)
	{
		Completed.ExecuteIfBound(SpawnedObject);
	};
	CreateNewObjectInPool(Request);
}

// Is code async version of TakeFromPool() that calls callback functions when the object is ready
void UPoolManagerSubsystem::TakeFromPool(const UClass* ObjectClass, const FTransform& Transform/* = FTransform::Identity*/, const TFunction<void(UObject*)>& Completed/* = nullptr*/)
{
	UObject* PoolObject = TakeFromPoolOrNull(ObjectClass, Transform);
	if (PoolObject)
	{
		if (Completed != nullptr)
		{
			Completed(PoolObject);
		}
		return;
	}

	FSpawnRequest Request;
	Request.Class = ObjectClass;
	Request.Transform = Transform;
	Request.Callbacks.OnPostSpawned = Completed;
	CreateNewObjectInPool(Request);
}

// Is internal function to find object in pool or return null
UObject* UPoolManagerSubsystem::TakeFromPoolOrNull_Implementation(const UClass* ObjectClass, const FTransform& Transform)
{
	if (!ensureMsgf(ObjectClass, TEXT("%s: 'ObjectClass' is not specified"), *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	FPoolContainer* Pool = FindPool(ObjectClass);
	if (!Pool)
	{
		// Pool is not registered
		return nullptr;
	}

	// Try to find first object contained in the Pool by its class that is inactive and ready to be taken from pool
	UObject* FoundObject = nullptr;
	for (const FPoolObjectData& PoolObjectIt : Pool->PoolObjects)
	{
		if (PoolObjectIt.IsFree())
		{
			FoundObject = PoolObjectIt.Get();
			break;
		}
	}

	if (!FoundObject)
	{
		// No free objects in pool
		return nullptr;
	}

	Pool->GetFactoryChecked().OnTakeFromPool(FoundObject, Transform);

	SetObjectStateInPool(EPoolObjectState::Active, FoundObject, *Pool);

	return FoundObject;
}

// Returns the specified object to the pool and deactivates it if the object was taken from the pool before
void UPoolManagerSubsystem::ReturnToPool_Implementation(UObject* Object)
{
	FPoolContainer* Pool = Object ? FindPool(Object->GetClass()) : nullptr;
	if (!ensureMsgf(Pool, TEXT("ASSERT: [%i] %s:\n'Pool' is not not registered for '%s' object, can not return it to pool!"), __LINE__, *FString(__FUNCTION__), *GetNameSafe(Object)))
	{
		return;
	}

	Pool->GetFactoryChecked().OnReturnToPool(Object);

	SetObjectStateInPool(EPoolObjectState::Inactive, Object, *Pool);
}

/*********************************************************************************************
 * Advanced
 ********************************************************************************************* */

// Adds specified object as is to the pool by its class to be handled by the Pool Manager
bool UPoolManagerSubsystem::RegisterObjectInPool_Implementation(UObject* Object, EPoolObjectState PoolObjectState/* = EPoolObjectState::Inactive*/)
{
	if (!Object)
	{
		return false;
	}

	const UClass* ObjectClass = Object->GetClass();
	FPoolContainer& Pool = FindPoolOrAdd(ObjectClass);

	if (Pool.FindInPool(Object))
	{
		// Already contains in pool
		return false;
	}

	FPoolObjectData& ObjectDataRef = Pool.PoolObjects.Emplace_GetRef(Object);
	ObjectDataRef.bIsActive = PoolObjectState == EPoolObjectState::Active;

	SetObjectStateInPool(PoolObjectState, Object, Pool);

	return true;
}

// Always creates new object and adds it to the pool by its class
void UPoolManagerSubsystem::CreateNewObjectInPool_Implementation(FSpawnRequest& InRequest)
{
	// Always register new object in pool once it is spawned
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	InRequest.Callbacks.OnPreConstructed = [WeakThis, InRequest](UObject* Object)
	{
		if (UPoolManagerSubsystem* PoolManager = WeakThis.Get())
		{
			PoolManager->RegisterObjectInPool(Object, EPoolObjectState::Active);
		}
	};

	FindPoolOrAdd(InRequest.Class).GetFactoryChecked().RequestSpawn(InRequest);
}

/*********************************************************************************************
 * Advanced - Factories
 ********************************************************************************************* */

// Registers new factory to be used by the Pool Manager when dealing with objects of specific class and its children
void UPoolManagerSubsystem::AddFactory(TSubclassOf<UPoolFactory_UObject> FactoryClass)
{
	const UClass* ObjectClass = GetObjectClassByFactory(FactoryClass);
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %s:\n'ObjectClass' is not set for next factory: %s"), __LINE__, *FString(__FUNCTION__), *FactoryClass->GetName()))
	{
		return;
	}

	UPoolFactory_UObject* NewFactory = NewObject<UPoolFactory_UObject>(this, FactoryClass);
	AllFactoriesInternal.Emplace(ObjectClass, NewFactory);
}

// Removes factory from the Pool Manager by its class
void UPoolManagerSubsystem::RemoveFactory(TSubclassOf<UPoolFactory_UObject> FactoryClass)
{
	const UClass* ObjectClass = GetObjectClassByFactory(FactoryClass);
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %s:\n'ObjectClass' is not set for next factory: %s"), __LINE__, *FString(__FUNCTION__), *FactoryClass->GetName()))
	{
		return;
	}

	const TObjectPtr<UPoolFactory_UObject>* FactoryPtr = AllFactoriesInternal.Find(ObjectClass);
	if (!ensureMsgf(FactoryPtr, TEXT("ASSERT: [%i] %s:\nFactory is not found for next class: %s"), __LINE__, *FString(__FUNCTION__), *ObjectClass->GetName()))
	{
		return;
	}

	UPoolFactory_UObject* Factory = *FactoryPtr;
	if (IsValid(Factory))
	{
		Factory->ConditionalBeginDestroy();
	}

	AllFactoriesInternal.Remove(ObjectClass);
}

// Traverses the class hierarchy to find the closest registered factory for a given object type or its ancestors
UPoolFactory_UObject* UPoolManagerSubsystem::FindPoolFactoryChecked(const UClass* ObjectClass) const
{
	checkf(ObjectClass, TEXT("ERROR: [%i] %s:\n'ObjectClass' is null!"), __LINE__, *FString(__FUNCTION__));

	const TObjectPtr<UPoolFactory_UObject>* FoundFactory = nullptr;
	const UClass* CurrentClass = ObjectClass;

	// This loop will keep traversing up the hierarchy until a registered factory is found or the root is reached
	while (CurrentClass != nullptr)
	{
		FoundFactory = AllFactoriesInternal.Find(CurrentClass);
		if (FoundFactory)
		{
			break; // Exit the loop if a factory is found
		}
		CurrentClass = CurrentClass->GetSuperClass(); // Otherwise, move up the class hierarchy
	}

	checkf(FoundFactory, TEXT("ERROR: [%i] %s:\n'FoundFactory' is null for next object class: %s"), __LINE__, *FString(__FUNCTION__), *GetNameSafe(ObjectClass));
	return *FoundFactory;
}

// Returns default class of object that is handled by given factory
const UClass* UPoolManagerSubsystem::GetObjectClassByFactory(const TSubclassOf<UPoolFactory_UObject>& FactoryClass)
{
	if (!ensureMsgf(FactoryClass, TEXT("ASSERT: [%i] %s:\n'FactoryClass' is null!"), __LINE__, *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	const UPoolFactory_UObject* FactoryCDO = CastChecked<UPoolFactory_UObject>(FactoryClass->GetDefaultObject());
	return FactoryCDO->GetObjectClass();
}

// Creates all possible Pool Factories to be used by the Pool Manager when dealing with objects
void UPoolManagerSubsystem::InitializeAllFactories()
{
	if (!AllFactoriesInternal.IsEmpty())
	{
		// Already initialized
		return;
	}

	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (!It->IsChildOf(UPoolFactory_UObject::StaticClass())
			|| It->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		AddFactory(*It);
	}
}

// Destroys all Pool Factories that are used by the Pool Manager when dealing with objects
void UPoolManagerSubsystem::ClearAllFactories()
{
	for (const TTuple<TObjectPtr<const UClass>, TObjectPtr<UPoolFactory_UObject>>& FactoryIt : AllFactoriesInternal)
	{
		if (IsValid(FactoryIt.Value))
		{
			FactoryIt.Value->ConditionalBeginDestroy();
		}
	}

	AllFactoriesInternal.Empty();
}

/*********************************************************************************************
 * Empty Pool
 ********************************************************************************************* */

// Destroy all object of a pool by a given class
void UPoolManagerSubsystem::EmptyPool_Implementation(const UClass* ObjectClass)
{
	FPoolContainer* Pool = FindPool(ObjectClass);
	if (!ensureMsgf(Pool, TEXT("%s: 'Pool' is not valid"), *FString(__FUNCTION__)))
	{
		return;
	}

	UPoolFactory_UObject& Factory = Pool->GetFactoryChecked();
	TArray<FPoolObjectData>& PoolObjects = Pool->PoolObjects;
	for (int32 Index = PoolObjects.Num() - 1; Index >= 0; --Index)
	{
		UObject* ObjectIt = PoolObjects.IsValidIndex(Index) ? PoolObjects[Index].Get() : nullptr;
		if (IsValid(ObjectIt))
		{
			Factory.Destroy(ObjectIt);
		}
	}

	PoolObjects.Empty();
}

// Destroy all objects in all pools that are handled by the Pool Manager
void UPoolManagerSubsystem::EmptyAllPools_Implementation()
{
	const int32 PoolsNum = PoolsInternal.Num();
	for (int32 Index = PoolsNum - 1; Index >= 0; --Index)
	{
		const UClass* ObjectClass = PoolsInternal.IsValidIndex(Index) ? PoolsInternal[Index].ObjectClass : nullptr;
		EmptyPool(ObjectClass);
	}

	PoolsInternal.Empty();
}

// Destroy all objects in Pool Manager based on a predicate functor
void UPoolManagerSubsystem::EmptyAllByPredicate(const TFunctionRef<bool(const UObject* Object)> Predicate)
{
	const int32 PoolsNum = PoolsInternal.Num();
	for (int32 PoolIndex = PoolsNum - 1; PoolIndex >= 0; --PoolIndex)
	{
		if (!PoolsInternal.IsValidIndex(PoolIndex))
		{
			continue;
		}

		FPoolContainer& PoolIt = PoolsInternal[PoolIndex];
		UPoolFactory_UObject& Factory = PoolIt.GetFactoryChecked();
		TArray<FPoolObjectData>& PoolObjectsRef = PoolIt.PoolObjects;

		const int32 ObjectsNum = PoolObjectsRef.Num();
		for (int32 ObjectIndex = ObjectsNum - 1; ObjectIndex >= 0; --ObjectIndex)
		{
			UObject* ObjectIt = PoolObjectsRef.IsValidIndex(ObjectIndex) ? PoolObjectsRef[ObjectIndex].Get() : nullptr;
			if (!IsValid(ObjectIt)
				|| !Predicate(ObjectIt))
			{
				continue;
			}

			Factory.Destroy(ObjectIt);

			PoolObjectsRef.RemoveAt(ObjectIndex);
		}
	}
}

/*********************************************************************************************
 * Getters
 ********************************************************************************************* */

// Returns current state of specified object
EPoolObjectState UPoolManagerSubsystem::GetPoolObjectState_Implementation(const UObject* Object) const
{
	const UClass* ObjectClass = Object ? Object->GetClass() : nullptr;
	const FPoolContainer* Pool = FindPool(ObjectClass);
	const FPoolObjectData* PoolObject = Pool ? Pool->FindInPool(Object) : nullptr;

	if (!PoolObject
		|| !PoolObject->IsValid())
	{
		// Is not contained in any pool
		return EPoolObjectState::None;
	}

	return PoolObject->IsActive() ? EPoolObjectState::Active : EPoolObjectState::Inactive;
}

// Returns true is specified object is handled by Pool Manager
bool UPoolManagerSubsystem::ContainsObjectInPool_Implementation(const UObject* Object) const
{
	return GetPoolObjectState(Object) != EPoolObjectState::None;
}

bool UPoolManagerSubsystem::ContainsClassInPool_Implementation(const UClass* ObjectClass) const
{
	return FindPool(ObjectClass) != nullptr;
}

// Returns true if specified object is handled by the Pool Manager and was taken from its pool
bool UPoolManagerSubsystem::IsActive_Implementation(const UObject* Object) const
{
	return GetPoolObjectState(Object) == EPoolObjectState::Active;
}

// Returns true if handled object is inactive and ready to be taken from pool
bool UPoolManagerSubsystem::IsFreeObjectInPool_Implementation(const UObject* Object) const
{
	return GetPoolObjectState(Object) == EPoolObjectState::Inactive;
}

// Returns true if object is known by Pool Manager
bool UPoolManagerSubsystem::IsRegistered_Implementation(const UObject* Object) const
{
	return GetPoolObjectState(Object) != EPoolObjectState::None;
}

/*********************************************************************************************
 * Protected methods
 ********************************************************************************************* */

// Is called on initialization of the Pool Manager instance
void UPoolManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	InitializeAllFactories();

#if WITH_EDITOR
	if (GEditor
		&& !GEditor->IsPlaySessionInProgress() // Is Editor and not in PIE
		&& !GEditor->OnWorldDestroyed().IsBoundToObject(this))
	{
		// Editor pool manager instance has different lifetime than PIE pool manager instance,
		// So, to prevent memory leaks, clear all pools on switching levels in Editor
		TWeakObjectPtr<UPoolManagerSubsystem> WeakPoolManager(this);
		auto OnWorldDestroyed = [WeakPoolManager](const UWorld* World)
		{
			if (!World || !World->IsEditorWorld()
				|| !GEditor || GEditor->IsPlaySessionInProgress())
			{
				// Not Editor world or in PIE
				return;
			}

			if (UPoolManagerSubsystem* PoolManager = WeakPoolManager.Get())
			{
				PoolManager->EmptyAllPools();
			}
		};

		GEditor->OnWorldDestroyed().AddWeakLambda(this, OnWorldDestroyed);
	}
#endif // WITH_EDITOR
}

// Is called on deinitialization of the Pool Manager instance
void UPoolManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	ClearAllFactories();
}

// Returns the pointer to found pool by specified class
FPoolContainer& UPoolManagerSubsystem::FindPoolOrAdd(const UClass* ObjectClass)
{
	if (FPoolContainer* Pool = FindPool(ObjectClass))
	{
		return *Pool;
	}

	FPoolContainer& Pool = PoolsInternal.AddDefaulted_GetRef();
	Pool.ObjectClass = ObjectClass;
	Pool.Factory = FindPoolFactoryChecked(ObjectClass);
	return Pool;
}

// Returns the pointer to found pool by specified class
FPoolContainer* UPoolManagerSubsystem::FindPool(const UClass* ObjectClass)
{
	if (!ObjectClass)
	{
		return nullptr;
	}

	return PoolsInternal.FindByPredicate([ObjectClass](const FPoolContainer& It)
	{
		return It.ObjectClass == ObjectClass;
	});
}

// Activates or deactivates the object if such object is handled by the Pool Manager
void UPoolManagerSubsystem::SetObjectStateInPool_Implementation(EPoolObjectState NewState, UObject* InObject, FPoolContainer& InPool)
{
	FPoolObjectData* PoolObject = InPool.FindInPool(InObject);
	if (!ensureMsgf(PoolObject && PoolObject->IsValid(), TEXT("ASSERT: [%i] %s:\n'PoolObject' is not registered in given pool for class: %s"), __LINE__, *FString(__FUNCTION__), *GetNameSafe(InPool.ObjectClass)))
	{
		return;
	}

	PoolObject->bIsActive = NewState == EPoolObjectState::Active;

	InPool.GetFactoryChecked().OnChangedStateInPool(NewState, InObject);
}
