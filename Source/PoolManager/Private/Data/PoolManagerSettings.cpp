// Copyright (c) Yevhenii Selivanov

#include "Data/PoolManagerSettings.h"
//---
#include "Factories/PoolFactory_UObject.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolManagerSettings)

// Returns all Pool Factories that will be used by the Pool Manager
void UPoolManagerSettings::GetPoolFactories(TArray<UClass*>& OutBlueprintPoolFactories) const
{
	if (!OutBlueprintPoolFactories.IsEmpty())
	{
		OutBlueprintPoolFactories.Empty();
	}

	for (const TSoftClassPtr<UPoolFactory_UObject>& It : PoolFactories)
	{
		if (UClass* PoolFactoryClass = It.LoadSynchronous())
		{
			OutBlueprintPoolFactories.Emplace(PoolFactoryClass);
		}
	}
}
