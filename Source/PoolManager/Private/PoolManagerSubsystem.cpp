// Copyright (c) Yevhenii Selivanov

#include "PoolManagerSubsystem.h"
//---
#include "Engine/World.h"
#include "GameFramework/Actor.h"
//---
#if WITH_EDITOR
#include "Editor.h"
#endif // WITH_EDITOR
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolManagerSubsystem)

// It's almost farthest possible location where deactivated actors are placed
#define VECTOR_HALF_WORLD_MAX FVector(HALF_WORLD_MAX - HALF_WORLD_MAX * THRESH_VECTOR_NORMALIZED)

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

// Get the object from a pool by specified class
UObject* UPoolManagerSubsystem::TakeFromPool_Implementation(const UClass* ClassInPool, const FTransform& Transform)
{
	if (!ensureMsgf(ClassInPool, TEXT("%s: 'ClassInPool' is not specified"), *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	// Try to get free object from the pool and return
	if (UObject* PoolObject = GetFreeObjectInPool(ClassInPool))
	{
		if (AActor* Actor = Cast<AActor>(PoolObject))
		{
			Actor->SetActorTransform(Transform);
		}

		SetActive(true, PoolObject);

		return PoolObject;
	}

	// Since there is no free object in the pool, create a new one
	return CreateNewObjectInPool(ClassInPool, Transform, EPoolObjectState::Active);
}

// Returns the specified object to the pool and deactivates it if the object was taken from the pool before
void UPoolManagerSubsystem::ReturnToPool_Implementation(UObject* Object)
{
	SetActive(false, Object);
}

// Adds specified object as is to the pool by its class to be handled by the Pool Manager
bool UPoolManagerSubsystem::RegisterObjectInPool_Implementation(UObject* Object, EPoolObjectState PoolObjectState/* = EPoolObjectState::Inactive*/)
{
	if (!Object)
	{
		return false;
	}

	const UClass* ActorClass = Object->GetClass();
	FPoolContainer* Pool = FindPool(ActorClass);
	if (!Pool)
	{
		const int32 PoolIndex = PoolsInternal.Emplace(FPoolContainer(ActorClass));
		Pool = &PoolsInternal[PoolIndex];
	}

	if (!ensureMsgf(Pool, TEXT("%s: 'Pool' is not valid"), *FString(__FUNCTION__)))
	{
		return false;
	}

	if (Pool->FindInPool(Object))
	{
		// Already contains in pool
		return false;
	}

	FPoolObjectData PoolObject(Object);

	if (const AActor* Actor = Cast<AActor>(Object))
	{
		// Decide by its location should it be activated or not if only state is not specified
		switch (PoolObjectState)
		{
		case EPoolObjectState::None:
			PoolObject.bIsActive = !Actor->GetActorLocation().Equals(VECTOR_HALF_WORLD_MAX);
			break;
		case EPoolObjectState::Active:
			PoolObject.bIsActive = true;
			break;
		case EPoolObjectState::Inactive:
			PoolObject.bIsActive = false;
			break;
		default:
			checkf(false, TEXT("%s: Invalid plugin enumeration type. Need to add a handle for that case here"), *FString(__FUNCTION__));
			break;
		}
	}

	Pool->PoolObjects.Emplace(PoolObject);

	SetActive(PoolObject.bIsActive, Object);

	return true;
}

// Always creates new object and adds it to the pool by its class
UObject* UPoolManagerSubsystem::CreateNewObjectInPool_Implementation(const UClass* ObjectClass, const FTransform& Transform, EPoolObjectState PoolObjectState)
{
	UWorld* World = GetWorld();
	if (!ensureMsgf(World, TEXT("%s: 'World' is not valid"), *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	FPoolContainer* Pool = FindPool(ObjectClass);
	if (!Pool)
	{
		const int32 PoolIndex = PoolsInternal.Emplace(FPoolContainer(ObjectClass));
		Pool = &PoolsInternal[PoolIndex];
	}

	UObject* CreatedObject;
	if (ObjectClass->IsChildOf<AActor>())
	{
		UClass* ClassToSpawn = const_cast<UClass*>(ObjectClass);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel; // Always keep new objects on Persistent level
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.bDeferConstruction = true; // Delay construction to add it to the pool first
		CreatedObject = World->SpawnActor(ClassToSpawn, &Transform, SpawnParameters);
	}
	else
	{
		CreatedObject = NewObject<UObject>(World, ObjectClass);
	}

	checkf(CreatedObject, TEXT("CRITICAL ERROR: %s: 'CreatedObject' is not valid"), *FString(__FUNCTION__))

	FPoolObjectData PoolObjectData;
	PoolObjectData.PoolObject = CreatedObject;
	// Set activity here instead of calling UPoolManagerSubsystem::SetActive since new object never was inactivated before to switch the state
	PoolObjectData.bIsActive = PoolObjectState == EPoolObjectState::Active;
	Pool->PoolObjects.Emplace(MoveTemp(PoolObjectData));

	if (AActor* SpawnedActor = Cast<AActor>(CreatedObject))
	{
		// Call construction script since it was delayed before to add it to the pool first
		SpawnedActor->FinishSpawning(Transform);
	}

	return CreatedObject;
}

// Destroy all object of a pool by a given class
void UPoolManagerSubsystem::EmptyPool_Implementation(const UClass* ClassInPool)
{
	FPoolContainer* Pool = FindPool(ClassInPool);
	if (!ensureMsgf(Pool, TEXT("%s: 'Pool' is not valid"), *FString(__FUNCTION__)))
	{
		return;
	}

	TArray<FPoolObjectData>& PoolObjects = Pool->PoolObjects;
	for (int32 Index = PoolObjects.Num() - 1; Index >= 0; --Index)
	{
		UObject* ObjectIt = PoolObjects.IsValidIndex(Index) ? PoolObjects[Index].Get() : nullptr;
		if (!IsValid(ObjectIt))
		{
			continue;
		}

		if (AActor* Actor = Cast<AActor>(ObjectIt))
		{
			Actor->Destroy();
		}
		else
		{
			ObjectIt->ConditionalBeginDestroy();
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
		const UClass* ClassInPool = PoolsInternal.IsValidIndex(Index) ? PoolsInternal[Index].ClassInPool : nullptr;
		EmptyPool(ClassInPool);
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

		TArray<FPoolObjectData>& PoolObjectsRef = PoolsInternal[PoolIndex].PoolObjects;
		const int32 ObjectsNum = PoolObjectsRef.Num();
		for (int32 ObjectIndex = ObjectsNum - 1; ObjectIndex >= 0; --ObjectIndex)
		{
			UObject* ObjectIt = PoolObjectsRef.IsValidIndex(ObjectIndex) ? PoolObjectsRef[ObjectIndex].Get() : nullptr;
			if (!IsValid(ObjectIt)
				|| !Predicate(ObjectIt))
			{
				continue;
			}

			if (AActor* Actor = Cast<AActor>(ObjectIt))
			{
				Actor->Destroy();
			}
			else
			{
				ObjectIt->ConditionalBeginDestroy();
			}

			PoolObjectsRef.RemoveAt(ObjectIndex);
		}
	}
}

// Activates or deactivates the object if such object is handled by the Pool Manager
void UPoolManagerSubsystem::SetActive(bool bShouldActivate, UObject* Object)
{
	const UWorld* World = Object ? Object->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	const UClass* ClassInPool = Object ? Object->GetClass() : nullptr;
	FPoolContainer* Pool = FindPool(ClassInPool);
	FPoolObjectData* PoolObject = Pool ? Pool->FindInPool(Object) : nullptr;
	if (!PoolObject
		|| !PoolObject->IsValid())
	{
		return;
	}

	PoolObject->bIsActive = bShouldActivate;

	AActor* Actor = PoolObject->Get<AActor>();
	if (!Actor)
	{
		return;
	}

	if (!bShouldActivate)
	{
		// SetCollisionEnabled is not replicated, client collides with hidden actor, so move it
		Actor->SetActorLocation(VECTOR_HALF_WORLD_MAX);
	}

	Actor->SetActorHiddenInGame(!bShouldActivate);
	Actor->SetActorEnableCollision(bShouldActivate);
	Actor->SetActorTickEnabled(bShouldActivate);
}

// Returns current state of specified object
EPoolObjectState UPoolManagerSubsystem::GetPoolObjectState_Implementation(const UObject* Object) const
{
	const UClass* ClassInPool = Object ? Object->GetClass() : nullptr;
	const FPoolContainer* Pool = FindPool(ClassInPool);
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

bool UPoolManagerSubsystem::ContainsClassInPool_Implementation(const UClass* ClassInPool) const
{
	return FindPool(ClassInPool) != nullptr;
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

// Returns first object contained in the Pool by its class that is inactive and ready to be taken from pool
UObject* UPoolManagerSubsystem::GetFreeObjectInPool_Implementation(const UClass* ObjectClass) const
{
	const FPoolContainer* Pool = FindPool(ObjectClass);
	if (!Pool)
	{
		return nullptr;
	}

	// Try to find ready object to return
	for (const FPoolObjectData& PoolObjectIt : Pool->PoolObjects)
	{
		if (PoolObjectIt.IsFree())
		{
			return PoolObjectIt.Get();
		}
	}

	// There is no free object to be taken
	return nullptr;
}

// Returns true if object is known by Pool Manager
bool UPoolManagerSubsystem::IsRegistered_Implementation(const UObject* Object) const
{
	return GetPoolObjectState(Object) != EPoolObjectState::None;
}

// Is called on initialization of the Pool Manager instance
void UPoolManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

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

// Returns the pointer to found pool by specified class
FPoolContainer* UPoolManagerSubsystem::FindPool(const UClass* ClassInPool)
{
	if (!ClassInPool)
	{
		return nullptr;
	}

	return PoolsInternal.FindByPredicate([ClassInPool](const FPoolContainer& It)
	{
		return It.ClassInPool == ClassInPool;
	});
}
