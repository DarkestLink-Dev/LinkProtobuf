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
		bUseRTTI = true;
		PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
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
#elif UE_4_24_OR_LATER
		ShadowVariableWarningLevel = WarningLevel.Off;
#else
		bEnableShadowVariableWarnings = false;
#endif
#if UE_5_5_OR_LATER
        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;
#else
		bEnableUndefinedIdentifierWarnings = false;
#endif
        string ThirdPartyDir = Path.Combine(PluginDirectory, "Source","ThirdParty");
		string ProjectFilePath = Target.ProjectFile.ToString();
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
	        if (RuntimeInformation.ProcessArchitecture == Architecture.X64)
	        {
				string Win64Protolib = Path.Combine(ThirdPartyDir, "Win64/lib");
				PublicAdditionalLibraries.AddRange(new string[]
				{
					Path.Combine(Win64Protolib, "libprotobuf.lib"),
				});
            }

            if (RuntimeInformation.ProcessArchitecture == Architecture.Arm64)
            {
                string WinArm64Protolib = Path.Combine(ThirdPartyDir, "WinArm64/lib");
                PublicAdditionalLibraries.AddRange(new string[]
                {
                    Path.Combine(WinArm64Protolib, "libprotobuf.lib"),
                });
            }
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
