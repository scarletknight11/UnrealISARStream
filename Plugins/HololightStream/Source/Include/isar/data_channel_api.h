/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_DATA_CHANNEL_API_H
#define ISAR_DATA_CHANNEL_API_H

#include "isar/isar_api.h"
#include "isar/types.h"
#include "isar/version.h"

ISAR_CPP_NS_BEGIN

// Provider
// -----------------------------------------------------------------------------

/// @brief Implement this interface for your data channel provider.
/// @details A number of function pointers to be populated by your channel provider to be called from within ISAR
typedef struct IsarDataChannelProvider {
    /// @brief User data which will be passed on each function call
    void* userData;

    /// @brief Check to confirm whether the remote version of the channel is supported. This will only be called
    ///        on channels created with a client connection
    /// @param userData User data
    /// @return Indicates whether the remote description is supported by this provider
    bool (*IsRemoteSupported)(void* userData, IsarChannelDescription remoteDescription);

    /// @brief Call to indicate whether the channel is supported by the remote endpoint
    /// @param userData User data
    /// @param supported True if the channel is supported
    void (*OnSupportedChanged)(void* userData, bool supported);

    /// @brief Call to indicate whether channel's connection state has changed
    /// @param userData User data
    /// @param connected True if connected, false if not
    void (*OnConnectedChanged)(void* userData, bool connected);

    /// @brief Call to receive data from the remote endpoint
    /// @param userData   User data
    /// @param data       Byte buffer containing data
    /// @param size       Size of the byte buffer
    void (*OnDataReceived)(void* userData, const uint8_t* data, uint32_t size);
} IsarDataChannelProvider;

// Function Pointers
// -----------------------------------------------------------------------------

/// @brief Create a data channel for sending data to the remote connection. MUST be called prior to StartConnection.
/// @param connection           [in]: ISAR connection pointer
/// @param provider             [in]: Reference to a struct of functions which will be used by ISAR to interact provider
/// @param channelDescription   [in]: Description for creating the channel
/// @param dataChannel          [out]: Data channel pointer. To be passed back into data channel functions functions.
/// @return eNone - Successfully created the channel.    \n
///         eInvalidArgument - connection is invalid.  \n
///         eAlreadyConnected - Already connected to a remote connection. This function can only be called prior to StartConnection. \n
///         eDataChannel_AlreadyExists - The data channel with description name already exists. \n
///         eDataChannel_Creation - Failed to create the channel.    \n
typedef IsarError (*IsarDataChannelCreate)(IsarConnection connection,
                                        const IsarChannelDescription channelDescription,
                                        const IsarDataChannelProvider provider,
                                        IsarDataChannel* dataChannel);

/// @brief Try to open the data channel to begin sending and receiving data messages. The channel will be officially open when the
///        OnConnectedChanged provider callback is called. This call is only supported by data channels created with server connection
///        types.
/// @param dataChannel [in]: ISAR data channel pointer
/// @return eNone - Successfully opened the channel.    \n
///         eInvalidArgument - dataChannel is invalid.  \n
///         eDataChannel_Unsupported - The channel is not supported by the remote connection. \n
///         eDataChannel_InvalidConnection - The connection type does not support this call. \n
///         eDataChannel_Open - The channel could not be opened. \n
typedef IsarError (*IsarDataChannelOpen)(IsarDataChannel dataChannel);

/// @brief Create a data channel for sending data to the remote connection
/// @param dataChannel      [in]: Data channel pointer
/// @param provider         [in]: Reference to a struct of functions which will be used by ISAR to interact provider
/// @return eNone - Successfully queued data for sending to the remote endpoint.    \n
///         eInvalidArgument - dataChannel is invalid.  \n
///         eDataChannel_Unsupported - The channel is not supported by the remote connection. \n
///         eNotConnected - The data channel is not yet connected. \n
///         eDataChannel_MessageTooLong - The channel was specified as not requiring large messages but the message exceeds the limit. \n
typedef IsarError (*IsarDataChannelPushData)(IsarDataChannel dataChannel, const uint8_t* buffer, uint32_t size);

/// @brief Close the data channel to stop sending and receiving messages
/// @param dataChannel      [in]: Data channel pointer
/// @return eNone - Successfully closed the channel.    \n
///         eInvalidArgument - dataChannel is invalid.  \n
///         eNotConnected - The data channel is not yet connected. \n
typedef IsarError (*IsarDataChannelClose)(IsarDataChannel dataChannel);

/// @brief Destroy data channel to no longer be established with the remote connection
/// @param dataChannel      [in]: Data channel pointer
/// @return eNone - Successfully destroyed the channel.    \n
///         eInvalidArgument - dataChannel is invalid.  \n
typedef IsarError (*IsarDataChannelDestroy)(IsarDataChannel* dataChannel);

// API
// -----------------------------------------------------------------------------

typedef struct IsarDataChannelApi{
    IsarDataChannelCreate create;
    IsarDataChannelOpen open;
    IsarDataChannelPushData pushData;
    IsarDataChannelClose close;
    IsarDataChannelDestroy destroy;
} IsarDataChannelApi;

/// @brief Instantiate the data channel api for managing external data channels.
/// @param api  [in/out]: The API function pointers to populate
/// @return eNone - Successfully populated the api.    \n
///         eInvalidArgument - api has not been instantiated.  \n
ISAR_API(IsarError) Isar_DataChannel_CreateApi(IsarDataChannelApi* api);

ISAR_CPP_NS_END

#endif // ISAR_DATA_CHANNEL_API_H
