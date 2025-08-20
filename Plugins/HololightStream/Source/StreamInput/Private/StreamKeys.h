/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_STREAMKEYS_H
#define HOLOLIGHT_UNREAL_STREAMKEYS_H

#include "InputCoreTypes.h"

namespace stream::keys
{
// Magic Leap Controller Keys
const FKey MagicLeapController_Left_Menu_Click("MagicLeapController_Left_Menu_Click");
const FKey MagicLeapController_Left_Trigger_Click("MagicLeapController_Left_Trigger_Click");
const FKey MagicLeapController_Left_Trigger_Axis("MagicLeapController_Left_Trigger_Axis");
const FKey MagicLeapController_Left_Trackpad_X("MagicLeapController_Left_Trackpad_X");
const FKey MagicLeapController_Left_Trackpad_Y("MagicLeapController_Left_Trackpad_Y");
const FKey MagicLeapController_Left_Trackpad_2D("MagicLeapController_Left_Trackpad_2D");
const FKey MagicLeapController_Left_Trackpad_Click("MagicLeapController_Left_Trackpad_Click");
const FKey MagicLeapController_Left_Shoulder_Click("MagicLeapController_Left_Shoulder_Click");

const FKey MagicLeapController_Right_Menu_Click("MagicLeapController_Right_Menu_Click");
const FKey MagicLeapController_Right_Trigger_Click("MagicLeapController_Right_Trigger_Click");
const FKey MagicLeapController_Right_Trigger_Axis("MagicLeapController_Right_Trigger_Axis");
const FKey MagicLeapController_Right_Trackpad_X("MagicLeapController_Right_Trackpad_X");
const FKey MagicLeapController_Right_Trackpad_Y("MagicLeapController_Right_Trackpad_Y");
const FKey MagicLeapController_Right_Trackpad_2D("MagicLeapController_Right_Trackpad_2D");
const FKey MagicLeapController_Right_Trackpad_Click("MagicLeapController_Right_Trackpad_Click");
const FKey MagicLeapController_Right_Shoulder_Click("MagicLeapController_Right_Shoulder_Click");

// Lenovo VRX Controller Keys
const FKey LenovoVRXController_Left_X_Click("LenovoVRXController_Left_X_Click");
const FKey LenovoVRXController_Left_Y_Click("LenovoVRXController_Left_Y_Click");
const FKey LenovoVRXController_Left_Menu_Click("LenovoVRXController_Left_Menu_Click");
const FKey LenovoVRXController_Left_Grip_Click("LenovoVRXController_Left_Grip_Click");
const FKey LenovoVRXController_Left_Grip_Axis("LenovoVRXController_Left_Grip_Axis");
const FKey LenovoVRXController_Left_Trigger_Click("LenovoVRXController_Left_Trigger_Click");
const FKey LenovoVRXController_Left_Trigger_Axis("LenovoVRXController_Left_Trigger_Axis");
const FKey LenovoVRXController_Left_Thumbstick_2D("LenovoVRXController_Left_Thumbstick_2D");
const FKey LenovoVRXController_Left_Thumbstick_X("LenovoVRXController_Left_Thumbstick_X");
const FKey LenovoVRXController_Left_Thumbstick_Y("LenovoVRXController_Left_Thumbstick_Y");
const FKey LenovoVRXController_Left_Thumbstick_Click("LenovoVRXController_Left_Thumbstick_Click");
const FKey LenovoVRXController_Left_Thumbrest_Touch("LenovoVRXController_Left_Thumbrest_Touch");

const FKey LenovoVRXController_Right_A_Click("LenovoVRXController_Right_A_Click");
const FKey LenovoVRXController_Right_B_Click("LenovoVRXController_Right_B_Click");
const FKey LenovoVRXController_Right_Menu_Click("LenovoVRXController_Right_Menu_Click");
const FKey LenovoVRXController_Right_Grip_Click("LenovoVRXController_Right_Grip_Click");
const FKey LenovoVRXController_Right_Grip_Axis("LenovoVRXController_Right_Grip_Axis");
const FKey LenovoVRXController_Right_Trigger_Click("LenovoVRXController_Right_Trigger_Click");
const FKey LenovoVRXController_Right_Trigger_Axis("LenovoVRXController_Right_Trigger_Axis");
const FKey LenovoVRXController_Right_Thumbstick_2D("LenovoVRXController_Right_Thumbstick_2D");
const FKey LenovoVRXController_Right_Thumbstick_X("LenovoVRXController_Right_Thumbstick_X");
const FKey LenovoVRXController_Right_Thumbstick_Y("LenovoVRXController_Right_Thumbstick_Y");
const FKey LenovoVRXController_Right_Thumbstick_Click("LenovoVRXController_Right_Thumbstick_Click");
const FKey LenovoVRXController_Right_Thumbrest_Touch("LenovoVRXController_Right_Thumbrest_Touch");

// Pico Touch Controller Keys
const FKey PICOTouch_Left_X_Click("PICOTouch_Left_X_Click");
const FKey PICOTouch_Left_Y_Click("PICOTouch_Left_Y_Click");
const FKey PICOTouch_Left_Menu_Click("PICOTouch_Left_Menu_Click");
const FKey PICOTouch_Left_Grip_Click("PICOTouch_Left_Grip_Click");
const FKey PICOTouch_Left_Grip_Axis("PICOTouch_Left_Grip_Axis");
const FKey PICOTouch_Left_Trigger_Click("PICOTouch_Left_Trigger_Click");
const FKey PICOTouch_Left_Trigger_Axis("PICOTouch_Left_Trigger_Axis");
const FKey PICOTouch_Left_Thumbstick_2D("PICOTouch_Left_Thumbstick_2D");
const FKey PICOTouch_Left_Thumbstick_X("PICOTouch_Left_Thumbstick_X");
const FKey PICOTouch_Left_Thumbstick_Y("PICOTouch_Left_Thumbstick_Y");
const FKey PICOTouch_Left_Thumbstick_Click("PICOTouch_Left_Thumbstick_Click");
const FKey PICOTouch_Left_Thumbrest_Touch("PICOTouch_Left_Thumbrest_Touch");

const FKey PICOTouch_Right_A_Click("PICOTouch_Right_A_Click");
const FKey PICOTouch_Right_B_Click("PICOTouch_Right_B_Click");
const FKey PICOTouch_Right_Grip_Click("PICOTouch_Right_Grip_Click");
const FKey PICOTouch_Right_Grip_Axis("PICOTouch_Right_Grip_Axis");
const FKey PICOTouch_Right_Trigger_Click("PICOTouch_Right_Trigger_Click");
const FKey PICOTouch_Right_Trigger_Axis("PICOTouch_Right_Trigger_Axis");
const FKey PICOTouch_Right_Thumbstick_2D("PICOTouch_Right_Thumbstick_2D");
const FKey PICOTouch_Right_Thumbstick_X("PICOTouch_Right_Thumbstick_X");
const FKey PICOTouch_Right_Thumbstick_Y("PICOTouch_Right_Thumbstick_Y");
const FKey PICOTouch_Right_Thumbstick_Click("PICOTouch_Right_Thumbstick_Click");
const FKey PICOTouch_Right_Thumbrest_Touch("PICOTouch_Right_Thumbrest_Touch");

// Vive Focus 3 Controller Keys
const FKey Focus3_Left_X_Click("Focus3_Left_X_Click");
const FKey Focus3_Left_Y_Click("Focus3_Left_Y_Click");
const FKey Focus3_Left_Menu_Click("Focus3_Left_Menu_Click");
const FKey Focus3_Left_Grip_Click("Focus3_Left_Grip_Click");
const FKey Focus3_Left_Grip_Axis("Focus3_Left_Grip_Axis");
const FKey Focus3_Left_Trigger_Click("Focus3_Left_Trigger_Click");
const FKey Focus3_Left_Trigger_Axis("Focus3_Left_Trigger_Axis");
const FKey Focus3_Left_Thumbstick_X("Focus3_Left_Thumbstick_X");
const FKey Focus3_Left_Thumbstick_Y("Focus3_Left_Thumbstick_Y");
const FKey Focus3_Left_Thumbstick_2D("Focus3_Left_Thumbstick_2D");
const FKey Focus3_Left_Thumbstick_Click("Focus3_Left_Thumbstick_Click");
const FKey Focus3_Left_Thumbrest_Touch("Focus3_Left_Thumbrest_Touch");

const FKey Focus3_Right_A_Click("Focus3_Right_A_Click");
const FKey Focus3_Right_B_Click("Focus3_Right_B_Click");
const FKey Focus3_Right_Grip_Click("Focus3_Right_Grip_Click");
const FKey Focus3_Right_Grip_Axis("Focus3_Right_Grip_Axis");
const FKey Focus3_Right_Trigger_Click("Focus3_Right_Trigger_Click");
const FKey Focus3_Right_Trigger_Axis("Focus3_Right_Trigger_Axis");
const FKey Focus3_Right_Thumbstick_X("Focus3_Right_Thumbstick_X");
const FKey Focus3_Right_Thumbstick_Y("Focus3_Right_Thumbstick_Y");
const FKey Focus3_Right_Thumbstick_2D("Focus3_Right_Thumbstick_2D");
const FKey Focus3_Right_Thumbstick_Click("Focus3_Right_Thumbstick_Click");
const FKey Focus3_Right_Thumbrest_Touch("Focus3_Right_Thumbrest_Touch");

// Logitech MX Ink Stylus Keys
const FKey LogitechMXInk_Left_ClusterBack_Click("LogitechMXInk_Left_ClusterBack_Click");
const FKey LogitechMXInk_Left_ClusterBack_DoubleTap("LogitechMXInk_Left_ClusterBack_DoubleTap");
const FKey LogitechMXInk_Left_ClusterFront_Click("LogitechMXInk_Left_ClusterFront_Click");
const FKey LogitechMXInk_Left_ClusterFront_DoubleTap("LogitechMXInk_Left_ClusterFront_DoubleTap");
const FKey LogitechMXInk_Left_ClusterMiddle_Click("LogitechMXInk_Left_ClusterMiddle_Click");
const FKey LogitechMXInk_Left_ClusterMiddle_Axis("LogitechMXInk_Left_ClusterMiddle_Axis");
const FKey LogitechMXInk_Left_Tip_Click("LogitechMXInk_Left_Tip_Click");
const FKey LogitechMXInk_Left_Tip_Axis("LogitechMXInk_Left_Tip_Axis");
const FKey LogitechMXInk_Left_Docked_Click("LogitechMXInk_Left_Docked_Click");

const FKey LogitechMXInk_Right_ClusterBack_Click("LogitechMXInk_Right_ClusterBack_Click");
const FKey LogitechMXInk_Right_ClusterBack_DoubleTap("LogitechMXInk_Right_ClusterBack_DoubleTap");
const FKey LogitechMXInk_Right_ClusterFront_Click("LogitechMXInk_Right_ClusterFront_Click");
const FKey LogitechMXInk_Right_ClusterFront_DoubleTap("LogitechMXInk_Right_ClusterFront_DoubleTap");
const FKey LogitechMXInk_Right_ClusterMiddle_Click("LogitechMXInk_Right_ClusterMiddle_Click");
const FKey LogitechMXInk_Right_ClusterMiddle_Axis("LogitechMXInk_Right_ClusterMiddle_Axis");
const FKey LogitechMXInk_Right_Tip_Click("LogitechMXInk_Right_Tip_Click");
const FKey LogitechMXInk_Right_Tip_Axis("LogitechMXInk_Right_Tip_Axis");
const FKey LogitechMXInk_Right_Docked_Click("LogitechMXInk_Right_Docked_Click");

// Hand Interaction Keys
const FKey HololightStreamHand_Left_Menu_Click("HololightStreamHand_Left_Menu_Click");
const FKey HololightStreamHand_Left_Select_Click("HololightStreamHand_Left_Select_Click");
const FKey HololightStreamHand_Left_Select_Axis("HololightStreamHand_Left_Select_Axis");
const FKey HololightStreamHand_Left_Grip_Click("HololightStreamHand_Left_Grip_Click");
const FKey HololightStreamHand_Left_Grip_Axis("HololightStreamHand_Left_Grip_Axis");

const FKey HololightStreamHand_Right_Menu_Click("HololightStreamHand_Right_Menu_Click");
const FKey HololightStreamHand_Right_Select_Click("HololightStreamHand_Right_Select_Click");
const FKey HololightStreamHand_Right_Select_Axis("HololightStreamHand_Right_Select_Axis");
const FKey HololightStreamHand_Right_Grip_Click("HololightStreamHand_Right_Grip_Click");
const FKey HololightStreamHand_Right_Grip_Axis("HololightStreamHand_Right_Grip_Axis");

// Additional support keys
const FKey OculusTouch_Left_Thumbrest_Touch("OculusTouch_Left_Thumbrest_Touch");
const FKey OculusTouch_Right_Thumbrest_Touch("OculusTouch_Right_Thumbrest_Touch");
}

#endif // HOLOLIGHT_UNREAL_STREAMKEYS_H