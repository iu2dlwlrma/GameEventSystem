// Copyright LetsGo. All Rights Reserved.

using UnrealBuildTool;

/**
 * GameEventSystemTest 模块构建配置
 * 该模块是GameEventSystem的自动化测试模块，提供全面的单元测试覆盖
 */
public class GameEventSystemTest : ModuleRules
{
	public GameEventSystemTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.Never;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"GameEventSystem",  // 被测试的模块
				"AutomationTest"   // 自动化测试框架
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"UnrealEd",
				"AutomationController"  // Unreal的自动化测试框架
			});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"EditorStyle",
					"Slate",
					"SlateCore",
					"ToolMenus"
				}
			);
		}
	}
} 