// Copyright (c) Yevhenii Selivanov

#include "PoolManagerSubsystem.h"
//---
#include "Factories/PoolFactory_UObject.h"
#include "Data/PoolManagerSettings.h"
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

	const UWorld* World = OptionalWorldContext
		                      ? GEngine->GetWorldFromContextObject(OptionalWorldContext, EGetWorldErrorMode::ReturnNull)
		                      : GEngine->GetCurrentPlayWorld();
#if WITH_EDITOR
	if (!World && GIsEditor && GEditor)
	{
		World = GEditor->IsPlaySessionInProgress()
			        ? (GEditor->GetCurrentPlayWorld() ? GEditor->GetCurrentPlayWorld() : (GEditor->GetPIEWorldContext() ? GEditor->GetPIEWorldContext()->World() : nullptr))
			        : GEditor->GetEditorWorldContext().World();
		World = World ? World : GWorld;
	}
#endif

	UPoolManagerSubsystem* FoundPoolManager = World ? Cast<UPoolManagerSubsystem>(World->GetSubsystemBase(OptionalClass)) : nullptr;
	if (!ensureMsgf(FoundPoolManager, TEXT("%hs: 'Can not find Pool Manager for %s class in %s world"), __FUNCTION__, *OptionalClass->GetName(), *World->GetName()))
	{
		return nullptr;
	}

	return FoundPoolManager;
}

/*********************************************************************************************
 * Take From Pool (single object)
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

	FSpawnRequest Request(ObjectClass);
	Request.Transform = Transform;
	Request.Callbacks.OnPostSpawned = [Completed](const FPoolObjectData& It)
	{
		Completed.ExecuteIfBound(It.PoolObject);
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

	FSpawnRequest Request(ObjectClass);
	Request.Transform = Transform;
	Request.Callbacks.OnPostSpawned = Completed;
	return CreateNewObjectInPool(Request);
}

// Is internal function to find object in pool or return null
const FPoolObjectData* UPoolManagerSubsystem::TakeFromPoolOrNull(const UClass* ObjectClass, const FTransform& Transform/* = FTransform::Identity*/)
{
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %hs:\n'ObjectClass' is not specified!"), __LINE__, __FUNCTION__))
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

/*********************************************************************************************
 * Take From Pool (multiple objects)
 ********************************************************************************************* */

// Is the same as BPTakeFromPool() but for multiple objects
void UPoolManagerSubsystem::BPTakeFromPoolArray(const UClass* ObjectClass, int32 Amount, const FOnTakenFromPoolArray& Completed)
{
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %hs:\n'ObjectClass' is not specified!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// --- Take if free objects in pool first
	TArray<FSpawnRequest> InRequests;
	FSpawnRequest::MakeRequests(/*out*/InRequests, ObjectClass, Amount);
	TArray<FPoolObjectData> FreeObjectsData;
	TakeFromPoolArrayOrNull(/*out*/FreeObjectsData, InRequests);

	const int32 Difference = InRequests.Num() - FreeObjectsData.Num();
	if (Difference == 0)
	{
		// All objects are taken from pool
		TArray<UObject*> OutObjects;
		FPoolObjectData::Conv_PoolDataToObjects(OutObjects, FreeObjectsData);
		Completed.ExecuteIfBound(OutObjects);
		return;
	}

	// --- Create the rest of objects
	TArray<FPoolObjectHandle> OutHandles;
	FPoolObjectHandle::Conv_ObjectsToHandles(OutHandles, FreeObjectsData);
	FSpawnRequest::FilterRequests(/*out*/InRequests, FreeObjectsData, Difference);
	CreateNewObjectsArrayInPool(InRequests, OutHandles, [Completed](const TArray<FPoolObjectData>& OutObjects)
	{
		TArray<UObject*> Objects;
		FPoolObjectData::Conv_PoolDataToObjects(Objects, OutObjects);
		Completed.ExecuteIfBound(Objects);
	});
}

// Is code-overridable alternative version of BPTakeFromPoolArray() that calls callback functions when all objects of the same class are ready
void UPoolManagerSubsystem::TakeFromPoolArray(TArray<FPoolObjectHandle>& OutHandles, const UClass* ObjectClass, int32 Amount, const FOnSpawnAllCallback& Completed)
{
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %hs:\n'ObjectClass' is not specified!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	TArray<FSpawnRequest> InRequests;
	FSpawnRequest::MakeRequests(/*out*/InRequests, ObjectClass, Amount);
	TArray<FPoolObjectData> FreeObjectsData;
	TakeFromPoolArrayOrNull(/*out*/FreeObjectsData, InRequests);
	FPoolObjectHandle::Conv_ObjectsToHandles(OutHandles, FreeObjectsData);

	const int32 Difference = InRequests.Num() - FreeObjectsData.Num();
	if (Difference == 0)
	{
		// All objects are taken from pool
		if (Completed)
		{
			Completed(FreeObjectsData);
		}
		return;
	}

	// --- Create the rest of objects
	FSpawnRequest::FilterRequests(/*out*/InRequests, FreeObjectsData, Difference);
	CreateNewObjectsArrayInPool(InRequests, OutHandles, Completed);
}

// Is alternative version of TakeFromPool() that can process multiple requests of different classes and different transforms at once
void UPoolManagerSubsystem::TakeFromPoolArray(TArray<FPoolObjectHandle>& OutHandles, TArray<FSpawnRequest>& InRequests, const FOnSpawnAllCallback& Completed)
{
	if (!ensureMsgf(!InRequests.IsEmpty(), TEXT("ASSERT: [%i] %hs:\n'InOutRequests' is empty!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// --- Take if free objects in pool first
	TArray<FPoolObjectData> FreeObjectsData;
	TakeFromPoolArrayOrNull(/*out*/FreeObjectsData, InRequests);
	FPoolObjectHandle::Conv_ObjectsToHandles(OutHandles, FreeObjectsData);

	const int32 Difference = InRequests.Num() - FreeObjectsData.Num();
	if (Difference == 0)
	{
		// All objects are taken from pool
		if (Completed)
		{
			Completed(FreeObjectsData);
		}
		return;
	}

	// --- Create the rest of objects
	FSpawnRequest::FilterRequests(/*out*/InRequests, FreeObjectsData, Difference);
	CreateNewObjectsArrayInPool(InRequests, OutHandles, Completed);
}

// Is alternative version of TakeFromPoolArrayOrNull() to find multiple object in pool or return null
void UPoolManagerSubsystem::TakeFromPoolArrayOrNull(TArray<FPoolObjectData>& OutObjects, TArray<FSpawnRequest>& InRequests)
{
	if (!OutObjects.IsEmpty())
	{
		OutObjects.Empty();
	}

	for (FSpawnRequest& ItRef : InRequests)
	{
		if (const FPoolObjectData* ObjectData = TakeFromPoolOrNull(ItRef.GetClass(), ItRef.Transform))
		{
			ItRef.Handle = ObjectData->Handle;
			OutObjects.Emplace(*ObjectData);
		}
	}
}

/*********************************************************************************************
 * Return To Pool (single object)
 ********************************************************************************************* */

// Returns the specified object to the pool and deactivates it if the object was taken from the pool before
bool UPoolManagerSubsystem::ReturnToPool_Implementation(UObject* Object)
{
	if (!ensureMsgf(Object, TEXT("ASSERT: [%i] %hs:\n'Object' is null!"), __LINE__, __FUNCTION__))
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
	if (!ensureMsgf(Handle.IsValid(), TEXT("ASSERT: [%i] %hs:\n'Handle' is not valid!"), __LINE__, __FUNCTION__))
	{
		return false;
	}

	const FPoolContainer& Pool = FindPoolOrAdd(Handle.GetObjectClass());
	if (const FPoolObjectData* ObjectData = Pool.FindInPool(Handle))
	{
		const bool bSucceed = ReturnToPool(ObjectData->PoolObject);
		return ensureMsgf(bSucceed, TEXT("ASSERT: [%i] %hs:\nFailed to return object to the Pool by given object!"), __LINE__, __FUNCTION__);
	}

	// It's exclusive feature of Handles:
	// cancel spawn request if object returns to pool faster than it is spawned
	FSpawnRequest OutRequest;
	const bool bSucceed = Pool.GetFactoryChecked().DequeueSpawnRequestByHandle(Handle, OutRequest);
	return ensureMsgf(bSucceed, TEXT("ASSERT: [%i] %hs:\nGiven Handle is not known by Pool Manager and is not even in spawning queue!"), __LINE__, __FUNCTION__);
}

/*********************************************************************************************
 * Return To Pool (multiple objects) 
 ********************************************************************************************* */

// Is the same as ReturnToPool() but for multiple objects
bool UPoolManagerSubsystem::ReturnToPoolArray_Implementation(const TArray<UObject*>& Objects)
{
	bool bSucceed = true;
	for (UObject* ObjectIt : Objects)
	{
		bSucceed &= ReturnToPool(ObjectIt);
	}
	return bSucceed;
}

// Is the same as ReturnToPool() but for multiple handle
bool UPoolManagerSubsystem::ReturnToPoolArray(const TArray<FPoolObjectHandle>& Handles)
{
	bool bSucceed = true;
	for (const FPoolObjectHandle& HandleIt : Handles)
	{
		bSucceed &= ReturnToPool(HandleIt);
	}
	return bSucceed;
}

/*********************************************************************************************
 * Advanced
 ********************************************************************************************* */

// Adds specified object as is to the pool by its class to be handled by the Pool Manager
bool UPoolManagerSubsystem::RegisterObjectInPool_Implementation(const FPoolObjectData& InData)
{
	if (!ensureMsgf(InData.PoolObject, TEXT("ASSERT: [%i] %hs:\n'PoolObject' is not valid, can't registed it in the Pool!"), __LINE__, __FUNCTION__))
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
		Data.Handle = FPoolObjectHandle::NewHandle(ObjectClass);
	}

	Pool.PoolObjects.Emplace(Data);

	SetObjectStateInPool(Data.GetState(), *Data.PoolObject, Pool);

	return true;
}

// Always creates new object and adds it to the pool by its class
FPoolObjectHandle UPoolManagerSubsystem::CreateNewObjectInPool_Implementation(const FSpawnRequest& InRequest)
{
	if (!ensureMsgf(InRequest.GetClass(), TEXT("ASSERT: [%i] %hs:\n'Class' is not null in the Spawn Request!"), __LINE__, __FUNCTION__))
	{
		return FPoolObjectHandle::EmptyHandle;
	}

	FSpawnRequest Request = InRequest;
	if (!Request.Handle.IsValid())
	{
		// Hash can be unset that is fine, generate new one
		Request.Handle = FPoolObjectHandle::NewHandle(Request.GetClass());
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

	const FPoolContainer& Pool = FindPoolOrAdd(Request.GetClass());
	Pool.GetFactoryChecked().RequestSpawn(Request);

	return Request.Handle;
}

// Is the same as CreateNewObjectInPool() but for multiple objects
void UPoolManagerSubsystem::CreateNewObjectsArrayInPool(TArray<FSpawnRequest>& InRequests, TArray<FPoolObjectHandle>& OutAllHandles, const FOnSpawnAllCallback& Completed/*= nullptr*/)
{
	TArray<FPoolObjectHandle> NewHandles;
	FPoolObjectHandle::Conv_RequestsToHandles(/*out*/NewHandles, InRequests);
	OutAllHandles.Append(NewHandles);

	// --- Process OnEachSpawned only if Completed is set
	FOnSpawnCallback OnEachSpawned = nullptr;
	if (Completed)
	{
		const FPoolObjectHandle& LastHandleRequest = InRequests.Last().Handle;
		const TWeakObjectPtr<const ThisClass> WeakThis(this);
		OnEachSpawned = [WeakThis, OutAllHandles, LastHandleRequest, Completed](const FPoolObjectData& ObjectData)
		{
			const UPoolManagerSubsystem* PoolManager = WeakThis.Get();
			if (!PoolManager
				|| ObjectData.Handle != LastHandleRequest)
			{
				// Not last object are spawned yet
				// We can rely on LastHandle because the order of requests in queue is preserved
				return;
			}

			TArray<FPoolObjectData> OutObjects;
			PoolManager->FindPoolObjectsByHandles(OutObjects, OutAllHandles);

			ensureMsgf(OutObjects.Num() == OutAllHandles.Num(), TEXT("ASSERT: [%i] %hs:\n'OutObjects %i != AllHandles %i: It is last Spawn Request is processed, however some of objects failed to spawn or have been destroyed!"), __LINE__, __FUNCTION__, OutObjects.Num(), OutAllHandles.Num());

			Completed(OutObjects);
		};
	}

	for (FSpawnRequest& It : InRequests)
	{
		It.Callbacks.OnPostSpawned = OnEachSpawned;
		CreateNewObjectInPool(It);
	}
}

/*********************************************************************************************
 * Advanced - Factories
 ********************************************************************************************* */

// Registers new factory to be used by the Pool Manager when dealing with objects of specific class and its children
void UPoolManagerSubsystem::AddFactory(TSubclassOf<UPoolFactory_UObject> FactoryClass)
{
	const UClass* ObjectClass = GetObjectClassByFactory(FactoryClass);
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %hs:\n'ObjectClass' is not set for next factory: %s"), __LINE__, __FUNCTION__, *FactoryClass->GetName())
		|| AllFactoriesInternal.Contains(ObjectClass))
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
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %hs:\n'ObjectClass' is not set for next factory: %s"), __LINE__, __FUNCTION__, *FactoryClass->GetName()))
	{
		return;
	}

	const TObjectPtr<UPoolFactory_UObject>* FactoryPtr = AllFactoriesInternal.Find(ObjectClass);
	if (!ensureMsgf(FactoryPtr, TEXT("ASSERT: [%i] %hs:\nFactory is not found for next class: %s"), __LINE__, __FUNCTION__, *ObjectClass->GetName()))
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
	checkf(ObjectClass, TEXT("ERROR: [%i] %hs:\n'ObjectClass' is null!"), __LINE__, __FUNCTION__);

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

	checkf(FoundFactory, TEXT("ERROR: [%i] %hs:\n'FoundFactory' is null for next object class: %s"), __LINE__, __FUNCTION__, *GetNameSafe(ObjectClass));
	return *FoundFactory;
}

// Returns default class of object that is handled by given factory
const UClass* UPoolManagerSubsystem::GetObjectClassByFactory(const TSubclassOf<UPoolFactory_UObject>& FactoryClass)
{
	if (!ensureMsgf(FactoryClass, TEXT("ASSERT: [%i] %hs:\n'FactoryClass' is null!"), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}

	const UPoolFactory_UObject* FactoryCDO = CastChecked<UPoolFactory_UObject>(FactoryClass->GetDefaultObject());
	return FactoryCDO->GetObjectClass();
}

// Creates all possible Pool Factories to be used by the Pool Manager when dealing with objects
void UPoolManagerSubsystem::InitializeAllFactories()
{
	TArray<UClass*> AllPoolFactories;
	UPoolManagerSettings::Get().GetPoolFactories(/*out*/AllPoolFactories);
	for (UClass* FactoryClass : AllPoolFactories)
	{
		AddFactory(FactoryClass);
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
	if (!ensureMsgf(Pool, TEXT("%hs: 'Pool' is not valid"), __FUNCTION__))
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
const FPoolObjectData& UPoolManagerSubsystem::FindPoolObjectByHandle(const FPoolObjectHandle& Handle) const
{
	const FPoolContainer* Pool = FindPool(Handle.GetObjectClass());
	const FPoolObjectData* ObjectData = Pool ? Pool->FindInPool(Handle) : nullptr;
	return ObjectData ? *ObjectData : FPoolObjectData::EmptyObject;
}

// Returns handle associated with given object
const FPoolObjectHandle& UPoolManagerSubsystem::FindPoolHandleByObject(const UObject* Object) const
{
	const FPoolContainer* Pool = Object ? FindPool(Object->GetClass()) : nullptr;
	const FPoolObjectData* ObjectData = Pool ? Pool->FindInPool(*Object) : nullptr;
	return ObjectData ? ObjectData->Handle : FPoolObjectHandle::EmptyHandle;
}

// Returns from all given handles only valid ones
void UPoolManagerSubsystem::FindPoolObjectsByHandles(TArray<FPoolObjectData>& OutObjects, const TArray<FPoolObjectHandle>& InHandles) const
{
	for (const FPoolObjectHandle& HandleIt : InHandles)
	{
		FPoolObjectData PoolObject = FindPoolObjectByHandle(HandleIt);
		if (PoolObject.IsValid())
		{
			OutObjects.Emplace(MoveTemp(PoolObject));
		}
	}
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
	checkf(ObjectClass, TEXT("ERROR: [%i] %hs:\n'ObjectClass' is null!"), __LINE__, __FUNCTION__);

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
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %hs:\n'ObjectClass' is null!"), __LINE__, __FUNCTION__))
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
	if (!ensureMsgf(PoolObject && PoolObject->IsValid(), TEXT("ASSERT: [%i] %hs:\n'PoolObject' is not registered in given pool for class: %s"), __LINE__, __FUNCTION__, *GetNameSafe(InPool.ObjectClass)))
	{
		return;
	}

	PoolObject->bIsActive = NewState == EPoolObjectState::Active;

	InPool.GetFactoryChecked().OnChangedStateInPool(NewState, &InObject);
}
