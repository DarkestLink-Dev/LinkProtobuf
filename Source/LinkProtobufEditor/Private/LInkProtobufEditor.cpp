// Copyright DarkestLink-Dev 2025 All Rights Reserved.

#include "LinkProtobufEditor.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ISettingsModule.h"
#include "LinkProtobufEditorFunctionLibrary.h"
#include "LinkProtobufEditorSettings.h"
#include "LinkProtobufEditorSettingsCustomization.h"

DEFINE_LOG_CATEGORY(LogProtoEditor);
#define LOCTEXT_NAMESPACE "FLinkProtobufEditorModule"

void FLinkProtobufEditorModule::StartupModule()
{

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "LinkProtobuf",
			LOCTEXT("RuntimeSettingsName", "LinkProtobuf"),
			LOCTEXT("RuntimeSettingsDescription", "Configure LinkProtobuf Plugin"),
			GetMutableDefault<ULinkProtobufEditorSettings>());
	}
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomClassLayout(
			"LinkProtobufEditorSettings",
			FOnGetDetailCustomizationInstance::CreateStatic(&FLinkProtobufEditorSettingsCustomization::MakeInstance)
		);
}

void FLinkProtobufEditorModule::ShutdownModule()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "LinkProtobuf");
	}
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout("LinkProtobufEditorSettings");
	}
}
IMPLEMENT_MODULE(FLinkProtobufEditorModule, LinkProtobufEditor)
#undef LOCTEXT_NAMESPACE
