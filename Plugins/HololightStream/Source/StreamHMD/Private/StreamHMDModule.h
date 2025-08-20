/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_STREAMHMDMODULE_H
#define HOLOLIGHT_UNREAL_STREAMHMDMODULE_H

#include "IStreamHMDModule.h"
#include "FStreamHMD.h"

class IXRTrackingSystem;

/*
* This module allows HMD input to be used with HL streaming
*/
class FStreamHMDModule : public IStreamHMDModule
{
public:
	FStreamHMDModule();
	FStreamHMD* GetStreamHMD() const override;

	/** IModuleInterface implementation */
	void StartupModule() override;
	void ShutdownModule() override;
	/** End IModuleInterface implementation */

private:
	/** IHeadMountedDisplayModule implementation */
	TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> CreateTrackingSystem() override;
	FString GetModuleKeyName() const override { return FString(TEXT("StreamHMD")); }
	bool IsHMDConnected() override { return true; }
	/** End IHeadMountedDisplayModule implementation */

	bool InitRenderBridge();
	TRefCountPtr<FStreamRenderBridge> m_renderBridge;
};

#endif // HOLOLIGHT_UNREAL_STREAMHMDMODULE_H