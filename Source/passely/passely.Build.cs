using UnrealBuildTool;

public class passely : ModuleRules
{
    public passely(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "DeveloperSettings",
                "EnhancedInput",
                "Engine",
                "GameplayTags",
                "InputCore"
            }
        );
    }
}
