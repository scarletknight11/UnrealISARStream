/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_FSTREAMHMDSWAPCHAIN_H
#define HOLOLIGHT_UNREAL_FSTREAMHMDSWAPCHAIN_H

#include "StreamHMDCommon.h"

#include "XRSwapChain.h"

class FStreamXRSwapchain : public FXRSwapChain
{
public:
	FStreamXRSwapchain(TArray<FTextureRHIRef>&& inRHITextureSwapChain, const FTextureRHIRef& inRHITexture,
					   XrSwapchain inHandle);
	~FStreamXRSwapchain() override = default;

	void IncrementSwapChainIndex_RHIThread() override final;
	XrSwapchain GetHandle() { return m_handle; }
	static uint8 GetNearestSupportedSwapchainFormat(uint8 requestedFormat,
													TFunction<uint32(uint8)> toPlatformFormat = nullptr);

protected:
	XrSwapchain m_handle;
	/** Whether the image associated with the swapchain has been acquired. */
	std::atomic<bool> m_imageAcquired;
	/** Whether the image associated with the swapchain is ready for being written to. */
	std::atomic<bool> m_imageReady;
};

FXRSwapChainPtr CreateSwapchain_D3D11(uint8 format, uint8& outActualFormat, uint32 sizeX, uint32 sizeY,
									  uint32 arraySize, uint32 numMips, uint32 numSamples,
									  ETextureCreateFlags createFlags, const FClearValueBinding& clearValueBinding,
									  ETextureCreateFlags auxiliaryCreateFlags = ETextureCreateFlags::None);
FXRSwapChainPtr CreateSwapchain_D3D12(uint8 format, uint8& outActualFormat, uint32 sizeX, uint32 sizeY,
									  uint32 arraySize, uint32 numMips, uint32 numSamples,
									  ETextureCreateFlags createFlags, const FClearValueBinding& clearValueBinding,
									  ETextureCreateFlags auxiliaryCreateFlags = ETextureCreateFlags::None);

#endif // HOLOLIGHT_UNREAL_FSTREAMHMDSWAPCHAIN_H