// Copyright 2025 DarkestLink-Dev

using System.IO;
using UnrealBuildTool;

public class LinkProtobufEditor : ModuleRules
{
	public LinkProtobufEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.AddRange(
			new string[]
			{
                // Make protobuf headers visible to the editor module that compiles .pb.cc if any
                Path.Combine(ModuleDirectory, "../LinkProtobufRuntime/include"),
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				// Add other private include paths required here
			}
		);
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

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{

			}
		);

	}
}