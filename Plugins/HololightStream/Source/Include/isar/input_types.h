/**
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */
#ifdef __cplusplus
#ifdef _MSC_VER
#pragma warning(disable: 26812) // unscoped enums
#endif // _MSC_VER
#endif // __cplusplus

#ifndef ISAR_INPUT_TYPES_H
#define ISAR_INPUT_TYPES_H

#include "isar/isar_api.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h> // NaN: nanf("")

// TODO(viktor): move out of public header
#include <string.h> // memcmp, memmove, memset
#define ISAR_EQUAL_MEMORY(_destination, _source, _length) (!memcmp((_destination), (_source), (_length)))
#define ISAR_MOVE_MEMORY(_destination, _source, _length) memmove((_destination), (_source), (_length))
#define ISAR_COPY_MEMORY(_destination, _source, _length) memcpy((_destination), (_source), (_length))
#define ISAR_FILL_MEMORY(_destination, _length, _fill) memset((_destination), (_fill), (_length))
#define ISAR_FILL_MEMORY_TYPE(_type, _fill) memset(&(_type), (_fill), sizeof(_type))
#define ISAR_ZERO_MEMORY(_destination, _length) memset((_destination), 0, (_length))
#define ISAR_ZERO_MEMORY_TYPE(_type) memset(&(_type), 0, sizeof(_type))

ISAR_CPP_NS_BEGIN

typedef struct IsarGuid {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  uint8_t  data4[8];
} IsarGuid;

inline bool IsarGuid_equals(const IsarGuid* lhs, const IsarGuid* rhs)
{
	const uint64_t* lhsFirst = (const uint64_t*) &lhs->data1;
	const uint64_t* lhsSecond = (const uint64_t*) &lhs->data4;
	const uint64_t* rhsFirst =  (const uint64_t*) &rhs->data1;
	const uint64_t* rhsSecond = (const uint64_t*) &rhs->data4;
	return (*lhsFirst) == (*rhsFirst) && (*lhsSecond) == (*rhsSecond);
}

typedef struct IsarMatrix4x4 {
	// Format: column-major - unity / hlsl shaders
	float m00, m10, m20, m30;  // 16 bytes // x_axis[x, y, z, w]
	float m01, m11, m21, m31;  // 16 bytes // y_axis[x, y, z, w]
	float m02, m12, m22, m32;  // 16 bytes // z_axis[x, y, z, w]
	float m03, m13, m23, m33;  // 16 bytes //    pos[x, y, z, w]
} IsarMatrix4x4;  // 64 bytes

typedef struct IsarVector2 {
	float x, y;
} IsarVector2;

typedef struct IsarVector3 {
	float x, y, z;  // 12 bytes
} IsarVector3;     // 12 bytes

typedef struct IsarVector4 {
	float x, y, z, w;  // 16 bytes
} IsarVector4;        // 16 bytes
typedef IsarVector4 IsarQuaternion;

typedef struct IsarPose {
	IsarVector3 position;        // 12 bytes
	IsarQuaternion orientation;  // 16 bytes
} IsarPose;                    // 28 bytes

typedef struct IsarFov {
	float left, right, up, down;  // 16 bytes
} IsarFov;        // 16 bytes

// Input Data Types
typedef struct IsarXrPose_Deprecated {
	// Frames rendered with this pose should be pushed with this timestamp value
	int64_t timestamp;  // 8 bytes

	// View matrices
	IsarMatrix4x4 viewLeft;   //  64 bytes
	IsarMatrix4x4 viewRight;  //  64 bytes

	// Projection Matrices
	IsarMatrix4x4 projLeft;   //  64 bytes
	IsarMatrix4x4 projRight;  //  64 bytes
} IsarXrPose_Deprecated;                // 256 bytes

// Input Data Types
typedef struct IsarXrPose {
	// Frames rendered with this pose should be pushed with this timestamp value.
	int64_t frameTimestamp;  // 8 bytes
	// Poses creation timestamp.
	int64_t poseTimestamp;  // 8 bytes

	// Poses
	IsarPose poseLeft;	// 28 Bytes
	IsarPose poseRight; // 28 Bytes

	// FOV
	IsarFov fovLeft;	// 16 Bytes
	IsarFov fovRight;	// 16 Bytes
} IsarXrPose;           // 104 bytes

// Input Event Data Types

typedef enum IsarInputType {
	// InteractionManager
	IsarInputType_SOURCE_DETECTED = 0,
	IsarInputType_SOURCE_LOST,
	IsarInputType_SOURCE_PRESSED,
	IsarInputType_SOURCE_UPDATED,
	IsarInputType_SOURCE_RELEASED,

	IsarInputType_COUNT,
	IsarInputType_MIN = 0,
	IsarInputType_MAX = IsarInputType_COUNT - 1,
	IsarInputType_UNKNOWN = 0xFFFFFFFF,  // force sizeof int32
} IsarInputType;
static_assert(sizeof(IsarInputType) == sizeof(uint32_t),
							"enums need to be 32 bits, because bindings expect them to be");

	inline char const* IsarSpatialInputType_to_str(IsarInputType type) {
	switch (type) {
		// InteractionManager
		ISAR_STRINGIFY_ENUM_CASE(IsarInputType_SOURCE_DETECTED);
		ISAR_STRINGIFY_ENUM_CASE(IsarInputType_SOURCE_LOST);
		ISAR_STRINGIFY_ENUM_CASE(IsarInputType_SOURCE_PRESSED);
		ISAR_STRINGIFY_ENUM_CASE(IsarInputType_SOURCE_UPDATED);
		ISAR_STRINGIFY_ENUM_CASE(IsarInputType_SOURCE_RELEASED);

		ISAR_STRINGIFY_ENUM_CASE(IsarInputType_COUNT);
		default:
			assert(false && "Unknown input event type!!!");
			return ISAR_STRINGIFY(IsarInputType_UNKNOWN);
	}
}

#pragma region  // input data types


typedef enum IsarSpatialInteractionSourceHandedness {
	IsarSpatialInteractionSourceHandedness_UNSPECIFIED = 0,
	IsarSpatialInteractionSourceHandedness_LEFT = 1,
	IsarSpatialInteractionSourceHandedness_RIGHT = 2,
} IsarSpatialInteractionSourceHandedness;
static_assert(sizeof(IsarSpatialInteractionSourceHandedness) == sizeof(int32_t),
							"enums need to be 32 bits, because bindings expect them to be");

// TODO(viktor): convert forward & up to a quaternion and use IsarPose
typedef struct IsarHeadPose {
	IsarVector3 position;          // 12 bytes
	IsarVector3 forwardDirection;  // 12 bytes
	IsarVector3 upDirection;       // 12 bytes
} IsarHeadPose;                  // 36 bytes

typedef enum IsarXRControllerFeatureKind
{
	// Hands
	IsarXRControllerFeatureKind_HAND_PALM = 0,
	IsarXRControllerFeatureKind_HAND_WRIST = 1,
	IsarXRControllerFeatureKind_HAND_THUMB_METACARPAL = 2,
	IsarXRControllerFeatureKind_HAND_THUMB_PROXIMAL = 3,
	IsarXRControllerFeatureKind_HAND_THUMB_DISTAL = 4,
	IsarXRControllerFeatureKind_HAND_THUMB_TIP = 5,
	IsarXRControllerFeatureKind_HAND_INDEX_METACARPAL = 6,
	IsarXRControllerFeatureKind_HAND_INDEX_PROXIMAL = 7,
	IsarXRControllerFeatureKind_HAND_INDEX_INTERMEDIATE = 8,
	IsarXRControllerFeatureKind_HAND_INDEX_DISTAL = 9,
	IsarXRControllerFeatureKind_HAND_INDEX_TIP = 10,
	IsarXRControllerFeatureKind_HAND_MIDDLE_METACARPAL = 11,
	IsarXRControllerFeatureKind_HAND_MIDDLE_PROXIMAL = 12,
	IsarXRControllerFeatureKind_HAND_MIDDLE_INTERMEDIATE = 13,
	IsarXRControllerFeatureKind_HAND_MIDDLE_DISTAL = 14,
	IsarXRControllerFeatureKind_HAND_MIDDLE_TIP = 15,
	IsarXRControllerFeatureKind_HAND_RING_METACARPAL = 16,
	IsarXRControllerFeatureKind_HAND_RING_PROXIMAL = 17,
	IsarXRControllerFeatureKind_HAND_RING_INTERMEDIATE = 18,
	IsarXRControllerFeatureKind_HAND_RING_DISTAL = 19,
	IsarXRControllerFeatureKind_HAND_RING_TIP = 20,
	IsarXRControllerFeatureKind_HAND_LITTLE_METACARPAL = 21,
	IsarXRControllerFeatureKind_HAND_LITTLE_PROXIMAL = 22,
	IsarXRControllerFeatureKind_HAND_LITTLE_INTERMEDIATE = 23,
	IsarXRControllerFeatureKind_HAND_LITTLE_DISTAL = 24,
	IsarXRControllerFeatureKind_HAND_LITTLE_TIP = 25,

	// Buttons
	IsarXRControllerFeatureKind_BUTTON_HOME = 26,
	IsarXRControllerFeatureKind_BUTTON_MENU = 27,
	IsarXRControllerFeatureKind_BUTTON_SETTINGS = 28,
	IsarXRControllerFeatureKind_BUTTON_A = 29,
	IsarXRControllerFeatureKind_BUTTON_B = 30,
	IsarXRControllerFeatureKind_BUTTON_X = 31,
	IsarXRControllerFeatureKind_BUTTON_Y = 32,
	IsarXRControllerFeatureKind_BUTTON_PRIMARY_BUMPER = 33,
	IsarXRControllerFeatureKind_BUTTON_SECONDARY_BUMPER = 34,
	IsarXRControllerFeatureKind_BUTTON_PRIMARY_ANALOG_STICK_PRESS = 35,
	IsarXRControllerFeatureKind_BUTTON_SECONDARY_ANALOG_STICK_PRESS = 36,
	IsarXRControllerFeatureKind_BUTTON_PRIMARY_THUMB_REST = 37,
	IsarXRControllerFeatureKind_BUTTON_SECONDARY_THUMB_REST = 38,
	IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS = 39,
	IsarXRControllerFeatureKind_BUTTON_SECONDARY_TRIGGER_PRESS = 40,
	IsarXRControllerFeatureKind_BUTTON_PRIMARY_SQUEEZE_PRESS = 41,
	IsarXRControllerFeatureKind_BUTTON_SECONDARY_SQUEEZE_PRESS = 42,
	// Axis1D
	IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER = 43,
	IsarXRControllerFeatureKind_AXIS1D_SECONDARY_TRIGGER = 44,
	IsarXRControllerFeatureKind_AXIS1D_PRIMARY_SQUEEZE = 45,
	IsarXRControllerFeatureKind_AXIS1D_SECONDARY_SQUEEZE = 46,

	// Axis2D
	IsarXRControllerFeatureKind_AXIS2D_PRIMARY_ANALOG_STICK = 47,
	IsarXRControllerFeatureKind_AXIS2D_SECONDARY_ANALOG_STICK = 48,

	// Treated as a button
	IsarXRControllerFeatureKind_DOCKED = 49,

} IsarXRControllerFeatureKind;

typedef enum IsarXRControllerType
{
	IsarXRControllerType_HoloLens_Hands = 0,
	IsarXRControllerType_Meta_Quest_Hands = 1,
	IsarXRControllerType_Meta_Quest_2_Controller = 2,
	IsarXRControllerType_Magic_Leap_2_Hands = 3,
	IsarXRControllerType_Magic_Leap_2_Controller = 4,
	IsarXRControllerType_Meta_Quest_Pro_Controller = 5,
	IsarXRControllerType_Meta_Quest_3_Controller = 6,
	IsarXRControllerType_Lenovo_VRX_Hands = 7,
	IsarXRControllerType_Lenovo_VRX_Controller = 8,
	IsarXRControllerType_Logitech_MX_Ink_Stylus = 9,
	IsarXRControllerType_Pico_4_Ultra_Hands = 10,
	IsarXRControllerType_Pico_4_Ultra_Controller = 11,
	IsarXRControllerType_HTC_Vive_Focus_Hands = 12,
	IsarXRControllerType_HTC_Vive_Focus_3_Controller = 13,
	IsarXRControllerType_HTC_Vive_Focus_Vision_Controller = 14,
	IsarXRControllerType_Meta_Quest_3S_Controller = 15,
	IsarXRControllerType_HTC_Vive_XR_Elite_Controller = 16,
	IsarXRControllerType_Apple_Vision_Pro_Hands = 17,
	IsarXRControllerType_COUNT

	// Extend for more controller types (Vive, SteamVR, WMR Controller, whatever...)
} IsarXRControllerType;

typedef enum IsarJointPoseAccuracy /*: int32_t*/ {
	IsarJointPoseAccuracy_HIGH = 0,
	IsarJointPoseAccuracy_APPROXIMATE = 1,
} IsarJointPoseAccuracy;

typedef struct IsarJointPose {
	IsarQuaternion orientation;      // 16 bytes
	IsarVector3 position;            // 12 bytes
	float radius;                   //  4 bytes
	IsarJointPoseAccuracy accuracy;  //  4 bytes
} IsarJointPose;                   // 36 bytes

typedef struct IsarHandPose {
	IsarJointPose jointPoses[26]; // TODO: don't hard code  // 936 bytes
} IsarHandPose;                                             // 1040 bytes

typedef enum IsarSpatialInteractionSourceStateFlags {
	IsarInteractionSourceStateFlags_NONE = 0 << 0,
	IsarInteractionSourceStateFlags_GRASPED = 1 << 0,
	IsarInteractionSourceStateFlags_ANY_PRESSED = 1 << 1,
	IsarInteractionSourceStateFlags_TOUCHPAD_PRESSED = 1 << 2,
	IsarInteractionSourceStateFlags_THUMBSTICK_PRESSED = 1 << 3,
	IsarInteractionSourceStateFlags_SELECT_PRESSED = 1 << 4,
	IsarInteractionSourceStateFlags_MENU_PRESSED = 1 << 5,
	IsarInteractionSourceStateFlags_TOUCHPAD_TOUCHED = 1 << 6,
} IsarSpatialInteractionSourceStateFlags;
static_assert(sizeof(IsarSpatialInteractionSourceStateFlags) == sizeof(uint32_t),
							"enums need to be 32 bits, because bindings expect them to be");

typedef struct IsarButton{
	uint32_t identifier;
	bool value;
}IsarButton;

typedef struct IsarAxis1D{
	uint32_t identifier;
	float value;
}IsarAxis1D;

typedef struct IsarAxis2D{
	uint32_t identifier;
	IsarVector2 value;
}IsarAxis2D;

typedef enum IsarButtonType{
	IsarButtonKind_HOME = 0,
	IsarButtonKind_MENU = 1,
	IsarButtonKind_SETTINGS = 2,
	IsarButtonKind_A = 3,
	IsarButtonKind_B = 4,
	IsarButtonKind_X = 5,
	IsarButtonKind_Y = 6,
	IsarButtonKind_BUMPER_LEFT = 7,
	IsarButtonKind_BUMPER_RIGHT = 8,
	IsarButtonKind_LEFT_STICK_PRESS = 9,
	IsarButtonKind_RIGHT_STICK_PRESS = 10,
	IsarAxis2DKind_PRIMARY_THUMB_REST = 11,
	IsarAxis2DKind_SECONDARY_THUMB_REST = 12,
	IsarButtonKind_DOCKED_LEFT = 13,
	IsarButtonKind_DOCKED_RIGHT = 14,
	IsarButtonKind_COUNT
} IsarButtonType;

typedef enum IsarAxis1DType{
	IsarAxis1DKind_PRIMARY_TRIGGER = 0,
	IsarAxis1DKind_SECONARDY_TRIGGER = 1,
	IsarAxis1DKind_PRIMARY_SQUEEZE = 2,
	IsarAxis1DKind_SECONDARY_SQUEEZE = 3,
	IsarAxis1DKind_COUNT
} IsarAxis1DType;

typedef enum IsarAxis2DType
{
	IsarAxis2DKind_PRIMARY_STICK = 0,
	IsarAxis2DKind_SECONDARY_STICK = 1,
	IsarAxis2DKind_PRIMARY_CONTROL_PAD = 2,
	IsarAxis2DKind_SECONDARY_CONTROL_PAD = 3,
	IsarAxis2DKind_COUNT
} IsarAxis2DType;

typedef struct IsarControllerData{
	uint32_t controllerIdentifier;
	IsarSpatialInteractionSourceHandedness handedness;
	IsarHeadPose headPose;
	IsarPose controllerPose;
	IsarPose pointerPose;
	IsarPose tipPose;

	IsarHandPose handData;
	IsarButton* buttons;
	uint32_t buttonsLength;
	IsarAxis1D* axis1D;
	uint32_t axis1DLength;
	IsarAxis2D* axis2D;
	uint32_t axis2DLength;

} IsarControllerData;

typedef struct IsarInteractionSourceState {
	IsarControllerData controllerData; // TODO: inline the content of this
} IsarInteractionSourceState;  // 1216 bytes

typedef struct IsarSpatialInputDataInteractionSourceDetected {
	IsarInteractionSourceState interactionSourceState;  // 176 bytes
} IsarSpatialInputDataInteractionSourceDetected;        // 176 bytes

typedef struct IsarSpatialInputDataInteractionSourceLost {
	IsarInteractionSourceState interactionSourceState;  // 176 bytes
} IsarSpatialInputDataInteractionSourceLost;            // 16 bytes

typedef struct IsarSpatialInputDataInteractionSourcePressed {
	IsarInteractionSourceState interactionSourceState;  // 176 bytes
} IsarSpatialInputDataInteractionSourcePressed;         // 16 bytes

typedef struct IsarSpatialInputDataInteractionSourceUpdated {
	IsarInteractionSourceState interactionSourceState;  // 176 bytes
} IsarSpatialInputDataInteractionSourceUpdated;         // 16 bytes

typedef struct IsarSpatialInputDataInteractionSourceReleased {
	IsarInteractionSourceState interactionSourceState;  // 176 bytes
} IsarSpatialInputDataInteractionSourceReleased;        // 16 bytes

typedef union IsarSpatialInputData {
	/// InteractionManager events
	IsarSpatialInputDataInteractionSourceDetected sourceDetected; // 176 bytes
	IsarSpatialInputDataInteractionSourceLost sourceLost;         //  16 bytes
	IsarSpatialInputDataInteractionSourcePressed sourcePressed;   //  16 bytes
	IsarSpatialInputDataInteractionSourceUpdated sourceUpdated;   //  16 bytes
	IsarSpatialInputDataInteractionSourceReleased sourceReleased;

} IsarSpatialInputData;

typedef struct IsarSpatialInput {
	IsarInputType type;  //   4 bytes
	IsarSpatialInputData data;  // 176 bytes
} IsarSpatialInput;

typedef enum IsarHapticType {
	IsarHapticType_STOP = 0,
	IsarHapticType_VIBRATION,
	IsarHapticType_PCM_VIBRATION,
	IsarHapticType_AMPLITUDE_ENVELOPE_VIBRATION,
	IsarHapticType_COUNT,
	IsarHapticType_MIN = 0,
	IsarHapticType_MAX = IsarHapticType_COUNT - 1,

	IsarHapticType_UNKNOWN = 0xFFFFFFFF, // force sizeof int32
} IsarHapticType;

typedef enum IsarHapticChannel {
	IsarHapticChannel_BODY = 0,
	IsarHapticChannel_TRIGGER,
	IsarHapticChannel_THUMB_REST,
	IsarHapticChannel_COUNT,
	IsarHapticChannel_MIN = 0,
	IsarHapticChannel_MAX = IsarHapticChannel_COUNT - 1,

	IsarHapticChannel_UNKNOWN = 0xFFFFFFFF, // force sizeof int32
} IsarHapticChannel;

typedef struct IsarHapticStop {
	uint32_t controllerIdentifier;
	IsarSpatialInteractionSourceHandedness handedness;
	IsarHapticChannel channel;
} IsarHapticStop;

typedef struct IsarHapticVibration {
	uint32_t controllerIdentifier;
	IsarSpatialInteractionSourceHandedness handedness;
	IsarHapticChannel channel;
	int64_t duration;
	float frequency;
	float amplitude;
} IsarHapticVibration;

typedef struct IsarHapticPcmVibration {
	uint32_t controllerIdentifier;
	IsarSpatialInteractionSourceHandedness handedness;
	IsarHapticChannel channel;
	uint32_t bufferSize;
	uint8_t* buffer;
	float sampleRate;
	bool append;
} IsarHapticPcmVibration;

typedef struct IsarHapticAmplitudeEnvelopeVibration {
	uint32_t controllerIdentifier;
	IsarSpatialInteractionSourceHandedness handedness;
	IsarHapticChannel channel;
	int64_t duration;
	uint32_t amplitudeCount;
	float* amplitudes;
} IsarHapticAmplitudeEnvelopeVibration;

typedef union IsarHapticData {
	IsarHapticStop hapticStop;
	IsarHapticVibration hapticVibration;
	IsarHapticPcmVibration hapticPcmVibration;
	IsarHapticAmplitudeEnvelopeVibration hapticAmplitudeEnvelopeVibration;
} IsarHapticData;

typedef struct IsarHaptic {
	IsarHapticType type;
	IsarHapticData data;
} IsarHaptic;

inline IsarSpatialInput IsarSpatialInput_create() {
	IsarSpatialInput spatialInput;
#ifndef NDEBUG
	ISAR_FILL_MEMORY_TYPE(spatialInput, 0xCDCDCDCD);
#else
	ISAR_ZERO_MEMORY_TYPE(spatialInput);
#endif

	return spatialInput;
}
#pragma endregion  // input data types

#pragma region  // InteractionManager events

inline IsarSpatialInput IsarSpatialInput_createSourceDetected(
		IsarInteractionSourceState interactionSourceState) {
	IsarSpatialInput result = IsarSpatialInput_create();

	result.type = IsarInputType_SOURCE_DETECTED;
	result.data.sourceDetected.interactionSourceState = interactionSourceState;
	return result;
}

inline IsarSpatialInput IsarSpatialInput_createSourceLost(
		IsarInteractionSourceState interactionSourceState) {
	IsarSpatialInput result = IsarSpatialInput_create();

	result.type = IsarInputType_SOURCE_LOST;
	result.data.sourceDetected.interactionSourceState = interactionSourceState;
	return result;
}

inline IsarSpatialInput IsarSpatialInput_createSourcePressed(
		IsarInteractionSourceState interactionSourceState) {
	IsarSpatialInput result = IsarSpatialInput_create();

	result.type = IsarInputType_SOURCE_PRESSED;
	result.data.sourceDetected.interactionSourceState = interactionSourceState;
	return result;
}

inline IsarSpatialInput IsarSpatialInput_createSourceUpdated(
		IsarInteractionSourceState interactionSourceState) {
	IsarSpatialInput result = IsarSpatialInput_create();

	result.type = IsarInputType_SOURCE_UPDATED;
	result.data.sourceDetected.interactionSourceState = interactionSourceState;
	return result;
}

inline IsarSpatialInput IsarSpatialInput_createSourceReleased(
		IsarInteractionSourceState interactionSourceState) {
	IsarSpatialInput result = IsarSpatialInput_create();

	result.type = IsarInputType_SOURCE_RELEASED;
	result.data.sourceDetected.interactionSourceState = interactionSourceState;
	return result;
}

#pragma endregion  // InteractionManager events

ISAR_CPP_NS_END

#endif  // ISAR_INPUT_TYPES_H
