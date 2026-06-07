using UnrealBuildTool;

public class SQLUISamples : ModuleRules
{
	public SQLUISamples(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",
			"SQLUICore",
			"SQLUIWidgets"
		});
	}
}
