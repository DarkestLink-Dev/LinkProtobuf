// Copyright DarkestLink-Dev 2025 All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "IAssetTools.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProtoEditor, Log, All);

static EAssetTypeCategories::Type GProtobufCategory = EAssetTypeCategories::None;

class FLinkProtobufEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};


