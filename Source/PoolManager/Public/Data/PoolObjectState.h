// Copyright (c) Yevhenii Selivanov

#pragma once

#include "PoolObjectState.generated.h"

/**
 * States of the object in Pool
 */
UENUM(BlueprintType)
enum class EPoolObjectState : uint8
{
	///< Is not handled by Pool Manager
	None,
	///< Contains in pool, is free and ready to be taken
	Inactive,
	///< Was taken from pool and can be returned back.
	Active
};
