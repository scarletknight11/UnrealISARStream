/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_FSTREAMRENDERBRIDGE_H
#define HOLOLIGHT_UNREAL_FSTREAMRENDERBRIDGE_H

#include "CoreMinimal.h"
#include "XRRenderBridge.h"
#include "XRSwapChain.h"

class FStreamHMD;
class FRHICommandListImmediate;

class STREAMHMD_API FStreamRenderBridge : public FXRRenderBridge
{
public:
	FStreamRenderBridge();
	~FStreamRenderBridge() override = default;

	void SetStreamHMD(FStreamHMD* inHMD) { m_streamHMD = inHMD; }
	virtual FXRSwapChainPtr CreateSwapchain(uint8 format, uint8& outActualFormat, uint32 sizeX,
											uint32 sizeY, uint32 arraySize, uint32 numMips, uint32 numSamples,
											ETextureCreateFlags createFlags,
											const FClearValueBinding& clearValueBinding,
											ETextureCreateFlags auxiliaryCreateFlags = ETextureCreateFlags::None) = 0;

	FXRSwapChainPtr CreateSwapchain(FRHITexture* texture, ETextureCreateFlags createFlags)
	{
		if (!texture)
		{
			return nullptr;
		}

		uint8 unusedOutFormat = 0;
		return CreateSwapchain(texture->GetFormat(),
							   unusedOutFormat,
							   texture->GetSizeX(),
							   texture->GetSizeY(),
							   1,
							   texture->GetNumMips(),
							   texture->GetNumSamples(),
							   texture->GetFlags() | createFlags,
							   texture->GetClearBinding());
	}

	/** FRHICustomPresent */
	bool Present(int32& inOutSyncInterval) override;
	void BeginDrawing() override;

	virtual bool Support10BitSwapchain() const { return false; }

	virtual bool HDRGetMetaDataForStereo(EDisplayOutputFormat& outDisplayOutputFormat,
										 EDisplayColorGamut& outDisplayColorGamut, bool& outbHDRSupported)
	{
		return false;
	}

	virtual void SetSkipRate(uint32 skipRate)
	{
	}

	virtual void HMDOnFinishRendering_RHIThread();

protected:
	FStreamHMD* m_streamHMD;
};

FStreamRenderBridge* CreateRenderBridge_D3D11();
FStreamRenderBridge* CreateRenderBridge_D3D12();

#endif // HOLOLIGHT_UNREAL_FSTREAMRENDERBRIDGE_H