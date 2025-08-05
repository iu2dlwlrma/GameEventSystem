// Copyright LetsGo. All Rights Reserved.

using UnrealBuildTool;

public class GameEventNode : ModuleRules
{
	public GameEventNode(ReadOnlyTargetRules Target) : base(Target)
	{
		
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
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
						"KismetWidgets", 
				}
		);
	}
}
