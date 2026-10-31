using UnrealBuildTool;
using System.Collections.Generic;

public class Asgn3Target : TargetRules
{
	public Asgn3Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "Asgn3" } );
	}
}
