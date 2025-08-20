/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "FStreamInput.h"
#include "PlayerMappableInputConfig.h"
#include "StreamKeys.h"
#include "Features/IModularFeatures.h"

#include "EnhancedInputLibrary.h"
#include "EnhancedInputSubsystemInterface.h"
#include "EnhancedInputModule.h"
#include "EnhancedInputDeveloperSettings.h"

#include "Async/Async.h"

#if WITH_EDITOR
#include "EnhancedInputEditorSubsystem.h"
#endif

#include <WindowsNumerics.h>

// To fix the issue where Windows headers change our function names
#ifdef GetObject
#undef GetObject
#endif

using namespace isar;

namespace stream_source_names
{
static const FName LEFT("Left");
static const FName RIGHT("Right");
static const FName LEFT_AIM("LeftAim");
static const FName RIGHT_AIM("RightAim");
static const FName LEFT_PALM("LeftPalm");
static const FName RIGHT_PALM("RightPalm");
}

static const std::unordered_map<isar::IsarXRControllerType, std::string> DEVICE_NAMES = {
	{isar::IsarXRControllerType::IsarXRControllerType_HoloLens_Hands, "Hololens Hand"},
	{isar::IsarXRControllerType::IsarXRControllerType_Meta_Quest_Hands, "Quest Hand"},
	{isar::IsarXRControllerType::IsarXRControllerType_Meta_Quest_2_Controller, "Quest 2 Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_Magic_Leap_2_Hands, "Magic Leap 2 Hand"},
	{isar::IsarXRControllerType::IsarXRControllerType_Magic_Leap_2_Controller, "Magic Leap 2 Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_Meta_Quest_Pro_Controller, "Quest Pro Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_Meta_Quest_3_Controller, "Quest 3 Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_Lenovo_VRX_Hands, "VRX Hand"},
	{isar::IsarXRControllerType::IsarXRControllerType_Lenovo_VRX_Controller, "VRX Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_Logitech_MX_Ink_Stylus, "MX Ink Stylus"},
	{isar::IsarXRControllerType::IsarXRControllerType_Pico_4_Ultra_Hands, "Pico 4 Ultra Hand"},
	{isar::IsarXRControllerType::IsarXRControllerType_Pico_4_Ultra_Controller, "Pico 4 Ultra Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_HTC_Vive_Focus_Hands, "Vive Focus Hand"},
	{isar::IsarXRControllerType::IsarXRControllerType_HTC_Vive_Focus_3_Controller, "Vive Focus 3 Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_HTC_Vive_Focus_Vision_Controller, "Vive Focus Vision Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_HTC_Vive_XR_Elite_Controller, "Vive XR Elite Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_Meta_Quest_3S_Controller, "Quest 3S Controller"},
	{isar::IsarXRControllerType::IsarXRControllerType_Apple_Vision_Pro_Hands, "Apple Vision Pro Hand"}
};


// Map controllerType and handedness to a device id to use for registration/deregistration
inline uint32_t MapToDeviceID(IsarInteractionSourceState sourceState)
{
	// x2 for 2 hands and +1 because the HMD always has first id
	return (sourceState.controllerData.controllerIdentifier * 2) + (uint32_t)sourceState.controllerData.handedness + 1u;
}

FStreamInput::FStreamInput()
	  : m_streamConnection(nullptr)
	  , m_serverApi(nullptr)
	  , m_messageHandler(new FGenericApplicationMessageHandler())
	  , m_actionsAttached(false)
{
	IModularFeatures::Get().RegisterModularFeature(IMotionController::GetModularFeatureName(),
												   static_cast<IMotionController*>(this));
	IModularFeatures::Get().RegisterModularFeature(IHandTracker::GetModularFeatureName(),
												   static_cast<IHandTracker*>(this));

	using namespace stream::keys;

	m_2DAxisMap.Add(EKeys::OculusTouch_Left_Thumbstick_2D.GetFName(),
					{EKeys::OculusTouch_Left_Thumbstick_X.GetFName(), EKeys::OculusTouch_Left_Thumbstick_Y.GetFName()});
	m_2DAxisMap.Add(EKeys::OculusTouch_Right_Thumbstick_2D.GetFName(),
					{EKeys::OculusTouch_Right_Thumbstick_X.GetFName(),
					 EKeys::OculusTouch_Right_Thumbstick_Y.GetFName()});
	m_2DAxisMap.Add(MagicLeapController_Left_Trackpad_2D.GetFName(),
					{MagicLeapController_Left_Trackpad_X.GetFName(), MagicLeapController_Left_Trackpad_Y.GetFName()});
	m_2DAxisMap.Add(MagicLeapController_Right_Trackpad_2D.GetFName(),
					{MagicLeapController_Right_Trackpad_X.GetFName(), MagicLeapController_Right_Trackpad_Y.GetFName()});
	m_2DAxisMap.Add(LenovoVRXController_Left_Thumbstick_2D.GetFName(),
					{LenovoVRXController_Left_Thumbstick_X.GetFName(),
					 LenovoVRXController_Left_Thumbstick_Y.GetFName()});
	m_2DAxisMap.Add(LenovoVRXController_Right_Thumbstick_2D.GetFName(),
					{LenovoVRXController_Right_Thumbstick_X.GetFName(),
					 LenovoVRXController_Right_Thumbstick_Y.GetFName()});
	m_2DAxisMap.Add(PICOTouch_Left_Thumbstick_2D.GetFName(),
					{PICOTouch_Left_Thumbstick_X.GetFName(), PICOTouch_Left_Thumbstick_Y.GetFName()});
	m_2DAxisMap.Add(PICOTouch_Right_Thumbstick_2D.GetFName(),
					{PICOTouch_Right_Thumbstick_X.GetFName(), PICOTouch_Right_Thumbstick_Y.GetFName()});
	m_2DAxisMap.Add(Focus3_Left_Thumbstick_2D.GetFName(),
					{Focus3_Left_Thumbstick_X.GetFName(), Focus3_Left_Thumbstick_Y.GetFName()});
	m_2DAxisMap.Add(Focus3_Right_Thumbstick_2D.GetFName(),
					{Focus3_Right_Thumbstick_X.GetFName(), Focus3_Right_Thumbstick_Y.GetFName()});
}

FStreamInput::~FStreamInput()
{
	IModularFeatures::Get().UnregisterModularFeature(IMotionController::GetModularFeatureName(),
													 static_cast<IMotionController*>(this));
	IModularFeatures::Get().UnregisterModularFeature(IHandTracker::GetModularFeatureName(),
													 static_cast<IHandTracker*>(this));
}

void FStreamInput::SetStreamApi(isar::IsarConnection connection, IsarServerApi* serverApi)
{
	m_streamConnection = connection;
	m_serverApi = serverApi;

	auto connectStateHandler = [](IsarConnectionState newState, void* userData){ 
		reinterpret_cast<FStreamInput*>(userData)->OnConnectionStateChanged(newState); 
		};

	m_serverApi->registerConnectionStateHandler(m_streamConnection, connectStateHandler, this);
}

void FStreamInput::Start()
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// Attempt to load the default input config from the input settings.
	const UEnhancedInputDeveloperSettings* inputSettings = GetDefault<UEnhancedInputDeveloperSettings>();
	if (inputSettings)
	{
		for (const auto& context : inputSettings->DefaultMappingContexts)
		{
			if (context.InputMappingContext)
			{
				TStrongObjectPtr<const UInputMappingContext> obj(context.InputMappingContext.LoadSynchronous());
				m_inputMappingContextToPriorityMap.Add(obj, context.Priority);
			}
			else
			{
				UE_LOG(LogHMD, Warning,
					   TEXT(
						   "Default Mapping Contexts contains an Input Mapping Context set to \"None\", ignoring for Stream Actions."
					   ));
			}
		}
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	m_useEnhancedActions = !m_inputMappingContextToPriorityMap.IsEmpty();
	if (m_useEnhancedActions)
	{
		for (auto& controller : m_xrControllers)
		{
			MapEnhancedActions(controller);
		}
	}
}

void FStreamInput::Stop()
{
	m_inputMappingContextToPriorityMap.Reset();
}

void FStreamInput::OnConnectionStateChanged(IsarConnectionState newState)
{
	m_connected = (newState == IsarConnectionState_CONNECTED);

	if (m_connected)
	{
		return;
	}
		
	for (auto controller : m_xrControllers)
	{
		FStreamControllerStateInfo newStateInfo
		{
			.ControllerName = FName(DEVICE_NAMES.at(controller.controllerType).c_str()),
			.NewTrackingStatus = ETrackingStatus::NotTracked,
			.Type = (EXRVisualType)controller.deviceType,
			.Hand = (EControllerHand)(controller.handedness - 1)
		};

		// Actors can have functionalities that need to be done on the Game Thread
		AsyncTask(ENamedThreads::GameThread,
					[newStateInfo, controllerStateHandlers = m_controllerStateHandlers]()
					{
						for (auto controllerStateHandler : controllerStateHandlers)
						{
							controllerStateHandler.GetInterface()->Execute_OnControllerStateChanged(
								controllerStateHandler.GetObject(), newStateInfo);
						}
					});
	}

	m_xrControllers.clear();
}

void FStreamInput::EnumerateSources(TArray<FMotionControllerSource>& sourcesOut) const
{
	check(IsInGameThread());

	sourcesOut.Add(stream_source_names::LEFT);
	sourcesOut.Add(stream_source_names::RIGHT);
	sourcesOut.Add(stream_source_names::LEFT_AIM);
	sourcesOut.Add(stream_source_names::RIGHT_AIM);
	sourcesOut.Add(stream_source_names::LEFT_PALM);
	sourcesOut.Add(stream_source_names::RIGHT_PALM);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

bool FStreamInput::SetPlayerMappableInputConfig(TObjectPtr<class UPlayerMappableInputConfig> inputConfig)
{
	TSet<TObjectPtr<UInputMappingContext>> mappingContexts;
	inputConfig->GetMappingContexts().GetKeys(mappingContexts);
	return AttachInputMappingContexts(mappingContexts);
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

bool FStreamInput::AttachInputMappingContexts(const TSet<TObjectPtr<UInputMappingContext>>& mappingContexts)
{
	for (const auto& context : mappingContexts)
	{
		m_inputMappingContextToPriorityMap.Add(TStrongObjectPtr<UInputMappingContext>(context), 0);
	}

	m_useEnhancedActions = !m_inputMappingContextToPriorityMap.IsEmpty();
	if (m_useEnhancedActions)
		for (auto& controller : m_xrControllers)
			MapEnhancedActions(controller);

	return true;
}

inline IsarVector3 RotateVectorByQuaternion(IsarVector3 const& isarValue, IsarQuaternion const& isarRotation)
{
	using namespace Windows::Foundation::Numerics;
	float3 value = reinterpret_cast<float3 const&>(isarValue);
	quaternion rotation = reinterpret_cast<quaternion const&>(isarRotation);
	float3 outValue = transform(value, rotation);
	return reinterpret_cast<IsarVector3 const&>(outValue);
}

inline IsarVector3 ApplyControlerOffset(IsarPose const& inputPose, IsarVector3 offsetVector)
{
	IsarVector3 outputPosition = inputPose.position;

	IsarVector3 rotatedOffset = RotateVectorByQuaternion(offsetVector, inputPose.orientation);
	outputPosition.x += rotatedOffset.x;
	outputPosition.y += rotatedOffset.y;
	outputPosition.z += rotatedOffset.z;

	return outputPosition;
}

FStreamInput::StreamControllerUpdateData FStreamInput::CreateUpdateData(const IsarInteractionSourceState& sourceState)
{
	StreamControllerUpdateData data{};

	data.controllerPose = sourceState.controllerData.controllerPose;

	// Apply additional required controller offset for proper visualization (version 2024.0 and earlier
	// versions executed this on the client device directly)(Only Quest controllers and the Stylus)
	switch ((IsarXRControllerType)sourceState.controllerData.controllerIdentifier)
	{
		case IsarXRControllerType::IsarXRControllerType_Meta_Quest_2_Controller:
		case IsarXRControllerType::IsarXRControllerType_Meta_Quest_Pro_Controller:
		case IsarXRControllerType::IsarXRControllerType_Meta_Quest_3_Controller:
		case IsarXRControllerType::IsarXRControllerType_Meta_Quest_3S_Controller:
		{
			// These hard coded values corrected controller model visualization
			data.controllerPose.position = ApplyControlerOffset(data.controllerPose, IsarVector3(0, 0.03f, -0.04f));
			break;
		}
		case IsarXRControllerType::IsarXRControllerType_Logitech_MX_Ink_Stylus:
		{
			// These hard coded values corrected stylus model visualization
			data.controllerPose.position = ApplyControlerOffset(data.controllerPose, IsarVector3(0, 0.03f, -0.1f));
			break;
		}
		default:
		{
			break;
		}
	}

	data.pointerPose = sourceState.controllerData.pointerPose;
	data.handData = sourceState.controllerData.handData;

	data.buttons.resize(sourceState.controllerData.buttonsLength);
	memcpy(data.buttons.data(), sourceState.controllerData.buttons, data.buttons.size() * sizeof(IsarButton));
	data.axis1D.resize(sourceState.controllerData.axis1DLength);
	memcpy(data.axis1D.data(), sourceState.controllerData.axis1D, data.axis1D.size() * sizeof(IsarAxis1D));
	data.axis2D.resize(sourceState.controllerData.axis2DLength);
	memcpy(data.axis2D.data(), sourceState.controllerData.axis2D, data.axis2D.size() * sizeof(IsarAxis2D));

	return data;
}

void FStreamInput::Tick(float deltaTime)
{
	if (!m_connected)
		return;

	uint32_t outputCount = 0;
	auto err = m_serverApi->pullSpatialInput(m_streamConnection, nullptr, 0, &outputCount);

	// No input yet received
	if (err || !outputCount)
		return;

	std::vector<IsarSpatialInput> spatialInput{};
	spatialInput.resize(outputCount);

	err = m_serverApi->pullSpatialInput(m_streamConnection, spatialInput.data(), spatialInput.size(), nullptr);
	if (err)
		return;

	for (auto& input : spatialInput)
	{
		auto sourceState = reinterpret_cast<IsarInteractionSourceState&>(input.data);
		switch (input.type)
		{
			case IsarInputType_SOURCE_PRESSED:
			case IsarInputType_SOURCE_RELEASED:
			case IsarInputType_SOURCE_UPDATED:
			{
				auto deviceId = MapToDeviceID(sourceState);
				auto controller = std::find_if(m_xrControllers.begin(), m_xrControllers.end(),
											   [deviceId](auto const& e) { return e.deviceId == deviceId; });

				if (controller == m_xrControllers.end())
				{
					break;
				}

				controller->state = ControllerTrackingState::Tracking;
				controller->updateData = CreateUpdateData(sourceState);
				break;
			}
			case IsarInputType_SOURCE_DETECTED:
			{
				HandleInputSourceDetected(sourceState);
				break;
			}
			case IsarInputType_SOURCE_LOST:
			{
				auto deviceId = MapToDeviceID(sourceState);
				auto controller = std::find_if(m_xrControllers.begin(), m_xrControllers.end(),
											   [deviceId](auto const& e) { return e.deviceId == deviceId; });

				if (controller == m_xrControllers.end())
				{
					break;
				}

				FStreamControllerStateInfo newStateInfo
				{
					.ControllerName = FName(DEVICE_NAMES.at(controller->controllerType).c_str()),
					.NewTrackingStatus = ETrackingStatus::NotTracked,
					.Type = (EXRVisualType)controller->deviceType,
					.Hand = (EControllerHand)(controller->handedness - 1)
				};

				// Actors can have functionalities that need to be done on the Game Thread
				AsyncTask(ENamedThreads::GameThread,
						  [newStateInfo, controllerStateHandlers = m_controllerStateHandlers]()
						  {
							  for (auto controllerStateHandler : controllerStateHandlers)
							  {
								  controllerStateHandler.GetInterface()->Execute_OnControllerStateChanged(
									  controllerStateHandler.GetObject(), newStateInfo);
							  }
						  });

				controller->state = ControllerTrackingState::Lost;
				m_xrControllers.erase(controller);
				break;
			}
			default: break;
		}

		// Freeing the heap memory before overriding the variable so we don't have heap corruption
		free(sourceState.controllerData.buttons);
		sourceState.controllerData.buttons = nullptr;
		free(sourceState.controllerData.axis1D);
		sourceState.controllerData.axis1D = nullptr;
		free(sourceState.controllerData.axis2D);
		sourceState.controllerData.axis2D = nullptr;
	}
}

void FStreamInput::SendControllerEvents()
{
	if (!m_connected)
		return;

	IPlatformInputDeviceMapper& deviceMapper = IPlatformInputDeviceMapper::Get();

	for (auto& controller : m_xrControllers)
	{
		if (controller.state != ControllerTrackingState::Tracking)
			return;
		auto& sourceData = controller.updateData;

		if (m_useEnhancedActions)
		{
			// Buttons
			for (uint32_t i = 0; i < sourceData.buttons.size(); i++)
			{
				isar::IsarXRControllerFeatureKind featureKind;
				// Setting the docked state alone because IsarXRControllerFeatureKind_DOCKED is not in sequence of the buttons group in IsarXRControllerFeatureKind enum.
				if (sourceData.buttons[i].identifier == IsarButtonKind_DOCKED_LEFT || sourceData.buttons[i].identifier
					== IsarButtonKind_DOCKED_RIGHT)
					featureKind = (isar::IsarXRControllerFeatureKind)(isar::IsarXRControllerFeatureKind_DOCKED);
				else
					featureKind = (isar::IsarXRControllerFeatureKind)(sourceData.buttons[i].identifier +
						isar::IsarXRControllerFeatureKind_BUTTON_HOME);

				if (!controller.streamToEnhancedActions.contains(featureKind))
					continue;
				auto inputValue = FInputActionValue(sourceData.buttons[i].value);
				for (auto& mapping : controller.streamToEnhancedActions[featureKind])
				{
					auto injectSubsystemInput = [inputValue, mapping](IEnhancedInputSubsystemInterface* subsystem)
					{
						if (subsystem)
						{
							subsystem->InjectInputForAction(mapping.Action, inputValue, mapping.Modifiers,
															mapping.Triggers);
						}
					};

					IEnhancedInputModule::Get().GetLibrary()->ForEachSubsystem(injectSubsystemInput);
#if WITH_EDITOR
					if (GEditor)
					{
						// UEnhancedInputLibrary::ForEachSubsystem only enumerates runtime subsystems.
						injectSubsystemInput(GEditor->GetEditorSubsystem<UEnhancedInputEditorSubsystem>());
					}
#endif
				}
			}

			// Axis1D
			for (uint32_t i = 0; i < sourceData.axis1D.size(); i++)
			{
				auto featureKind = (isar::IsarXRControllerFeatureKind)(sourceData.axis1D[i].identifier +
					isar::IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS);
				if (controller.streamToEnhancedActions.contains(featureKind))
				{
					auto inputValue = FInputActionValue(sourceData.axis1D[i].value > 0.9f);
					for (auto& mapping : controller.streamToEnhancedActions[featureKind])
					{
						auto injectSubsystemInput = [inputValue, mapping](IEnhancedInputSubsystemInterface* subsystem)
						{
							if (subsystem)
							{
								subsystem->InjectInputForAction(mapping.Action, inputValue, mapping.Modifiers,
																mapping.Triggers);
							}
						};

						IEnhancedInputModule::Get().GetLibrary()->ForEachSubsystem(injectSubsystemInput);
#if WITH_EDITOR
						if (GEditor)
						{
							// UEnhancedInputLibrary::ForEachSubsystem only enumerates runtime subsystems.
							injectSubsystemInput(GEditor->GetEditorSubsystem<UEnhancedInputEditorSubsystem>());
						}
#endif
					}
				}

				featureKind = (isar::IsarXRControllerFeatureKind)(sourceData.axis1D[i].identifier +
					isar::IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER);
				if (!controller.streamToEnhancedActions.contains(featureKind))
					continue;
				auto inputValue = FInputActionValue(sourceData.axis1D[i].value);
				for (auto& mapping : controller.streamToEnhancedActions[featureKind])
				{
					auto injectSubsystemInput = [inputValue, mapping](IEnhancedInputSubsystemInterface* subsystem)
					{
						if (subsystem)
						{
							subsystem->InjectInputForAction(mapping.Action, inputValue, mapping.Modifiers,
															mapping.Triggers);
						}
					};

					IEnhancedInputModule::Get().GetLibrary()->ForEachSubsystem(injectSubsystemInput);
#if WITH_EDITOR
					if (GEditor)
					{
						// UEnhancedInputLibrary::ForEachSubsystem only enumerates runtime subsystems.
						injectSubsystemInput(GEditor->GetEditorSubsystem<UEnhancedInputEditorSubsystem>());
					}
#endif
				}
			}

			// Axis2D
			for (uint32_t i = 0; i < sourceData.axis2D.size(); i++)
			{
				auto featureKind = (isar::IsarXRControllerFeatureKind)(sourceData.axis2D[i].identifier +
					isar::IsarXRControllerFeatureKind_AXIS2D_PRIMARY_ANALOG_STICK);
				if (!controller.streamToEnhancedActions.contains(featureKind))
					continue;
				for (auto& mapping : controller.streamToEnhancedActions[featureKind])
				{
					FInputActionValue inputValue;
					if (mapping.Action->ValueType == EInputActionValueType::Axis2D)
						inputValue = FInputActionValue(
							FVector2D(sourceData.axis2D[i].value.x, sourceData.axis2D[i].value.y));
					else if (mapping.Key.GetFName().ToString().Contains("_X"))
						inputValue = FInputActionValue(sourceData.axis2D[i].value.x);
					else
						inputValue = FInputActionValue(sourceData.axis2D[i].value.y);

					auto injectSubsystemInput = [inputValue, mapping](IEnhancedInputSubsystemInterface* subsystem)
					{
						if (subsystem)
						{
							subsystem->InjectInputForAction(mapping.Action, inputValue, mapping.Modifiers,
															mapping.Triggers);
						}
					};

					IEnhancedInputModule::Get().GetLibrary()->ForEachSubsystem(injectSubsystemInput);
#if WITH_EDITOR
					if (GEditor)
					{
						// UEnhancedInputLibrary::ForEachSubsystem only enumerates runtime subsystems.
						injectSubsystemInput(GEditor->GetEditorSubsystem<UEnhancedInputEditorSubsystem>());
					}
#endif
				}
			}
		}
		else
		{
			// Buttons
			for (uint32_t i = 0; i < sourceData.buttons.size(); i++)
			{
				isar::IsarXRControllerFeatureKind featureKind;
				// Setting the docked state alone because IsarXRControllerFeatureKind_DOCKED is not in sequence of the buttons group in IsarXRControllerFeatureKind enum.
				if (sourceData.buttons[i].identifier == IsarButtonKind_DOCKED_LEFT || sourceData.buttons[i].identifier
					== IsarButtonKind_DOCKED_RIGHT)
					featureKind = (isar::IsarXRControllerFeatureKind)(isar::IsarXRControllerFeatureKind_DOCKED);
				else
					featureKind = (isar::IsarXRControllerFeatureKind)(sourceData.buttons[i].identifier +
						isar::IsarXRControllerFeatureKind_BUTTON_HOME);

				if (!controller.streamToKeyName.contains(featureKind))
					continue;
				if (sourceData.buttons[i].value)
					m_messageHandler->OnControllerButtonPressed(controller.streamToKeyName[featureKind],
																deviceMapper.GetPrimaryPlatformUser(),
																deviceMapper.GetDefaultInputDevice(), /*IsRepeat =*/
																false);
				else
					m_messageHandler->OnControllerButtonReleased(controller.streamToKeyName[featureKind],
																 deviceMapper.GetPrimaryPlatformUser(),
																 deviceMapper.GetDefaultInputDevice(), /*IsRepeat =*/
																 false);
			}

			// Axis1D
			for (uint32_t i = 0; i < sourceData.axis1D.size(); i++)
			{
				auto featureKind = (isar::IsarXRControllerFeatureKind)(sourceData.axis1D[i].identifier +
					isar::IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS);
				if (controller.streamToKeyName.contains(featureKind))
				{
					bool buttonPress = sourceData.axis1D[i].value > 0.9f;
					if (buttonPress)
						m_messageHandler->OnControllerButtonPressed(controller.streamToKeyName[featureKind],
																	deviceMapper.GetPrimaryPlatformUser(),
																	deviceMapper.GetDefaultInputDevice(), /*IsRepeat =*/
																	false);
					else
						m_messageHandler->OnControllerButtonReleased(controller.streamToKeyName[featureKind],
																	 deviceMapper.GetPrimaryPlatformUser(),
																	 deviceMapper.GetDefaultInputDevice(),
																	 /*IsRepeat =*/false);
				}

				featureKind = (isar::IsarXRControllerFeatureKind)(sourceData.axis1D[i].identifier +
					isar::IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER);
				if (!controller.streamToKeyName.contains(featureKind))
					continue;
				m_messageHandler->OnControllerAnalog(controller.streamToKeyName[featureKind],
													 deviceMapper.GetPrimaryPlatformUser(),
													 deviceMapper.GetDefaultInputDevice(), sourceData.axis1D[i].value);
			}

			// Axis2D - Legacy system only supports 2D as two paired 1D axes
			for (uint32_t i = 0; i < sourceData.axis2D.size(); i++)
			{
				auto featureKind = (isar::IsarXRControllerFeatureKind)(sourceData.axis2D[i].identifier +
					isar::IsarXRControllerFeatureKind_AXIS2D_PRIMARY_ANALOG_STICK);
				if (!controller.streamToKeyName.contains(featureKind))
					continue;
				auto& keyPair = m_2DAxisMap[controller.streamToKeyName[featureKind]];
				m_messageHandler->OnControllerAnalog(keyPair.first, deviceMapper.GetPrimaryPlatformUser(),
													 deviceMapper.GetDefaultInputDevice(),
													 sourceData.axis2D[i].value.x);
				m_messageHandler->OnControllerAnalog(keyPair.second, deviceMapper.GetPrimaryPlatformUser(),
													 deviceMapper.GetDefaultInputDevice(),
													 sourceData.axis2D[i].value.y);
			}
		}
	}
}

void FStreamInput::SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& inMessageHandler)
{
	m_messageHandler = inMessageHandler;
}

void FStreamInput::HandleInputSourceDetected(IsarInteractionSourceState const& sourceState)
{
	uint32_t deviceId = MapToDeviceID(sourceState);

	// Check if a controller with that already exists (Don't create over existent controller which waits for being disconnected probably)
	auto existingController = std::find_if(m_xrControllers.begin(), m_xrControllers.end(),
										   [deviceId](auto const& e) { return e.deviceId == deviceId; });
	if (existingController != m_xrControllers.end())
	{
		existingController->updateData = CreateUpdateData(sourceState);
		return;
	}

	StreamController controller{};
	controller.deviceId = deviceId;
	controller.state = ControllerTrackingState::Detected;
	controller.handedness = sourceState.controllerData.handedness;
	controller.controllerType = (IsarXRControllerType)sourceState.controllerData.controllerIdentifier;
	controller.updateData = CreateUpdateData(sourceState);

	using namespace stream::keys;

	auto& featureToKeyMap = controller.streamToKeyName;
	switch (controller.controllerType)
	{
		case IsarXRControllerType_Meta_Quest_2_Controller:
		case IsarXRControllerType_Meta_Quest_Pro_Controller:
		case IsarXRControllerType_Meta_Quest_3_Controller:
		case IsarXRControllerType_Meta_Quest_3S_Controller: controller.deviceType = TrackedDeviceType::Controller;
			switch (controller.handedness)
			{
				case IsarSpatialInteractionSourceHandedness_LEFT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_MENU, EKeys::OculusTouch_Left_Menu_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_X,
													 EKeys::OculusTouch_Left_X_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_Y,
													 EKeys::OculusTouch_Left_Y_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_THUMB_REST,
													 OculusTouch_Left_Thumbrest_Touch.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_ANALOG_STICK_PRESS,
													 EKeys::OculusTouch_Left_Thumbstick_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_PRIMARY_ANALOG_STICK,
													 EKeys::OculusTouch_Left_Thumbstick_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS,
													 EKeys::OculusTouch_Left_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER,
													 EKeys::OculusTouch_Left_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_SQUEEZE_PRESS,
													 EKeys::OculusTouch_Left_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_SQUEEZE,
													 EKeys::OculusTouch_Left_Grip_Axis.GetFName());
					break;
				case IsarSpatialInteractionSourceHandedness_RIGHT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_A, EKeys::OculusTouch_Right_A_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_B,
													 EKeys::OculusTouch_Right_B_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_THUMB_REST,
													 OculusTouch_Right_Thumbrest_Touch.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_ANALOG_STICK_PRESS,
													 EKeys::OculusTouch_Right_Thumbstick_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_SECONDARY_ANALOG_STICK,
													 EKeys::OculusTouch_Right_Thumbstick_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_TRIGGER_PRESS,
													 EKeys::OculusTouch_Right_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_TRIGGER,
													 EKeys::OculusTouch_Right_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_SQUEEZE_PRESS,
													 EKeys::OculusTouch_Right_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_SQUEEZE,
													 EKeys::OculusTouch_Right_Grip_Axis.GetFName());
					break;
				default: break;
			}
			break;
		case IsarXRControllerType_Magic_Leap_2_Controller: controller.deviceType = TrackedDeviceType::Controller;
			switch (controller.handedness)
			{
				case IsarSpatialInteractionSourceHandedness_LEFT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_MENU, MagicLeapController_Left_Menu_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_ANALOG_STICK_PRESS,
													 MagicLeapController_Left_Trackpad_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_PRIMARY_ANALOG_STICK,
													 MagicLeapController_Left_Trackpad_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS,
													 MagicLeapController_Left_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER,
													 MagicLeapController_Left_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_BUMPER,
													 MagicLeapController_Left_Shoulder_Click.GetFName());
					break;
				case IsarSpatialInteractionSourceHandedness_RIGHT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_MENU, MagicLeapController_Right_Menu_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_ANALOG_STICK_PRESS,
													 MagicLeapController_Right_Trackpad_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_SECONDARY_ANALOG_STICK,
													 MagicLeapController_Right_Trackpad_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_TRIGGER_PRESS,
													 MagicLeapController_Right_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_TRIGGER,
													 MagicLeapController_Right_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_BUMPER,
													 MagicLeapController_Right_Shoulder_Click.GetFName());
					break;
				default: break;
			}
			break;
		case IsarXRControllerType_Lenovo_VRX_Controller: controller.deviceType = TrackedDeviceType::Controller;
			switch (controller.handedness)
			{
				case IsarSpatialInteractionSourceHandedness_LEFT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_MENU, LenovoVRXController_Left_Menu_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_X,
													 LenovoVRXController_Left_X_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_Y,
													 LenovoVRXController_Left_Y_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_THUMB_REST,
													 LenovoVRXController_Left_Thumbrest_Touch.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_ANALOG_STICK_PRESS,
													 LenovoVRXController_Left_Thumbstick_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_PRIMARY_ANALOG_STICK,
													 LenovoVRXController_Left_Thumbstick_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS,
													 LenovoVRXController_Left_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER,
													 LenovoVRXController_Left_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_SQUEEZE_PRESS,
													 LenovoVRXController_Left_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_SQUEEZE,
													 LenovoVRXController_Left_Grip_Axis.GetFName());
					break;
				case IsarSpatialInteractionSourceHandedness_RIGHT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_MENU, LenovoVRXController_Right_Menu_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_A,
													 LenovoVRXController_Right_A_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_B,
													 LenovoVRXController_Right_B_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_THUMB_REST,
													 LenovoVRXController_Right_Thumbrest_Touch.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_ANALOG_STICK_PRESS,
													 LenovoVRXController_Right_Thumbstick_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_SECONDARY_ANALOG_STICK,
													 LenovoVRXController_Right_Thumbstick_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_TRIGGER_PRESS,
													 LenovoVRXController_Right_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_TRIGGER,
													 LenovoVRXController_Right_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_SQUEEZE_PRESS,
													 LenovoVRXController_Right_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_SQUEEZE,
													 LenovoVRXController_Right_Grip_Axis.GetFName());
					break;
				default: break;
			}
			break;
		case IsarXRControllerType_Logitech_MX_Ink_Stylus: controller.deviceType = TrackedDeviceType::Controller;
			switch (controller.handedness)
			{
				case IsarSpatialInteractionSourceHandedness_LEFT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_X, LogitechMXInk_Left_ClusterBack_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_Y,
													 LogitechMXInk_Left_ClusterBack_DoubleTap.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_THUMB_REST,
													 LogitechMXInk_Left_ClusterFront_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_ANALOG_STICK_PRESS,
													 LogitechMXInk_Left_ClusterFront_DoubleTap.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS,
													 LogitechMXInk_Left_ClusterMiddle_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER,
													 LogitechMXInk_Left_ClusterMiddle_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_SQUEEZE_PRESS,
													 LogitechMXInk_Left_Tip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_SQUEEZE,
													 LogitechMXInk_Left_Tip_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_DOCKED,
													 LogitechMXInk_Left_Docked_Click.GetFName());
					break;
				case IsarSpatialInteractionSourceHandedness_RIGHT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_A, LogitechMXInk_Right_ClusterBack_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_B,
													 LogitechMXInk_Right_ClusterBack_DoubleTap.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_THUMB_REST,
													 LogitechMXInk_Right_ClusterFront_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_ANALOG_STICK_PRESS,
													 LogitechMXInk_Right_ClusterFront_DoubleTap.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_TRIGGER_PRESS,
													 LogitechMXInk_Right_ClusterMiddle_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_TRIGGER,
													 LogitechMXInk_Right_ClusterMiddle_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_SQUEEZE_PRESS,
													 LogitechMXInk_Right_Tip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_SQUEEZE,
													 LogitechMXInk_Right_Tip_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_DOCKED,
													 LogitechMXInk_Right_Docked_Click.GetFName());
					break;
				default: break;
			}
			break;
		case IsarXRControllerType_Pico_4_Ultra_Controller: controller.deviceType = TrackedDeviceType::Controller;
			switch (controller.handedness)
			{
				case IsarSpatialInteractionSourceHandedness_LEFT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_MENU, PICOTouch_Left_Menu_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_X,
													 PICOTouch_Left_X_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_Y,
													 PICOTouch_Left_Y_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_THUMB_REST,
													 PICOTouch_Left_Thumbrest_Touch.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_ANALOG_STICK_PRESS,
													 PICOTouch_Left_Thumbstick_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_PRIMARY_ANALOG_STICK,
													 PICOTouch_Left_Thumbstick_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS,
													 PICOTouch_Left_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER,
													 PICOTouch_Left_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_SQUEEZE_PRESS,
													 PICOTouch_Left_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_SQUEEZE,
													 PICOTouch_Left_Grip_Axis.GetFName());
					break;
				case IsarSpatialInteractionSourceHandedness_RIGHT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_A, PICOTouch_Right_A_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_B,
													 PICOTouch_Right_B_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_THUMB_REST,
													 PICOTouch_Right_Thumbrest_Touch.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_ANALOG_STICK_PRESS,
													 PICOTouch_Right_Thumbstick_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_SECONDARY_ANALOG_STICK,
													 PICOTouch_Right_Thumbstick_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_TRIGGER_PRESS,
													 PICOTouch_Right_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_TRIGGER,
													 PICOTouch_Right_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_SQUEEZE_PRESS,
													 PICOTouch_Right_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_SQUEEZE,
													 PICOTouch_Right_Grip_Axis.GetFName());
					break;
				default: break;
			}
			break;
		case IsarXRControllerType_HTC_Vive_Focus_3_Controller:
		case IsarXRControllerType_HTC_Vive_Focus_Vision_Controller:
		case IsarXRControllerType_HTC_Vive_XR_Elite_Controller: controller.deviceType =
				TrackedDeviceType::Controller;
			switch (controller.handedness)
			{
				case IsarSpatialInteractionSourceHandedness_LEFT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_MENU, Focus3_Left_Menu_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_X,
													 Focus3_Left_X_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_Y,
													 Focus3_Left_Y_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_THUMB_REST,
													 Focus3_Left_Thumbrest_Touch.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_ANALOG_STICK_PRESS,
													 Focus3_Left_Thumbstick_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_PRIMARY_ANALOG_STICK,
													 Focus3_Left_Thumbstick_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS,
													 Focus3_Left_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER,
													 Focus3_Left_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_SQUEEZE_PRESS,
													 Focus3_Left_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_SQUEEZE,
													 Focus3_Left_Grip_Axis.GetFName());
					break;
				case IsarSpatialInteractionSourceHandedness_RIGHT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_A, Focus3_Right_A_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_B,
													 Focus3_Right_B_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_THUMB_REST,
													 Focus3_Right_Thumbrest_Touch.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_ANALOG_STICK_PRESS,
													 Focus3_Right_Thumbstick_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS2D_SECONDARY_ANALOG_STICK,
													 Focus3_Right_Thumbstick_2D.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_TRIGGER_PRESS,
													 Focus3_Right_Trigger_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_TRIGGER,
													 Focus3_Right_Trigger_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_SQUEEZE_PRESS,
													 Focus3_Right_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_SQUEEZE,
													 Focus3_Right_Grip_Axis.GetFName());
					break;
				default: break;
			}
			break;
		case IsarXRControllerType_HoloLens_Hands:
		case IsarXRControllerType_Meta_Quest_Hands:
		case IsarXRControllerType_Magic_Leap_2_Hands:
		case IsarXRControllerType_Lenovo_VRX_Hands:
		case IsarXRControllerType_Pico_4_Ultra_Hands:
		case IsarXRControllerType_HTC_Vive_Focus_Hands:
		case IsarXRControllerType_Apple_Vision_Pro_Hands: controller.deviceType = TrackedDeviceType::Hand;
			switch (controller.handedness)
			{
				case IsarSpatialInteractionSourceHandedness_LEFT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_MENU, HololightStreamHand_Left_Menu_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_TRIGGER_PRESS,
													 HololightStreamHand_Left_Select_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_TRIGGER,
													 HololightStreamHand_Left_Select_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_PRIMARY_SQUEEZE_PRESS,
													 HololightStreamHand_Left_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_PRIMARY_SQUEEZE,
													 HololightStreamHand_Left_Grip_Axis.GetFName());
					break;
				case IsarSpatialInteractionSourceHandedness_RIGHT: featureToKeyMap.insert_or_assign(
						IsarXRControllerFeatureKind_BUTTON_MENU, HololightStreamHand_Right_Menu_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_TRIGGER_PRESS,
													 HololightStreamHand_Right_Select_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_TRIGGER,
													 HololightStreamHand_Right_Select_Axis.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_BUTTON_SECONDARY_SQUEEZE_PRESS,
													 HololightStreamHand_Right_Grip_Click.GetFName());
					featureToKeyMap.insert_or_assign(IsarXRControllerFeatureKind_AXIS1D_SECONDARY_SQUEEZE,
													 HololightStreamHand_Right_Grip_Axis.GetFName());
					break;
				default: break;
			}
			break;
		default: break;
	}
	if (m_useEnhancedActions)
	{
		MapEnhancedActions(controller);
	}

	m_xrControllers.push_back(controller);

	FStreamControllerStateInfo newStateInfo
	{
		.ControllerName = FName(DEVICE_NAMES.at(controller.controllerType).c_str()),
		.NewTrackingStatus = ETrackingStatus::Tracked,
		.Type = (EXRVisualType)controller.deviceType,
		.Hand = (EControllerHand)(controller.handedness - 1)
	};

	// Actors can have functionalities that need to be done on the Game Thread
	AsyncTask(ENamedThreads::GameThread,
			  [newStateInfo, controllerStateHandlers = m_controllerStateHandlers]()
			  {
				  for (auto controllerStateHandler : controllerStateHandlers)
				  {
					  controllerStateHandler.GetInterface()->Execute_OnControllerStateChanged(
						  controllerStateHandler.GetObject(), newStateInfo);
				  }
			  });
}

bool FStreamInput::GetControllerOrientationAndPosition(const int32 controllerIndex, const FName motionSource,
													   FRotator& outOrientation, FVector& outPosition,
													   float worldToMetersScale) const
{
	outPosition = FVector::ZeroVector;
	outOrientation = FRotator::ZeroRotator;

	if (!m_connected)
		return false;

	IsarVector3 position;
	IsarQuaternion orientation;
	auto it = m_xrControllers.end();

	if (motionSource == stream_source_names::LEFT || motionSource == stream_source_names::LEFT_PALM || motionSource ==
		stream_source_names::LEFT_AIM)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(), [](auto const& controller)
		{
			return controller.handedness == IsarSpatialInteractionSourceHandedness_LEFT;
		});

	}
	else if (motionSource == stream_source_names::RIGHT || motionSource == stream_source_names::RIGHT_PALM ||
		motionSource ==
		stream_source_names::RIGHT_AIM)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(), [](auto const& controller)
		{
			return controller.handedness == IsarSpatialInteractionSourceHandedness_RIGHT;
		});
	}

	if (it == m_xrControllers.end())
		return false;

	if (motionSource == stream_source_names::LEFT_AIM || motionSource == stream_source_names::RIGHT_AIM)
	{
		position = it->updateData.pointerPose.position;
		orientation = it->updateData.pointerPose.orientation;
	}
	else
	{
		position = it->updateData.controllerPose.position;
		orientation = it->updateData.controllerPose.orientation;
	}

	outPosition = FVector(-position.z * worldToMetersScale, position.x * worldToMetersScale,
						  position.y * worldToMetersScale);
	outOrientation = FRotator(FQuat(-orientation.z, orientation.x, orientation.y, -orientation.w));

	return true;
}

ETrackingStatus FStreamInput::GetControllerTrackingStatus(const int32 controllerIndex, const FName motionSource) const
{
	if (!m_connected)
		return ETrackingStatus::NotTracked;

	auto it = m_xrControllers.end();
	if (motionSource == stream_source_names::LEFT || motionSource == stream_source_names::LEFT_PALM || motionSource ==
		stream_source_names::LEFT_AIM)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(), [](auto const& controller)
		{
			return controller.handedness == IsarSpatialInteractionSourceHandedness_LEFT;
		});
	}
	else if (motionSource == stream_source_names::RIGHT || motionSource == stream_source_names::RIGHT_PALM ||
		motionSource ==
		stream_source_names::RIGHT_AIM)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(), [](auto const& controller)
		{
			return controller.handedness == IsarSpatialInteractionSourceHandedness_RIGHT;
		});
	}

	if (it != m_xrControllers.end() && it->state == ControllerTrackingState::Tracking)
	{
		return ETrackingStatus::Tracked;
	}

	return ETrackingStatus::NotTracked;
}

bool FStreamInput::IsHandTrackingStateValid() const
{
	if (!m_connected)
		return false;

	auto it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(), [](auto const& controller)
	{
		return controller.deviceType == TrackedDeviceType::Hand;
	});

	return it != m_xrControllers.end();
}

bool FStreamInput::GetKeypointState(EControllerHand hand, EHandKeypoint keypoint, FTransform& outTransform,
									float& outRadius) const
{
	if (!m_connected)
		return false;

	auto it = m_xrControllers.end();
	if (hand == EControllerHand::Left)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(), [](auto const& controller)
		{
			return controller.deviceType == TrackedDeviceType::Hand && controller.handedness ==
				IsarSpatialInteractionSourceHandedness_LEFT;
		});
	}
	else if (hand == EControllerHand::Right)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(), [](auto const& controller)
		{
			return controller.deviceType == TrackedDeviceType::Hand && controller.handedness ==
				IsarSpatialInteractionSourceHandedness_RIGHT;
		});
	}

	if (it == m_xrControllers.end() || it->state != ControllerTrackingState::Tracking)
		return false;

	auto joint = it->updateData.handData.jointPoses[(uint32)keypoint];
	outTransform = FTransform(ToFQuat(joint.orientation), ToFVector(joint.position));
	outRadius = joint.radius;

	return true;
}

bool FStreamInput::GetAllKeypointStates(EControllerHand hand, TArray<FVector>& outPositions,
										TArray<FQuat>& outRotations, TArray<float>& outRadii) const
{
	if (!m_connected)
		return false;

	auto it = m_xrControllers.end();
	if (hand == EControllerHand::Left)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(), [](auto const& controller)
		{
			return controller.deviceType == TrackedDeviceType::Hand && controller.handedness ==
				IsarSpatialInteractionSourceHandedness_LEFT;
		});
	}
	else if (hand == EControllerHand::Right)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(), [](auto const& controller)
		{
			return controller.deviceType == TrackedDeviceType::Hand && controller.handedness ==
				IsarSpatialInteractionSourceHandedness_RIGHT;
		});
	}

	if (it == m_xrControllers.end() || it->state != ControllerTrackingState::Tracking)
		return false;

	outPositions.Empty(EHandKeypointCount);
	outRotations.Empty(EHandKeypointCount);
	outRadii.Empty(EHandKeypointCount);
	for (int i = 0; i <= isar::IsarXRControllerFeatureKind_HAND_LITTLE_TIP; i++)
	{
		auto joint = it->updateData.handData.jointPoses[i];
		outPositions.Add(ToFVector(joint.position));
		outRotations.Add(ToFQuat(joint.orientation));
		outRadii.Add(joint.radius);
	}

	return true;
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
bool FStreamInput::GetAllKeypointStates(EControllerHand hand, TArray<FVector>& outPositions,
										TArray<FQuat>& outRotations, TArray<float>& outRadii, bool& outIsTracked) const
{
	outIsTracked = GetAllKeypointStates(hand, outPositions, outRotations, outRadii);
	return outIsTracked;
}
#endif

void FStreamInput::MapEnhancedActions(StreamController& controller)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	for (const auto& mappingContext : m_inputMappingContextToPriorityMap)
	{
		for (const FEnhancedActionKeyMapping& mapping : mappingContext.Key->GetMappings())
		{
			if (!mapping.Action)
			{
				continue;
			}

			for (auto& [feature, keyName] : controller.streamToKeyName)
			{
				if (mapping.Key.GetFName() == keyName)
				{
					controller.streamToEnhancedActions[feature].push_back(mapping);
				}

				if (m_2DAxisMap.Contains(keyName))
				{
					if (m_2DAxisMap[keyName].first == mapping.Key.GetFName())
						controller.streamToEnhancedActions[feature].push_back(mapping);
					if (m_2DAxisMap[keyName].second == mapping.Key.GetFName())
						controller.streamToEnhancedActions[feature].push_back(mapping);
				}
			}
		}
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

DeviceInfo FStreamInput::GetDeviceInfo(EControllerHand hand)
{
	if (!m_connected)
	{
		return {-1, "", FVector::ZeroVector, FQuat::Identity};
	}

	auto it = m_xrControllers.end();
	if (hand == EControllerHand::Left)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(),
						  [](auto const& e) {
							  return e.handedness == IsarSpatialInteractionSourceHandedness_LEFT;
						  });
	}
	else if (hand == EControllerHand::Right)
	{
		it = std::find_if(m_xrControllers.begin(), m_xrControllers.end(),
						  [](auto const& e) {
							  return e.handedness == IsarSpatialInteractionSourceHandedness_RIGHT;
						  });
	}

	if (it == m_xrControllers.end() || it->state != ControllerTrackingState::Tracking)
	{
		return {-1, "", FVector::ZeroVector, FQuat::Identity};
	}

	
	auto outPosition = FVector(-it->updateData.controllerPose.position.z,
								it->updateData.controllerPose.position.x,
								it->updateData.controllerPose.position.y);

	auto outOrientation = FQuat(-it->updateData.controllerPose.orientation.z,
								 it->updateData.controllerPose.orientation.x,
								 it->updateData.controllerPose.orientation.y,
								-it->updateData.controllerPose.orientation.w);

	return {(int32_t)it->deviceId, DEVICE_NAMES.at(it->controllerType), outPosition, outOrientation};
}

void FStreamInput::RegisterControllerStateHandler(
	TScriptInterface<IStreamControllerStateHandler> controllerStateHandler)
{
	m_controllerStateHandlers.Add(controllerStateHandler);
}

void FStreamInput::UnregisterControllerStateHandler(
	TScriptInterface<IStreamControllerStateHandler> controllerStateHandler)
{
	m_controllerStateHandlers.Remove(controllerStateHandler);
}