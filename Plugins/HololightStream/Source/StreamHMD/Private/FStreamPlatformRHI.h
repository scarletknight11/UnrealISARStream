/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_FSTREAMPLATFORMRHI_H
#define HOLOLIGHT_UNREAL_FSTREAMPLATFORMRHI_H

#include "HAL/Platform.h"

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#define XR_USE_PLATFORM_WIN32		1
#define XR_USE_GRAPHICS_API_D3D11	1
#define XR_USE_GRAPHICS_API_D3D12	1
#endif

#if PLATFORM_WINDOWS
#define XR_USE_GRAPHICS_API_OPENGL	1
#endif

#if PLATFORM_ANDROID
#define XR_USE_PLATFORM_ANDROID 1
#define XR_USE_GRAPHICS_API_OPENGL_ES	1
#endif

#if PLATFORM_WINDOWS || PLATFORM_ANDROID || PLATFORM_LINUX
#define XR_USE_GRAPHICS_API_VULKAN 1
#endif

//-------------------------------------------------------------------------------------------------
// D3D11
//-------------------------------------------------------------------------------------------------

#ifdef XR_USE_GRAPHICS_API_D3D11
#include "ID3D11DynamicRHI.h"
#endif // XR_USE_GRAPHICS_API_D3D11

//-------------------------------------------------------------------------------------------------
// D3D12
//-------------------------------------------------------------------------------------------------

#ifdef XR_USE_GRAPHICS_API_D3D12
#include "ID3D12DynamicRHI.h"
#endif // XR_USE_GRAPHICS_API_D3D12

#endif // HOLOLIGHT_UNREAL_FSTREAMPLATFORMRHI_H