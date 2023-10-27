// Copyright (c) Yevhenii Selivanov.

using UnrealBuildTool;

public class PoolManagerEditor : ModuleRules
{
    public PoolManagerEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Latest;
        bEnableNonInlinedGenCppWarnings = true;

        PublicDependencyModuleNames.AddRange(new[]
            {
                "Core"
                , "BlueprintGraph" // Created UK2Node_TakeFromPool
            }
        );

		PrivateDependencyModuleNames.AddRange(new[]
			{
				"CoreUObject", "Engine", "Slate", "SlateCore" // Core
				, "UnrealEd"
				, "KismetCompiler"
				// My modules
				, "PoolManager"
			}
		);
    }
}