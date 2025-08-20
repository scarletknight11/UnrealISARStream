/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_SERVER_API_H
#define ISAR_SERVER_API_H

#include "isar/server_types.h"
#include "isar/graphics_api_config.h"

ISAR_CPP_NS_BEGIN

// Function Pointers
// -----------------------------------------------------------------------------

/// @brief Initalize the XR video track for sending rendered images
/// @details Must be called after createConnection but before openConnection. May be called on any thread.
/// @param connection [in]: A valid server connection handle
/// @param gfxConfig [in]: The information about the graphics devices and frame information. If depth
///						   information will also be rendered, populate the @ref depthRenderTargetDescription
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection \n
///			eVideoSource - Failed to the video source for converting and passing the video \n
///			eVideoTrack - Failed to create a the video track for sending the video \n
typedef IsarError (*IsarServerInitVideoTrack)(IsarConnection connection, IsarGraphicsApiConfig gfxConfig);

/// @brief Send an XR frame to the client
/// @details This must be called on the rendering thread. If not, graphics exceptions will occur.
/// @param connection [in]: A valid server connection handle
/// @param frame [in]: GPU frame data and the timestamp of the pose used for rendering the frame.
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
typedef IsarError (*IsarServerPushFrame)(IsarConnection connection, IsarGraphicsApiFrame frame);

/// @brief Send a audio data to the client
/// @param connection [in]: A valid server connection handle
/// @param data [in]: The data to be sent to the client
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
typedef IsarError (*IsarServerPushAudioData)(IsarConnection connection, IsarAudioData data);

/// @brief Enable an audio/video track
/// @param connection [in]: A valid server connection handle
/// @param enabled [in]: Indicating whether to enable the track. 1 = true, 0 = false.
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
typedef IsarError (*IsarServerSetTrackEnabled)(IsarConnection connection, int32_t enabled);

/// @brief Enable the camera capture on the client
/// @param connection [in]: A valid server connection handle
/// @param enabled [in]: Indicating whether to enable the track. 1 = true, 0 = false.
/// @param configuration [in]: The camera's resolution and framerate to be captured.
/// @param properties [in]: Additional properites for capturing.
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
typedef IsarError (*IsarServerSetCameraCaptureEnabled)(IsarConnection connection, int32_t enabled, IsarCameraConfiguration configuration, IsarCameraProperties properties);

/// @brief Try to pull a camera frame received from the client connection
/// @details If successful, the frame will be received on the GPU in NV12 format. It is the reponsibility
/// 		 of the consumer to convert this to a readable format.
/// @param connection [in]: A valid server connection handle
/// @param frame [out]: The populated frame and it's data
/// @param metadata [out]: Additional metadata pertaining to the frame
/// @param width [out]: Width of the frame texture
/// @param height [out]: Height of the frame texture
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
/// 		eNoFrame - A frame has not yet been received
typedef IsarError (*IsarServerPullCameraCaptureFrame)(IsarConnection connection, OUT IsarGraphicsApiFrame* frame, OUT IsarCameraMetadata* metadata, OUT int32_t* width, OUT int32_t* height);

/// @brief Acquire a previously pulled camera frame from @ref IsarServerPullCameraCaptureFrame as a CPU image
/// @param connection [in]: A valid server connection handle
/// @param formatToAcquire [in]: The format of the image to acquire
/// @param data [in/out]: Allocated storage for populating with the frame data
/// @param dataSize [in]: The amount of space in bytes allocated in @ref data
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
/// 		eNoFrame - A frame was not successfully pulled with @ref IsarServerPullCameraCaptureFrame
typedef IsarError (*IsarServerAcquireCameraCpuImage)(IsarConnection connection, IsarTextureFormat formatToAcquire, uint8_t *data, uint32_t dataSize);

/// @brief Deprecated please use pullViewPose
typedef void (*IsarServerRegisterViewPoseHandler)(IsarConnection connection, IsarViewPoseReceivedCallback cb, void* userData);

/// @brief Deprecated please use pullViewPose
typedef void (*IsarServerUnregisterViewPoseHandler)(IsarConnection connection, IsarViewPoseReceivedCallback cb, void* userData);

/// @brief Deprecated please use pullSpatialInput
typedef void (*IsarServerRegisterSpatialInputHandler)(IsarConnection connection, IsarSpatialInputReceivedCallback cb, void* userData);

/// @brief Deprecated please use pullSpatialInput
typedef void (*IsarServerUnregisterSpatialInputHandler)(IsarConnection connection, IsarSpatialInputReceivedCallback cb, void* userData);

/// @brief Register a handler to receive microphone data callbacks
/// @param connection [in]: A valid server connection handle
/// @param cb [in]: The callback to be registered
/// @param userData [in]: User data which will be passed to the callback
typedef void (*IsarServerRegisterMicrophoneCaptureHandler)(IsarConnection connection, IsarServerAudioDataReceivedCallback cb, void* userData);

/// @brief Unregister a handler to stop receiving microphone data callbacks
/// @param connection [in]: A valid server connection handle
/// @param cb [in]: The callback to be unregistered
/// @param userData [in]: User data which will be passed to the callback
typedef void (*IsarServerUnregisterMicrophoneCaptureHandler)(IsarConnection connection, IsarServerAudioDataReceivedCallback cb, void* userData);

/// @brief Register a handler to receive stats data callbacks
/// @param connection [in]: A valid server connection handle
/// @param cb [in]: The callback to be registered
/// @param userData [in]: User data which will be passed to the callback
typedef void (*IsarServerRegisterStatsHandler)(IsarConnection connection, IsarServerStatsCallback cb, void* userData);

/// @brief Unregister a handler to stop receiving stats data callbacks
/// @param connection [in]: A valid server connection handle
/// @param cb [in]: The callback to be unregistered
/// @param userData [in]: User data which will be passed to the callback
typedef void (*IsarServerUnregisterStatsHandler)(IsarConnection connection, IsarServerStatsCallback cb, void* userData);

/// @brief Asynchronously request for stats to be gathered. Once gathered, the data will be received
///		   on the callbacks registered through @ref IsarServerRegisterStatsHandler
/// @param connection [in]: A valid server connection handle
typedef void (*IsarServerGetStats)(IsarConnection connection);

/// @brief Set the server bitrate
/// @param connection [in]: A valid server connection handle
/// @param bitrateKbps [in]: Bitrate value to be set (Kbps)
/// @return eNone - Successful \n
typedef IsarError (*IsarSetBitrate)(IsarConnection connection, int32_t bitrateKbps);

/// @brief Send haptic input
/// @param connection [in]: A valid server connection handle
/// @param haptic [in]: Haptic data
/// @return eNone - Successful \n
typedef IsarError (*IsarPushHaptic)(IsarConnection connection, IsarHaptic const* haptic);

/// @brief Get the support status of the camera stream
/// @param connection [in]: A valid server connection handle
/// @return Supported/Not supported \n
typedef bool (*IsarCameraSupported)(IsarConnection connection);

/// @brief Pull the view pose which will be used for rendering
/// @param connection [in]: A valid server connection handle
/// @param pose [out]: The pose to be populated for rendering
/// @param predicted [in]: Whether the pose should be predicted based on the current frame round trip latency
typedef IsarError (*IsarServerPullViewPose)(IsarConnection connection, IsarXrPose* pose);

/// @brief Pull a specified number of received inputs
/// @details This call must be made twice. The first time to query the current number of inputs received and the second to populate the data.
/// 	     The data is then owned by the caller and must be free'd manually by the consumer.
/// @param connection [in]: A valid server connection handle
/// @param spatialInput [out]: The spatial input buffer to be populated. If nullptr, the number of current inputs in the queue will be returned in outputCount.
///							   If not null, inputCount must specify the size of the buffer. When populated, the memory within spatialInput must be manually
///							   free'd by the consumer.
/// @param inputCount [in]: The size of the spatialInput buffer
/// @param outputCount [out]: Returned size of the input queue. If nullptr, spatialInput must point to the buffer and inputCount specifying the size
typedef IsarError (*IsarServerPullSpatialInput)(IsarConnection connection, IsarSpatialInput* spatialInput, uint32_t inputCount, uint32_t* outputCount);


/// @brief configure pose prediction
/// @param connection [in]: A valid server connection handle
/// @param config [in]: Pose prediction
/// @return eNone - Successful \n
typedef IsarError (*IsarConfigurePosePrediction)(IsarConnection connection, IsarPosePredictionConfig config);

/// @brief Enable or disable passthrough if it is supported
/// @param connection [in]: A valid server connection handle
/// @param enable [in]: Enable or disable
/// @return Whether setting was successful \n
typedef bool (*IsarTrySetPassthroughMode)(IsarConnection connection, bool enable);

/// @brief Get the current passthrough mode
/// @param connection [in]: A valid server connection handle
/// @return Current status of passthrough \n
typedef bool (*IsarGetPassthroughMode)(IsarConnection connection);

// API
// -----------------------------------------------------------------------------

/// @brief Connection specific function pointers
typedef struct IsarServerApi {
	/// @brief The current version of ISAR being used
	IsarVersion version;

	IsarCreateConnection createConnection;
	IsarOpenConnection openConnection;
	IsarCloseConnection closeConnection;
	IsarDestroyConnection destroyConnection;

	IsarRegisterConnectionStateHandler registerConnectionStateHandler;
	IsarUnregisterConnectionStateHandler unregisterConnectionStateHandler;

	IsarServerInitVideoTrack initVideoTrack;
	IsarServerPushFrame pushFrame;

	IsarServerSetCameraCaptureEnabled setCameraCaptureEnabled;
	IsarServerPullCameraCaptureFrame pullCameraCaptureFrame;
	IsarServerAcquireCameraCpuImage acquireCameraCpuImage;

	IsarServerSetTrackEnabled setAudioTrackEnabled;
	IsarServerPushAudioData pushAudioData;

	IsarServerSetTrackEnabled setMicrophoneCaptureEnabled;
	IsarServerRegisterMicrophoneCaptureHandler registerMicrophoneCaptureHandler;
	IsarServerUnregisterMicrophoneCaptureHandler unregisterMicrophoneCaptureHandler;

	IsarServerRegisterViewPoseHandler registerViewPoseHandler;
	IsarServerUnregisterViewPoseHandler unregisterViewPoseHandler;

	IsarServerRegisterSpatialInputHandler registerSpatialInputHandler;
	IsarServerUnregisterSpatialInputHandler unregisterSpatialInputHandler;

	IsarServerRegisterStatsHandler registerStatsHandler;
	IsarServerUnregisterStatsHandler unregisterStatsHandler;
	IsarServerGetStats getStats;

	IsarGetConnectionInfo getConnectionInfo;
	IsarSetBitrate setBitrate;

	IsarPushHaptic pushHaptic;

	IsarCameraSupported cameraSupported;

	IsarServerPullViewPose pullViewPose;
	IsarServerPullSpatialInput pullSpatialInput;

	IsarConfigurePosePrediction configurePosePrediction;

	IsarTrySetPassthroughMode trySetPassthroughMode;
	IsarGetPassthroughMode getPassthroughMode;
} IsarServerApi;

/// @brief Creates a server API
/// @param api [in/out] Server API to be populated by the function
/// @return eNone - Successfully populated the api.    \n
///         eInvalidArgument - api has not been instantiated.  \n
ISAR_API(IsarError) Isar_Server_CreateApi(IsarServerApi* api);

ISAR_CPP_NS_END

#endif  // ISAR_SERVER_API_H
