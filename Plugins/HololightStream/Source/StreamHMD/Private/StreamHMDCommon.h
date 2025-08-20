/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

// This header is created to ensure Stream headers (mainly Windows headers) are included after Unreal Engine headers,
// so the definitions inside Windows headers do not break Unreal definitions

#ifndef HOLOLIGHT_UNREAL_STREAMHMDCOMMON_H
#define HOLOLIGHT_UNREAL_STREAMHMDCOMMON_H

// Unreal Engine Includes
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "ScreenRendering.h"
#include "HeadMountedDisplayBase.h"
#include "HeadMountedDisplayTypes.h"
#include "AudioMixerDevice.h"

// UE Slate Includes
#include "Slate/SceneViewport.h"

// Stream Includes
#include "isar/types.h"
#include "isar/server_api.h"
#include "isar/graphics_api_config.h"
#include "streamxr.h"
#include "StreamCore.h"

#endif // HOLOLIGHT_UNREAL_STREAMHMDCOMMON_H