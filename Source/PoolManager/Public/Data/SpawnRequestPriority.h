// Copyright (c) Yevhenii Selivanov

#pragma once

#include "SpawnRequestPriority.generated.h"

/**
 * Priority that affects the order of processing requests.
 * Higher priority requests are processed first.
 */
UENUM(BlueprintType)
enum class ESpawnRequestPriority : uint8
{
	None UMETA(Hidden),
	///< Adds to the end of the queue, good for most situations
	Normal,
	///< Request will be handled after all High requests, but before Normal, which increases processing speed
	Medium,
	///< Guarantees that the new request will be added to the beginning of the queue for next-frame processing; useful for urgent activations, especially when it is very important to have no delay, but other requests will be delayed
	High,
	///< Immediately handles the request in the current frame, without adding it to a queue; should not be used in bulk requests as this request is handled synchronously, which might cause a CPU freeze if called too many times
	Critical,
};
