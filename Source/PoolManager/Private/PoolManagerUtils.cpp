// Copyright (c) Yevhenii Selivanov

#include "PoolManagerUtils.h"

// PoolManager
#include "Data/SpawnRequest.h"

// UE
#include "GameFeatureData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolManagerUtils)

// Creates a spawn request for pool objects
FSpawnRequest UPoolManagerUtils::MakeSpawnRequest(TSubclassOf<UObject> ObjectClass, const FTransform& Transform, ESpawnRequestPriority Priority /* = ESpawnRequestPriority::Normal*/)
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

// Returns true if the given class belongs to the game feature plugin identified by GameFeatureData
bool UPoolManagerUtils::IsPoolInGameFeaturePlugin(const UClass* ObjectClass, const UGameFeatureData* GameFeatureData)
{
	if (!ObjectClass || !GameFeatureData)
	{
		return false;
	}

	// Extract plugin name from GameFeatureData package
	const FString GameFeaturePackageName = GetNameSafe(GameFeatureData->GetOutermost());
	const int32 PluginSlashIdx = GameFeaturePackageName.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1);
	if (PluginSlashIdx == INDEX_NONE)
	{
		return false;
	}
	const FString PluginName = GameFeaturePackageName.Mid(1, PluginSlashIdx - 1);

	// Resolve class module name from its package
	const FString ClassPackageName = GetNameSafe(ObjectClass->GetOutermost());
	static const FString ScriptPrefix = TEXT("/Script/");
	FString ModuleName;
	if (ClassPackageName.StartsWith(ScriptPrefix))
	{
		// C++ class: /Script/ModuleName -> ModuleName
		ModuleName = ClassPackageName.RightChop(ScriptPrefix.Len());
	}
	else
	{
		// Blueprint class: /ModuleName/... -> ModuleName
		const int32 ClassSlashIdx = ClassPackageName.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1);
		if (ClassSlashIdx != INDEX_NONE)
		{
			ModuleName = ClassPackageName.Mid(1, ClassSlashIdx - 1);
		}
	}

	return !PluginName.IsEmpty()
	       && !ModuleName.IsEmpty()
	       && ModuleName.StartsWith(PluginName);
}
