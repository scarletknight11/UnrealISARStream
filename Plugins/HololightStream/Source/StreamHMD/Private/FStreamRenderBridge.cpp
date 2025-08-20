/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "FStreamRenderBridge.h"
#include "FStreamHMD.h"
#include "FStreamHMDSwapChain.h"

#if PLATFORM_WINDOWS
#include "ID3D11DynamicRHI.h"
#include "ID3D12DynamicRHI.h"
#endif

FStreamRenderBridge::FStreamRenderBridge() : m_streamHMD(nullptr)
{
}

bool FStreamRenderBridge::Present(int32& inOutSyncInterval)
{
	bool needsNativePresent = true;
	if (m_streamHMD)
	{
		HMDOnFinishRendering_RHIThread();
		needsNativePresent = !m_streamHMD->IsStandaloneStereoOnlyDevice();
	}
	inOutSyncInterval = 0; // VSync off
	return needsNativePresent;
}

void FStreamRenderBridge::BeginDrawing()
{
}

void FStreamRenderBridge::HMDOnFinishRendering_RHIThread()
{
	if (m_streamHMD)
	{
		m_streamHMD->OnFinishRendering_RHIThread();
	}
}

class FD3D11RenderBridge : public FStreamRenderBridge
{
public:
	FD3D11RenderBridge() : FStreamRenderBridge()
	{
	}

	virtual FXRSwapChainPtr CreateSwapchain(uint8 format, uint8& outActualFormat, uint32 sizeX, uint32 sizeY,
											uint32 arraySize, uint32 numMips, uint32 numSamples,
											ETextureCreateFlags createFlags,
											const FClearValueBinding& clearValueBinding,
											ETextureCreateFlags auxiliaryCreateFlags) override final
	{
		if (IsRHID3D12())
		{
			return CreateSwapchain_D3D12(format, outActualFormat, sizeX, sizeY, arraySize, numMips, numSamples,
										 createFlags, clearValueBinding, auxiliaryCreateFlags);
		}
		return CreateSwapchain_D3D11(format, outActualFormat, sizeX, sizeY, arraySize, numMips, numSamples, createFlags,
									 clearValueBinding, auxiliaryCreateFlags);
	}

	virtual void UpdateViewport(const class FViewport& viewport, class FRHIViewport* inViewportRHI) override
	{
	}

	virtual void OnBackBufferResize() override
	{
	}
};

FStreamRenderBridge* CreateRenderBridge_D3D11() { return new FD3D11RenderBridge(); }

class FD3D12RenderBridge : public FStreamRenderBridge
{
public:
	FD3D12RenderBridge() : FStreamRenderBridge()
	{
	}

	virtual FXRSwapChainPtr CreateSwapchain(uint8 format, uint8& outActualFormat, uint32 sizeX, uint32 sizeY,
											uint32 arraySize, uint32 numMips, uint32 numSamples,
											ETextureCreateFlags createFlags,
											const FClearValueBinding& clearValueBinding,
											ETextureCreateFlags auxiliaryCreateFlags) override final
	{
		return CreateSwapchain_D3D12(format, outActualFormat, sizeX, sizeY, arraySize, numMips, numSamples, createFlags,
									 clearValueBinding, auxiliaryCreateFlags);
	}
};

FStreamRenderBridge* CreateRenderBridge_D3D12() { return new FD3D12RenderBridge(); }
