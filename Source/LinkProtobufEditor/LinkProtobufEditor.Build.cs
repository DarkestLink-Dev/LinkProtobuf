// Copyright DarkestLink-Dev 2025 All Rights Reserved. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class LinkProtobufEditor : ModuleRules
{
	public LinkProtobufEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Projects",
				"AssetTools",
				"LinkProtobufRuntime",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"ToolMenus",
				"EditorStyle",
				"Slate",
				"SlateCore",
				"UnrealEd",
			}
		);

	}
}