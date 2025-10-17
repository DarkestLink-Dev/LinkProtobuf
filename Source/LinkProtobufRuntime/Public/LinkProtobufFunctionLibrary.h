// Copyright DarkestLink-Dev 2025 All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "google/protobuf/message.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LinkProtobufRuntime.h"
#include "LinkProtobufFunctionLibrary.generated.h"


UCLASS()
class LINKPROTOBUFRUNTIME_API ULinkProtobufFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Proto", meta = (DisplayName = "Convert Struct To Proto Binary  String", CustomStructureParam = "Struct", AutoCreateRefTerm = "Struct"))
	static UPARAM(DisplayName = "Success") bool StructToBinaryProtoString(const int32& Struct, FString& OutProtoBinaryString);
	DECLARE_FUNCTION(execStructToBinaryProtoString);

	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Proto", meta = (DisplayName = "Convert Struct To Proto Binary  Bytes", CustomStructureParam = "Struct", AutoCreateRefTerm = "Struct"))
	static UPARAM(DisplayName = "Success") bool StructToBinaryProtoBytes(const int32& Struct, TArray<uint8>& OutProtoBinaryBytes);
	DECLARE_FUNCTION(execStructToBinaryProtoBytes);

	static bool ConvertStructToBinaryProtoBytes(const UStruct* StructDefinition, const void* Struct, TArray<uint8>& OutProtoBinaryBytes);

	static bool ConvertStructToBinaryProtoString(const UStruct* StructDefinition, const void* Struct, std::string& OutProtoBinaryString);

private:
	// Helper function to extract common struct to protobuf message conversion logic
	template<typename SerializeFunc>
	static bool ConvertStructToProtoInternal(const UStruct* StructDefinition, const void* Struct, SerializeFunc&& Serialize);

public:
	static bool DeserializeStructToMessage(UScriptStruct* StructDefinition, const void* Struct, google::protobuf::Message& TargetMsg,google::protobuf::FieldDescriptor* MsgFieldDescriptor);

	static bool SerializeMessageToBinaryString(google::protobuf::Message* message, std::string& OutProtoBinaryString, const FString& StructName = TEXT("Unknown"));

	static bool SerializeMessageToBinaryBytes(google::protobuf::Message* message, TArray<uint8>& OutProtoBinaryBytes, const FString& StructName = TEXT("Unknown"));

	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Proto", meta = (DisplayName = "Convert Proto Binary Bytes To Struct", CustomStructureParam = "ResultStruct", AutoCreateRefTerm = "ResultStruct"))
	static UPARAM(DisplayName="Success") bool ProtoBinaryBytesToStruct(UScriptStruct* StructDefinition, bool bAllowIncomplete,const TArray<uint8>& ProtoBinaryBytes, int32& ResultStruct);
	DECLARE_FUNCTION(execProtoBinaryBytesToStruct);

	static bool ConvertProtoBinaryBytesToStruct(UScriptStruct* StructDefinition, bool bAllowIncomplete, const TArray<uint8>& ProtoBinaryBytes, void* ResultStruct);

	static bool FillProtoMessageIntoUStruct(const google::protobuf::Message& Msg, UScriptStruct* StructDefinition, void* DestStruct);

	static bool SetFieldValue(google::protobuf::Message* targetMsg, const google::protobuf::FieldDescriptor* field, FProperty* property, const void* containerPtr);

	static TArray<FString> ParseArrayString(const FString& ArrayString);

	static EProto3Type AssignProtoType(const FProperty* InProp);

	static FString ProtoStringAssignProp(const FProperty* InProp);

	static FString GetPureNameOfProperty(const FProperty* InProp);

	static void ClearArenaCache(google::protobuf::Arena*ArenaPtr);

};
