/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "StreamInputModule.h"
#include "StreamKeys.h"

#include <functional>

IMPLEMENT_MODULE(FStreamInputModule, StreamInput)

#define LOCTEXT_NAMESPACE "StreamInputModule"

void FStreamInputModule::StartupModule()
{
	IInputDeviceModule::StartupModule();

	AddKeys();

	m_inputDevice = MakeShared<FStreamInput>();

	IStreamHMD* streamHMD = nullptr;
	auto* xrSystem = GEngine->XRSystem.Get();
	try
	{
		streamHMD = (xrSystem ? static_cast<IStreamHMD*>(xrSystem) : nullptr);
		if (streamHMD)
		{
			streamHMD->SetInputModule(static_cast<IStreamExtension*>(m_inputDevice.Get()));
			streamHMD->SetDeviceInfoCallback(
			    std::bind(&FStreamInput::GetDeviceInfo, m_inputDevice, std::placeholders::_1));
		}
	}
	catch (std::exception& e)
	{
		UE_LOG(LogHMD, Error, TEXT("Failed to initialize Hololight Stream Input with the exception: %hs, input may not work properly."), e.what());
	}
	catch (...)
	{
		UE_LOG(LogHMD, Error, TEXT("Failed to initialize Hololight Stream Input, input may not work properly."));
	}
}

TSharedPtr<class IInputDevice> FStreamInputModule::CreateInputDevice(
	const TSharedRef<FGenericApplicationMessageHandler>& inMessageHandler)
{
	if (m_inputDevice)
		m_inputDevice->SetMessageHandler(inMessageHandler);
	return m_inputDevice;
}

inline void AddNonExistingKey(const TArray<FKey>& existingKeys, const FKeyDetails& keyDetails)
{
	if (!existingKeys.Contains(keyDetails.GetKey()))
	{
		EKeys::AddKey(keyDetails);
	}
}

inline void AddNonExistingPairedKey(const TArray<FKey>& existingKeys, const FKeyDetails& pairedKeyDetails,
									FKey keyX, FKey keyY)
{
	if (!existingKeys.Contains(pairedKeyDetails.GetKey()))
	{
		EKeys::AddPairedKey(pairedKeyDetails, keyX, keyY);
	}
}

inline void AddML2Keys(TArray<FKey> existingKeys)
{
	using namespace stream::keys;

	if (EKeys::GetMenuCategoryDisplayName("MagicLeapController").ToString() != "Magic Leap Controller")
	{
		EKeys::AddMenuCategoryDisplayInfo("MagicLeapController",
										  LOCTEXT("MagicLeapControllerSubCategory", "Magic Leap Controller"),
										  TEXT("GraphEditor.PadEvent_16x"));
	}

	// Left ML2
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Left_Menu_Click,
								  LOCTEXT("MagicLeapController_Left_Menu_Click", "Magic Leap (L) Menu"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Left_Trigger_Click,
								  LOCTEXT("MagicLeapController_Left_Trigger_Click", "Magic Leap (L) Trigger"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Left_Trigger_Axis,
								  LOCTEXT("MagicLeapController_Left_Trigger_Axis", "Magic Leap (L) Trigger Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Left_Trackpad_X,
								  LOCTEXT("MagicLeapController_Left_Trackpad_X", "Magic Leap (L) Trackpad X-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Left_Trackpad_Y,
								  LOCTEXT("MagicLeapController_Left_Trackpad_Y", "Magic Leap (L) Trackpad Y-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingPairedKey(
		existingKeys,
		FKeyDetails(MagicLeapController_Left_Trackpad_2D,
					LOCTEXT("MagicLeapController_Left_Trackpad_2D", "Magic Leap (L) Trackpad 2D-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey,
					"MagicLeapController"),
		MagicLeapController_Left_Trackpad_X, MagicLeapController_Left_Trackpad_Y);
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Left_Trackpad_Click,
								  LOCTEXT("MagicLeapController_Left_Trackpad_Click", "Magic Leap (L) Trackpad Click"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Left_Shoulder_Click,
								  LOCTEXT("MagicLeapController_Left_Shoulder_Click", "Magic Leap (L) Shoulder"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));

	// Right ML2
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Right_Menu_Click,
								  LOCTEXT("MagicLeapController_Right_Menu_Click", "Magic Leap (R) Menu"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Right_Trigger_Click,
								  LOCTEXT("MagicLeapController_Right_Trigger_Click", "Magic Leap (R) Trigger"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Right_Trigger_Axis,
								  LOCTEXT("MagicLeapController_Right_Trigger_Axis", "Magic Leap (R) Trigger Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Right_Trackpad_X,
								  LOCTEXT("MagicLeapController_Right_Trackpad_X", "Magic Leap (R) Trackpad X-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Right_Trackpad_Y,
								  LOCTEXT("MagicLeapController_Right_Trackpad_Y", "Magic Leap (R) Trackpad Y-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingPairedKey(
		existingKeys,
		FKeyDetails(MagicLeapController_Right_Trackpad_2D,
					LOCTEXT("MagicLeapController_Right_Trackpad_2D", "Magic Leap (R) Trackpad 2D-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey,
					"MagicLeapController"),
		MagicLeapController_Right_Trackpad_X, MagicLeapController_Right_Trackpad_Y);
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Right_Trackpad_Click,
								  LOCTEXT("MagicLeapController_Right_Trackpad_Click", "Magic Leap (R) Trackpad Click"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(MagicLeapController_Right_Shoulder_Click,
								  LOCTEXT("MagicLeapController_Right_Shoulder_Click", "Magic Leap (R) Shoulder"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "MagicLeapController"));
}

inline void AddVRXKeys(TArray<FKey> existingKeys)
{
	using namespace stream::keys;

	if (EKeys::GetMenuCategoryDisplayName("LenovoVRXController").ToString() != "Lenovo VRX Controller")
	{
		EKeys::AddMenuCategoryDisplayInfo("LenovoVRXController",
										  LOCTEXT("LenovoVRXControllerSubCategory", "Lenovo VRX Controller"),
										  TEXT("GraphEditor.PadEvent_16x"));
	}

	// Left Lenovo VRX
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_X_Click,
								  LOCTEXT("LenovoVRXController_Left_X_Click", "Lenovo VRX (L) X Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_Y_Click,
								  LOCTEXT("LenovoVRXController_Left_Y_Click", "Lenovo VRX (L) Y Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_Menu_Click,
								  LOCTEXT("LenovoVRXController_Left_Menu_Click", "Lenovo VRX (L) Menu"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_Grip_Click,
								  LOCTEXT("LenovoVRXController_Left_Grip_Click", "Lenovo VRX (L) Grip"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_Grip_Axis,
								  LOCTEXT("LenovoVRXController_Left_Grip_Axis", "Lenovo VRX (L) Grip Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_Trigger_Click,
								  LOCTEXT("LenovoVRXController_Left_Trigger_Click", "Lenovo VRX (L) Trigger"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_Trigger_Axis,
								  LOCTEXT("LenovoVRXController_Left_Trigger_Axis", "Lenovo VRX (L) Trigger Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_Thumbstick_X,
								  LOCTEXT("LenovoVRXController_Left_Thumbstick_X", "Lenovo VRX (L) Thumbstick X-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_Thumbstick_Y,
								  LOCTEXT("LenovoVRXController_Left_Thumbstick_Y", "Lenovo VRX (L) Thumbstick Y-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingPairedKey(
		existingKeys,
		FKeyDetails(LenovoVRXController_Left_Thumbstick_2D,
					LOCTEXT("LenovoVRXController_Left_Thumbstick_2D", "Lenovo VRX (L) Thumbstick 2D-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey,
					"LenovoVRXController"),
		LenovoVRXController_Left_Thumbstick_X, LenovoVRXController_Left_Thumbstick_Y);
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LenovoVRXController_Left_Thumbstick_Click,
					LOCTEXT("LenovoVRXController_Left_Thumbstick_Click", "Lenovo VRX (L) Thumbstick Button"),
					FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Left_Thumbrest_Touch,
								  LOCTEXT("LenovoVRXController_Left_Thumbrest_Touch", "Lenovo VRX (L) Thumbrest Touch"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));

	// Right Lenovo VRX
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Right_A_Click,
								  LOCTEXT("LenovoVRXController_Right_A_Click", "Lenovo VRX (R) A Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Right_B_Click,
								  LOCTEXT("LenovoVRXController_Right_B_Click", "Lenovo VRX (R) B Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Right_Menu_Click,
								  LOCTEXT("LenovoVRXController_Right_Menu_Click", "Lenovo VRX (R) Menu"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Right_Grip_Click,
								  LOCTEXT("LenovoVRXController_Right_Grip_Click", "Lenovo VRX (R) Grip"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Right_Grip_Axis,
								  LOCTEXT("LenovoVRXController_Right_Grip_Axis", "Lenovo VRX (R) Grip Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Right_Trigger_Click,
								  LOCTEXT("LenovoVRXController_Right_Trigger_Click", "Lenovo VRX (R) Trigger"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Right_Trigger_Axis,
								  LOCTEXT("LenovoVRXController_Right_Trigger_Axis", "Lenovo VRX (R) Trigger Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Right_Thumbstick_X,
								  LOCTEXT("LenovoVRXController_Right_Thumbstick_X", "Lenovo VRX (R) Thumbstick X-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LenovoVRXController_Right_Thumbstick_Y,
								  LOCTEXT("LenovoVRXController_Right_Thumbstick_Y", "Lenovo VRX (R) Thumbstick Y-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "LenovoVRXController"));
	AddNonExistingPairedKey(
		existingKeys,
		FKeyDetails(LenovoVRXController_Right_Thumbstick_2D,
					LOCTEXT("LenovoVRXController_Right_Thumbstick_2D", "Lenovo VRX (R) Thumbstick 2D-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey,
					"LenovoVRXController"),
		LenovoVRXController_Right_Thumbstick_X, LenovoVRXController_Right_Thumbstick_Y);
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LenovoVRXController_Right_Thumbstick_Click,
					LOCTEXT("LenovoVRXController_Right_Thumbstick_Click", "Lenovo VRX (R) Thumbstick Button"),
					FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "LenovoVRXController"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LenovoVRXController_Right_Thumbrest_Touch,
					LOCTEXT("LenovoVRXController_Right_Thumbrest_Touch", "Lenovo VRX (R) Thumbrest Touch"),
					FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "LenovoVRXController"));
}

inline void AddPICOTouchKeys(TArray<FKey> existingKeys)
{
	using namespace stream::keys;

	if (EKeys::GetMenuCategoryDisplayName("PICOTouch").ToString() != "PICO Touch")
	{
		EKeys::AddMenuCategoryDisplayInfo("PICOTouch", LOCTEXT("PICOTouchSubCategory", "PICO Touch"),
										  TEXT("GraphEditor.PadEvent_16x"));
	}

	// Left Pico Touch
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Left_X_Click, LOCTEXT("PICOTouch_Left_X_Click", "PICO Touch (L) X Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Left_Y_Click, LOCTEXT("PICOTouch_Left_Y_Click", "PICO Touch (L) Y Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Left_Menu_Click,
								  LOCTEXT("PICOTouch_Left_Menu_Click", "PICO Touch (L) Menu"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Left_Grip_Click,
								  LOCTEXT("PICOTouch_Left_Grip_Click", "PICO Touch (L) Grip"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(PICOTouch_Left_Grip_Axis, LOCTEXT("PICOTouch_Left_Grip_Axis", "PICO Touch (L) Grip Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Left_Trigger_Click,
								  LOCTEXT("PICOTouch_Left_Trigger_Click", "PICO Touch (L) Trigger"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(PICOTouch_Left_Trigger_Axis, LOCTEXT("PICOTouch_Left_Trigger_Axis", "PICO Touch (L) Trigger Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Left_Thumbstick_X,
								  LOCTEXT("PICOTouch_Left_Thumbstick_X", "PICO Touch (L) Thumbstick X-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Left_Thumbstick_Y,
								  LOCTEXT("PICOTouch_Left_Thumbstick_Y", "PICO Touch (L) Thumbstick Y-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "PICOTouch"));
	AddNonExistingPairedKey(
		existingKeys,
		FKeyDetails(PICOTouch_Left_Thumbstick_2D,
					LOCTEXT("PICOTouch_Left_Thumbstick_2D", "PICO Touch (L) Thumbstick 2D-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"),
		PICOTouch_Left_Thumbstick_X, PICOTouch_Left_Thumbstick_Y);
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Left_Thumbstick_Click,
								  LOCTEXT("PICOTouch_Left_Thumbstick_Click", "PICO Touch (L) Thumbstick"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Left_Thumbrest_Touch,
								  LOCTEXT("PICOTouch_Left_Thumbrest_Touch", "PICO Touch (L) Thumbrest Touch"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));

	// Right Pico Touch
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Right_A_Click, LOCTEXT("PICOTouch_Right_A_Click", "PICO Touch (R) A Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Right_B_Click, LOCTEXT("PICOTouch_Right_B_Click", "PICO Touch (R) B Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Right_Grip_Click,
								  LOCTEXT("PICOTouch_Right_Grip_Click", "PICO Touch (R) Grip"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(PICOTouch_Right_Grip_Axis, LOCTEXT("PICOTouch_Right_Grip_Axis", "PICO Touch (R) Grip Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Right_Trigger_Click,
								  LOCTEXT("PICOTouch_Right_Trigger_Click", "PICO Touch (R) Trigger"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Right_Trigger_Axis,
								  LOCTEXT("PICOTouch_Right_Trigger_Axis", "PICO Touch (R) Trigger Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Right_Thumbstick_X,
								  LOCTEXT("PICOTouch_Right_Thumbstick_X", "PICO Touch (R) Thumbstick X-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Right_Thumbstick_Y,
								  LOCTEXT("PICOTouch_Right_Thumbstick_Y", "PICO Touch (R) Thumbstick Y-Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "PICOTouch"));
	AddNonExistingPairedKey(
		existingKeys,
		FKeyDetails(PICOTouch_Right_Thumbstick_2D,
					LOCTEXT("PICOTouch_Right_Thumbstick_2D", "PICO Touch (R) Thumbstick 2D-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"),
		PICOTouch_Right_Thumbstick_X, PICOTouch_Right_Thumbstick_Y);
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Right_Thumbstick_Click,
								  LOCTEXT("PICOTouch_Right_Thumbstick_Click", "PICO Touch (R) Thumbstick"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(PICOTouch_Right_Thumbrest_Touch,
								  LOCTEXT("PICOTouch_Right_Thumbrest_Touch", "PICO Touch (R) Thumbrest Touch"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOTouch"));
}

inline void AddFocus3Keys(TArray<FKey> existingKeys)
{
	using namespace stream::keys;

	if (EKeys::GetMenuCategoryDisplayName("Focus3").ToString() != "HTC Focus3")
	{
		EKeys::AddMenuCategoryDisplayInfo("Focus3", LOCTEXT("Focus3SubCategory", "HTC Focus3"),
										  TEXT("GraphEditor.PadEvent_16x"));
	}

	// Left Vive Focus 3
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Left_X_Click, LOCTEXT("Focus3_Left_X_Click", "Focus3 (L) X Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Left_Y_Click, LOCTEXT("Focus3_Left_Y_Click", "Focus3 (L) Y Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Left_Menu_Click, LOCTEXT("Focus3_Left_Menu_Click", "Focus3 (L) Menu"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Left_Grip_Click, LOCTEXT("Focus3_Left_Grip_Click", "Focus3 (L) Grip"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Left_Grip_Axis, LOCTEXT("Focus3_Left_Grip_Axis", "Focus3 (L) Grip Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Left_Trigger_Click, LOCTEXT("Focus3_Left_Trigger_Click", "Focus3 (L) Trigger"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(Focus3_Left_Trigger_Axis, LOCTEXT("Focus3_Left_Trigger_Axis", "Focus3 (L) Trigger Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(Focus3_Left_Thumbstick_X, LOCTEXT("Focus3_Left_Thumbstick_X", "Focus3 (L) Thumbstick X-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(Focus3_Left_Thumbstick_Y, LOCTEXT("Focus3_Left_Thumbstick_Y", "Focus3 (L) Thumbstick Y-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingPairedKey(
		existingKeys,
		FKeyDetails(Focus3_Left_Thumbstick_2D, LOCTEXT("Focus3_Left_Thumbstick_2D", "Focus3 (L) Thumbstick 2D-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "Focus3"),
		Focus3_Left_Thumbstick_X, Focus3_Left_Thumbstick_Y);
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Left_Thumbstick_Click,
								  LOCTEXT("Focus3_Left_Thumbstick_Click", "Focus3 (L) Thumbstick"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Left_Thumbrest_Touch,
								  LOCTEXT("Focus3_Left_Thumbrest_Touch", "Focus3 (L) Thumbrest Touch"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));

	// Right Vive Focus 3
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Right_A_Click, LOCTEXT("Focus3_Right_A_Click", "Focus3 (R) A Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Right_B_Click, LOCTEXT("Focus3_Right_B_Click", "Focus3 (R) B Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Right_Grip_Click, LOCTEXT("Focus3_Right_Grip_Click", "Focus3 (R) Grip"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Right_Grip_Axis, LOCTEXT("Focus3_Right_Grip_Axis", "Focus3 (R) Grip Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Right_Trigger_Click,
								  LOCTEXT("Focus3_Right_Trigger_Click", "Focus3 (R) Trigger"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(Focus3_Right_Trigger_Axis, LOCTEXT("Focus3_Right_Trigger_Axis", "Focus3 (R) Trigger Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(Focus3_Right_Thumbstick_X, LOCTEXT("Focus3_Right_Thumbstick_X", "Focus3 (R) Thumbstick X-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(Focus3_Right_Thumbstick_Y, LOCTEXT("Focus3_Right_Thumbstick_Y", "Focus3 (R) Thumbstick Y-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingPairedKey(
		existingKeys,
		FKeyDetails(Focus3_Right_Thumbstick_2D, LOCTEXT("Focus3_Right_Thumbstick_2D", "Focus3 (R) Thumbstick 2D-Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "Focus3"),
		Focus3_Right_Thumbstick_X, Focus3_Right_Thumbstick_Y);
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Right_Thumbstick_Click,
								  LOCTEXT("Focus3_Right_Thumbstick_Click", "Focus3 (R) Thumbstick"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(Focus3_Right_Thumbrest_Touch,
								  LOCTEXT("Focus3_Right_Thumbrest_Touch", "Focus3 (R) Thumbrest Touch"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "Focus3"));
}

inline void AddMXInkKeys(TArray<FKey> existingKeys)
{
	using namespace stream::keys;

	if (EKeys::GetMenuCategoryDisplayName("MXInk").ToString() != "Logitech MX Ink Stylus")
	{
		EKeys::AddMenuCategoryDisplayInfo("MXInk", LOCTEXT("MXInkSubCategory", "Logitech MX Ink Stylus"),
										  TEXT("GraphEditor.PadEvent_16x"));
	}

	// Left MX Ink
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Left_ClusterBack_Click,
								  LOCTEXT("LogitechMXInk_Left_ClusterBack_Click", "Logitech MX Ink (L) Cluster Back"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LogitechMXInk_Left_ClusterBack_DoubleTap,
					LOCTEXT("LogitechMXInk_Left_ClusterBack_DoubleTap", "Logitech MX Ink (L) Cluster Back Double Tap"),
					FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Left_ClusterFront_Click,
								  LOCTEXT("LogitechMXInk_Left_ClusterFront_Click", "Logitech MX Ink (L) Cluster Front"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Left_ClusterFront_DoubleTap,
								  LOCTEXT("LogitechMXInk_Left_ClusterFront_DoubleTap",
										  "Logitech MX Ink (L) Cluster Front Double Tap"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LogitechMXInk_Left_ClusterMiddle_Click,
					LOCTEXT("LogitechMXInk_Left_ClusterMiddle_Click", "Logitech MX Ink (L) Cluster Middle"),
					FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LogitechMXInk_Left_ClusterMiddle_Axis,
					LOCTEXT("LogitechMXInk_Left_ClusterMiddle_Axis", "Logitech MX Ink (L) Cluster Middle Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Left_Tip_Click,
								  LOCTEXT("LogitechMXInk_Left_Tip_Click", "Logitech MX Ink (L) Tip Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LogitechMXInk_Left_Tip_Axis, LOCTEXT("LogitechMXInk_Left_Tip_Axis", "Logitech MX Ink (L) Tip Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Left_Docked_Click,
								  LOCTEXT("LogitechMXInk_Left_Docked_Click", "Logitech MX Ink (L) Docked"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));

	// Right MX Ink
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Right_ClusterBack_Click,
								  LOCTEXT("LogitechMXInk_Right_ClusterBack_Click", "Logitech MX Ink (R) Cluster Back"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LogitechMXInk_Right_ClusterBack_DoubleTap,
					LOCTEXT("LogitechMXInk_Right_ClusterBack_DoubleTap", "Logitech MX Ink (R) Cluster Back Double Tap"),
					FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LogitechMXInk_Right_ClusterFront_Click,
					LOCTEXT("LogitechMXInk_Right_ClusterFront_Click", "Logitech MX Ink (R) Cluster Front"),
					FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Right_ClusterFront_DoubleTap,
								  LOCTEXT("LogitechMXInk_Right_ClusterFront_DoubleTap",
										  "Logitech MX Ink (R) Cluster Front Double Tap"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LogitechMXInk_Right_ClusterMiddle_Click,
					LOCTEXT("LogitechMXInk_Right_ClusterMiddle_Click", "Logitech MX Ink (R) Cluster Middle"),
					FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(
		existingKeys,
		FKeyDetails(LogitechMXInk_Right_ClusterMiddle_Axis,
					LOCTEXT("LogitechMXInk_Right_ClusterMiddle_Axis", "Logitech MX Ink (R) Cluster Middle Axis"),
					FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Right_Tip_Click,
								  LOCTEXT("LogitechMXInk_Right_Tip_Click", "Logitech MX Ink (R) Tip Press"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Right_Tip_Axis,
								  LOCTEXT("LogitechMXInk_Right_Tip_Axis", "Logitech MX Ink (R) Tip Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "MXInk"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(LogitechMXInk_Right_Docked_Click,
								  LOCTEXT("LogitechMXInk_Right_Docked_Click", "Logitech MX Ink (R) Docked"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MXInk"));
}

inline void AddStreamHandKeys(TArray<FKey> existingKeys)
{
	using namespace stream::keys;

	if (EKeys::GetMenuCategoryDisplayName("HololightStreamHand").ToString() != "Hololight Stream Hand Interaction")
	{
		EKeys::AddMenuCategoryDisplayInfo("HololightStreamHand", LOCTEXT("StreamHandSubCategory", "Hololight Stream Hand Interaction"),
										  TEXT("GraphEditor.PadEvent_16x"));
	}

	// Left Stream Hand
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Left_Menu_Click,
								  LOCTEXT("HololightStreamHand_Left_Menu_Click", "Hololight Stream Hand Interaction (L) Menu"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "HololightStreamHand"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Left_Select_Click,
								  LOCTEXT("HololightStreamHand_Left_Select_Click", "Hololight Stream Hand Interaction (L) Select"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "HololightStreamHand"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Left_Select_Axis,
								  LOCTEXT("HololightStreamHand_Left_Select_Axis", "Hololight Stream Hand Interaction (L) Select Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "HololightStreamHand"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Left_Grip_Click,
								  LOCTEXT("HololightStreamHand_Left_Grip_Click", "Hololight Stream Hand Interaction (L) Grip"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "HololightStreamHand"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Left_Grip_Axis,
								  LOCTEXT("HololightStreamHand_Left_Grip_Axis", "Hololight Stream Hand Interaction (L) Grip Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "HololightStreamHand"));

	// Right Stream Hand
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Right_Menu_Click,
								  LOCTEXT("HololightStreamHand_Right_Menu_Click", "Hololight Stream Hand Interaction (R) Menu"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "HololightStreamHand"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Right_Select_Click,
								  LOCTEXT("HololightStreamHand_Right_Select_Click", "Hololight Stream Hand Interaction (R) Select"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "HololightStreamHand"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Right_Select_Axis,
								  LOCTEXT("HololightStreamHand_Right_Select_Axis", "Hololight Stream Hand Interaction (R) Select Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "HololightStreamHand"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Right_Grip_Click,
								  LOCTEXT("HololightStreamHand_Right_Grip_Click", "Hololight Stream Hand Interaction (R) Grip"),
								  FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "HololightStreamHand"));
	AddNonExistingKey(existingKeys,
					  FKeyDetails(HololightStreamHand_Right_Grip_Axis,
								  LOCTEXT("HololightStreamHand_Right_Grip_Axis", "Hololight Stream Hand Interaction (R) Grip Axis"),
								  FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey,
								  "HololightStreamHand"));
}

inline void AddAdditionalKeys(TArray<FKey> existingKeys)
{
	using namespace stream::keys;
	// Additional support keys
	AddNonExistingKey(existingKeys, FKeyDetails(OculusTouch_Left_Thumbrest_Touch,
												LOCTEXT("OculusTouch_Left_Thumbrest_Touch",
														"Oculus Touch (L) Thumbrest Touch"),
												FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
												"OculusTouch"));
	AddNonExistingKey(existingKeys, FKeyDetails(OculusTouch_Right_Thumbrest_Touch,
												LOCTEXT("OculusTouch_Right_Thumbrest_Touch",
														"Oculus Touch (R) Thumbrest Touch"),
												FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey,
												"OculusTouch"));
}

void FStreamInputModule::AddKeys()
{
	TArray<FKey> existingKeys;
	EKeys::GetAllKeys(existingKeys);

	AddML2Keys(existingKeys);
	AddVRXKeys(existingKeys);
	AddPICOTouchKeys(existingKeys);
	AddFocus3Keys(existingKeys);
	AddMXInkKeys(existingKeys);
	AddStreamHandKeys(existingKeys);
	AddAdditionalKeys(existingKeys);
}

#undef LOCTEXT_NAMESPACE