// Copyright DarkestLink-Dev 2025 All Rights Reserved.

#include "LinkProtobufEditorSettings.h"

#include "LinkProtobufEditor.h"
#include "Interfaces/IPluginManager.h"

ULinkProtobufEditorSettings::ULinkProtobufEditorSettings()
{
	ProtobufGeneratePath = GetDefaultProtobufGenPath();
	ProtocExecutePath= GetDefaultProtocExecPath();
}

FString ULinkProtobufEditorSettings::GetDefaultProtobufGenPath() const
{
	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("LinkProtobuf")))
	{
		FString PluginDir = Plugin->GetBaseDir();
		FString ProjectFilePath = FPaths::GetProjectFilePath();
		if (!ProjectFilePath.IsEmpty())
		{
			FString ProjectName = FPaths::GetBaseFilename(ProjectFilePath);
			return FPaths::Combine(PluginDir, TEXT("Source"), TEXT("ProtoSource"), ProjectName);
		}
	}
	return FString();
}

FString ULinkProtobufEditorSettings::GetDefaultProtocExecPath() const
{
	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("LinkProtobuf")))
	{
		FString PluginDir = Plugin->GetBaseDir();
		FString ProtoThirdPartyDir = FPaths::Combine(PluginDir, TEXT("Source"), TEXT("ThirdParty"));
		// Set the ProtocExecutePath based on the platform
		if (PLATFORM_WINDOWS)
		{
			return FPaths::Combine(ProtoThirdPartyDir, TEXT("Win64/protoc"));
		}
		else if (PLATFORM_LINUX)
		{
			return FPaths::Combine(ProtoThirdPartyDir, TEXT("Linux/protoc"));
		}
		else if (PLATFORM_MAC)
		{
			return FPaths::Combine(ProtoThirdPartyDir, TEXT("Mac/protoc"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("LinkProtobufEditorSettings: Unsupported platform!"));
			return FString();
		}
	}
	else
	{
		UE_LOG(LogProtoEditor, Error, TEXT("LinkProtobufEditorSettings: LinkProtobuf plugin not found!"));
	}
	return FString();
}

