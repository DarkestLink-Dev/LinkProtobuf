// Copyright 2025 DarkestLink-Dev

#include "LinkProtobufEditorSettingsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "LinkProtobufEditorSettings.h"
#include "LinkProtobufEditorFunctionLibrary.h"
#include "Engine/UserDefinedStruct.h"
#include "Misc/MessageDialog.h"
#if ENGINE_MAJOR_VERSION >= 5&& ENGINE_MINOR_VERSION >= 5
#include "StructUtils/UserDefinedStruct.h"
#endif

#define LOCTEXT_NAMESPACE "LinkProtobufEditorSettings"
TSharedRef<IDetailCustomization> FLinkProtobufEditorSettingsCustomization::MakeInstance()
{
	return MakeShareable(new FLinkProtobufEditorSettingsCustomization);
}

void FLinkProtobufEditorSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() > 0)
	{
		EditingObjPtr = Objects[0];
	}

	IDetailCategoryBuilder& GenerateCategory = DetailBuilder.EditCategory(TEXT("Link Protobuf Generate"));
	GenerateCategory.AddCustomRow(FText::FromString(TEXT("Generate From Settings")))
	.WholeRowContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SButton)
			.ContentPadding(FMargin(8.0f, 4.0f))
			.OnClicked(FOnClicked::CreateSP(this, &FLinkProtobufEditorSettingsCustomization::OnButtonGenerateProtoFromSettings))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Generate ProtoFiles From Settings")))
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
			]
		]
	];
	//ProtobufGeneratePath is Temporarily suspended
	IDetailCategoryBuilder& ProtoFilePathCategory = DetailBuilder.EditCategory(TEXT("Link Protobuf Environment"));
	TSharedPtr<IPropertyHandle> ProtobufGeneratePathHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULinkProtobufEditorSettings, ProtobufGeneratePath));
	IDetailPropertyRow& ProtoFilePathRow = ProtoFilePathCategory.AddProperty(ProtobufGeneratePathHandle);
	ProtoFilePathRow.CustomWidget()
	.NameContent()
	[
		ProtobufGeneratePathHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(300.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			ProtobufGeneratePathHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(15, 0)
		[
			SNew(SButton)
			.Text(FText::FromString("Reset to Default"))
			.OnClicked(FOnClicked::CreateSP(this, &FLinkProtobufEditorSettingsCustomization::OnResetProtobufGenPathClicked))
		]
	];

	IDetailCategoryBuilder& ProtocExecPathCategory = DetailBuilder.EditCategory(TEXT("Link Protobuf Environment"));
	TSharedPtr<IPropertyHandle> ProtocExecPathHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULinkProtobufEditorSettings, ProtocExecutePath));
	IDetailPropertyRow& ProtocExecPathRow = ProtocExecPathCategory.AddProperty(ProtocExecPathHandle);
	ProtocExecPathRow.CustomWidget()
	.NameContent()
	[
		ProtocExecPathHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(300.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			ProtocExecPathHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(10, 0)
		[
			SNew(SButton)
			.Text(FText::FromString("Reset to Default"))
			.OnClicked(FOnClicked::CreateSP(this, &FLinkProtobufEditorSettingsCustomization::OnResetProtocExecPathClicked))
		]
	];
}

FReply FLinkProtobufEditorSettingsCustomization::OnResetProtobufGenPathClicked()
{
	ULinkProtobufEditorSettings* Settings = GetMutableDefault<ULinkProtobufEditorSettings>();
	Settings->ProtobufGeneratePath= Settings->GetDefaultProtobufGenPath();
	Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());
	GConfig->Flush(false, Settings->GetClass()->GetConfigName());
	return FReply::Handled();
}

FReply FLinkProtobufEditorSettingsCustomization::OnResetProtocExecPathClicked()
{
	ULinkProtobufEditorSettings* Settings = GetMutableDefault<ULinkProtobufEditorSettings>();
	Settings->ProtocExecutePath= Settings->GetDefaultProtocExecPath();
	Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());
	GConfig->Flush(false, Settings->GetClass()->GetConfigName());
	return FReply::Handled();
}

FReply FLinkProtobufEditorSettingsCustomization::OnButtonGenerateProtoFromSettings()
{
	//Make sure it's a C++ Project
	if (!ULinkProtobufEditorFunctionLibrary::CheckCppProject())
	{
		return FReply::Handled();
	}
	TArray<UScriptStruct*> StructsToProcess;
	const ULinkProtobufEditorSettings* Settings = ULinkProtobufEditorSettings::Get();
	const TArray<TSoftObjectPtr<UUserDefinedStruct>>& SettingStructs = Settings->UserDefinedStructsForProtobuf;
	TArray<FString>MissingStructs;
	for (const TSoftObjectPtr<UUserDefinedStruct>& StructPtr : SettingStructs)
	{
		if (StructPtr.IsValid())
		{
			StructsToProcess.Add(StructPtr.Get());
		}
		else
		{
			MissingStructs.Add(StructPtr.GetAssetName());
		}
	}
	//Editor Warning
	if (MissingStructs.Num() > 0)
	{
		FString MissingStr = FString::Join(MissingStructs, TEXT("\n"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Some UserDefinedStructs are missing or invalid:\n%s \nPlease Rechoose them!"), *MissingStr)));
		return FReply::Handled();
	}
	//Make sure Plugin is in Project path.
	FString EnginePath=FPaths::EngineDir();
	if (Settings->GetDefaultProtobufGenPath().Contains(EnginePath))
	{
		const FText Title = LOCTEXT("Generate Path Error", "Generate Path Error");
		const FText Msg = LOCTEXT("Plugin Must Be In Project.Please Copy it to your Project Plugins Folder", "The Protobuf Generate Files must be in your project directory that can be Complied. Please copy the plugin to your project plugins folder and Restart from your IDE");
		FMessageDialog::Open(EAppMsgType::Ok, Msg);
		return FReply::Handled();
	}
	ULinkProtobufEditorFunctionLibrary::GenerateProtoFile(StructsToProcess);
	ULinkProtobufEditorFunctionLibrary::GenerateProtoCppFile();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE