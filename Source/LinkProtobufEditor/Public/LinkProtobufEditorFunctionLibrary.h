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
// Copyright DarkestLink-Dev 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LinkProtobufRuntime.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LinkProtobufEditorFunctionLibrary.generated.h"


USTRUCT(BlueprintType)
struct FProtoFieldKey
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite,Category = "LinkProtobuf|Editor")
	FString FieldName=TEXT("");
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "LinkProtobuf|Editor")
	FString UIDName=TEXT("");
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite,Category = "LinkProtobuf|Editor")
	bool bRepeated=false;

	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite,Category = "LinkProtobuf|Editor")
	EUnrealType UnrealType=EUnrealType::Normal;

	FProperty* ValueProperty;

	bool operator==(const FProtoFieldKey& Other) const
	{
		return FieldName == Other.FieldName;
	}
};

USTRUCT(BlueprintType)
struct FProtoStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite,Category = "LinkProtobuf|PreviewData")
	TArray<FProtoFieldKey> ProtoData;
};
FORCEINLINE uint32 GetTypeHash(const FProtoFieldKey& Key)
{
	return HashCombine(HashCombine(GetTypeHash(Key.FieldName), GetTypeHash(Key.UIDName)), GetTypeHash(Key.bRepeated));
}

UCLASS()
class LINKPROTOBUFEDITOR_API ULinkProtobufEditorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LinkProtobuf|Editor")
	FProtoStruct UserProtoData;

	UFUNCTION()
	static bool GenerateProtoFile(TArray<UScriptStruct*> TargetStructs);

	static void GenerateProtoMessageFromUStruct(UScriptStruct* TargetStruct,FString& RefProtoMessage);

	static void GenerateProtoMessageFromUStructArray(TArray<UScriptStruct*> TargetStructs, FString& MainProtoMessage);

	static void GenerateProtoCppFile();

	static FString GetProtoFilePath();

	bool GenerateProtoDataFromUStruct();

	static bool CheckCppProject();

	static bool RebuildThisPlugin();
protected:
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "LinkProtobuf|Editor")
	TArray<UScriptStruct*> ProtoBoundStructs;
	static TArray<FString> AssociatedStructs;
	static TArray<FString> AssociatedEnums;

};

