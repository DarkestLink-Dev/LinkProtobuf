// Copyright DarkestLink-Dev 2025 All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"

class FLinkProtobufEditorSettingsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
private:
	FReply OnResetProtobufGenPathClicked();
	FReply OnResetProtocExecPathClicked();
	FReply OnButtonGenerateProtoFromSettings();
	TWeakObjectPtr<UObject> EditingObjPtr;
};


