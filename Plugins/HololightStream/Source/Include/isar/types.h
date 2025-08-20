/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_API_TYPES_H
#define ISAR_API_TYPES_H

#include "isar/isar_api.h"
#include "isar/graphics_api_config.h"
#include "isar/input_types.h"
#include "isar/version.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

ISAR_CPP_NS_BEGIN

// Type aliases
// -----------------------------------------------------------------------------

/// @brief Handle to a connection
typedef void* IsarConnection;

/// @brief Handle to a data channel
typedef void* IsarDataChannel;

// Enums
// -----------------------------------------------------------------------------
typedef enum IsarColorSpaceType{
	IsarColorSpaceType_Gamma,
	IsarColorSpaceType_Linear,

    IsarColorSpaceType_INTERNAL_FORCE_INT32_SIZE = 0xFFFFFFFF
} IsarColorSpaceType;

typedef enum IsarCodecType{
	IsarCodecType_AUTO,
	IsarCodecType_H264,
	IsarCodecType_H265,
	IsarCodecType_VP8,
	IsarCodecType_VP9,
	IsarCodecType_AV1,
	IsarCodecType_H265_10Bit,
	IsarCodecType_AV1_10Bit,

	IsarCodecType_INTERNAL_FORCE_INT32_SIZE = 0xFFFFFFFF
} IsarCodecType;

typedef enum IsarDiagnosticOptions{
	IsarDiagnosticOptions_DISABLED = 0,
	IsarDiagnosticOptions_ENABLE_TRACING = (1 << 0),
	IsarDiagnosticOptions_ENABLE_EVENT_LOG = (1 << 1),
	IsarDiagnosticOptions_ENABLE_STATS_COLLECTOR = (1 << 2),

	IsarDiagnosticOptions_INTERNAL_FORCE_INT32_SIZE = 0xFFFFFFFF
} IsarDiagnosticOptions;

typedef enum IsarConnectionState {
	IsarConnectionState_DISCONNECTED,
	IsarConnectionState_CONNECTING,
	IsarConnectionState_CONNECTED,
	IsarConnectionState_CLOSING,
	IsarConnectionState_FAILED,

	IsarConnectionState_INTERNAL_FORCE_INT32_SIZE = 0xFFFFFFFF
} IsarConnectionState;
static_assert(sizeof(IsarConnectionState) == sizeof(int32_t), "enums need to be 32 bits for foreign function bindings");

typedef enum IsarChannelPriority{
	IsarChannelPriority_LOW,
	IsarChannelPriority_MED,
	IsarChannelPriority_HIGH,

	IsarChannelPriority_INTERNAL_FORCE_INT32_SIZE = 0xFFFFFFFF
} IsarChannelPriority;

typedef enum IsarDeviceType {
	IsarDeviceType_UNDEFINED = -1,
	IsarDeviceType_AR = 0,
	IsarDeviceType_VR,
	IsarDeviceType_MR,
	IsarDeviceType_PC,

	IsarClientType_INTERNAL_FORCE_INT32_SIZE = 0xFFFFFFFF
} IsarDeviceType;

typedef enum IsarError {
	eNone = 0,                  // This should only be used at the API boundary.
	eAlreadyInitialized,        // Init has already been called
	eInvalidHandle,             // handle is invalid or doesn't match global_handle
	ePeerConnectionFactory,     // CreatePeerConnectionFactory failed
	ePeerConnection,            // CreatePeerConnection failed
	eDataChannel_Creation,      // CreateDataChannel failed
	eDataChannel_AlreadyExists,      // CreateDataChannel failed
	eDataChannel_Unsupported,  	// unsupported function call
	eDataChannel_Open,      	// Open data channel
	eDataChannel_Send,          // Sending via DataChannel failed
	eDataChannel_MessageTooLong,// Message was too long to send
	eDataChannel_InvalidConnection,  	// the function is unsupported by the connection type
	eAddTrack,                  // AddTrack failed
	eVideoSource,
	eVideoTrack,
	eStartRtcEventLog,  // StartRtcEventLog failed
	eConfig_UnsupportedOrMissingRole,
	eConfig_UnsupportedOrMissingEncoder,
	eConfig_UnsupportedOrMissingDecoder,
	eConfig_UnsupportedOrMissingVideoSource,
	eConfig_SignalingInvalidOrMissing,
	eConfig_SignalingIpInvalidOrMissing,
	eConfig_SignalingPortInvalidOrMissing,
	eNotConnected,        // user wants to do something but isn't connected
	eFileOpen,            // failed to open file
	eConfigParse,         // failed to parse config file
	eSdpParse,            // failed to parse session decription or ICE candidate
	eNoFrame,             // user tried to pull frame but there is none
	eUnsupportedVersion,  // user tried to create api with unsupported version
	eInvalidArgument,
	eAudioTrack,          // failed to initialize audio track
	eAudioTrack_NotInitialized, // tried to perform an operation on audio track but there is none
	eAlreadyConnected,
	eNoInput,

	eUnknown = 0xFFFFFFFF,  // force sizeof int32
} IsarError;
static_assert(sizeof(IsarError) == sizeof(int32_t), "enums need to be 32 bits for foreign function bindings");

#define ISAR_RETURN_ON_ERROR(expr) if (IsarError const err = expr) return err

// Structs
// -----------------------------------------------------------------------------

/// @brief STUN/TURN server information used for generating ICE candidates
typedef struct IsarIceServerConfig {
	const wchar_t* url;
	const wchar_t* username;
	const wchar_t* password;
} IsarIceServerConfig;

/// @brief Information about the rendering configuration to be negotiated
typedef struct IsarRenderConfig {
	/// @brief The width of the XR frame (for a single view)
	uint32_t width;

	/// @brief The height of the XR frame (for a single view)
	uint32_t height;

	/// @brief The number of views in the XR frame
	uint32_t numViews;

	/// @brief The default encoder bitrate to be used in Kbps
	int32_t encoderBitrateKbps;

	/// @brief The framerate which will be used for rendering
	uint32_t framerate;

	/// @brief Whether depth should be used
	uint32_t depthEnabled;

	/// @brief Whether pose prediction should be used
	uint32_t posePredictionEnabled;
} IsarRenderConfig;

/// @brief Suggested signaling configuration
typedef struct IsarSignalingConfig
{
	const char* suggestedIpv4;
	uint32_t port;
} IsarSignalingConfig;

/// @brief Range of ports to be used for WebRTC connection. The range must be between 1024 - 65535 or will be unaccepted.
typedef struct IsarPortRange{
	uint32_t minPort;
	uint32_t maxPort;
} IsarPortRange;

/// @brief A collection of information required for remote rendering
typedef struct IsarConfig
{
	/// @brief Friendly name of the connection peer
	const char* friendlyName;

	/// @brief The preferred codec to use. This will only be used if both ends support it
	IsarCodecType codecPreference;

	/// @brief Diagnostics to be used. This is intended for internal use only
	IsarDiagnosticOptions diagnosticOptions;

	/// @brief The size of the @ref iceServers array
	uint32_t numIceServers;

	/// @brief An array of STUN/TURN server information
	IsarIceServerConfig* iceServers;

	/// @brief The render configuration to be used
	IsarRenderConfig renderConfig;

	/// @brief The suggested signaling configuration to use
	IsarSignalingConfig signalingConfig;

	/// @brief Type of the device
	IsarDeviceType deviceType;

	/// @brief The range of ports to be used for WebRTC connection. The range must be between 1024 - 65535 or will be unaccepted.
	IsarPortRange portRange;
} IsarConfig;

/// @brief Information about the connected remote end.
typedef struct IsarConnectionInfo {

	/// @brief Name of the remote end
	const char* remoteName;

	/// @brief Version of ISAR remote end uses
	IsarVersion remoteVersion;

	/// @brief The render configuration that is currently used
	IsarRenderConfig renderConfig;

	/// @brief Type of the remote device
	IsarDeviceType remoteDeviceType;

	///@brief The codec that is used on the connection
	IsarCodecType codecInUse;
} IsarConnectionInfo;


/// @brief Audio data for sending/receiving
typedef struct IsarAudioData {
	const void* data;
	int32_t bitsPerSample;
	int32_t sampleRate;
	size_t numberOfChannels;
	size_t samplesPerChannel;
} IsarAudioData;

/// @brief Details pertaining to the channel
typedef struct IsarChannelDescription{
	/// @brief The channel name. Used to find matching plugins on the remote endpoint.
	/// @details The following naming standard should follow <company-name>.<project-name>.<plugin-name>,
	///           where <company-name> is the name of the company, <project-name> is the name of the project and <plugin-name>
	///           is a name for the plugin.
	const char* name;

	/// @brief Current version of this channel. Required for matching channels version at the remote endpoint.
	IsarVersion version;

	/// @brief Priority of the channel
	/// @details The priority of is used by ISAR to determine the priority of the data for sending to the remote endpoint.
	///          It is recommended to leave plugins at low priority to avoid performance hits.
	IsarChannelPriority priority;

	/// @brief Whether the channel is reliable or not
	uint32_t reliable;

	/// @brief Requires the ability to send messages over 8KB. If this is set to true
	/// 		 the channel will be classified as reliable.
	uint32_t requiresLargeMessages;
} IsarChannelDescription;

/// @brief The interinsics of a camera captured frame
typedef struct IsarCameraIntrinsics {
	uint32_t width;
	uint32_t height;
	float focalLengthX;
	float focalLengthY;
	float cameraModelPrincipalPointX;
	float cameraModelPrincipalPointY;
	float distortionModelRadialK1;
	float distortionModelRadialK2;
	float distortionModelRadialK3;
	float distortionModelTangentialP1;
	float distortionModelTangentialP2;
} IsarCameraIntrinsics;

/// @brief The configuration of the camera capturer
typedef struct IsarCameraConfiguration {
	uint32_t width;
	uint32_t height;
	float framerate;
} IsarCameraConfiguration;

/// @brief Additional properties of the camera capturer
typedef struct IsarCameraProperties {
	uint32_t autoExposure;
	int64_t exposure;
	float exposureCompensation;
	int32_t whiteBalance;
} IsarCameraProperties;

/// @brief A collection of the camera frame metadata
typedef struct IsarCameraMetadata{
	IsarCameraIntrinsics intrinsics;
	IsarMatrix4x4 extrinsics;
	IsarCameraProperties properties;
} IsarCameraMetadata;

/// @brief Configuration of the pose prediction
typedef struct IsarPosePredictionConfig{
	uint8_t enabled;
	// Predict ahead or slowdown the prediction.
	float predictionTuner;
	// Maximum amount of time, in ms, that we can use to predict in the future.
	uint16_t predictionCap;
} IsarPosePredictionConfig;

typedef enum IsarCameraPropertiesSupport{
	IsarCameraPropertiesSupport_NONE = 0,
	IsarCameraPropertiesSupport_AUTO_EXPOSURE =  (1 << 0),
	IsarCameraPropertiesSupport_EXPOSURE = (1 << 1),
	IsarCameraPropertiesSupport_EXPOSURE_COMPENSATION = (1 << 2),
	IsarCameraPropertiesSupport_WHITE_BALANCE = (1 << 3),
} IsarCameraPropertiesSupport;

typedef struct IsarCameraPropertiesSettings{
	IsarCameraPropertiesSupport support;

	bool AutoExposureDefault;

	int64_t ExposureMax;
	int64_t ExposureMin;
	int64_t ExposureStep;
	int64_t ExposureDefault;

	float ExposureCompensationMax;
	float ExposureCompensationMin;
	float ExposureCompensationStep;
	float ExposureCompensationDefault;

	int32_t WhiteBalanceMax;
	int32_t WhiteBalanceMin;
	int32_t WhiteBalanceStep;
	int32_t WhiteBalanceDefault;
} IsarCameraPropertiesSettings;

// Callbacks
// -----------------------------------------------------------------------------

/// @brief A callback when the connection state has changed
/// @param newState [in]: The state the connection has changed into
/// @param userData [in]: User specific data which was passed when registering for the event
typedef void(*IsarConnectionStateChangedCallback)(IsarConnectionState newState, void* userData);

/// @brief A callback when the local Session Description has been generated
/// @details This callback will be triggered asynchronously when the application calls CreateOffer/CreateAnswer
/// @param sdp [in]: A null terminated string containing the generated Session Description
/// @param userData [in]: User specific data which was passed when registering for the event
typedef void(*IsarSdpCreatedCallback)(const char* sdp, void* userData);

/// @brief A callback when a local ICE candidate has been generated
/// @param sdpMid [in]: A null terminated string of the value of the mid section in the SDP
/// @param mLineIndex [in]: Index of the ice candidate in the SDP
/// @param sdpizedCandidate [in]: A null terminated string of the received ice candidate
typedef void (*IsarLocalIceCandidateCreatedCallback)(const char* sdpMLine, int32_t mLineIndex, const char* sdpizedIceCandidate, void* userData);

/// @brief A collection of callbacks pertaining to the connection state and signaling information.
/// @details This collection must be passed when instantiated a connection.
typedef struct IsarConnectionCallbacks {
	void* userData;
	IsarConnectionStateChangedCallback connectionStateChangedCb;
	IsarSdpCreatedCallback sdpCreatedCb;
	IsarLocalIceCandidateCreatedCallback localIceCandidateCreatedCb;
} IsarConnectionCallbacks;

/// @brief A callback when view pose data has been received
/// @param pose [in]: A pointer to the pose received
/// @param userData [in]: User specific data which was passed when registering for the event
typedef void(*IsarViewPoseReceivedCallback)(IsarXrPose const* pose, void* userData);

/// @brief A callback when input event data has been received
/// @param spatialInput [in]: A pointer to the input event received
/// @param userData [in]: User specific data which was passed when registering for the event
typedef void(*IsarSpatialInputReceivedCallback)(IsarSpatialInput const* spatialInput, void* userData);

/// @brief A callback when a track has been requested to be enabled
/// @param enabled [in]: Specifies whether the track is to be enabled or disabled
/// @param userData [in]: User specific data which was passed when registering for the event
typedef void(*IsarTrackEnabledCallback)(bool enabled, void* userData);

/// @brief A callback when the camera capture has been requested to be enabled
/// @param enabled [in]: Specifies whether the camera capture is to be enabled or disabled
/// @param configuration [in]: A pointer to the configuration to be used when caputuring
/// @param properties [in]: A pointer to the properties to be used when capturing
/// @param userData [in]: User specific data which was passed when registering for the event
typedef void (*IsarCameraCaptureEnabledCallback)(bool enabled,
    const IsarCameraConfiguration* configuration,
    const IsarCameraProperties* properties,
    void* userData);

// Shared Function Pointers
// -----------------------------------------------------------------------------

/// @brief Create and initalize the connection object with the specific configuration
/// @param config [in]: Connection configuration settings to be used for initalizing the connection
/// @param gfxConfig [in]: Connection graphics configuration settings to be used for initalizing the connection
/// @param connection [out]: A handle to the instantiated connection, required for all future calls to the connection
/// @return eNone - Successful \n
/// 		ePeerConnectionFactory - Failed to instanitate the peer connection factory for the specific config \n
///			ePeerConnection - Failed to instanitate the peer connection for the specific config \n
///			eAudioTrack - Failed to instanitate the audio track for sending local audio \n
typedef IsarError (*IsarCreateConnection)(const IsarConfig* config,
										  IsarGraphicsApiConfig gfxConfig,
										  IsarConnection* const OUT isarConnection);

/// @brief Open the connection process
/// @param connection [in]: A handle to the instantiated connection
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
typedef IsarError (*IsarOpenConnection)(IsarConnection IN connection);

/// @brief Close the connection
/// @param connection [in]: A handle to the instantiated connection
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
typedef IsarError (*IsarCloseConnection)(IsarConnection IN connection);

/// @brief Close the connection instance
/// @param connection [in/out]: A pointer to the handle to the instantiated connection, invalid after the call
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection
typedef IsarError (*IsarDestroyConnection)(IsarConnection* IN_OUT connection);

/// @brief Register a handler to receive connection callbacks
/// @param connection [in]: A valid connection handle
/// @param cb [in]: The callback to be registered
/// @param userData [in]: User data which will be passed to the callback
typedef void (*IsarRegisterConnectionStateHandler)(IsarConnection connection, IsarConnectionStateChangedCallback cb, void* userData);

/// @brief Unregister a handler to stop receiving connection callbacks
/// @param connection [in]: A valid server connection handle
/// @param cb [in]: The callback to be unregistered
/// @param userData [in]: User data which will be passed to the callback
typedef void (*IsarUnregisterConnectionStateHandler)(IsarConnection connection, IsarConnectionStateChangedCallback cb, void* userData);

/// @brief Get the information about the remote device
/// @details The returned reference to the @ref connectionInfo is only valid during the lifetime of the connection.
/// @param connection [in]: A valid server connection handle
/// @param connectionInfo [out]: The information about the remote device that is currently connected to the server.
/// @return eNone - Successful \n
///			eInvalidArgument - An invalid connection \n
///			eNotConnected - Client is not yet connected \n
typedef IsarError (*IsarGetConnectionInfo)(IsarConnection connection, OUT IsarConnectionInfo* connectionInfo);

ISAR_CPP_NS_END

#endif  // ISAR_API_TYPES_H
