/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

// This header is created to ensure Stream headers (mainly Windows headers) are included after Unreal Engine headers,
// so the definitions inside Windows headers do not break Unreal definitions

#ifndef HOLOLIGHT_UNREAL_STREAMHMDCOMMON_H
#define HOLOLIGHT_UNREAL_STREAMHMDCOMMON_H

// Unreal Engine Includes
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"

#include "StreamControllerStateHandler.h"
#include "IStreamHMD.h"

// Stream Includes
#include "StreamCore.h"
#include "isar/server_api.h"
#include "isar/types.h"
#include "isar/input_types.h"

#endif // HOLOLIGHT_UNREAL_STREAMHMDCOMMON_H