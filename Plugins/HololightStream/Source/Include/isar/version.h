/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_VERSION_H
#define ISAR_VERSION_H

#include "isar/isar_api.h"

// Create packed unsigned int with semantic version
#define ISAR_MAKE_PACKED_VERSION(major, minor, patch) \
    (((major) << 20) | ((minor) << 10) | (patch))

// This is the current version of the library
// change this value to increase the version
#define ISAR_LATEST_VERSION ISAR_MAKE_PACKED_VERSION(2025, 0, 0)

#define ISAR_NEXT_MAJOR_VERSION ISAR_LATEST_VERSION + ISAR_MAKE_PACKED_VERSION(1, 0, 0)
#define ISAR_NEXT_MINOR_VERSION ISAR_LATEST_VERSION + ISAR_MAKE_PACKED_VERSION(0, 1, 0)
#define ISAR_NEXT_PATCH_VERSION ISAR_LATEST_VERSION + ISAR_MAKE_PACKED_VERSION(0, 0, 1)

#define ISAR_INVALID_VERSION 0

// major 12 bits, minor 10 bits, patch 10 bits
#define ISAR_GET_VERSION_MAJOR(version) ((version) >> 20)
#define ISAR_GET_VERSION_MINOR(version) (((version) >> 10) & 0x3ff)
#define ISAR_GET_VERSION_PATCH(version) ((version) & 0x3FF)

ISAR_CPP_NS_BEGIN

// Type for representing api versions.
// The api version is represented as packed integer value.
// major 12 bits, minor 10 bits, patch 10 bits
typedef unsigned int IsarVersion;

ISAR_CPP_NS_END

#endif  // ISAR_VERSION_H
