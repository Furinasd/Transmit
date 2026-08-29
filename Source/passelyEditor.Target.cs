using UnrealBuildTool;
using System.Collections.Generic;

public class passelyEditorTarget : TargetRules
{
    public passelyEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("passely");
    }
}
