/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_ISTREAMHMD_H
#define HOLOLIGHT_UNREAL_ISTREAMHMD_H

#include "HeadMountedDisplayBase.h"
#include "InputCoreTypes.h"

#include <functional>
#include <string>

#include "IStreamExtension.h"

struct DeviceInfo
{
	int32_t deviceId;
	std::string deviceName;
	FVector position;
	FQuat orientation;
};

class STREAMHMD_API IStreamHMD : public FHeadMountedDisplayBase
{
public:
	IStreamHMD(IARSystemSupport* inARImplementation) : FHeadMountedDisplayBase(inARImplementation)
	{
	};

	virtual void SetInputModule(IStreamExtension* streamInput) = 0;
	virtual void SetMicrophoneCaptureStream(IStreamExtension* streamMicrophone) = 0;
	virtual void SetDeviceInfoCallback(const std::function<DeviceInfo(EControllerHand)>& functionPtr) = 0;
};

#endif // HOLOLIGHT_UNREAL_ISTREAMHMD_H