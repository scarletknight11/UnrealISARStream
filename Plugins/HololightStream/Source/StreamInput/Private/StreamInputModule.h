/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#pragma once

#include "StreamInputCommon.h"

#include "IInputDeviceModule.h"

#include "FStreamInput.h"

class FStreamInputModule : public IInputDeviceModule
{
public:
	FStreamInputModule() = default;
	~FStreamInputModule() override = default;

	void StartupModule() override;
	TSharedPtr<class IInputDevice> CreateInputDevice(
		const TSharedRef<FGenericApplicationMessageHandler>& inMessageHandler) override;

private:
	TSharedPtr<FStreamInput> m_inputDevice;

	void AddKeys();
};