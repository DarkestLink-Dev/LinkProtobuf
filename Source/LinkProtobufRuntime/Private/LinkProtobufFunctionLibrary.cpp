// Copyright 2025 DarkestLink-Dev
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

#include "LinkProtobufFunctionLibrary.h"
#include "google/protobuf/descriptor.h"
#include <google/protobuf/message.h>
#include <google/protobuf/dynamic_message.h>
#include "LinkProtobufRuntime.h"
#include "UObject/Field.h"
#include "UObject/TextProperty.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION>=1
#include "Blueprint/BlueprintExceptionInfo.h"
#endif
#include "google/protobuf/descriptor.pb.h"

#define LOCTEXT_NAMESPACE "ProtoAssetsFunctionLibrary"

using Descriptor           = google::protobuf::Descriptor;
using FieldDescriptor      = google::protobuf::FieldDescriptor;
using EnumDescriptor       = google::protobuf::EnumDescriptor;
using EnumValueDescriptor  = google::protobuf::EnumValueDescriptor;
using Message              = google::protobuf::Message;
using Reflection           = google::protobuf::Reflection;
using Arena                = google::protobuf::Arena;
using DescriptorPool       = google::protobuf::DescriptorPool;
using MessageFactory       = google::protobuf::MessageFactory;

TArray<FString> ULinkProtobufFunctionLibrary::ParseArrayString(const FString& ArrayString)
{
    TArray<FString> Values;
    FString CleanedString = ArrayString;

    CleanedString = CleanedString.TrimStartAndEnd();
    CleanedString = CleanedString.Replace(TEXT("("), TEXT(""));
    CleanedString = CleanedString.Replace(TEXT(")"), TEXT(""));
    CleanedString = CleanedString.Replace(TEXT("["), TEXT(""));
    CleanedString = CleanedString.Replace(TEXT("]"), TEXT(""));
    CleanedString = CleanedString.TrimStartAndEnd();

    if (CleanedString.IsEmpty())
    {
        return Values;
    }

    // Use ParseIntoArray to split and clean each value
    CleanedString.ParseIntoArray(Values, TEXT(","), true);
    for (FString& Val : Values)
    {
        Val = Val.TrimStartAndEnd();
        Val = Val.Replace(TEXT("\""), TEXT(""));
        Val = Val.Replace(TEXT("'"), TEXT(""));
    }

    return Values;
}


DEFINE_FUNCTION(ULinkProtobufFunctionLibrary::execStructToBinaryProtoString)
{
	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* ValueProperty = Stack.MostRecentProperty;
	void* ValuePtr = Stack.MostRecentPropertyAddress;
	PARAM_PASSED_BY_REF(OutProtoString, FStrProperty, FString);

	P_FINISH;

	if (!ValueProperty || !ValuePtr)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			LOCTEXT("StructToProtoString_MissingInputProperty", "Failed to resolve the input parameter for StructToProtoString.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}

	bool bResult;
	FStructProperty* const StructProperty = CastField<FStructProperty>(ValueProperty);
	if (!StructProperty)
	{
		bResult = false;
		*StaticCast<bool*>(RESULT_PARAM) = bResult;
		return;
	}

	std::string stdProtoString;
	bResult = P_THIS->ConvertStructToBinaryProtoString(StructProperty->Struct, ValuePtr,stdProtoString);
	OutProtoString=FString(UTF8_TO_TCHAR(stdProtoString.c_str()));

	*StaticCast<bool*>(RESULT_PARAM) = bResult;
}
DEFINE_FUNCTION(ULinkProtobufFunctionLibrary::execStructToBinaryProtoBytes)
{
	Stack.StepCompiledIn<FProperty>(nullptr);
	FProperty* ValueProperty = Stack.MostRecentProperty;
	void* ValuePtr = Stack.MostRecentPropertyAddress;
	PARAM_PASSED_BY_REF(OutProtoBinaryBytes, FArrayProperty, TArray<uint8>);

	P_FINISH;

	if (!ValueProperty || !ValuePtr)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			LOCTEXT("StructToProtoString_MissingInputProperty", "Failed to resolve the input parameter for StructToProtoString.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}

	bool bResult;
	FStructProperty* const StructProperty = CastField<FStructProperty>(ValueProperty);
	if (!StructProperty)
	{
		bResult = false;
		*StaticCast<bool*>(RESULT_PARAM) = bResult;
		return;
	}

	std::string OutProtoBinaryString;
	bResult = P_THIS->ConvertStructToBinaryProtoBytes(StructProperty->Struct, ValuePtr,OutProtoBinaryBytes);

	FString BytesAsHex;
	for (uint8 Byte : OutProtoBinaryBytes) {
		BytesAsHex += FString::Printf(TEXT("%02X "), Byte);
	}
	UE_LOG(LogProto, VeryVerbose, TEXT("Proto Raw Bytes: %s"), *BytesAsHex);

	*StaticCast<bool*>(RESULT_PARAM) = bResult;
}


bool ULinkProtobufFunctionLibrary::ConvertStructToBinaryProtoBytes(const UStruct* StructDefinition, const void* Struct, TArray<uint8>& OutProtoBinaryBytes)
{
    return ConvertStructToProtoInternal(StructDefinition, Struct,
        [&](google::protobuf::Message* message, const FString& StructName) -> bool {
            return SerializeMessageToBinaryBytes(message, OutProtoBinaryBytes, StructName);
        }
    );
}

bool ULinkProtobufFunctionLibrary::ConvertStructToBinaryProtoString(const UStruct* StructDefinition, const void* Struct, std::string& OutProtoBinaryString)
{
    return ConvertStructToProtoInternal(StructDefinition, Struct,
        [&](google::protobuf::Message* message, const FString& StructName) -> bool {
            return SerializeMessageToBinaryString(message, OutProtoBinaryString, StructName);
        }
    );
}
template<typename SerializeFunc>
bool ULinkProtobufFunctionLibrary::ConvertStructToProtoInternal(const UStruct* StructDefinition, const void* Struct, SerializeFunc&& Serialize)
{
	FString StructName = StructDefinition->GetName();
	UE_LOG(LogProto, Log, TEXT("Proto Converting struct: %s"), *StructName);

	std::string protoName = TCHAR_TO_UTF8(*StructName);
	const Descriptor* descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(protoName);
	if (!descriptor)
	{
		UE_LOG(LogProto, Error, TEXT("Proto Descriptor for %s not found"), *StructName);
		return false;
	}
	const Message* prototype = MessageFactory::generated_factory()->GetPrototype(descriptor);
	if (!prototype)
	{
		UE_LOG(LogProto, Error, TEXT("Proto Prototype for %s not found"), *StructName);
		return false;
	}
	Message* message = prototype->New();
	UScriptStruct* ScriptStruct = Cast<UScriptStruct>(const_cast<UStruct*>(StructDefinition));
	if (!ScriptStruct)
	{
		UE_LOG(LogProto, Error, TEXT("Proto ConvertStructToProtoInternal: StructDefinition is not UScriptStruct for %s"), *StructName);
		return false;
	}
	if (!DeserializeStructToMessage(ScriptStruct, Struct, *message, nullptr))
	{
		UE_LOG(LogProto, Error, TEXT("Proto DeserializeStructToMessage failed for %s"), *StructName);
		return false;
	}
	// Call the provided serialization function
	return Serialize(message, StructName);
}

bool ULinkProtobufFunctionLibrary::DeserializeStructToMessage(UScriptStruct* StructDefinition, const void* Struct,google::protobuf::Message& TargetMsg,google::protobuf::FieldDescriptor*)
{
    if (!StructDefinition || !Struct)
    {
        UE_LOG(LogProto, Error, TEXT("Proto DeserializeStructToMessage: invalid inputs"));
        return false;
    }

    const Descriptor* descriptor = TargetMsg.GetDescriptor();
    const Reflection* reflection = TargetMsg.GetReflection();
    if (!descriptor || !reflection)
    {
        UE_LOG(LogProto, Error, TEXT("Proto DeserializeStructToMessage: missing descriptor/reflection"));
        return false;
    }

    for (TFieldIterator<FProperty> It(StructDefinition); It; ++It)
    {
        FProperty* Property = *It;
    	FString PropertyName = GetPureNameOfProperty(Property);
        const void* ContainerPtr = Property->ContainerPtrToValuePtr<void>(Struct);

        std::string FieldName = TCHAR_TO_UTF8(*PropertyName);
        const FieldDescriptor* ItField = descriptor->FindFieldByName(FieldName);
        if (!ItField)
        {
            UE_LOG(LogProto, Warning, TEXT("Proto DeserializeStructToMessage: field %s not found, skip"), *PropertyName);
            continue;
        }
        // Repeated field handling
        if (ItField->is_repeated() && !ItField->is_map())
        {
            auto AddPrimitiveRepeatedElement = [&](const FProperty* ElemProp, const void* ElemPtr)
            {
                switch (ItField->type())
                {
                case FieldDescriptor::TYPE_INT32:
                    if (CastField<FIntProperty>(ElemProp) || CastField<FEnumProperty>(ElemProp))
                        reflection->AddInt32(&TargetMsg, ItField, *reinterpret_cast<const int32*>(ElemPtr));
                    return;
                case FieldDescriptor::TYPE_INT64:
                    if (CastField<FInt64Property>(ElemProp)) reflection->AddInt64(&TargetMsg, ItField, *reinterpret_cast<const int64*>(ElemPtr)); return;
                case FieldDescriptor::TYPE_UINT32:
                    if (CastField<FUInt32Property>(ElemProp)) reflection->AddUInt32(&TargetMsg, ItField, *reinterpret_cast<const uint32*>(ElemPtr));
                    else if (CastField<FIntProperty>(ElemProp)) reflection->AddUInt32(&TargetMsg, ItField, static_cast<uint32>(*reinterpret_cast<const int32*>(ElemPtr))); return;
                case FieldDescriptor::TYPE_UINT64:
                    if (CastField<FUInt64Property>(ElemProp)) reflection->AddUInt64(&TargetMsg, ItField, *reinterpret_cast<const uint64*>(ElemPtr)); return;
                case FieldDescriptor::TYPE_FLOAT:
                    if (CastField<FFloatProperty>(ElemProp)) reflection->AddFloat(&TargetMsg, ItField, *reinterpret_cast<const float*>(ElemPtr)); return;
                case FieldDescriptor::TYPE_DOUBLE:
                    if (CastField<FDoubleProperty>(ElemProp)) reflection->AddDouble(&TargetMsg, ItField, *reinterpret_cast<const double*>(ElemPtr));
                    else if (CastField<FFloatProperty>(ElemProp)) reflection->AddDouble(&TargetMsg, ItField, (double)*reinterpret_cast<const float*>(ElemPtr)); return;
                case FieldDescriptor::TYPE_BOOL:
                    if (CastField<FBoolProperty>(ElemProp)) reflection->AddBool(&TargetMsg, ItField, *reinterpret_cast<const bool*>(ElemPtr));
                    return;
                case FieldDescriptor::TYPE_ENUM:
                    if (ItField->enum_type())
                    {
                        int32 EnumNumber = 0;
                        if (const FEnumProperty* EP = CastField<FEnumProperty>(ElemProp))
                        {
                            FProperty* Under = EP->GetUnderlyingProperty();
                            if (CastField<FByteProperty>(Under)) EnumNumber = *reinterpret_cast<const uint8*>(ElemPtr);
                            else if (CastField<FIntProperty>(Under)) EnumNumber = *reinterpret_cast<const int32*>(ElemPtr);
                        }
                        else if (CastField<FByteProperty>(ElemProp))
                        {
                            EnumNumber = *reinterpret_cast<const uint8*>(ElemPtr);
                        }
                        else if (CastField<FIntProperty>(ElemProp))
                        {
                            EnumNumber = *reinterpret_cast<const int32*>(ElemPtr);
                        }
                        if (const EnumValueDescriptor* EVD = ItField->enum_type()->FindValueByNumber(EnumNumber))
                        {
                            reflection->AddEnum(&TargetMsg, ItField, EVD);
                        }
                    }
                    return;
                case FieldDescriptor::TYPE_STRING:
                    if (CastField<FStrProperty>(ElemProp))
                        reflection->AddString(&TargetMsg, ItField, TCHAR_TO_UTF8(**reinterpret_cast<const FString*>(ElemPtr)));
                    else if (CastField<FNameProperty>(ElemProp))
                        reflection->AddString(&TargetMsg, ItField, TCHAR_TO_UTF8(*reinterpret_cast<const FName*>(ElemPtr)->ToString()));
                    else if (CastField<FTextProperty>(ElemProp))
                        reflection->AddString(&TargetMsg, ItField, TCHAR_TO_UTF8(*reinterpret_cast<const FText*>(ElemPtr)->ToString()));
                    return;
                case FieldDescriptor::TYPE_BYTES:
                	if (CastField<FByteProperty>(ElemProp))
                	{
                		uint8 V = *reinterpret_cast<const uint8*>(ElemPtr);
                		std::string bytes(reinterpret_cast<const char*>(&V), 1);
                		reflection->AddString(&TargetMsg, ItField, bytes);
                		return;
                	}
                default: return;
                }
            };

            // TArray handling
            if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
            {
                FScriptArrayHelper ArrayHelper(ArrayProp, ContainerPtr);
                for (int32 i = 0; i < ArrayHelper.Num(); ++i)
                {
                    void* ElemPtr = ArrayHelper.GetRawPtr(i);
                    if (ItField->type() == FieldDescriptor::TYPE_MESSAGE)
                    {
                        if (const FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner))
                        {
                            const UScriptStruct* InnerStruct = InnerStructProp->Struct;
                            if (!InnerStruct) { UE_LOG(LogProto, Error, TEXT("Proto repeated array inner struct invalid %s"), *PropertyName); continue; }
                            Message* RepeatedMsg = reflection->AddMessage(&TargetMsg, ItField);
                            if (!RepeatedMsg) { UE_LOG(LogProto, Error, TEXT("Proto failed add repeated message %s"), *PropertyName); continue; }
                            if (!DeserializeStructToMessage(const_cast<UScriptStruct*>(InnerStruct), ElemPtr, *RepeatedMsg, const_cast<FieldDescriptor*>(ItField)))
                            {
                                UE_LOG(LogProto, Error, TEXT("Proto failed nested repeated fill %s"), *PropertyName);
                            }
                        }
                    }
                    else
                    {
                        AddPrimitiveRepeatedElement(ArrayProp->Inner, ElemPtr);
                    }
                }
                continue;
            }
            // TSet handling
            if (const FSetProperty* SetProp = CastField<FSetProperty>(Property))
            {
                FScriptSetHelper SetHelper(SetProp, ContainerPtr);
                for (int32 i = 0; i < SetHelper.Num(); ++i)
                {
                    if (!SetHelper.IsValidIndex(i)) continue;
                    void* ElemPtr = SetHelper.GetElementPtr(i);
                    if (ItField->type() == FieldDescriptor::TYPE_MESSAGE)
                    {
                        if (const FStructProperty* ElementStructProp = CastField<FStructProperty>(SetProp->ElementProp))
                        {
                            const UScriptStruct* InnerStruct = ElementStructProp->Struct;
                            if (!InnerStruct) { UE_LOG(LogProto, Error, TEXT("Proto repeated set inner struct invalid %s"), *PropertyName); continue; }
                            Message* RepeatedMsg = reflection->AddMessage(&TargetMsg, ItField);
                            if (!RepeatedMsg) { UE_LOG(LogProto, Error, TEXT("Proto failed add repeated message %s"), *PropertyName); continue; }
                            if (!DeserializeStructToMessage(const_cast<UScriptStruct*>(InnerStruct), ElemPtr, *RepeatedMsg, const_cast<FieldDescriptor*>(ItField)))
                            {
                                UE_LOG(LogProto, Error, TEXT("Proto failed nested repeated fill %s"), *PropertyName);
                            }
                        }
                    }
                    else
                    {
                        AddPrimitiveRepeatedElement(SetProp->ElementProp, ElemPtr);
                    }
                }
                continue;
            }
            // If repeated but property not an array/set we let generic path try
        }
        // Map type handling
        if (ItField->is_map())
        {
            if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
            {
                FScriptMapHelper MapHelper(MapProperty, ContainerPtr);

                const Descriptor* entryDesc = ItField->message_type();
                const FieldDescriptor* keyFd = entryDesc->FindFieldByName("key");
                const FieldDescriptor* valFd = entryDesc->FindFieldByName("value");
                if (!keyFd || !valFd)
                {
                    UE_LOG(LogProto, Error, TEXT("Proto DeserializeStructToMessage: invalid map entry descriptor for %s"), *PropertyName);
                    continue;
                }

                for (int32 idx = 0; idx < MapHelper.Num(); ++idx)
                {
                    if (!MapHelper.IsValidIndex(idx))
                        continue;

                    // Let parent message allocate and hold entry (consistent with its Arena), avoiding manual allocation and ownership issues
                    Message* entryMsg = reflection->AddMessage(&TargetMsg, ItField);
                    const Reflection* entryReflection = entryMsg->GetReflection();

                    // Set key
                    void* KeyPtr = MapHelper.GetKeyPtr(idx);
                    if (!SetFieldValue(entryMsg, keyFd, MapProperty->KeyProp, KeyPtr))
                    {
                        UE_LOG(LogProto, Warning, TEXT("Proto map key set failed for %s"), *PropertyName);
                    }

                    // Set value
                    void* ValPtr = MapHelper.GetValuePtr(idx);
                    if (valFd->type() == FieldDescriptor::TYPE_MESSAGE && CastField<FStructProperty>(MapProperty->ValueProp))
                    {
                        const UScriptStruct* InnerStruct = CastField<FStructProperty>(MapProperty->ValueProp)->Struct;
                        if (!InnerStruct)
                        {
                            UE_LOG(LogProto, Error, TEXT("Proto map value inner struct invalid for %s"), *PropertyName);
                            continue;
                        }
                        Message* nestedValue = entryReflection->MutableMessage(entryMsg, valFd);
                        if (!nestedValue)
                        {
                            UE_LOG(LogProto, Error, TEXT("Proto failed to get mutable map value message for %s"), *PropertyName);
                            continue;
                        }
                        if (!DeserializeStructToMessage(const_cast<UScriptStruct*>(InnerStruct), ValPtr, *nestedValue, const_cast<FieldDescriptor*>(valFd)))
                        {
                            UE_LOG(LogProto, Error, TEXT("Proto failed to fill nested map value for %s"), *PropertyName);
                            entryReflection->ClearField(entryMsg, valFd);
                        }
                    }
                    else
                    {
                        if (!SetFieldValue(entryMsg, valFd, MapProperty->ValueProp, ValPtr))
                        {
                            UE_LOG(LogProto, Warning, TEXT("Proto map value set failed for %s"), *PropertyName);
                        }
                    }
                }
            }
            continue;
        }

        // Nested message (non-map)
        if (ItField->type() == FieldDescriptor::TYPE_MESSAGE)
        {
            if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
            {
                const UScriptStruct* InnerStruct = StructProp->Struct;
                Message* targetNested = reflection->MutableMessage(&TargetMsg, ItField);
                if (!targetNested)
                {
                    UE_LOG(LogProto, Error, TEXT("Proto failed to get mutable nested message for %s"), *PropertyName);
                    continue;
                }
                if (!DeserializeStructToMessage(const_cast<UScriptStruct*>(InnerStruct), ContainerPtr, *targetNested, const_cast<FieldDescriptor*>(ItField)))
                {
                    UE_LOG(LogProto, Error, TEXT("Proto failed to fill nested message for %s"), *PropertyName);
                    reflection->ClearField(&TargetMsg, ItField);
                }
            }
            continue;
        }
		//normal
        SetFieldValue(&TargetMsg, ItField, Property, ContainerPtr);
    }

    return true;
}

bool ULinkProtobufFunctionLibrary::SerializeMessageToBinaryString(google::protobuf::Message* message, std::string& OutProtoBinaryString, const FString& StructName)
{
    if (!message)
    {
        UE_LOG(LogProto, Error, TEXT("Proto SerializeMessageToBinaryString: Null message pointer for %s"), *StructName);
        return false;
    }

    if (!message->IsInitialized())
    {
        UE_LOG(LogProto, Warning, TEXT("Proto Message for %s is not fully initialized, using partial serialization"), *StructName);
        bool bResult = message->SerializePartialToString(&OutProtoBinaryString);
        UE_LOG(LogProto, Log, TEXT("Proto Partial serialization %s for %s"),
               bResult ? TEXT("succeeded") : TEXT("failed"), *StructName);
        return bResult;
    }
    else
    {
        bool bResult = message->SerializeToString(&OutProtoBinaryString);
        UE_LOG(LogProto, Log, TEXT("Proto Full serialization %s for %s"),
               bResult ? TEXT("succeeded") : TEXT("failed"), *StructName);
        return bResult;
    }
}

bool ULinkProtobufFunctionLibrary::SerializeMessageToBinaryBytes(
	google::protobuf::Message* Message,
	TArray<uint8>& OutBytes,
	const FString& StructName)
{
	if (!Message)
	{
		UE_LOG(LogProto, Error, TEXT("Null message pointer for %s"), *StructName);
		return false;
	}

	std::string BinaryData;
	bool bSuccess = Message->SerializeToString(&BinaryData);

	if (!bSuccess)
	{
		UE_LOG(LogProto, Error, TEXT("Failed to serialize message for %s"), *StructName);
		return false;
	}

	OutBytes.SetNumUninitialized(BinaryData.size());
	FMemory::Memcpy(OutBytes.GetData(), BinaryData.data(), BinaryData.size());

	UE_LOG(LogProto, Log, TEXT("Serialized message (%s) to %d bytes"), *StructName, static_cast<int32>(BinaryData.size()));
	return true;
}


DEFINE_FUNCTION(ULinkProtobufFunctionLibrary::execProtoBinaryBytesToStruct)
{
    P_GET_OBJECT(UScriptStruct, StructDefinition);
    P_GET_UBOOL(bAllowIncomplete);
    P_GET_TARRAY_REF(uint8, ProtoBinaryBytes);
    Stack.StepCompiledIn<FProperty>(nullptr);
    FProperty* ValueProperty = Stack.MostRecentProperty;
    void* ValuePtr = Stack.MostRecentPropertyAddress;
    P_FINISH;
    if (!StructDefinition || !ValueProperty || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AccessViolation,
            LOCTEXT("ProtoBytesToStruct_MissingOutputProperty", "Failed to resolve parameters for ProtoBytesToStruct.")
        );
        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
        *StaticCast<bool*>(RESULT_PARAM) = false;
        return;
    }
    FStructProperty* const StructProperty = CastField<FStructProperty>(ValueProperty);
    if (!StructProperty || StructProperty->Struct != StructDefinition)
    {
        UE_LOG(LogProto, Error, TEXT("Proto ExecConvertProtoBinaryBytesToStruct: Struct mismatch"));
        *StaticCast<bool*>(RESULT_PARAM) = false;
        return;
    }
    bool bResult = P_THIS->ConvertProtoBinaryBytesToStruct(StructDefinition, bAllowIncomplete, ProtoBinaryBytes, ValuePtr);
    *StaticCast<bool*>(RESULT_PARAM) = bResult;
}

bool ULinkProtobufFunctionLibrary::ConvertProtoBinaryBytesToStruct(UScriptStruct* StructDefinition, bool bAllowIncomplete,
    const TArray<uint8>& ProtoBinaryBytes, void* ResultStruct)
{
    if (!StructDefinition || !ResultStruct)
    {
        UE_LOG(LogProto, Error, TEXT("Proto ConvertProtoBinaryBytesToStruct: invalid struct inputs"));
        return false;
    }
    if (ProtoBinaryBytes.Num() <= 0)
    {
        UE_LOG(LogProto, Warning, TEXT("Proto ConvertProtoBinaryBytesToStruct: empty bytes"));
        return false;
    }

    FString StructName = StructDefinition->GetName();
    std::string ProtoName = TCHAR_TO_UTF8(*StructName);
    const Descriptor* DescriptorPtr = DescriptorPool::generated_pool()->FindMessageTypeByName(ProtoName);
    if (!DescriptorPtr)
    {
        UE_LOG(LogProto, Error, TEXT("Proto ConvertProtoBinaryBytesToStruct: descriptor not found for %s"), *StructName);
        return false;
    }
    const Message* Prototype = MessageFactory::generated_factory()->GetPrototype(DescriptorPtr);
    if (!Prototype)
    {
        UE_LOG(LogProto, Error, TEXT("Proto ConvertProtoBinaryBytesToStruct: prototype not found for %s"), *StructName);
        return false;
    }

    Message* ParsedMsg = Prototype->New();
    bool bParseOk;
    if (bAllowIncomplete)
    {
        bParseOk = ParsedMsg->ParsePartialFromArray(ProtoBinaryBytes.GetData(), ProtoBinaryBytes.Num());
        UE_LOG(LogProto, Verbose, TEXT("Proto ParsePartial used (AllowIncomplete=true) for %s => %s"), *StructName, bParseOk ? TEXT("Success") : TEXT("Fail"));
    }
    else
    {
        bParseOk = ParsedMsg->ParseFromArray(ProtoBinaryBytes.GetData(), ProtoBinaryBytes.Num());
        UE_LOG(LogProto, Verbose, TEXT("Proto Parse used (AllowIncomplete=false) for %s => %s"), *StructName, bParseOk ? TEXT("Success") : TEXT("Fail"));
    }

    if (!bParseOk)
    {
        UE_LOG(LogProto, Error, TEXT("Proto ConvertProtoBinaryBytesToStruct: parse failed for %s (AllowIncomplete=%s)"), *StructName, bAllowIncomplete ? TEXT("true") : TEXT("false"));
        return false;
    }

    if (!bAllowIncomplete && !ParsedMsg->IsInitialized())
    {
        UE_LOG(LogProto, Error, TEXT("Proto message for %s not fully initialized (AllowIncomplete=false)"), *StructName);
        return false;
    }

    return FillProtoMessageIntoUStruct(*ParsedMsg, StructDefinition, ResultStruct);
}


bool ULinkProtobufFunctionLibrary::FillProtoMessageIntoUStruct(const google::protobuf::Message& Msg, UScriptStruct* StructDefinition, void* DestStruct)
{
    if (!StructDefinition || !DestStruct)
        return false;

    const Descriptor* F_Desc = Msg.GetDescriptor();
    const Reflection* F_Ref = Msg.GetReflection();
    if (!F_Desc || !F_Ref)
    {
    	UE_LOG(LogProto, Error, TEXT("Proto FillProtoMessageIntoUStruct: missing descriptor/reflection"));
    	return false;
    }

	//Basic lambda to write primitive value from protobuf field to property
    auto WritePrimitiveToProperty = [&](FProperty* Prop, const Message& EntryMsg ,void* Dest, const FieldDescriptor* Fd, int Index, bool bRepeated)->bool
    {
        if (!Prop || !Dest || !Fd)
        {
            UE_LOG(LogProto, Error, TEXT("Proto WritePrimitiveToProperty: invalid arguments"));
            return false;
        }
        const Reflection* EntyRef = EntryMsg.GetReflection();
        if (!EntyRef)
        {
            UE_LOG(LogProto, Error, TEXT("Proto WritePrimitiveToProperty: Reflection is nullptr"));
            return false;
        }
        auto EnsureIndex = [&](int Wanted)->bool
        {
            if (!bRepeated) return true;
            int Size = EntyRef->FieldSize(EntryMsg, Fd);
            if (Wanted < 0 || Wanted >= Size)
            {
                UE_LOG(LogProto, Error, TEXT("Proto WritePrimitiveToProperty: index %d out of range %d for field %s"), Wanted, Size, UTF8_TO_TCHAR(Fd->name().c_str()));
                return false;
            }
            return true;
        };
    	auto GetStringLikeValue = [&](std::string& Out) -> bool
    	{
    		if (!Fd || !EntyRef || Fd->containing_type() != EntryMsg.GetDescriptor())
    			return false;

    		const bool bStringOrBytes =
				Fd->type() == FieldDescriptor::TYPE_STRING ||
				Fd->type() == FieldDescriptor::TYPE_BYTES;

    		if (Fd->is_repeated())
    		{
    			const int size = EntyRef->FieldSize(EntryMsg, Fd);
    			if (Index < 0 || Index >= size)
    			{
    				UE_LOG(LogProto, Error, TEXT("Proto GetStringLikeValue: Index %d out of range %d for [%s]"),
				   Index, size, UTF8_TO_TCHAR(Fd->name().c_str()));
    				return false;
    			}
    			if (!bStringOrBytes)
    			{
    				UE_LOG(LogProto, Error, TEXT("Proto GetStringLikeValue: Field [%s] is repeated but not string/bytes (type=%d)"),
				   UTF8_TO_TCHAR(Fd->name().c_str()), (int)Fd->type());
    				return false;
    			}
    			Out = EntyRef->GetRepeatedString(EntryMsg, Fd, Index);
    			return true;
    		}
    		else
    		{
    			if (!bStringOrBytes)
    			{
    				UE_LOG(LogProto, Error, TEXT("Proto GetStringLikeValue: Field [%s] is not string/bytes (type=%d)"),
				   UTF8_TO_TCHAR(Fd->name().c_str()), (int)Fd->type());
    				return false;
    			}
    			Out = EntyRef->GetString(EntryMsg, Fd);
    			return true;
    		}
    	};
    	auto GetStringValue = GetStringLikeValue;
    	auto GetBytesValue = GetStringLikeValue;
        auto GetEnumNumber = [&]()->int32
        {
            if (bRepeated)
                return F_Ref->GetRepeatedEnumValue(EntryMsg, Fd, Index);
            else
                return F_Ref->GetEnumValue(EntryMsg, Fd);
        };

        switch (Fd->type())
        {
        case FieldDescriptor::TYPE_INT32:
            if (!EnsureIndex(Index)) return false;
            if (FIntProperty* IntP = CastField<FIntProperty>(Prop))
            {
                int32 V = bRepeated ? F_Ref->GetRepeatedInt32(EntryMsg, Fd, Index) : F_Ref->GetInt32(EntryMsg, Fd);
            	IntP->SetPropertyValue(Dest,V);
                return true;
            }
            break;
        case FieldDescriptor::TYPE_INT64:
            if (!EnsureIndex(Index)) return false;
            if (FInt64Property* Int64P = CastField<FInt64Property>(Prop))
            {
                int64 V = bRepeated ? F_Ref->GetRepeatedInt64(EntryMsg, Fd, Index) : F_Ref->GetInt64(EntryMsg, Fd);
            	Int64P->SetPropertyValue(Dest,V);
                return true;
            }
            break;
        case FieldDescriptor::TYPE_UINT32:
            if (!EnsureIndex(Index)) return false;
            if (FUInt32Property* UInt32P = CastField<FUInt32Property>(Prop))
            {
                uint32 V = bRepeated ? F_Ref->GetRepeatedUInt32(EntryMsg, Fd, Index) : F_Ref->GetUInt32(EntryMsg, Fd);
            	UInt32P->SetPropertyValue(Dest,V);
                return true;
            }
            if (FIntProperty* IntP2 = CastField<FIntProperty>(Prop))
            {
                uint32 V = bRepeated ? F_Ref->GetRepeatedUInt32(EntryMsg, Fd, Index) : F_Ref->GetUInt32(EntryMsg, Fd);
            	IntP2->SetPropertyValue(Dest,static_cast<int32>(V));
                return true;
            }
            break;
        case FieldDescriptor::TYPE_UINT64:
            if (!EnsureIndex(Index)) return false;
            if (FUInt64Property* UInt64P = CastField<FUInt64Property>(Prop))
            {
                uint64 V = bRepeated ? F_Ref->GetRepeatedUInt64(EntryMsg, Fd, Index) : F_Ref->GetUInt64(EntryMsg, Fd);
            	UInt64P->SetPropertyValue(Dest,V);
                return true;
            }
            if (FInt64Property* Int64P2 = CastField<FInt64Property>(Prop))
            {
                uint64 V = bRepeated ? F_Ref->GetRepeatedUInt64(EntryMsg, Fd, Index) : F_Ref->GetUInt64(EntryMsg, Fd);
            	Int64P2->SetPropertyValue(Dest,static_cast<int64>(V));
                return true;
            }
            break;
        case FieldDescriptor::TYPE_FLOAT:
            if (!EnsureIndex(Index)) return false;
            if (FFloatProperty* FloatP = CastField<FFloatProperty>(Prop))
            {
                float V = bRepeated ? F_Ref->GetRepeatedFloat(EntryMsg, Fd, Index) : F_Ref->GetFloat(EntryMsg, Fd);
            	FloatP->SetPropertyValue(Dest,V);
                return true;
            }
            break;
        case FieldDescriptor::TYPE_DOUBLE:
            if (!EnsureIndex(Index)) return false;
            if (FDoubleProperty* DoubleP = CastField<FDoubleProperty>(Prop))
            {
                double V = bRepeated ? F_Ref->GetRepeatedDouble(EntryMsg, Fd, Index) : F_Ref->GetDouble(EntryMsg, Fd);
            	DoubleP->SetPropertyValue(Dest,V);
                return true;
            }
            if (FFloatProperty* FloatP = CastField<FFloatProperty>(Prop))
            {
                double V = bRepeated ? F_Ref->GetRepeatedDouble(EntryMsg, Fd, Index) : F_Ref->GetDouble(EntryMsg, Fd);
            	FloatP->SetPropertyValue(Dest,V);
                return true;
            }
            break;
        case FieldDescriptor::TYPE_BOOL:
            if (!EnsureIndex(Index)) return false;
            if (FBoolProperty* BoolP = CastField<FBoolProperty>(Prop))
            {
                bool V = bRepeated ? F_Ref->GetRepeatedBool(EntryMsg, Fd, Index) : F_Ref->GetBool(EntryMsg, Fd);
                BoolP->SetPropertyValue(Dest, V);
                return true;
            }
            break;
        case FieldDescriptor::TYPE_ENUM:
            if (!EnsureIndex(Index)) return false;
	        {
		        int32 EnumNumber = GetEnumNumber();
        		if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
        		{
        			if (FProperty* Under = EnumProp->GetUnderlyingProperty())
        			{
        				if (FByteProperty* ByteP = CastField<FByteProperty>(Under))
        				{
        					ByteP->SetPropertyValue(Dest, static_cast<uint8>(EnumNumber));
        					return true;
        				}
        				if (FIntProperty* IntP = CastField<FIntProperty>(Under))
        				{
        					IntP->SetPropertyValue(Dest, EnumNumber);
        					return true;
        				}
        			}
        		}
	        }
            break;
        case FieldDescriptor::TYPE_STRING:
        {
            std::string S; if (!GetStringValue(S)) return false;
            if (FStrProperty* StrP = CastField<FStrProperty>(Prop))
            {
            	StrP->SetPropertyValue(Dest, UTF8_TO_TCHAR(S.c_str()));
            	return true;
            }
            if (FNameProperty* NameP = CastField<FNameProperty>(Prop))
            {
            	NameP->SetPropertyValue(Dest, UTF8_TO_TCHAR(S.c_str()));
            	return true;
            }
            if (FTextProperty* TextP = CastField<FTextProperty>(Prop))
            {
            	TextP->SetPropertyValue(Dest, FText::FromString(UTF8_TO_TCHAR(S.c_str())));
            	return true;
            }
            break;
        }
        case FieldDescriptor::TYPE_BYTES:
        	{
        		if (!EnsureIndex(Index)) return false;
        		std::string value; if (!GetBytesValue(value)) return false;
        		if (FByteProperty* BP = CastField<FByteProperty>(Prop))
        		{
        			uint8 V = value.empty() ? 0 : static_cast<uint8>(value[0]);
        			BP->SetPropertyValue(Dest, V);
        		}
        	}
        default:
            UE_LOG(LogProto, Warning, TEXT("Proto WritePrimitiveToProperty: unhandled fd type %d (%s)"), (int)Fd->type(), UTF8_TO_TCHAR(Fd->name().c_str()));
            return false;
        }
        UE_LOG(LogProto, Warning, TEXT("Proto WritePrimitiveToProperty: property type mismatch for field %s -> %s"), UTF8_TO_TCHAR(Fd->name().c_str()), *Prop->GetName());
        return false;
    };

    for (TFieldIterator<FProperty> It(StructDefinition); It; ++It)
    {
        FProperty* Prop = *It;
        FString FieldNameUE = GetPureNameOfProperty(Prop).Replace(TEXT(" "), TEXT(""));
        std::string ProtoFieldName = TCHAR_TO_UTF8(*FieldNameUE);
        const FieldDescriptor* FD = F_Desc->FindFieldByName(ProtoFieldName);
        if (!FD)
        {
        	UE_LOG(LogProto, Warning, TEXT("Proto WritePrimitiveToProperty: field %s not found in message %s, skip"), *FieldNameUE, UTF8_TO_TCHAR(F_Desc->name().c_str()));
			continue;
        }
		//Deserialize Map type
        if (FD->is_map())
        {
            FMapProperty* MapProp = CastField<FMapProperty>(Prop);
            if (!MapProp) continue;
            int EntryCount = F_Ref->FieldSize(Msg, FD);
            FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(DestStruct));
            for (int i=0;i<EntryCount;++i)
            {
                const Message& EntryMsg = F_Ref->GetRepeatedMessage(Msg, FD, i);
                const Descriptor* EntryDesc = EntryMsg.GetDescriptor();
                const Reflection* EntryRefl = EntryMsg.GetReflection();
                const FieldDescriptor* KeyFd = EntryDesc->FindFieldByName("key");
                const FieldDescriptor* ValFd = EntryDesc->FindFieldByName("value");
                int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
                void* KeyPtr = MapHelper.GetKeyPtr(NewIndex);
                void* ValPtr = MapHelper.GetValuePtr(NewIndex);
                if (KeyFd)
                    WritePrimitiveToProperty(MapProp->KeyProp, EntryMsg,KeyPtr, KeyFd, 0, false);
                if (ValFd)
                {
                    if (ValFd->type()==FieldDescriptor::TYPE_MESSAGE && CastField<FStructProperty>(MapProp->ValueProp))
                    {
                        const Message& NestedVal = EntryRefl->GetMessage(EntryMsg, ValFd);
                        FillProtoMessageIntoUStruct(NestedVal, CastField<FStructProperty>(MapProp->ValueProp)->Struct, ValPtr);
                    }
                    else
                    {
                        WritePrimitiveToProperty(MapProp->ValueProp, EntryMsg,ValPtr, ValFd, 0, false);
                    }
                }
            }
            MapHelper.Rehash();
            continue;
        }
        // Deserialize TArray type
        if (FD->is_repeated() && CastField<FArrayProperty>(Prop))
        {
            FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
            FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(DestStruct));
            int Count = F_Ref->FieldSize(Msg, FD);
            for (int i=0;i<Count;++i)
            {
                int32 NewIdx = ArrayHelper.AddValue();
                void* ElemPtr = ArrayHelper.GetRawPtr(NewIdx);
                if (FD->type()==FieldDescriptor::TYPE_MESSAGE)
                {
                    if (FStructProperty* ElemStructProp = CastField<FStructProperty>(ArrayProp->Inner))
                    {
                        const Message& SubMsg = F_Ref->GetRepeatedMessage(Msg, FD, i);
                        FillProtoMessageIntoUStruct(SubMsg, ElemStructProp->Struct, ElemPtr);
                    }
                }
                else
                {
                    WritePrimitiveToProperty(ArrayProp->Inner, Msg,ElemPtr, FD, i, true);
                }
            }
            continue;
        }
        // TSet
        if (FD->is_repeated() && CastField<FSetProperty>(Prop))
        {
            FSetProperty* SetProp = CastField<FSetProperty>(Prop);
            FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(DestStruct));
            int Count = F_Ref->FieldSize(Msg, FD);
            for (int i=0;i<Count;++i)
            {
                int32 NewIdx = SetHelper.AddDefaultValue_Invalid_NeedsRehash();
                void* ElemPtr = SetHelper.GetElementPtr(NewIdx);
                if (FD->type()==FieldDescriptor::TYPE_MESSAGE)
                {
                    if (FStructProperty* ElemStructProp = CastField<FStructProperty>(SetProp->ElementProp))
                    {
                        const Message& SubMsg = F_Ref->GetRepeatedMessage(Msg, FD, i);
                        FillProtoMessageIntoUStruct(SubMsg, ElemStructProp->Struct, ElemPtr);
                    }
                }
                else
                {
                    WritePrimitiveToProperty(SetProp->ElementProp, Msg, ElemPtr, FD, i, true);
                }
            }
            SetHelper.Rehash();
            continue;
        }
        // message
        if (FD->type()==FieldDescriptor::TYPE_MESSAGE && CastField<FStructProperty>(Prop))
        {
            FStructProperty* NestedProp = CastField<FStructProperty>(Prop);
            const Message& SubMsg = F_Ref->GetMessage(Msg, FD);
            FillProtoMessageIntoUStruct(SubMsg, NestedProp->Struct, Prop->ContainerPtrToValuePtr<void>(DestStruct));
            continue;
        }

        WritePrimitiveToProperty(Prop,Msg, Prop->ContainerPtrToValuePtr<void>(DestStruct), FD, 0, false);
    }
    return true;
}

bool ULinkProtobufFunctionLibrary::SetFieldValue(google::protobuf::Message* targetMsg,
                                                const google::protobuf::FieldDescriptor* field, FProperty* Property, const void* containerPtr)
{
	FString PropertyValue;
#if (ENGINE_MAJOR_VERSION > 5) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
	Property->ExportTextItem_Direct(PropertyValue, containerPtr, nullptr, nullptr, PPF_None);
#else
	Property->ExportText_Direct(PropertyValue, containerPtr, nullptr, nullptr, PPF_None);
#endif
	UE_LOG(LogProto, Log, TEXT("Proto Setting Property %s field %s with value %s"), *Property->GetName(), UTF8_TO_TCHAR(field->name().c_str()), *PropertyValue);
	const google::protobuf::Reflection* fieldReflection = targetMsg->GetReflection();
	const bool bIsRepeated = field->is_repeated();
	if (!targetMsg || !field) {
		UE_LOG(LogProto, Error, TEXT("Null target message or field descriptor"));
		UE_LOG(LogProto, Error, TEXT("Proto SetFieldValue FAILED for Property %s field %s"), *Property->GetName(), UTF8_TO_TCHAR(field->name().c_str()));
		return false;
	}
	if (!fieldReflection) {
		UE_LOG(LogProto, Error, TEXT("Null Reflection for target message"));
		UE_LOG(LogProto, Error, TEXT("Proto SetFieldValue FAILED for Property %s field %s"), *Property->GetName(), UTF8_TO_TCHAR(field->name().c_str()));
		return false;
	}
	if (bIsRepeated)
	{
		UE_LOG(LogProto, Log, TEXT("Proto Field %s is repeated"), UTF8_TO_TCHAR(field->name().c_str()));
	}
	// Handle basic types
	bool bSetResult = false;
	auto HandleBasicType = [&](auto ConvertFunc, auto SetFunc, auto AddFunc) {
		if (bIsRepeated) {
			TArray<FString> Values = ParseArrayString(PropertyValue);
			for (const FString& Val : Values) {
				(fieldReflection->*AddFunc)(targetMsg, field, ConvertFunc(*Val));
			}
		}
		else {
			(fieldReflection->*SetFunc)(targetMsg, field, ConvertFunc(*PropertyValue));
		}
		bSetResult = true;
		};

	auto HandleNestedStruct = [&]() {
		if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			const UScriptStruct* InnerStruct = StructProp->Struct;
			google::protobuf::Message* nestedMsg = fieldReflection->MutableMessage(targetMsg, field);
			if (!nestedMsg) {
				UE_LOG(LogProto, Error, TEXT("Proto Failed to create mutable message for nested struct"));
				return false;
			}
			if (!DeserializeStructToMessage(const_cast<UScriptStruct*>(InnerStruct), containerPtr, *nestedMsg, const_cast<google::protobuf::FieldDescriptor*>(field)))
			{
				UE_LOG(LogProto, Error, TEXT("Proto Failed to fill nested message recursively"));
				fieldReflection->ClearField(targetMsg, field);
				return false;
			}
			return true;
		}
		return false;
		};

	switch (field->type()) {
	case FieldDescriptor::TYPE_INT32:
		if (bIsRepeated) {
			TArray<FString> Values = ParseArrayString(PropertyValue);
			for (const FString& Val : Values) {
				fieldReflection->AddInt32(targetMsg, field, FCString::Atoi(*Val));
			}
		}
		else {
			fieldReflection->SetInt32(targetMsg, field, FCString::Atoi(*PropertyValue));
		}
		bSetResult = true;
		break;
	case FieldDescriptor::TYPE_INT64:
		HandleBasicType(FCString::Atoi64, &Reflection::SetInt64, &Reflection::AddInt64);
		break;
	case FieldDescriptor::TYPE_UINT32:
		HandleBasicType([](const TCHAR* Str) { return static_cast<uint32>(FCString::Strtoui64(Str, nullptr, 10)); }, &Reflection::SetUInt32, &Reflection::AddUInt32);
		break;
	case FieldDescriptor::TYPE_UINT64:
		HandleBasicType([](const TCHAR* Str) { return FCString::Strtoui64(Str, nullptr, 10); },
			&Reflection::SetUInt64, &Reflection::AddUInt64);
		break;
	case FieldDescriptor::TYPE_FLOAT:
		HandleBasicType(FCString::Atof, &Reflection::SetFloat, &Reflection::AddFloat);
		break;
	case FieldDescriptor::TYPE_DOUBLE:
		HandleBasicType(FCString::Atod, &Reflection::SetDouble, &Reflection::AddDouble);
		break;
	case FieldDescriptor::TYPE_BOOL:
		HandleBasicType([](const TCHAR* Str) {FString TempStr(Str); return TempStr.ToBool(); }, &Reflection::SetBool, &Reflection::AddBool);
		break;
	case FieldDescriptor::TYPE_STRING:
		if (bIsRepeated) {
			TArray<FString> Values = ParseArrayString(PropertyValue);
			for (const FString& Val : Values) {
				fieldReflection->AddString(targetMsg, field, TCHAR_TO_UTF8(*Val));
			}
		}
		else {
			fieldReflection->SetString(targetMsg, field, TCHAR_TO_UTF8(*PropertyValue));
		}
		bSetResult = true;
		break;
	case FieldDescriptor::TYPE_BYTES:
		if (bIsRepeated)
		{
			TArray<FString> Values = ParseArrayString(PropertyValue);
			for (const FString& Val : Values)
			{
				std::string BytesData = TCHAR_TO_UTF8(*Val);
				fieldReflection->AddString(targetMsg, field, BytesData);
			}
		}
		else
		{
			std::string BytesData = TCHAR_TO_UTF8(*PropertyValue);
			fieldReflection->SetString(targetMsg, field, BytesData);
		}
		bSetResult = true;
		break;
	case FieldDescriptor::TYPE_ENUM:
		if (bIsRepeated) {
			TArray<FString> Values = ParseArrayString(PropertyValue);
			for (const FString& Val : Values) {
				FString EnumStr = Val;
				const google::protobuf::EnumDescriptor* EnumDesc = field->enum_type();
				const google::protobuf::EnumValueDescriptor* EnumValue = EnumDesc->FindValueByName(TCHAR_TO_UTF8(*EnumStr));
				if (!EnumValue) {
					int32 EnumInt = FCString::Atoi(*Val);
					EnumValue = EnumDesc->FindValueByNumber(EnumInt);
				}
				fieldReflection->AddEnum(targetMsg, field, EnumValue);
			}
		}
		else {
			FString EnumStr = PropertyValue;
			const google::protobuf::EnumDescriptor* EnumDesc = field->enum_type();
			const google::protobuf::EnumValueDescriptor* EnumValue = EnumDesc->FindValueByName(TCHAR_TO_UTF8(*EnumStr));
			if (!EnumValue) {
				int32 EnumInt = FCString::Atoi(*PropertyValue);
				EnumValue = EnumDesc->FindValueByNumber(EnumInt);
			}
			fieldReflection->SetEnum(targetMsg, field, EnumValue);
		}
		bSetResult = true;
		break;
	case FieldDescriptor::TYPE_MESSAGE:
		bSetResult = HandleNestedStruct();
		break;
	default:
		UE_LOG(LogProto, Warning, TEXT("Proto Unhandled field type: %d"), field->type());
		UE_LOG(LogProto, Error, TEXT("Proto SetFieldValue FAILED for Property %s field %s"), *Property->GetName(), UTF8_TO_TCHAR(field->name().c_str()));
		return false;
	}

	UE_LOG(LogProto, Log, TEXT("Proto SetFieldValue SUCCESS for Property %s field %s value: %s"), *Property->GetName(), UTF8_TO_TCHAR(field->name().c_str()), *PropertyValue);

	return bSetResult;
}



EProto3Type ULinkProtobufFunctionLibrary::AssignProtoType(const FProperty* InProp)
{
	EProto3Type ProtoType = EProto3Type::Error;
	if (CastField<FDoubleProperty>(InProp))
	{
		ProtoType = EProto3Type::Double;
	}
	else if (CastField<FFloatProperty>(InProp))
	{
		ProtoType = EProto3Type::Float;
	}
	else if (CastField<FIntProperty>(InProp))
	{
		ProtoType = EProto3Type::Int32;
	}
	else if (CastField<FInt64Property>(InProp))
	{
		ProtoType = EProto3Type::Int64;
	}
	else if (CastField<FUInt32Property>(InProp))
	{
		ProtoType = EProto3Type::Uint32;
	}
	else if (CastField<FUInt64Property>(InProp))
	{
		ProtoType = EProto3Type::Uint64;
	}
	else if (CastField<FBoolProperty>(InProp))
	{
		ProtoType = EProto3Type::Bool;
	}
	else if (CastField<FStrProperty>(InProp) || CastField<FNameProperty>(InProp)||CastField<FTextProperty>(InProp))
	{
		ProtoType = EProto3Type::String;
	}
	else if (CastField<FByteProperty>(InProp))
	{
		ProtoType = EProto3Type::Bytes;
	}
	else if (CastField<FStructProperty>(InProp))
	{
		ProtoType= EProto3Type::Message;
	}
	else if (CastField<FEnumProperty>(InProp))
	{
		ProtoType = EProto3Type::Enum;
	}
	else{
		//TODO Other unsupported types
		UE_LOG(LogProto, Error, TEXT("Proto GenerateProtoFileFromUStruct: Unsupported property type for %s"), *InProp->GetName());
	}
	return ProtoType;
}

FString ULinkProtobufFunctionLibrary::ProtoStringAssignProp(const FProperty* InProp)
{
	FString ProtoTypeName = TEXT("unknown");
	if (CastField<FDoubleProperty>(InProp))
	{
		 return TEXT("double");
	}
	else if (CastField<FFloatProperty>(InProp))
	{
		return TEXT("float");
	}
	else if (CastField<FIntProperty>(InProp))
	{
		return TEXT("int32");
	}
	else if (CastField<FInt64Property>(InProp))
	{
		return TEXT("int64");
	}
	else if (CastField<FUInt32Property>(InProp))
	{
		return TEXT("uint32");
	}
	else if (CastField<FUInt64Property>(InProp))
	{
		return TEXT("uint64");
	}
	else if (CastField<FBoolProperty>(InProp))
	{
		return TEXT("bool");
	}
	else if (CastField<FStrProperty>(InProp) || CastField<FNameProperty>(InProp)||CastField<FTextProperty>(InProp))
	{
		return TEXT("string");
	}
	else if (CastField<FByteProperty>(InProp))
	{
		return TEXT("bytes");
	}
	else if (const FStructProperty* StructProperty=CastField<FStructProperty>(InProp))
	{
		return GetPureNameOfProperty(StructProperty);
	}
	else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(InProp))
	{
		return EnumProperty->GetEnum()->GetName();
	}
	else
	{
		return ProtoTypeName;
	}
}

FString ULinkProtobufFunctionLibrary::GetPureNameOfProperty(const FProperty* InProp)
{
	if (!InProp)
	{
		return TEXT("InvalidProperty");
	}
	return  InProp->GetAuthoredName().Replace(TEXT(" "), TEXT(""));
}



void ULinkProtobufFunctionLibrary::ClearArenaCache(google::protobuf::Arena*ArenaPtr)
{
	if (ArenaPtr)
	{
		delete ArenaPtr;
		ArenaPtr = nullptr;
	}
}



#undef LOCTEXT_NAMESPACE
