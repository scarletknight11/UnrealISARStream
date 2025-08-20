/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_CLIENT_TYPES_H
#define ISAR_CLIENT_TYPES_H

#include "isar/types.h"

ISAR_CPP_NS_BEGIN

// Callbacks
// -----------------------------------------------------------------------------

/// @brief Callback when a frame has been received from the server
/// @param frame The frame received along with timestamp of the pose used to render the frame
/// @param userData User data passed when registering the callback
typedef void (*IsarClientFrameCallback)(IsarGraphicsApiFrame frame, void* userData);

ISAR_CPP_NS_END

#endif // ISAR_CLIENT_TYPES_H
