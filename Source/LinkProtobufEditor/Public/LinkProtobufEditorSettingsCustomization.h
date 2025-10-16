// Copyright 2025 DarkestLink-Dev

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


