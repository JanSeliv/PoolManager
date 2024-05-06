// Copyright (c) Yevhenii Selivanov

#include "Factories/PoolFactory_Actor.h"
//---
#include "Engine/World.h"
#include "GameFramework/Actor.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolFactory_Actor)

// It's almost farthest possible location where deactivated actors are placed
#define VECTOR_HALF_WORLD_MAX FVector(HALF_WORLD_MAX - HALF_WORLD_MAX * THRESH_VECTOR_NORMALIZED)

// Is overridden to handle Actors-inherited classes
const UClass* UPoolFactory_Actor::GetObjectClass_Implementation() const
{
	return AActor::StaticClass();
}

/*********************************************************************************************
 * Creation
 ********************************************************************************************* */

// Is overridden to spawn actors using its engine's Spawn Actor method
UObject* UPoolFactory_Actor::SpawnNow_Implementation(const FSpawnRequest& Request)
{
	// Super is not called to Spawn Actor instead of NewObject

	UWorld* World = GetWorld();
	checkf(World, TEXT("ERROR: [%i] %hs:\n'World' is null!"), __LINE__, __FUNCTION__);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.OverrideLevel = World->PersistentLevel; // Always keep new objects on Persistent level
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.bDeferConstruction = true; // Delay construction to add it to the pool first
	SpawnParameters.bNoFail = true; // Do not fail if spawn fails
#if WITH_EDITORONLY_DATA
	SpawnParameters.bCreateActorPackage = false; // Do not bake this runtime actor into World Partition level
#endif

	return World->SpawnActor(Request.GetClassChecked<AActor>(), &Request.Transform, SpawnParameters);
}

// Is overridden to finish spawning the actor since it was deferred
void UPoolFactory_Actor::OnPreRegistered(const FSpawnRequest& Request, const FPoolObjectData& ObjectData)
{
	Super::OnPreRegistered(Request, ObjectData);

	AActor& SpawnedActor = ObjectData.GetChecked<AActor>();
	SpawnedActor.FinishSpawning(Request.Transform);
}

/*********************************************************************************************
 * Destruction
 ********************************************************************************************* */

// Is overridden to destroy given actor using its engine's Destroy Actor method
void UPoolFactory_Actor::Destroy_Implementation(UObject* Object)
{
	// Super is not called to Destroy Actor instead of ConditionalBeginDestroy

	AActor* Actor = CastChecked<AActor>(Object);
	checkf(IsValid(Actor), TEXT("ERROR: [%i] %hs:\n'IsValid(Actor)' is null!"), __LINE__, __FUNCTION__);
	Actor->Destroy();
}

/*********************************************************************************************
 * Pool
 ********************************************************************************************* */

// Is overridden to set transform to the actor before taking the object from its pool
void UPoolFactory_Actor::OnTakeFromPool_Implementation(UObject* Object, const FTransform& Transform)
{
	Super::OnTakeFromPool_Implementation(Object, Transform);

	AActor* Actor = CastChecked<AActor>(Object);
	Actor->SetActorTransform(Transform);
}

// Is overridden to reset transform to the actor before returning the object to its pool
void UPoolFactory_Actor::OnReturnToPool_Implementation(UObject* Object)
{
	Super::OnReturnToPool_Implementation(Object);

	// SetCollisionEnabled is not replicated, client collides with hidden actor, so move it far away
	AActor* Actor = CastChecked<AActor>(Object);
	Actor->SetActorLocation(VECTOR_HALF_WORLD_MAX);
}

// Is overridden to change visibility, collision, ticking, etc. according new state
void UPoolFactory_Actor::OnChangedStateInPool_Implementation(EPoolObjectState NewState, UObject* InObject)
{
	Super::OnChangedStateInPool_Implementation(NewState, InObject);

	AActor* Actor = CastChecked<AActor>(InObject);
	const bool bActivate = NewState == EPoolObjectState::Active;

	Actor->SetActorHiddenInGame(!bActivate);
	Actor->SetActorEnableCollision(bActivate);
	Actor->SetActorTickEnabled(bActivate);
}
