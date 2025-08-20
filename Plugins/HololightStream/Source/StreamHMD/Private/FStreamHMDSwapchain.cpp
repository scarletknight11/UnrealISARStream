/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "FStreamHMDSwapchain.h"

#include "PixelFormat.h"

#if PLATFORM_WINDOWS
#include "ID3D11DynamicRHI.h"
#include "ID3D12DynamicRHI.h"
#endif

#include <d3d12.h>

static TAutoConsoleVariable<int32> CVarStreamSwapchainRetryCount(
	TEXT("vr.StreamSwapchainRetryCount"),
	9,
	TEXT("Number of times the Stream plugin will attempt to wait for the next swapchain image."),
	ECVF_RenderThreadSafe);

FStreamXRSwapchain::FStreamXRSwapchain(TArray<FTextureRHIRef>&& inRHITextureSwapChain,
									   const FTextureRHIRef& inRHITexture,
									   XrSwapchain inHandle) : FXRSwapChain(MoveTemp(inRHITextureSwapChain),
																			inRHITexture),
															   m_handle(inHandle),
															   m_imageAcquired(false),
															   m_imageReady(false)
{
}

void FStreamXRSwapchain::IncrementSwapChainIndex_RHIThread()
{
	bool wasAcquired = false;
	m_imageAcquired.compare_exchange_strong(wasAcquired, true);
	SCOPED_NAMED_EVENT(AcquireImage, FColor::Red);
	uint32_t swapChainIndex = 0;
	RHITexture = RHITextureSwapChain[swapChainIndex];
	SwapChainIndex_RHIThread = swapChainIndex;
	UE_LOG(LogHMD, VeryVerbose,
		   TEXT(
			   "FStreamSwapchain::IncrementSwapChainIndex_RHIThread() Acquired image %d in swapchain %p metal texture: 0x%x"
		   ), swapChainIndex, reinterpret_cast<const void*>(m_handle), RHITexture.GetReference()->GetNativeResource());
}

uint8 FStreamXRSwapchain::GetNearestSupportedSwapchainFormat(uint8 requestedFormat,
															 TFunction<uint32(uint8)> toPlatformFormat /*= nullptr*/)
{
	// We only support R8 G8 B8 A8 for server
	return (toPlatformFormat(PF_R8G8B8A8) == requestedFormat) ? requestedFormat : PF_Unknown;
}

FXRSwapChainPtr CreateSwapchain_D3D11(uint8 format, uint8& outActualFormat, uint32 sizeX, uint32 sizeY,
									  uint32 arraySize, uint32 numMips, uint32 numSamples,
									  ETextureCreateFlags createFlags, const FClearValueBinding& clearValueBinding,
									  ETextureCreateFlags auxiliaryCreateFlags)
{
	TFunction<uint32(uint8)> toPlatformFormat = [](uint8 inFormat)
	{
		return GetID3D11DynamicRHI()->RHIGetSwapChainFormat(static_cast<EPixelFormat>(inFormat));
	};

	outActualFormat = format;
	XrSwapchain swapchain = 0;
	ID3D11DynamicRHI* d3d11RHI = GetID3D11DynamicRHI();
	ID3D11Texture2D* pTexture = nullptr;
	TArray<FTextureRHIRef> textureChain;
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = sizeX; // 2880 936 HoloLens 1 resolution, for HoloLens 2, it is 1440 936 // 4800 2400
	textureDesc.Height = sizeY; // HoloLens 1 aspect ratio
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	HRESULT hr = d3d11RHI->RHIGetDevice()->CreateTexture2D(&textureDesc, nullptr, &pTexture);
	if (hr != S_OK || !pTexture)
	{
		UE_LOG(LogTemp, Log, TEXT("Error:Failed to create texture for swapchain "));
		return FXRSwapChainPtr();
	}
	textureChain.Add(
		d3d11RHI->RHICreateTexture2DFromResource(EPixelFormat::PF_R8G8B8A8, createFlags, clearValueBinding, pTexture));

	return CreateXRSwapChain<FStreamXRSwapchain>(MoveTemp(textureChain), (FTextureRHIRef&)textureChain[0], swapchain);
	// For now no depth texture
}


FXRSwapChainPtr CreateSwapchain_D3D12(uint8 format, uint8& outActualFormat, uint32 sizeX, uint32 sizeY,
									  uint32 arraySize, uint32 numMips, uint32 numSamples,
									  ETextureCreateFlags createFlags, const FClearValueBinding& clearValueBinding,
									  ETextureCreateFlags auxiliaryCreateFlags)
{
	TFunction<uint32(uint8)> toPlatformFormat = [](uint8 inFormat)
	{
		return GetID3D12DynamicRHI()->RHIGetSwapChainFormat(static_cast<EPixelFormat>(inFormat));
	};

	outActualFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	XrSwapchain swapchain = 0;
	ID3D12DynamicRHI* d3d12RHI = GetID3D12DynamicRHI();
	TArray<FTextureRHIRef> textureChain;
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Color[0] = 1.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 0.0f;

	// Create a texture
	// Get the texture from Client
	D3D12_RESOURCE_DESC textureDesc = {};
	ID3D12Resource* pTexture = nullptr;
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Alignment = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Width = sizeX;
	textureDesc.Height = sizeY;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = GetID3D12DynamicRHI()->RHIGetDevice(0)->
		GetResourceAllocationInfo(0, 1, &textureDesc);
	ID3D12Device* pID3D12Device = GetID3D12DynamicRHI()->RHIGetDevice(0);
	HRESULT hr = pID3D12Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&pTexture));

	if (FAILED(hr) || S_FALSE == hr)
	{
		// Handle the error (e.g., log it or throw an exception)
		pTexture = nullptr;
		return FXRSwapChainPtr();
	}
	textureChain.Add(static_cast<FTextureRHIRef>((d3d12RHI->RHICreateTexture2DFromResource(EPixelFormat::PF_R8G8B8A8,
		createFlags, clearValueBinding, pTexture))));
	return CreateXRSwapChain<FStreamXRSwapchain>(MoveTemp(textureChain), (FTextureRHIRef&)textureChain[0], swapchain);
}