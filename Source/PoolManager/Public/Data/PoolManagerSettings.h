// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Engine/DeveloperSettings.h"
//---
#include "PoolManagerSettings.generated.h"

class UPoolFactory_UObject;

/**
 * Contains common settings data of the Pool Manager plugin.
 * Is set up in 'Project Settings' -> "Plugins" -> "Pool Manager".
 */
UCLASS(Config = "PoolManager", DefaultConfig, meta = (DisplayName = "Pool Manager"))
class POOLMANAGER_API UPoolManagerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Returns Project Settings data of the Pool Manager plugin. */
	static const FORCEINLINE UPoolManagerSettings& Get() { return *GetDefault<ThisClass>(); }

	/** Returns Project Settings data of the Pool Manager plugin. */
	UFUNCTION(BlueprintPure, Category = "Pool Manager")
	static const FORCEINLINE UPoolManagerSettings* GetPoolManagerSettings() { return &Get(); }

	/** Gets the settings container name for the settings, either Project or Editor */
	virtual FName GetContainerName() const override { return TEXT("Project"); }

	/** Gets the category for the settings, some high level grouping like, Editor, Engine, Game...etc. */
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Returns a limit of how many actors to spawn per frame. */
	UFUNCTION(BlueprintPure, Category = "Pool Manager")
	int32 GetSpawnObjectsPerFrame() const { return SpawnObjectsPerFrame; }

	/** Returns all Pool Factories that will be used by the Pool Manager. */
	UFUNCTION(BlueprintPure, Category = "Pool Manager")
	void GetPoolFactories(TArray<UClass*>& OutBlueprintPoolFactories) const;

protected:
	/** Set a limit of how many actors to spawn per frame. */
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Pool Manager", meta = (BlueprintProtected = "true"))
	int32 SpawnObjectsPerFrame;

	/** All Pool Factories that will be used by the Pool Manager. */
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Pool Manager", meta = (BlueprintProtected = "true"))
	TArray<TSoftClassPtr<UPoolFactory_UObject>> PoolFactories;
};
