// Copyright (c) Yevhenii Selivanov

#include "PoolManagerUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolManagerUtils)

// Creates a spawn request for pool objects
FSpawnRequest UPoolManagerUtils::MakeSpawnRequest(TSubclassOf<UObject> ObjectClass, const FTransform& Transform, ESpawnRequestPriority Priority/* = ESpawnRequestPriority::Normal*/)
{
	if (!ensureMsgf(ObjectClass, TEXT("ASSERT: [%i] %hs:\n'ObjectClass' is null, can't create spawn request!"), __LINE__, __FUNCTION__))
	{
		return FSpawnRequest();
	}

	FSpawnRequest Request(ObjectClass);
	Request.Transform = Transform;
	Request.Priority = Priority;
	return Request;
}
