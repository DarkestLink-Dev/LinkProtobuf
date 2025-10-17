// Copyright DarkestLink-Dev 2025 All Rights Reserved.

#include "LinkProtobufEditorFunctionLibrary.h"
#include "LinkProtobufEditor.h"
#include "LinkProtobufEditorSettings.h"
#include "LinkProtobufFunctionLibrary.h"
#include "ProjectDescriptor.h"
#include "Misc/FileHelper.h"
#include "Async/Async.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "ProtoAssetsFunctionLibrary"

TArray<FString> ULinkProtobufEditorFunctionLibrary::AssociatedStructs;
TArray<FString> ULinkProtobufEditorFunctionLibrary::AssociatedEnums;

bool ULinkProtobufEditorFunctionLibrary::GenerateProtoFile(TArray<UScriptStruct*> TargetStructs)
{
	FString ProtoContent;

	AssociatedStructs.Empty();
	AssociatedEnums.Empty();

	GenerateProtoMessageFromUStructArray(TargetStructs, ProtoContent);
	FString ProtoFilePath = GetProtoFilePath();
	bool bSuccess = FFileHelper::SaveStringToFile(ProtoContent, *ProtoFilePath,FFileHelper::EEncodingOptions::ForceUTF8);
	if (!bSuccess)
	{
#if WITH_EDITOR
		FText Title = LOCTEXT("ProtoSaveFailedTitle", "Save .proto Failed");
		FText Msg = LOCTEXT("ProtoSaveFailedContent", "Failed to save the .proto file. Please check that the 'Protobuf Generate Path' in LinkProtobuf settings exists and is writable.");
		FMessageDialog::Open(EAppMsgType::Ok, Msg);
#else
		UE_LOG(LogProtoEditor, Error, TEXT("Failed to save proto file: %s"), *ProtoFilePath);
#endif
	}

	return bSuccess;
}

void ULinkProtobufEditorFunctionLibrary::GenerateProtoMessageFromUStruct(UScriptStruct* TargetStruct, FString& RefProtoMessage)
{
	UE_LOG(LogProtoEditor, Log, TEXT("Generating proto message for struct: %s"), *TargetStruct->GetName());
	RefProtoMessage.Append(FString::Printf(TEXT("message %s {\n"), *TargetStruct->GetName()));
	FString ExtraProtoMessage;
	int32 FieldIndex = 0;
	for (TFieldIterator<FProperty> It(TargetStruct); It; ++It)
	{
		FProperty* Property = *It;
		FieldIndex += 1;
		if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
		{
			FString FieldType = ULinkProtobufFunctionLibrary::ProtoStringAssignProp(ArrayProp->Inner).Replace(TEXT(" "), TEXT(""));

			if (const FStructProperty* StructProp = CastField<FStructProperty>(ArrayProp->Inner))
			{
				if (!AssociatedStructs.Contains(StructProp->Struct->GetName()))
				{
					AssociatedStructs.Add(StructProp->Struct->GetName());
					FString SubMsg;
					UE_LOG(LogProtoEditor, Log, TEXT("Generating proto message for array element struct: %s"), *StructProp->Struct->GetName());
					GenerateProtoMessageFromUStruct(StructProp->Struct, SubMsg);
					ExtraProtoMessage.Append(SubMsg);
				}
				FieldType = StructProp->Struct->GetName();
			}
			else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(ArrayProp->Inner))
			{
				FString EnumName = EnumProp->GetEnum()->GetName();
				FieldType = EnumName;
				if (!AssociatedEnums.Contains(EnumName))
				{
					AssociatedEnums.Add(EnumName);
					UE_LOG(LogProtoEditor, Log, TEXT("Generating proto enum for array element: %s"), *EnumName);
					ExtraProtoMessage.Append(FString::Printf(TEXT("enum %s {\n"), *EnumName));
					for (int32 i = 0; i < EnumProp->GetEnum()->NumEnums(); ++i)
					{
						FString EnumValueName = EnumProp->GetEnum()->GetNameStringByIndex(i);
						if (!EnumValueName.EndsWith(TEXT("_MAX")))
						{
							int64 EnumValue = EnumProp->GetEnum()->GetValueByIndex(i);
							ExtraProtoMessage.Append(FString::Printf(TEXT("  %s = %lld;\n"), *EnumValueName, EnumValue));
						}
					}
					ExtraProtoMessage.Append(TEXT("}\n\n"));
				}
			}

			RefProtoMessage.Append(FString::Printf(TEXT("  repeated %s %s = %d;\n"), *FieldType, *ULinkProtobufFunctionLibrary::GetPureNameOfProperty(ArrayProp), FieldIndex));
		}
		else if (const FSetProperty* SetProp = CastField<FSetProperty>(Property))
		{
			FString FieldType = ULinkProtobufFunctionLibrary::ProtoStringAssignProp(SetProp->ElementProp);
			if (const FStructProperty* StructProp = CastField<FStructProperty>(SetProp->ElementProp))
			{
				if (!AssociatedStructs.Contains(StructProp->Struct->GetName()))
				{
					AssociatedStructs.Add(StructProp->Struct->GetName());
					FString SubMsg;
					UE_LOG(LogProtoEditor, Log, TEXT("Generating proto message for sub-struct: %s"), *StructProp->Struct->GetName());
					GenerateProtoMessageFromUStruct(StructProp->Struct, SubMsg);
					ExtraProtoMessage.Append(SubMsg);
				}
				FieldType = StructProp->Struct->GetName();
			}
			else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(SetProp->ElementProp))
			{
				FString EnumName = EnumProp->GetEnum()->GetName();
				FieldType = EnumName;
				if (!AssociatedEnums.Contains(EnumName))
				{
					AssociatedEnums.Add(EnumName);
					UE_LOG(LogProtoEditor, Log, TEXT("Generating proto enum for set element: %s"), *EnumName);
					ExtraProtoMessage.Append(FString::Printf(TEXT("enum %s {\n"), *EnumName));
					for (int32 i = 0; i < EnumProp->GetEnum()->NumEnums(); ++i)
					{
						FString EnumValueName = EnumProp->GetEnum()->GetNameStringByIndex(i);
						if (!EnumValueName.EndsWith(TEXT("_MAX")))
						{
							int64 EnumValue = EnumProp->GetEnum()->GetValueByIndex(i);
							ExtraProtoMessage.Append(FString::Printf(TEXT("  %s = %lld;\n"), *EnumValueName, EnumValue));
						}
					}
					ExtraProtoMessage.Append(TEXT("}\n\n"));
				}
			}
			RefProtoMessage.Append(FString::Printf(TEXT("  repeated %s %s = %d;\n"), *FieldType, *ULinkProtobufFunctionLibrary::GetPureNameOfProperty(Property), FieldIndex));
		}
		else if (const FMapProperty* MapProp = CastField<FMapProperty>(Property))
		{
			FString KeyType = ULinkProtobufFunctionLibrary::ProtoStringAssignProp(MapProp->KeyProp);
			FString ValueType = ULinkProtobufFunctionLibrary::ProtoStringAssignProp(MapProp->ValueProp);

			// Handle struct keys
			if (const FStructProperty* StructProp = CastField<FStructProperty>(MapProp->KeyProp))
			{
				if (!AssociatedStructs.Contains(StructProp->Struct->GetName()))
				{
					AssociatedStructs.Add(StructProp->Struct->GetName());
					FString SubMsg;
					UE_LOG(LogProtoEditor, Log, TEXT("Generating proto message for sub-struct: %s"), *StructProp->Struct->GetName());
					GenerateProtoMessageFromUStruct(StructProp->Struct, SubMsg);
					ExtraProtoMessage.Append(SubMsg);
				}
				KeyType = StructProp->Struct->GetName();
			}
			// Handle enum keys
			else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(MapProp->KeyProp))
			{
				FString EnumName = EnumProp->GetEnum()->GetName();
				KeyType = EnumName;
				if (!AssociatedEnums.Contains(EnumName))
				{
					AssociatedEnums.Add(EnumName);
					UE_LOG(LogProtoEditor, Log, TEXT("Generating proto enum: %s"), *EnumName);
					ExtraProtoMessage.Append(FString::Printf(TEXT("enum %s {\n"), *EnumName));
					for (int32 i = 0; i < EnumProp->GetEnum()->NumEnums(); ++i)
					{
						FString EnumValueName = EnumProp->GetEnum()->GetNameStringByIndex(i);
						if (!EnumValueName.EndsWith(TEXT("_MAX")))
						{
							int64 EnumValue = EnumProp->GetEnum()->GetValueByIndex(i);
							ExtraProtoMessage.Append(FString::Printf(TEXT("  %s = %lld;\n"), *EnumValueName, EnumValue));
						}
					}
					ExtraProtoMessage.Append(TEXT("}\n\n"));
				}
			}

			// Handle struct values
			if (const FStructProperty* StructProp = CastField<FStructProperty>(MapProp->ValueProp))
			{
				if (!AssociatedStructs.Contains(StructProp->Struct->GetName()))
				{
					AssociatedStructs.Add(StructProp->Struct->GetName());
					FString SubMsg;
					UE_LOG(LogProtoEditor, Log, TEXT("Generating proto message for sub-struct: %s"), *StructProp->Struct->GetName());
					GenerateProtoMessageFromUStruct(StructProp->Struct, SubMsg);
					ExtraProtoMessage.Append(SubMsg);
				}
				ValueType = StructProp->Struct->GetName();
			}
			// Handle enum values
			else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(MapProp->ValueProp))
			{
				FString EnumName = EnumProp->GetEnum()->GetName();
				ValueType = EnumName;
				if (!AssociatedEnums.Contains(EnumName))
				{
					AssociatedEnums.Add(EnumName);
					UE_LOG(LogProtoEditor, Log, TEXT("Generating proto enum: %s"), *EnumName);
					ExtraProtoMessage.Append(FString::Printf(TEXT("enum %s {\n"), *EnumName));
					for (int32 i = 0; i < EnumProp->GetEnum()->NumEnums(); ++i)
					{
						FString EnumValueName = EnumProp->GetEnum()->GetNameStringByIndex(i);
						if (!EnumValueName.EndsWith(TEXT("_MAX")))
						{
							int64 EnumValue = EnumProp->GetEnum()->GetValueByIndex(i);
							ExtraProtoMessage.Append(FString::Printf(TEXT("  %s = %lld;\n"), *EnumValueName, EnumValue));
						}
					}
					ExtraProtoMessage.Append(TEXT("}\n\n"));
				}
			}

			RefProtoMessage.Append(FString::Printf(TEXT("  map<%s,%s> %s = %d;\n"), *KeyType, *ValueType, *MapProp->GetDisplayNameText().ToString().Replace(TEXT(" "), TEXT("")), FieldIndex));
		}
		else if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			FString StructName = StructProp->Struct->GetName();
			RefProtoMessage.Append(FString::Printf(TEXT("  %s %s = %d;\n"), *StructName, *StructProp->GetDisplayNameText().ToString().Replace(TEXT(" "), TEXT("")), FieldIndex));
			if (!AssociatedStructs.Contains(StructName))
			{
				AssociatedStructs.Add(StructName);
				FString SubMsg;
				UE_LOG(LogProtoEditor, Log, TEXT("Generating proto message for sub-struct: %s"), *StructName);
				GenerateProtoMessageFromUStruct(StructProp->Struct, SubMsg);
				ExtraProtoMessage.Append(SubMsg);
			}
		}
		else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
		{
			FString EnumName = EnumProp->GetEnum()->GetName();
			RefProtoMessage.Append(FString::Printf(TEXT("  %s %s = %d;\n"), *EnumName, *EnumProp->GetDisplayNameText().ToString().Replace(TEXT(" "), TEXT("")), FieldIndex));
			if (!AssociatedEnums.Contains(EnumName))
			{
				AssociatedEnums.Add(EnumName);
				UE_LOG(LogProtoEditor, Log, TEXT("Generating proto enum: %s"), *EnumName);
				ExtraProtoMessage.Append(FString::Printf(TEXT("enum %s {\n"), *EnumName));
				for (int32 i = 0; i < EnumProp->GetEnum()->NumEnums(); ++i)
				{
					FString EnumValueName = EnumProp->GetEnum()->GetNameStringByIndex(i);
					if (!EnumValueName.EndsWith(TEXT("_MAX")))
					{
						int64 EnumValue = EnumProp->GetEnum()->GetValueByIndex(i);
						ExtraProtoMessage.Append(FString::Printf(TEXT("  %s = %lld;\n"), *EnumValueName, EnumValue));
					}
				}
				ExtraProtoMessage.Append(TEXT("}\n\n"));
			}
		}
		else
		{
			FString FieldType = ULinkProtobufFunctionLibrary::ProtoStringAssignProp(Property);
			if (FieldType != TEXT("unknown"))
			{
				RefProtoMessage.Append(FString::Printf(TEXT("  %s %s = %d;\n"), *FieldType, *ULinkProtobufFunctionLibrary::GetPureNameOfProperty(Property), FieldIndex));
			}
		}
	}
	RefProtoMessage.Append(TEXT("}\n\n"));
	if (!ExtraProtoMessage.IsEmpty())
	{
		RefProtoMessage.Append(ExtraProtoMessage);
	}
}


void ULinkProtobufEditorFunctionLibrary::GenerateProtoMessageFromUStructArray(TArray<UScriptStruct*> TargetStructs, FString& MainProtoMessage)
{
	if (TargetStructs.Num() == 0) return;

	MainProtoMessage.Append(TEXT("syntax = \"proto3\";\n\n"));
	FString ExtraProtoMessage;
	for (int32 i = 0; i < TargetStructs.Num(); ++i)
	{
		FString StructMsg;
		if (!AssociatedStructs.Contains(TargetStructs[i]->GetName()))
		{
			AssociatedStructs.Add(TargetStructs[i]->GetName());
			GenerateProtoMessageFromUStruct(TargetStructs[i], StructMsg);
			ExtraProtoMessage.Append(StructMsg);
		}
	}
	if (!ExtraProtoMessage.IsEmpty())
	{
		MainProtoMessage.Append(ExtraProtoMessage);
	}
}

void ULinkProtobufEditorFunctionLibrary::GenerateProtoCppFile()
{
    FString PlatformBash;
    FString CommandPrefix;

#if PLATFORM_WINDOWS
    PlatformBash = FPlatformMisc::GetEnvironmentVariable(TEXT("ComSpec"));
    CommandPrefix = TEXT("/C");
#elif PLATFORM_LINUX || PLATFORM_MAC
    // Planned to support in the future
    PlatformBash = TEXT("/bin/bash");
    CommandPrefix = TEXT("-c");
#else
#if WITH_EDITOR
    FText Msg = LOCTEXT("UnsupportedPlatformContent", "The current platform is not supported for protoc execution.");
    FMessageDialog::Open(EAppMsgType::Ok, Msg);
#else
    UE_LOG(LogProtoEditor, Error, TEXT("Unsupported platform for protoc execution."));
#endif
    return;
#endif

    // Verify the shell executable exists
    if (PlatformBash.IsEmpty() || !FPaths::FileExists(PlatformBash))
    {
#if WITH_EDITOR
        FText Msg = FText::Format(LOCTEXT("ShellNotFoundContent", "The shell executable was not found at the expected path: {0}. Please check your system command."), FText::FromString(PlatformBash));
        FMessageDialog::Open(EAppMsgType::Ok, Msg);
#else
        UE_LOG(LogProtoEditor, Error, TEXT("Shell not found at: %s"), *PlatformBash);
#endif
        return;
    }

    FString ProtocPath = ULinkProtobufEditorSettings::Get()->ProtocExecutePath;
    // Verify the protoc executable exists
    if (ProtocPath.IsEmpty() || !FPaths::FileExists(ProtocPath))
    {
#if WITH_EDITOR
        FText Msg = FText::Format(LOCTEXT("ProtocNotFoundContent", "The protoc executable was not found at the expected path: {0}. Please reinstall or configure the plugin."), FText::FromString(ProtocPath));
        FMessageDialog::Open(EAppMsgType::Ok, Msg);
#else
        UE_LOG(LogProtoEditor, Error, TEXT("Protoc executable not found at: %s"), *ProtocPath);
#endif
        return;
    }

    FString ProtoFilePath = GetProtoFilePath();
    FString ProtocCommand = FString::Printf(TEXT("%s protoc --proto_path=\"%s\" --cpp_out=\"%s\" \"%s\""),
        *CommandPrefix,
        *ULinkProtobufEditorSettings::Get()->GetDefaultProtobufGenPath(),
        *ULinkProtobufEditorSettings::Get()->GetDefaultProtobufGenPath(),
        *ProtoFilePath);

    FString CapturedPlatformBash = PlatformBash;
    FString CapturedProtocCommand = ProtocCommand;
    FString CapturedWorkingDir = FPaths::GetPath(ProtocPath);

    TFuture<void> Task=Async(EAsyncExecution::ThreadPool, [CapturedPlatformBash, CapturedProtocCommand, CapturedWorkingDir]() {
        int32 ReturnCode = -1;
        FString StdOut;
        FString StdErr;
        bool bSuccessLocal = FPlatformProcess::ExecProcess(*CapturedPlatformBash, *CapturedProtocCommand, &ReturnCode, &StdOut, &StdErr, *CapturedWorkingDir,true);

        AsyncTask(ENamedThreads::GameThread, [bSuccessLocal, ReturnCode, StdOut, StdErr, CapturedProtocCommand]() {
            UE_LOG(LogProtoEditor, Display, TEXT("Generate Protocpp (async): %s, Command: %s"), bSuccessLocal ? TEXT("Success") : TEXT("Failed"), *CapturedProtocCommand);
            UE_LOG(LogProtoEditor, Display, TEXT("Return code: %d"), ReturnCode);
            UE_LOG(LogProtoEditor, Display, TEXT("Command output: %s"), *StdOut);
            if (!StdErr.IsEmpty())
            {
                UE_LOG(LogProtoEditor, Warning, TEXT("Generate Protocpp Failed! Error output:\n%s"), *StdErr);
            }
#if WITH_EDITOR
        	const FText Msg = bSuccessLocal
			? FText::FromString(TEXT("ProtoCpp Generate Success. Please recompile your project from IDE."))
			: FText::FromString(TEXT("ProtoCpp Generate Failed. Check the logs for details."));
            FNotificationInfo Info(Msg);
			Info.ExpireDuration = 10.0f;
			Info.bFireAndForget = true;
			FSlateNotificationManager::Get().AddNotification(Info);
        	RebuildThisPlugin();
#endif
        	return;
        });
    });

}

FString ULinkProtobufEditorFunctionLibrary::GetProtoFilePath()
{
	return FPaths::Combine
	(
		ULinkProtobufEditorSettings::Get()->GetDefaultProtobufGenPath(),
		ULinkProtobufEditorSettings::Get()->ProtoFileName + TEXT(".proto")
	);
}

bool ULinkProtobufEditorFunctionLibrary::GenerateProtoDataFromUStruct()
{
	UserProtoData.ProtoData.Empty();
	if (ProtoBoundStructs.Num() == 0)
	{
		return false;
	}
	for (auto& ScriptStruct : ProtoBoundStructs)
	{
		for (TFieldIterator<FProperty> It(ScriptStruct); It; ++It)
		{
			FProperty* Prop = *It;

			FProtoFieldKey Key;
			Key.ValueProperty = Prop;

			Key.FieldName = Prop->GetDisplayNameText().ToString();
			Key.UIDName = Prop->GetName();

			if (Prop->IsA<FArrayProperty>())
			{
				Key.bRepeated = true;
				Key.UnrealType = EUnrealType::Array;
			}
			else if (Prop->IsA<FMapProperty>())
			{
				Key.UnrealType = EUnrealType::Map;
			}
			else if (Prop->IsA<FStructProperty>())
			{
				Key.UnrealType = EUnrealType::Struct;
			}
			else if (Prop->IsA<FSetProperty>())
			{
				Key.UnrealType = EUnrealType::Set;
			}
			else
			{
				Key.UnrealType = EUnrealType::Normal;
			}

			UserProtoData.ProtoData.Add(Key);
		}
	}

	return GenerateProtoFile(ProtoBoundStructs);
}

bool ULinkProtobufEditorFunctionLibrary::CheckCppProject()
{
	FProjectDescriptor Desc;
	FText Fail;
	const FString ProjFile = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	if (!ProjFile.IsEmpty() && Desc.Load(ProjFile, Fail))
	{
		if (Desc.Modules.Num() > 0)
		{
			UE_LOG(LogProtoEditor, Log, TEXT("Project %s is a C++ project."), *Desc.Description);
			return true;
		}
	}
	const FText Title = LOCTEXT("CheckCppProjectTitle", "Check C++ Project Failed");
	const FText Msg = FText::Format(LOCTEXT("CheckCppProjectContent", "Failed to load the project descriptor from {0}. Please ensure your project is a C++ project."), FText::FromString(ProjFile));
#if WITH_EDITOR
	FMessageDialog::Open(EAppMsgType::Ok, Msg);
#endif
	UE_LOG(LogProtoEditor, Error, TEXT("Check C++ Project Failed."));
	return false;
}

bool ULinkProtobufEditorFunctionLibrary::RebuildThisPlugin()
{
	FString EngineRoot = FPaths::EngineDir();
	FString ProjectFile = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	FString PluginName = TEXT("LinkProtobuf");

#if PLATFORM_WINDOWS
	FString UBTPath = FPaths::Combine(EngineRoot, TEXT("Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe"));
#else
	FString UBTPath = FPaths::Combine(EngineRoot, TEXT("Binaries/DotNET/UnrealBuildTool/UnrealBuildTool"));
#endif

	// 命令参数
	FString Args = FString::Printf(
		TEXT("\"%s\" Development Editor -Project=\"%s\" -Module=%s -TargetType=Editor -Progress -NoHotReloadFromIDE"),
		&*FApp::GetProjectName(),
		*ProjectFile,
		*PluginName
	);

	UE_LOG(LogTemp, Log, TEXT("Rebuilding plugin via UBT: %s %s"), *UBTPath, *Args);

	int32 ReturnCode = -1;
	void* PipeRead = nullptr;
	void* PipeWrite = nullptr;
	FPlatformProcess::CreatePipe(PipeRead, PipeWrite);

	FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		*UBTPath,
		*Args,
		true, false, false,
		nullptr, 0,
		nullptr, PipeWrite
	);

	if (!ProcHandle.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to launch UnrealBuildTool!"));
		FPlatformProcess::ClosePipe(PipeRead, PipeWrite);
		return false;
	}

	// 等待编译完成
	FString Output;
	while (FPlatformProcess::IsProcRunning(ProcHandle))
	{
		FString NewOutput = FPlatformProcess::ReadPipe(PipeRead);
		if (!NewOutput.IsEmpty())
		{
			Output += NewOutput;
		}
		FPlatformProcess::Sleep(0.1f);
	}

	FPlatformProcess::GetProcReturnCode(ProcHandle, &ReturnCode);
	FPlatformProcess::CloseProc(ProcHandle);
	FPlatformProcess::ClosePipe(PipeRead, PipeWrite);

	UE_LOG(LogTemp, Log, TEXT("UBT Finished with code %d"), ReturnCode);
	UE_LOG(LogTemp, Log, TEXT("%s"), *Output);

	return ReturnCode == 0;
}

#undef LOCTEXT_NAMESPACE
