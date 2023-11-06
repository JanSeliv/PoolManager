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
	const FPoolObjectData* ObjectData = TakeFromPoolOrNull(ObjectClass, Transform);
	if (ObjectData)
	{
		// Found in pool
		Completed.ExecuteIfBound(ObjectData->PoolObject);
		return;
	}

	FSpawnRequest Request;
	Request.Class = ObjectClass;
	Request.Transform = Transform;
	Request.Callbacks.OnPostSpawned = [Completed](const FPoolObjectData& ObjectData)
	{
		Completed.ExecuteIfBound(ObjectData.PoolObject);
	};
	CreateNewObjectInPool(Request);
}

// Is code async version of TakeFromPool() that calls callback functions when the object is ready
FPoolObjectHandle UPoolManagerSubsystem::TakeFromPool(const UClass* ObjectClass, const FTransform& Transform/* = FTransform::Identity*/, const FOnSpawnCallback& Completed/* = nullptr*/)
{
	const FPoolObjectData* ObjectData = TakeFromPoolOrNull(ObjectClass, Transform);
	if (ObjectData)
	{
		if (Completed != nullptr)
		{
			Completed(*ObjectData);
		}

		return ObjectData->Handle;
	}

	FSpawnRequest Request;
	Request.Class = ObjectClass;
	Request.Transform = Transform;
	Request.Callbacks.OnPostSpawned = Completed;
	return CreateNewObjectInPool(Request);
}

// Is internal function to find object in pool or return null
const FPoolObjectData* UPoolManagerSubsystem::TakeFromPoolOrNull(const UClass* ObjectClass, const FTransform& Transform)
{
	if (!ensureMsgf(ObjectClass, TEXT("%s: 'ObjectClass' is not specified"), *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	FPoolContainer* Pool = FindPool(ObjectClass);
	if (!Pool)
	{
		// Pool is not registered that is ok for this function, so it returns null
		// Outer will create new object and register it in pool
		return nullptr;
	}

	// Try to find first object contained in the Pool by its class that is inactive and ready to be taken from pool
	const FPoolObjectData* FoundData = nullptr;
	for (const FPoolObjectData& DataIt : Pool->PoolObjects)
	{
		if (DataIt.IsFree())
		{
			FoundData = &DataIt;
			break;
		}
	}

	if (!FoundData)
	{
		// No free objects in pool
		return nullptr;
	}

	UObject& InObject = FoundData->GetChecked();

	Pool->GetFactoryChecked().OnTakeFromPool(&InObject, Transform);

	SetObjectStateInPool(EPoolObjectState::Active, InObject, *Pool);

	return FoundData;
}

// Returns the specified object to the pool and deactivates it if the object was taken from the pool before
bool UPoolManagerSubsystem::ReturnToPool_Implementation(UObject* Object)
{
	if (!ensureMsgf(Object, TEXT("ASSERT: [%i] %s:\n'Object' is null!"), __LINE__, *FString(__FUNCTION__)))
	{
		return false;
	}
	
	FPoolContainer& Pool = FindPoolOrAdd(Object->GetClass());
	Pool.GetFactoryChecked().OnReturnToPool(Object);

	SetObjectStateInPool(EPoolObjectState::Inactive, *Object, Pool);

	return true;
}

// Alternative to ReturnToPool() to return object to the pool by its handle
bool UPoolManagerSubsystem::ReturnToPool(const FPoolObjectHandle& Handle)
{
	if (!ensureMsgf(Handle.IsValid(), TEXT("ASSERT: [%i] %s:\n'Handle' is not valid!"), __LINE__, *FString(__FUNCTION__)))
	{
		return false;
	}

	const FPoolContainer& Pool = FindPoolOrAdd(Handle.GetObjectClass());
	if (const FPoolObjectData* ObjectData = Pool.FindInPool(Handle))
	{
		const bool bSucceed = ReturnToPool(ObjectData->PoolObject);
		return ensureMsgf(bSucceed, TEXT("ASSERT: [%i] %s:\nFailed to return object to the Pool by given object!"), __LINE__, *FString(__FUNCTION__));
	}

	// It's exclusive feature of Handles:
	// cancel spawn request if object returns to pool faster than it is spawned
	FSpawnRequest OutRequest;
	const bool bSucceed = Pool.GetFactoryChecked().DequeueSpawnRequestByHandle(Handle, OutRequest);
	return ensureMsgf(bSucceed, TEXT("ASSERT: [%i] %s:\nGiven Handle is not known by Pool Manager and is not even in spawning queue!"), __LINE__, *FString(__FUNCTION__));
}

/*********************************************************************************************
 * Advanced
 ********************************************************************************************* */

// Adds specified object as is to the pool by its class to be handled by the Pool Manager
bool UPoolManagerSubsystem::RegisterObjectInPool_Implementation(const FPoolObjectData& InData)
{
	if (!ensureMsgf(InData.PoolObject, TEXT("ASSERT: [%i] %s:\n'PoolObject' is not valid, can't registed it in the Pool!"), __LINE__, *FString(__FUNCTION__)))
	{
		return false;
	}

	const UClass* ObjectClass = InData.PoolObject.GetClass();
	FPoolContainer& Pool = FindPoolOrAdd(ObjectClass);

	if (Pool.FindInPool(*InData.PoolObject))
	{
		// Already contains in pool
		return false;
	}

	FPoolObjectData Data = InData;
	if (!Data.Handle.IsValid())
	{
		// Hash can be unset that is fine, generate new one
		Data.Handle = FPoolObjectHandle::NewHandle(*ObjectClass);
	}

	Pool.PoolObjects.Emplace(Data);

	SetObjectStateInPool(Data.GetState(), *Data.PoolObject, Pool);

	return true;
}

// Always creates new object and adds it to the pool by its class
FPoolObjectHandle UPoolManagerSubsystem::CreateNewObjectInPool_Implementation(const FSpawnRequest& InRequest)
{
	if (!ensureMsgf(InRequest.Class, TEXT("ASSERT: [%i] %s:\n'Class' is not null in the Spawn Request!"), __LINE__, *FString(__FUNCTION__)))
	{
		return FPoolObjectHandle::EmptyHandle;
	}

	FSpawnRequest Request = InRequest;
	if (!Request.Handle.IsValid())
	{
		// Hash can be unset that is fine, generate new one
		Request.Handle = FPoolObjectHandle::NewHandle(*Request.Class);
	}

	// Always register new object in pool once it is spawned
	const TWeakObjectPtr<ThisClass> WeakThis(this);
	Request.Callbacks.OnPreRegistered = [WeakThis](const FPoolObjectData& ObjectData)
	{
		if (UPoolManagerSubsystem* PoolManager = WeakThis.Get())
		{
			PoolManager->RegisterObjectInPool(ObjectData);
		}
	};

	const FPoolContainer& Pool = FindPoolOrAdd(Request.Class);
	Pool.GetFactoryChecked().RequestSpawn(Request);

	return Request.Handle;
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
	const FPoolObjectData* PoolObject = Pool ? Pool->FindInPool(*Object) : nullptr;

	if (!PoolObject
		|| !PoolObject->IsValid())
	{
		// Is not contained in any pool
		return EPoolObjectState::None;
	}

	return PoolObject->GetState();
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

// Returns number of free objects in pool by specified class
int32 UPoolManagerSubsystem::GetFreeObjectsNum_Implementation(const UClass* ObjectClass) const
{
	const FPoolContainer* Pool = FindPool(ObjectClass);
	if (!Pool)
	{
		return 0;
	}

	int32 FreeObjectsNum = 0;
	for (const FPoolObjectData& PoolObjectIt : Pool->PoolObjects)
	{
		if (PoolObjectIt.IsFree())
		{
			++FreeObjectsNum;
		}
	}
	return FreeObjectsNum;
}

// Returns true if object is known by Pool Manager
bool UPoolManagerSubsystem::IsRegistered_Implementation(const UObject* Object) const
{
	return GetPoolObjectState(Object) != EPoolObjectState::None;
}

// Returns number of registered objects in pool by specified class
int32 UPoolManagerSubsystem::GetRegisteredObjectsNum_Implementation(const UClass* ObjectClass) const
{
	const FPoolContainer* Pool = FindPool(ObjectClass);
	if (!Pool)
	{
		return 0;
	}

	int32 RegisteredObjectsNum = 0;
	for (const FPoolObjectData& PoolObjectIt : Pool->PoolObjects)
	{
		if (PoolObjectIt.IsValid())
		{
			++RegisteredObjectsNum;
		}
	}

	return RegisteredObjectsNum;
}

// Returns the object associated with given handle
UObject* UPoolManagerSubsystem::FindPoolObjectByHandle(const FPoolObjectHandle& Handle) const
{
	const FPoolContainer* Pool = FindPool(Handle.GetObjectClass());
	const FPoolObjectData* ObjectData = Pool ? Pool->FindInPool(Handle) : nullptr;
	return ObjectData ? ObjectData->PoolObject : nullptr;
}

// Returns handle associated with given object
const FPoolObjectHandle& UPoolManagerSubsystem::FindPoolHandleByObject(const UObject* Object) const
{
	const FPoolContainer* Pool = Object ? FindPool(Object->GetClass()) : nullptr;
	const FPoolObjectData* ObjectData = Pool ? Pool->FindInPool(*Object) : nullptr;
	return ObjectData ? ObjectData->Handle : FPoolObjectHandle::EmptyHandle;
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
		const TWeakObjectPtr<ThisClass> WeakThis(this);
		auto OnWorldDestroyed = [WeakThis](const UWorld* World)
		{
			if (!World || !World->IsEditorWorld()
				|| !GEditor || GEditor->IsPlaySessionInProgress())
			{
				// Not Editor world or in PIE
				return;
			}

			if (UPoolManagerSubsystem* PoolManager = WeakThis.Get())
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
	checkf(ObjectClass, TEXT("ERROR: [%i] %s:\n'ObjectClass' is null!"), __LINE__, *FString(__FUNCTION__));

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
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %s:\n'ObjectClass' is null!"), __LINE__, *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	return PoolsInternal.FindByPredicate([ObjectClass](const FPoolContainer& It)
	{
		return It.ObjectClass == ObjectClass;
	});
}

// Activates or deactivates the object if such object is handled by the Pool Manager
void UPoolManagerSubsystem::SetObjectStateInPool(EPoolObjectState NewState, UObject& InObject, FPoolContainer& InPool)
{
	FPoolObjectData* PoolObject = InPool.FindInPool(InObject);
	if (!ensureMsgf(PoolObject && PoolObject->IsValid(), TEXT("ASSERT: [%i] %s:\n'PoolObject' is not registered in given pool for class: %s"), __LINE__, *FString(__FUNCTION__), *GetNameSafe(InPool.ObjectClass)))
	{
		return;
	}

	PoolObject->bIsActive = NewState == EPoolObjectState::Active;

	InPool.GetFactoryChecked().OnChangedStateInPool(NewState, &InObject);
}
