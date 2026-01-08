// Copyright (c) Yevhenii Selivanov

#pragma once

struct FPoolObjectData;

typedef TFunction<void(const FPoolObjectData&)> FOnSpawnCallback;
typedef TFunction<void(const TArray<FPoolObjectData>&)> FOnSpawnAllCallback;

/**
 * Contains the functions that are called when the object is spawned.
 */
struct POOLMANAGER_API FSpawnCallbacks
{
	/** Returns complete object data before registration in the Pool. */
	FOnSpawnCallback OnPreRegistered = nullptr;

	/** Returns already spawned and registered object. */
	FOnSpawnCallback OnPostSpawned = nullptr;
};
