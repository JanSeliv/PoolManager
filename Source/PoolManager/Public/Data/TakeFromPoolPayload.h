// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Math/Transform.h"

#include "TakeFromPoolPayload.generated.h"

/**
 * Contains the payload data when taking an object from the pool.
 */
USTRUCT(BlueprintType)
struct FTakeFromPoolPayload
{
	GENERATED_BODY()

	/** Transform of the object to take from pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	FTransform Transform = FTransform::Identity;

	/** Is true whenever the object is newly spawned instead of taken from existing pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	bool bIsNewSpawned = false;
};
