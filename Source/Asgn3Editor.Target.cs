using UnrealBuildTool;
using System.Collections.Generic;

public class Asgn3EditorTarget : TargetRules
{
	public Asgn3EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "Asgn3" } );
	}
}
