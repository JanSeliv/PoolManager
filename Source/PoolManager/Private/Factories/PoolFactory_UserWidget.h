// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Factories/PoolFactory_UObject.h"
//---
#include "PoolFactory_UserWidget.generated.h"

/**
 * Is responsible for managing User Widgets, it properly creates and destroys them.
 */
UCLASS()
class POOLMANAGER_API UPoolFactory_UserWidget : public UPoolFactory_UObject
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Setup overrides
	 ********************************************************************************************* */
public:
	/** Is overridden to handle UserWidget-inherited classes. */
	virtual const UClass* GetObjectClass_Implementation() const override;

	/*********************************************************************************************
	 * Creation
	 ********************************************************************************************* */
public:
	/** Is overridden to create the User Widget using its engine's 'Create Widget' method. */
	virtual UObject* SpawnNow_Implementation(const FSpawnRequest& Request) override;

	/*********************************************************************************************
	 * Destruction
	 ********************************************************************************************* */
public:
	/** Is overridden to destroy handled User Widget using its engine's RemoveFromParent method. */
	virtual void Destroy_Implementation(UObject* Object) override;

	/*********************************************************************************************
	 * Pool
	 ********************************************************************************************* */
public:
	/** Is overridden to change visibility according new state. */
	virtual void OnChangedStateInPool_Implementation(EPoolObjectState NewState, UObject* InObject) override;
};
