/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_SERVER_TYPES_H
#define ISAR_SERVER_TYPES_H

#include "isar/types.h"

ISAR_CPP_NS_BEGIN

// Callbacks
// -----------------------------------------------------------------------------

/// @brief Callback when audio has been received from the client
/// @param audioData A pointer to the audio data received
/// @param userData User data passed when registering the callback
typedef void(*IsarServerAudioDataReceivedCallback)(IsarAudioData const* audioData, void* userData);

/// @brief Callback when connection statistics have been generated
/// @param statsData A pointer to the statistics data. To interpret this data, the functions exposed from stats_api.h must be used
/// @param userData User data passed when registering the callback
typedef void(*IsarServerStatsCallback)(const void* statsData, void* userData);

ISAR_CPP_NS_END

#endif  // ISAR_SERVER_TYPES_H
