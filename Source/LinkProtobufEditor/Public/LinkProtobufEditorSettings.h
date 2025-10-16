// Copyright 2025 DarkestLink-Dev

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LinkProtobufEditorSettings.generated.h"

class UUserDefinedStruct;

UCLASS(config = EditorPerProjectUserSettings, defaultconfig)
class LINKPROTOBUFEDITOR_API ULinkProtobufEditorSettings : public UObject
{
	GENERATED_BODY()
	ULinkProtobufEditorSettings();

public:
	FString GetDefaultProtobufGenPath() const;
	FString GetDefaultProtocExecPath() const;
	static const ULinkProtobufEditorSettings*Get() { return GetDefault<ULinkProtobufEditorSettings>(); }
	UPROPERTY(Config, EditAnywhere, Category = "Link Protobuf Environment", meta=(ToolTip="Do not modify this value unless you are understand. Changing it may cause complie error."))
	FString ProtoFileName=TEXT("User");
	/*
	 *Temporarily suspended
	 *Must be in the ModuleDir that can be compiled with protobuf reflection, if set to other path only packaged version could find proto file descriptor Unreal Editor will not recognize.
	 */
	FString ProtobufGeneratePath;
	UPROPERTY(Config, EditAnywhere, Category = "Link Protobuf Environment", meta=(ToolTip="Do not modify this value unless you are understand. Changing it may prevent protoc from being found or executed."))
	FString ProtocExecutePath;
	//Add Your DefinedStruct here to generate .proto file
	UPROPERTY(config, EditAnywhere, Category = "Link Protobuf User Settings", meta=(AllowedClasses="/Script/Engine.UserDefinedStruct"))
	TArray<TSoftObjectPtr<UUserDefinedStruct>> UserDefinedStructsForProtobuf;
};
