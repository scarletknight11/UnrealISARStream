/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "StreamInputBlueprintLibrary.h"

#include "Features/IModularFeatures.h"

#include "FStreamInput.h"

inline static FStreamInput* GetStreamInput()
{
	FName motionControllerName("Stream");
	TArray<IMotionController*> motionControllers =
	    IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(
	        IMotionController::GetModularFeatureName());

	for (auto* itr : motionControllers)
	{
		if (itr->GetMotionControllerDeviceTypeName() == motionControllerName)
		{
			return static_cast<FStreamInput*>(itr);
		}
	}

	return nullptr;
}

void UStreamInputBlueprintLibrary::RegisterControllerStateHandler(
    TScriptInterface<IStreamControllerStateHandler> controllerStateHandler)
{
	if (auto* streamInput = GetStreamInput())
	{
		streamInput->RegisterControllerStateHandler(controllerStateHandler);
	}
}

void UStreamInputBlueprintLibrary::UnregisterControllerStateHandler(
    TScriptInterface<IStreamControllerStateHandler> controllerStateHandler)
{
	if (auto* streamInput = GetStreamInput())
	{
		streamInput->UnregisterControllerStateHandler(controllerStateHandler);
	}
}
