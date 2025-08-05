#region

using UnrealBuildTool;

#endregion
public class GameEventNode : ModuleRules
{
	public GameEventNode(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.Never;


		PrivateDependencyModuleNames.AddRange(
				new string[] {
						"Core",
						"CoreUObject",
						"Engine",
						"KismetCompiler",
						"PropertyEditor",
						"GameEventSystem",
						"UnrealEd",
						"Slate",
						"SlateCore",
						"ToolMenus",
						"InputCore",
						"EditorStyle",
						"GraphEditor",
						"EditorWidgets",
						"GameplayTags"
				}
		);

		PublicDependencyModuleNames.AddRange(
				new string[] {
						"BlueprintGraph",
						"KismetWidgets"
				}
		);
	}
}
