/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef GRAPHICS_API_CONFIG_H
#define GRAPHICS_API_CONFIG_H

#include <stdint.h>

#ifdef _WIN32
#include <d3d11.h>
#include <d3d12.h>
#endif

#include "isar_api.h"
#include "isar/input_types.h"

// Cross-platform Graphics API structs
// -----------------------------------------------------------------------------

ISAR_CPP_NS_BEGIN

typedef enum IsarGraphicsApiType{
	IsarGraphicsApiType_D3D11,
	IsarGraphicsApiType_D3D12,
	IsarGraphicsApiType_COUNT,
	IsarGraphicsApiType_MAX = 0xFFFFFFFF,
} IsarGraphicsApiType;

typedef enum IsarTextureFormat{
	IsarTextureFormat_RGBA32,
	IsarTextureFormat_NV12,
	IsarTextureFormat_P010,

	IsarTextureFormat_Count,
	IsarTextureFormat_MAX = 0xFFFFFFFF,
} IsarTextureFormat;

typedef struct IsarFocusPlane{
	IsarVector3 position;
	IsarVector3 normal;
	IsarVector3 velocity;
} IsarFocusPlane;

typedef struct IsarFrameInfo{
	IsarXrPose pose;
	IsarTextureFormat textureFormat;

	float zNear;
	float zFar;

	uint32_t hasFocusPlane;
	IsarFocusPlane focusPlane;

	bool passthroughEnabled;
} IsarFrameInfo;

typedef struct IsarD3D11GraphicsApiFrame {
	ID3D11Texture2D* frame;
	ID3D11Texture2D* depthFrame;
	uint32_t subresourceIndex;
} IsarD3D11GraphicsApiFrame;

typedef struct IsarD3D12GraphicsApiFrame {
	ID3D12Resource* frame;
	ID3D12Resource* depthFrame;
	uint32_t subresourceIndex;
	uint64_t frameFenceValue;
} IsarD3D12GraphicsApiFrame;

typedef struct IsarGraphicsApiFrame {
	IsarFrameInfo info;
	IsarGraphicsApiType graphicsApiType;
	union
	{
		IsarD3D11GraphicsApiFrame d3d11;
		IsarD3D12GraphicsApiFrame d3d12;
	};
} IsarGraphicsApiFrame;

typedef struct IsarD3D11GraphicsApiConfig{
	ID3D11Device* device;
} IsarD3D11GraphicsApiConfig;

typedef struct IsarD3D12GraphicsApiConfig{
	ID3D12Device* device;
	ID3D12CommandQueue* commandQueue;
	ID3D12Fence* fence;
} IsarD3D12GraphicsApiConfig;

typedef struct IsarGraphicsApiConfig {
	IsarGraphicsApiType graphicsApiType;
	union
	{
		IsarD3D11GraphicsApiConfig d3d11;
		IsarD3D12GraphicsApiConfig d3d12;
	};
} IsarGraphicsApiConfig;


ISAR_CPP_NS_END

#endif  // GRAPHICS_API_CONFIG_H
