/*
 *  Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_CLIENT_API_H
#define ISAR_CLIENT_API_H

#include "isar/client_types.h"
#include "isar/graphics_api_config.h"

ISAR_CPP_NS_BEGIN

// Function Pointers
// -----------------------------------------------------------------------------

/// @brief Try to pull an XR frame received from the server connection
/// @details If successful, the frame will be received on the GPU in NV12 format. It is the reponsibility
/// 		 of the consumer to convert this to a readable format.
/// @param connection [in]: A valid client connection handle
/// @param frame [out]: The populated frame and it's data
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
///			eNoFrame - No frame exists to pull \n
typedef IsarError (*IsarClientPullFrame)(IsarConnection connection, OUT IsarGraphicsApiFrame* frame);

/// @brief Initalize the camera track for sending client captured video
/// @param connection [in]: A valid client connection handle
/// @param gfxConfig [in]: Configuration pertraining to the graphics device and frame information
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
///			eVideoSource - Could not create the video source for passing the captured frame \n
///			eVideoTrack - Could not create the video track for sending the captured frame \n
typedef IsarError (*IsarClientInitCameraTrack)(IsarConnection connection, IsarGraphicsApiConfig gfxConfig);

/// @brief Register a handler for enabling the camera capture
/// @param connection [in]: A valid client connection handle
/// @param cb [in]: The callback to be registered
/// @param userData [in]: User data which will be passed to the callback
typedef void (*IsarClientRegisterCameraCaptureEnabledHandler)(IsarConnection connection, IsarCameraCaptureEnabledCallback cb, void* userData);

/// @brief Unregister a specified handler for enabling the camera capture
/// @param connection [in]: A valid client connection handle
/// @param cb [in]: The callback to be unregistered
/// @param userData [in]: The user data to be unregistered
typedef void (*IsarClientUnregisterCameraCaptureEnabledHandler)(IsarConnection connection, IsarCameraCaptureEnabledCallback cb, void* userData);

/// @brief Send a view pose to the server for rendering
/// @param connection [in]: A valid client connection handle
/// @param pose [in]: The pose data to send
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
///			eDataChannel_Send - Could not send the data to the server \n
typedef IsarError (*IsarClientPushViewPose_Deprecated)(IsarConnection connection, IsarXrPose_Deprecated const* pose);

/// @brief Send a view pose to the server for rendering
/// @param connection [in]: A valid client connection handle
/// @param pose [in]: The pose data to send
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
///			eDataChannel_Send - Could not send the data to the server \n
typedef IsarError (*IsarClientPushViewPose)(IsarConnection connection, IsarXrPose const* pose);

/// @brief Send a spatial input to the server
/// @param connection [in]: A valid client connection handle
/// @param spatialInput [in]: The spatial input data to send
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
///			eDataChannel_Send - Could not send the data to the server \n
typedef IsarError (*IsarClientPushSpatialInput)(IsarConnection connection, IsarSpatialInput const* spatialInput);

/// @brief Send a camera frame to the server
/// @param connection [in]: A valid client connection handle
/// @param frame [in]: The frame data to be sent. This must be on the GPU and only NV12 textures are supported.
/// @param metadata [in]: Additional metadata pertaining to the frame
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
typedef IsarError (*IsarClientPushCameraFrame)(IsarConnection connection, IsarGraphicsApiFrame frame, IsarCameraMetadata const* metadata);


/// @brief Acquire a previously pulled frame from @ref IsarClientPullFrame as a CPU image
/// @details This function works for Win32 builds, for WinUWP this will always return eNoFrame
/// @param connection [in]: A valid client connection handle
/// @param formatToAcquire [in]: The format of the image to acquire
/// @param data [in/out]: Allocated storage for populating with the frame data
/// @param dataSize [in]: The amount of space in bytes allocated in @ref data
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
/// 		eNoFrame - Either a frame was not successfully pulled with @ref IsarClientPullFrame or this function is called in WinUWP build
typedef IsarError (*IsarClientAcquireCpuImage)(IsarConnection connection, IsarTextureFormat formatToAcquire, uint8_t *data, uint32_t dataSize);
// API
// -----------------------------------------------------------------------------

/// @brief Connection specific function pointers
typedef struct IsarClientApi {
	/// @brief The current version of ISAR being used
	IsarVersion version;

	IsarCreateConnection createConnection;
	IsarOpenConnection openConnection;
	IsarCloseConnection closeConnection;
	IsarDestroyConnection destroyConnection;

	IsarRegisterConnectionStateHandler registerConnectionStateHandler;
	IsarUnregisterConnectionStateHandler unregisterConnectionStateHandler;

	IsarGetConnectionInfo getConnectionInfo;

	IsarClientPullFrame pullFrame;

	IsarClientInitCameraTrack initCameraTrack;
	IsarClientPushCameraFrame pushCameraFrame;
	IsarClientRegisterCameraCaptureEnabledHandler registerCameraCaptureEnabledHandler;
	IsarClientUnregisterCameraCaptureEnabledHandler unregisterCameraCaptureEnabledHandler;

	IsarClientPushViewPose_Deprecated pushViewPose_Deprecated;
	IsarClientPushViewPose pushViewPose;
	IsarClientPushSpatialInput pushSpatialInput;
	IsarClientAcquireCpuImage acquireCpuImage;
} IsarClientApi;

/// @brief Creates a client API
/// @param api [in/out] Client API to be populated by the function
/// @return eNone - Successfully populated the api.    \n
///         eInvalidArgument - api has not been instantiated.  \n
ISAR_API(IsarError) Isar_Client_CreateApi(IsarClientApi* api);

ISAR_CPP_NS_END

#endif  // REMOTING_CLIENT_API_H
