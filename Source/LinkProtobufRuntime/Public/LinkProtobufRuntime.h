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
#include "Modules/ModuleManager.h"
#include "LinkProtobufRuntime.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProto, Log, All);


UENUM(BlueprintType)
enum class EProto3Type : uint8
{
	Error,
	Double,
	Float,
	Int32,
	Int64,
	Uint32,
	Uint64,
	Bool,
	String,
	Bytes,
	Enum,
	Message
};

UENUM(BlueprintType)
enum class EUnrealType : uint8
{
	Normal,
	Enum,
	Struct,
	Array,
	Map,
	Set
};

class FLinkProtobufRuntimeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
