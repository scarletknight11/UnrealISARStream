# Hololight Stream Unreal Engine Plugin Content

This manual provides an overview of the content included in the Hololight Stream Unreal Engine Plugin.

## Controller Models

This package includes models of the controllers for devices supported by Hololight Stream. These models are used for controller visualization in the [VR Pawn](#3-vr-pawn).

## Virtual Reality Template

The provided Virtual Reality Template is a modified version of Unreal Engine's VR template (version 5.5). This template includes additional features supported by Hololight Stream.

### Hand Tracking

Hololight Stream supports hand tracking on compatible devices. The hand model included in this package is used for visualizing hand tracking.

### Input

Users can assign key mappings to Input Actions in Unreal Engine. The Hololight Stream plugin adds input keys (Buttons, Axis 1D, Axis 2D) for supported devices, allowing users to map them freely.

The Input Mapping Contexts provided in this package include key mappings for all supported devices, aligning with the Input Actions from the VR Template. These Input Mapping Contexts must be added to the Default Mapping Contexts in the Project Settings.

### Blueprints

#### 1. Custom Game Instance

A custom Game Instance is used to maintain the state of the Hololight Stream connection across different levels. Since the Game Instance is not reset when changing levels, the connection state is preserved.

#### 2. Widget Menu

Additional buttons have been added to the Widget Menu:

**Passthrough:** Uses Hololight Stream's Passthrough functions to set the passthrough mode and adjust the Sky Sphere's visibility accordingly. Note that the Alpha Output setting must be enabled for Passthrough to function properly.

**Toggle Audio:** Uses Hololight Stream's audio streaming functions to enable or disable audio streaming. This action can also be performed in the [VR Pawn](#3-vr-pawn) Blueprint.

**Toggle Microphone:** Uses the Audio Capture Component attached to the [VR Pawn](#3-vr-pawn) to start or stop microphone capture streaming. It also controls the Stream Audio Capture Extension Component, which is required for proper microphone capture streaming. This action can also be performed in the [VR Pawn](#3-vr-pawn) Blueprint.

**Disconnect:** Disables and re-enables the HMD to restart the Hololight Stream connection. This disconnects the client from the server and re-establishes the connection for a new client.

#### 3. VR Pawn

The VR Pawn in Unreal Engine's VR Template manages device and motion controller tracking, teleportation, and the widget menu. It also handles Input Actions related to teleportation, rotation, grabbing, and the widget menu. The Hololight Stream plugin introduces the following modifications to VR Pawn:

**a. Implementing the Stream Connection State Handler interface:**

When a new connection state is received, the VR Pawn updates the connection state in the Custom Game Instance. If the state is Connected, it adjusts the visibility of the Sky Sphere based on the Passthrough mode. The VR Pawn registers itself as an interface implementation in the Event BeginPlay and unregisters in the Event EndPlay.

**b. Implementing the Stream Controller State Handler interface:**

When a new controller state is received, the VR Pawn updates the controller and hand visualizations based on the tracking status and connected device. The VR Pawn registers itself as an interface implementation in the Event BeginPlay and unregisters in the Event EndPlay.

**c. Updating hand bones:**

If hand tracking is active in the current connection, the VR Pawn updates the hand bones using the Get Hand Tracking State Blueprint function.

##### VR Pawn Modifications

The VR Pawn has also been modified as follows:

**Grip Space:**

Hololight Stream does not currently support Grip Space. References to Grip Spaces (LeftGrip, RightGrip) have been replaced with either Left and Right or LeftPalm and RightPalm in functions that previously used Grip Spaces.
These changes apply to the Motion Controller Components and the GrabComponent Blueprint located in the same folder.
