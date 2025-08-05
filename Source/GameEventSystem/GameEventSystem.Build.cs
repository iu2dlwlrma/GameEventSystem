#region

using UnrealBuildTool;

#endregion
public class GameEventSystem : ModuleRules
{
	public GameEventSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.Never;

		PublicDependencyModuleNames.AddRange(
				new string[] {
						"Core",
						"Engine",
						"GameplayTags",
						"DeveloperSettings"
				});

		PrivateDependencyModuleNames.AddRange(
				new string[] {
						"CoreUObject"
				});

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(
					new string[] {
							"BlueprintGraph"
					}
			);

			PrivateDependencyModuleNames.AddRange(
					new string[] {
							"UnrealEd",
							"EditorFramework",
							"KismetCompiler",
							"EditorStyle",
							"Slate",
							"SlateCore",
							"Settings"
					}
			);
		}
	}
}
