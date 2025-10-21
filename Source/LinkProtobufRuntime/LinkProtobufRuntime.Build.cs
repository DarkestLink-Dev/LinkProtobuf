// Copyright DarkestLink-Dev 2025 All Rights Reserved.

using System;
using System.IO;
using System.Runtime.InteropServices;
using UnrealBuildTool;

public class LinkProtobufRuntime : ModuleRules
{
	public LinkProtobufRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Json",
				"JsonUtilities",
			}
		);
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
			}
		);
		#if UE_5_6_OR_LATER
        CppCompileWarningSettings.ShadowVariableWarningLevel = WarningLevel.Off;
        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;
		#elif UE_5_5_OR_LATER
		ShadowVariableWarningLevel = WarningLevel.Off;
		UndefinedIdentifierWarningLevel = WarningLevel.Off;
		#elif  UE_5_4_OR_LATER
		ShadowVariableWarningLevel = WarningLevel.Off;
		bEnableUndefinedIdentifierWarnings = false;
		#else
		bEnableShadowVariableWarnings = false;
		bEnableUndefinedIdentifierWarnings = false;
		#endif

        string ThirdPartyDir = Path.Combine(PluginDirectory, "Source","ThirdParty");
		string ProtoSourceDir = Path.Combine(ModuleDirectory, "Public", "ProtoSource");
		PublicIncludePaths.Add(ProtoSourceDir);
		if (!Directory.Exists(ProtoSourceDir))
		{
			Directory.CreateDirectory(ProtoSourceDir);
		}
        PublicSystemIncludePaths.AddRange(new string[]
        {
            //Protobuf header files
            Path.Combine(ThirdPartyDir,"include"),
            //Generated Proto files
            ProtoSourceDir,
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
	        string Win64Protolib = Path.Combine(ThirdPartyDir, "Win64/lib");
	        string WinArm64Protolib = Path.Combine(ThirdPartyDir, "WinArm64/lib");

			#if UE_5_3_OR_LATER
	        if (Target.Architecture == UnrealArch.X64)
	        {
		        PublicAdditionalLibraries.AddRange(new string[]
		        {
			        Path.Combine(Win64Protolib, "libprotobuf.lib"),
		        });
	        }
            if (Target.Architecture == UnrealArch.Arm64)
            {
                PublicAdditionalLibraries.AddRange(new string[]
                {
                    Path.Combine(WinArm64Protolib, "libprotobuf.lib"),
                });
            }
            #else
			PublicAdditionalLibraries.AddRange(new string[]
			{
				Path.Combine(Win64Protolib, "libprotobuf.lib"),
			});
            #endif
        }

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            var ABI = new[] { "armeabi-v7a", "arm64-v8a", "x86_64" };
            foreach (var abi in ABI)
            {
                string AndroidProtolib = Path.Combine(ThirdPartyDir, "Android", abi, "lib");
                PublicAdditionalLibraries.AddRange(new string[]
                {
                    Path.Combine(AndroidProtolib, "libprotobuf.a"),
                });
            }
        }

        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string LinuxProtolib = Path.Combine(ThirdPartyDir, "Linux", "lib");
            PublicAdditionalLibraries.AddRange(new string[]
            {
                Path.Combine(LinuxProtolib, "libprotobuf.a"),
            });
        }

#if UE_5_0_OR_LATER
        if (Target.Platform == UnrealTargetPlatform.LinuxArm64)
#else
        if (Target.Platform == UnrealTargetPlatform.LinuxAArch64)
#endif
        {
            string LinuxAArchProtolib = Path.Combine(ThirdPartyDir, "LinuxAArch64", "lib");
            PublicAdditionalLibraries.AddRange(new string[]
            {
                Path.Combine(LinuxAArchProtolib, "libprotobuf.a"),
            });
        }

	}
}
