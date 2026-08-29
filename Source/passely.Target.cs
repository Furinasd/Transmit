using UnrealBuildTool;
using System.Collections.Generic;

public class passelyTarget : TargetRules
{
    public passelyTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("passely");
    }
}
