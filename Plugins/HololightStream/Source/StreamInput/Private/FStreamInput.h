/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_FSTREAMINPUT_H
#define HOLOLIGHT_UNREAL_FSTREAMINPUT_H

#include "StreamInputCommon.h"

#include "IInputDevice.h"
#include "XRMotionControllerBase.h"
#include "IHandTracker.h"
#include "InputMappingContext.h"
#include "UObject/ObjectPtr.h"
#include "UObject/StrongObjectPtr.h"
#include "Runtime/Launch/Resources/Version.h"

#include "IStreamExtension.h"

#include <vector>
#include <unordered_map>
#include <utility>

using namespace isar;

class STREAMINPUT_API FStreamInput : public IInputDevice, public FXRMotionControllerBase, public IHandTracker, public IStreamExtension
{
public:
	FStreamInput();
	~FStreamInput() override;

	// IStreamInput
	void SetStreamApi(isar::IsarConnection connection, isar::IsarServerApi* serverApi) override;
	void Start() override;
	void Stop() override;
	// Does not do anything, unused interface function
	void SetConnected(bool connected) override {};

	// IInputDevice
	void Tick(float deltaTime) override;
	void SendControllerEvents() override;
	void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& inMessageHandler) override;
	bool Exec(UWorld* inWorld, const TCHAR* cmd, FOutputDevice& ar) override { return true; };

	void SetChannelValue(int32 controllerId, FForceFeedbackChannelType channelType, float value) override
	{
	};

	void SetChannelValues(int32 controllerId, const FForceFeedbackValues& values) override
	{
	};

	// FXRMotionControllerBase
	FName GetMotionControllerDeviceTypeName() const override
	{
		return FName(TEXT("Stream"));
	};

	bool GetControllerOrientationAndPosition(const int32 controllerIndex, const FName motionSource,
													 FRotator& outOrientation, FVector& outPosition,
													 float worldToMetersScale) const override;

	ETrackingStatus	GetControllerTrackingStatus(const int32 controllerIndex, const FName motionSource) const override;

	void EnumerateSources(TArray<FMotionControllerSource>& sourcesOut) const override;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	bool SetPlayerMappableInputConfig(TObjectPtr<class UPlayerMappableInputConfig> inputConfig) override;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
	bool AttachInputMappingContexts(const TSet<TObjectPtr<UInputMappingContext>>& mappingContexts) override;
#else
	bool AttachInputMappingContexts(const TSet<TObjectPtr<UInputMappingContext>>& MappingContexts);
#endif

	// IHandTracker
	FName GetHandTrackerDeviceTypeName() const override
	{
		return FName(TEXT("Stream"));
	};

	bool IsHandTrackingStateValid() const override;
	bool GetKeypointState(EControllerHand hand, EHandKeypoint keypoint, FTransform& outTransform,
						  float& outRadius) const override;
	bool GetAllKeypointStates(EControllerHand hand, TArray<FVector>& outPositions, TArray<FQuat>& outRotations,
							  TArray<float>& outRadii) const override;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	bool GetAllKeypointStates(EControllerHand hand, TArray<FVector>& outPositions, TArray<FQuat>& outRotations,
	                          TArray<float>& outRadii, bool& outIsTracked) const override;
#endif
	
	DeviceInfo GetDeviceInfo(EControllerHand hand);

	void RegisterControllerStateHandler(TScriptInterface<IStreamControllerStateHandler> controllerStateHandler);
	void UnregisterControllerStateHandler(TScriptInterface<IStreamControllerStateHandler> controllerStateHandler);

private:
	enum class ControllerTrackingState
	{
		Detected,
		Tracking,
		Lost
	};

	enum class TrackedDeviceType
	{
		Controller,
		Hand
	};

	struct StreamControllerUpdateData
	{
		IsarPose controllerPose;
		IsarPose pointerPose;
		IsarHandPose handData;
		std::vector<IsarButton> buttons;
		std::vector<IsarAxis1D> axis1D;
		std::vector<IsarAxis2D> axis2D;
	};

	struct StreamController
	{
		uint32_t deviceId;
		IsarSpatialInteractionSourceHandedness handedness;
		IsarXRControllerType controllerType;
		TrackedDeviceType deviceType;
		StreamControllerUpdateData updateData;
		ControllerTrackingState state = ControllerTrackingState::Detected;
		std::unordered_map<IsarXRControllerFeatureKind, FName> streamToKeyName;
		std::unordered_map<IsarXRControllerFeatureKind, std::vector<FEnhancedActionKeyMapping>> streamToEnhancedActions;
	};

	IsarConnection m_streamConnection;
	IsarServerApi* m_serverApi;
	bool m_connected = false;

	std::vector<StreamController> m_xrControllers;
	TMap<FName, std::pair<FName, FName>> m_2DAxisMap;
	bool m_useEnhancedActions = false;

	/** handler to send all messages to */
	TSharedRef<FGenericApplicationMessageHandler> m_messageHandler;

	bool m_actionsAttached;
	TMap<TStrongObjectPtr<const UInputMappingContext>, uint32> m_inputMappingContextToPriorityMap;

	TArray<TScriptInterface<IStreamControllerStateHandler>> m_controllerStateHandlers;

	StreamControllerUpdateData CreateUpdateData(const IsarInteractionSourceState& sourceState);

	void OnConnectionStateChanged(IsarConnectionState newState);
	void HandleInputSourceDetected(IsarInteractionSourceState const& sourceState);

	void MapEnhancedActions(StreamController& controller);
};

#endif // HOLOLIGHT_UNREAL_FSTREAMINPUT_H