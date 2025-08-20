/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "FStreamHMD.h"
#include "FStreamHMDSwapchain.h"
#include "StreamHMDSettings.h"

#include "Misc/FileHelper.h"
#include "Misc/App.h"
#include "Misc/MessageDialog.h"
#include "ClearQuad.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Features/IModularFeatures.h"
#include "RendererInterface.h"
#include "IHandTracker.h"
#include "Async/Async.h"

#if WITH_EDITOR
#include "UnrealEdMisc.h"
#include "ISettingsModule.h"
#endif

#include <d3d11_1.h>
#include <d3d12.h>
#include <WindowsNumerics.h>

//JSON
#include "Serialization/JsonSerializer.h"
#include "PostProcess/PostProcessHMD.h"
#include "GameFramework/WorldSettings.h"

#if PLATFORM_WINDOWS
#include "ID3D11DynamicRHI.h"
#include "ID3D12DynamicRHI.h"
#endif

#include <chrono>
#include <thread>

#define LOCTEXT_NAMESPACE "FStreamHMD"

class FStreamCorrectionPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FStreamCorrectionPS, Global);

public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& parameters)
	{
		return true;
	}

	FStreamCorrectionPS(const ShaderMetaType::CompiledShaderInitializerType& initializer) :
		FGlobalShader(initializer)
	{
		m_SceneTexture.Bind(initializer.ParameterMap, TEXT("SceneTexture"));
		m_SceneSampler.Bind(initializer.ParameterMap, TEXT("SceneSampler"));
	}

	FStreamCorrectionPS() = default;

	void SetParameters(FRHIBatchedShaderParameters& batchedParameters, FRHITexture* sceneTextureRHI)
	{
		SetTextureParameter(batchedParameters,
							m_SceneTexture,
							m_SceneSampler,
							TStaticSamplerState<SF_Point>::GetRHI(),
		                    sceneTextureRHI);
	}

	static const TCHAR* GetFunctionName()
	{
		return TEXT("StreamCorrectionPS");
	}

private:
	LAYOUT_FIELD(FShaderResourceParameter, m_SceneTexture);
	LAYOUT_FIELD(FShaderResourceParameter, m_SceneSampler);
};

IMPLEMENT_SHADER_TYPE(, FStreamCorrectionPS, TEXT("/Plugin/HololightStream/StreamCorrectionPixelShader.usf"),
						TEXT("StreamCorrectionPS"), SF_Pixel);

/** Helper function for acquiring the appropriate FSceneViewport */
FSceneViewport* FindSceneViewport()
{
	if (!GIsEditor)
	{
		UGameEngine* gameEngine = Cast<UGameEngine>(GEngine);
		return gameEngine->SceneViewport.Get();
	}
#if WITH_EDITOR
	else
	{
		UEditorEngine* editorEngine = CastChecked<UEditorEngine>(GEngine);
		FSceneViewport* pViewport = (FSceneViewport*)editorEngine->GetPIEViewport();
		if (pViewport != nullptr && pViewport->IsStereoRenderingAllowed())
		{
			// PIE is setup for stereo rendering
			return pViewport;
		}
		else
		{
			// Check to see if the active editor viewport is drawing in stereo mode
			FSceneViewport* pEditorViewport = (FSceneViewport*)editorEngine->GetActiveViewport();
			if (pEditorViewport != nullptr && pEditorViewport->IsStereoRenderingAllowed())
			{
				return pEditorViewport;
			}
		}
	}
#endif
	return nullptr;
}

FIntPoint GeneratePixelDensitySize(const XrViewConfigurationView& config, const float pixelDensity)
{
	FIntPoint densityAdjustedSize =
	{
		FMath::CeilToInt(config.recommendedImageRectWidth * pixelDensity),
		FMath::CeilToInt(config.recommendedImageRectHeight * pixelDensity)
	};

	// We quantize in order to be consistent with the rest of the engine in creating our buffers.
	// Interestingly, we need to be a bit careful with this quantization during target alloc because
	// some runtime compositors want/expect targets that match the recommended size. Some runtimes
	// might blit from a 'larger' size to the recommended size. This could happen with quantization
	// factors that don't align with the recommended size.
	QuantizeSceneBufferSize(densityAdjustedSize, densityAdjustedSize);

	return densityAdjustedSize;
}

enum FStreamHMD::ETextureCopyModifier : uint8
{
	Opaque,
	TransparentAlphaPassthrough,
	PremultipliedAlphaBlend,
};


FStreamHMD::~FStreamHMD()
{
	UE_LOG(LogHMD, Display, TEXT("Destroy StreamHMD context"));
	if(m_connectionCreated)
	{
		if (m_microphoneCaptureStream) // Microphone Capture Stream is not guaranteed to be available
		{
			m_microphoneCaptureStream->Stop();
		}

		auto err = m_serverApi.closeConnection(m_streamConnection);
		if (err != IsarError::eNone)
		{
			// Write error to output
			UE_LOG(LogHMD, Error, TEXT("Error in Close Connection, Status: %d"), err);
		}

		err = m_serverApi.destroyConnection(&m_streamConnection);
		if (err != IsarError::eNone || m_streamConnection)
		{
			// Write error to output
			UE_LOG(LogHMD, Error, TEXT("Error in Destroy Connection, Status: %d"), err);
		}
	}
}


FStreamHMD::FStreamHMD(const FAutoRegister& autoRegister, TRefCountPtr<FStreamRenderBridge>& inRenderBridge) :
	IStreamHMD(nullptr),
	FHMDSceneViewExtension(autoRegister),
	m_renderBridge(inRenderBridge),
	m_pD3D12Device(nullptr),
	m_pD3D11Device(nullptr),
	m_stereoEnabled(false),
	m_nViews(2),
	m_audioListener(MakeShared<FStreamAudioListener>()),
	m_connectionCreated(false)
{
	const ERHIInterfaceType rhiType = GDynamicRHI ? RHIGetInterfaceType() : ERHIInterfaceType::Hidden;
	// Only D3D11/D3D12 is supported
	if (!(rhiType == ERHIInterfaceType::D3D11 || rhiType == ERHIInterfaceType::D3D12))
	{
		UE_LOG(LogHMD, Error, TEXT("Unsupported Graphics type"));
		return;
	}

	auto err = Isar_Server_CreateApi(&m_serverApi);
	if (err != isar::IsarError::eNone)
	{
		UE_LOG(LogHMD, Error, TEXT("Failed to Initialise Stream Instance"));
		return;
	}

	if (IsRHID3D12())
	{
		m_gfxApiType = IsarGraphicsApiType_D3D12;
		ID3D12DynamicRHI* pRHI12 = GetID3D12DynamicRHI();
		m_deviceType = 2; // hls::kUnrealGfxRendererD3D12;
		check(pRHI12);
		const int32 deviceIndex = 0;
		m_pD3D12Device = pRHI12->RHIGetDevice(deviceIndex);
		if (!m_pD3D12Device)
		{
			UE_LOG(LogHMD, Error, TEXT("Failed to get D3D12 Device from RHI for device index %d"), deviceIndex);
			return;
		}
		m_pD3D12CommandQueue = pRHI12->RHIGetCommandQueue();

		HRESULT hr = m_pD3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pD3D12Fence));
		if (FAILED(hr))
		{
			UE_LOG(LogHMD, Error, TEXT("Failed to create Fence"));
			return;
		}
	}
	else
	{
		m_gfxApiType = IsarGraphicsApiType_D3D11;
		ID3D11DynamicRHI* pRHI11 = GetID3D11DynamicRHI();
		m_deviceType = 0; // hls::kUnrealGfxRendererD3D11;
		check(pRHI11);
		m_pD3D11Device = pRHI11->RHIGetDevice();
	}

	ReconfigureForShaderPlatform(GMaxRHIShaderPlatform);
#if WITH_EDITOR
	ISettingsModule* module = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (module)
	{
		module->RegisterSettings("Project", "Plugins", "HololightStreamSettings",
								 LOCTEXT("StreamHMDSettingsName", "Hololight Stream Settings"),
								 LOCTEXT("StreamHMDSettingsDesc",
										 "Configure the Hololight Stream connection settings"),
								 GetMutableDefault<UStreamHMDSettings>()
			);
	}
#endif
	m_width = 4128;
	m_height = 2208;
}

FName FStreamHMD::GetHMDName() const
{
	if(!m_connected)
	{
		return GetSystemName();
	}

	return FName(m_connectionInfo.remoteName);
}

bool FStreamHMD::ReconfigureForShaderPlatform(EShaderPlatform newShaderPlatform)
{
	UE::StereoRenderUtils::FStereoShaderAspects aspects(newShaderPlatform);
	m_configuredShaderPlatform = newShaderPlatform;

	return true;
}

float FStreamHMD::GetWorldToMetersScale() const
{
	return m_worldToMeters;
}

void FStreamHMD::SetupViewFamily(FSceneViewFamily& inViewFamily)
{
	inViewFamily.EngineShowFlags.MotionBlur = 0;
	inViewFamily.EngineShowFlags.HMDDistortion = false;
	inViewFamily.EngineShowFlags.StereoRendering = IsStereoEnabled();
}

void FStreamHMD::SetupView(FSceneViewFamily& inViewFamily, FSceneView& inView)
{
}

void FStreamHMD::BeginRenderViewFamily(FSceneViewFamily& inViewFamily)
{
	m_pipelinedLayerStateRendering.projectionLayers.SetNum(2);
	if (SpectatorScreenController)
	{
		SpectatorScreenController->BeginRenderViewFamily();
	}
}

void FStreamHMD::PreRenderView_RenderThread(FRDGBuilder& graphBuilder, FSceneView& inView)
{
	check(IsInRenderingThread());
}

void FStreamHMD::PreRenderViewFamily_RenderThread(FRDGBuilder& graphBuilder, FSceneViewFamily& inViewFamily)
{
	check(IsInRenderingThread());
	if (SpectatorScreenController)
	{
		SpectatorScreenController->UpdateSpectatorScreenMode_RenderThread();
	}
}

IStereoRenderTargetManager* FStreamHMD::GetRenderTargetManager()
{
	return this;
}

FXRRenderBridge* FStreamHMD::GetActiveRenderBridge_GameThread(bool useSeparateRenderTarget)
{
	return m_renderBridge;
}

void FStreamHMD::SetPixelDensity(const float newDensity)
{
	// We have to update the RT state because the new swapchain will be allocated (FSceneViewport::InitRHI + AllocateRenderTargetTexture)
	// before we call OnBeginRendering_GameThread.
	check(IsInGameThread());
	m_pipelinedFrameStateGame.pixelDensity = FMath::Min(newDensity, m_runtimePixelDensityMax);

	// We have to update the RT state because the new swapchain will be allocated (FSceneViewport::InitRHI + AllocateRenderTargetTexture)
	// before we call OnBeginRendering_GameThread.
	ENQUEUE_RENDER_COMMAND(UpdatePixelDensity)(
		[this, PixelDensity = m_pipelinedFrameStateGame.pixelDensity](FRHICommandListImmediate&)
		{
			m_pipelinedFrameStateRendering.pixelDensity = PixelDensity;
		});
}

void FStreamHMD::GetMotionControllerData(UObject* worldContext, const EControllerHand hand,
										 FXRMotionControllerData& motionControllerData)
{
	motionControllerData.DeviceName = FName(m_getDeviceInfoCallback(hand).deviceName.c_str());
	motionControllerData.ApplicationInstanceID = FApp::GetInstanceId();
	motionControllerData.DeviceVisualType = EXRVisualType::Controller;
	motionControllerData.TrackingStatus = ETrackingStatus::NotTracked;
	motionControllerData.HandIndex = hand;
	motionControllerData.bValid = false;

	FName handTrackerName("Stream");
	TArray<IHandTracker*> handTrackers = IModularFeatures::Get().GetModularFeatureImplementations<IHandTracker>(
		IHandTracker::GetModularFeatureName());
	IHandTracker* handTracker = nullptr;
	for (auto itr : handTrackers)
	{
		if (itr->GetHandTrackerDeviceTypeName() == handTrackerName)
		{
			handTracker = itr;
			break;
		}
	}

	if ((hand == EControllerHand::Left) || (hand == EControllerHand::Right))
	{
		FName motionControllerName("Stream");
		TArray<IMotionController*> motionControllers = IModularFeatures::Get()
			.GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName());
		IMotionController* motionController = nullptr;
		for (auto itr : motionControllers)
		{
			if (itr->GetMotionControllerDeviceTypeName() == motionControllerName)
			{
				motionController = itr;
				break;
			}
		}

		if (motionController)
		{
			bool success = false;
			FVector position = FVector::ZeroVector;
			FRotator rotation = FRotator::ZeroRotator;
			FTransform trackingToWorld = GetTrackingToWorldTransform();
			FName aimSource = hand == EControllerHand::Left ? FName("LeftAim") : FName("RightAim");
			success = motionController->GetControllerOrientationAndPosition(
				0, aimSource, rotation, position, m_worldToMeters);
			if (success)
			{
				motionControllerData.AimPosition = trackingToWorld.TransformPosition(position);
				motionControllerData.AimRotation = trackingToWorld.TransformRotation(FQuat(rotation));
			}
			motionControllerData.bValid |= success;

			FName gripSource = hand == EControllerHand::Left ? FName("LeftGrip") : FName("RightGrip");
			success = motionController->GetControllerOrientationAndPosition(
				0, gripSource, rotation, position, m_worldToMeters);
			if (success)
			{
				motionControllerData.GripPosition = trackingToWorld.TransformPosition(position);
				motionControllerData.GripRotation = trackingToWorld.TransformRotation(FQuat(rotation));
			}
			motionControllerData.bValid |= success;

			FName palmSource = hand == EControllerHand::Left ? FName("LeftPalm") : FName("RightPalm");
			success = motionController->GetControllerOrientationAndPosition(
				0, palmSource, rotation, position, m_worldToMeters);
			if (success)
			{
				motionControllerData.PalmPosition = trackingToWorld.TransformPosition(position);
				motionControllerData.PalmRotation = trackingToWorld.TransformRotation(FQuat(rotation));
			}
			motionControllerData.bValid |= success;

			motionControllerData.TrackingStatus = motionController->GetControllerTrackingStatus(0, palmSource);
		}

		if (handTracker && handTracker->IsHandTrackingStateValid())
		{
			motionControllerData.DeviceVisualType = EXRVisualType::Hand;

			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			motionControllerData.bValid |= handTracker->GetAllKeypointStates(
				hand, motionControllerData.HandKeyPositions, motionControllerData.HandKeyRotations,
				motionControllerData.HandKeyRadii);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
			check(!motionControllerData.bValid || (motionControllerData.HandKeyPositions.Num() == EHandKeypointCount &&
				motionControllerData.HandKeyRotations.Num() == EHandKeypointCount && motionControllerData.HandKeyRadii.
				Num() == EHandKeypointCount));

			FTransform trackingToWorld = GetTrackingToWorldTransform();
			// Above check succeeds when the data is not valid, so we check the array size here in case it is empty
			for (int i = 0; i < motionControllerData.HandKeyPositions.Num(); i++)
			{
				motionControllerData.HandKeyPositions[i] = trackingToWorld.TransformPosition(
					motionControllerData.HandKeyPositions[i] * m_worldToMeters);
				motionControllerData.HandKeyRotations[i] = trackingToWorld.TransformRotation(
					motionControllerData.HandKeyRotations[i]);
				motionControllerData.HandKeyRadii[i] *= m_worldToMeters;
			}
		}
	}

	motionControllerData.bIsGrasped = false;
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
inline static FLazyName ToMotionSourceName(const EControllerHand hand, const EXRControllerPoseType xrControllerPoseType)
{
	static FLazyName leftAim = "LeftAim";
	static FLazyName leftGrip = "LeftGrip";
	static FLazyName leftPalm = "LeftPalm";
	static FLazyName rightAim = "RightAim";
	static FLazyName rightGrip = "RightGrip";
	static FLazyName rightPalm = "RightPalm";
	if (hand == EControllerHand::Left)
	{
		switch (xrControllerPoseType)
		{
			case EXRControllerPoseType::Aim: return leftAim;
			case EXRControllerPoseType::Grip: return leftGrip;
			case EXRControllerPoseType::Palm: return leftPalm;
			default: check(false); return leftGrip;
		}
	}
	else
	{
		switch (xrControllerPoseType)
		{
			case EXRControllerPoseType::Aim: return rightAim;
			case EXRControllerPoseType::Grip: return rightGrip;
			case EXRControllerPoseType::Palm: return rightPalm;
			default: check(false); return rightGrip;
		}
	}
}

void FStreamHMD::GetMotionControllerState(UObject* worldContext, const EXRSpaceType xrSpaceType,
                                          const EControllerHand hand, const EXRControllerPoseType xrControllerPoseType,
                                          FXRMotionControllerState& motionControllerState)
{
	motionControllerState.DeviceName = FName(m_getDeviceInfoCallback(hand).deviceName.c_str());
	motionControllerState.ApplicationInstanceID = FApp::GetInstanceId();
	motionControllerState.TrackingStatus = ETrackingStatus::NotTracked;
	motionControllerState.Hand = hand;
	motionControllerState.XRSpaceType = xrSpaceType;
	motionControllerState.XRControllerPoseType = xrControllerPoseType;
	motionControllerState.bValid = false;

	if ((hand == EControllerHand::Left) || (hand == EControllerHand::Right))
	{
		FName motionControllerName("Stream");
		TArray<IMotionController*> motionControllers =
		    IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(
		        IMotionController::GetModularFeatureName());
		IMotionController* motionController = nullptr;
		for (auto itr : motionControllers)
		{
			if (itr->GetMotionControllerDeviceTypeName() == motionControllerName)
			{
				motionController = itr;
				break;
			}
		}

		if (motionController)
		{
			// Handle the pose that is actually being requested
			FName motionSource = ToMotionSourceName(hand, xrControllerPoseType);
			FVector position = FVector::ZeroVector;
			FRotator rotation = FRotator::ZeroRotator;
			FTransform trackingToWorld =
			    xrSpaceType == EXRSpaceType::UnrealWorldSpace ? GetTrackingToWorldTransform() : FTransform::Identity;
			const float worldToMeters =
			    xrSpaceType == EXRSpaceType::UnrealWorldSpace ? GetWorldToMetersScale() : 100.0f;
			bool success = motionController->GetControllerOrientationAndPosition(0, motionSource, rotation, position,
			                                                                     worldToMeters);
			if (success)
			{
				motionControllerState.ControllerLocation = trackingToWorld.TransformPosition(position);
				motionControllerState.ControllerRotation = trackingToWorld.TransformRotation(FQuat(rotation));
			}
			motionControllerState.bValid |= success;

			motionControllerState.TrackingStatus = motionController->GetControllerTrackingStatus(0, motionSource);

			// The grip transform in Unreal space is provided for XRVisualizationFunctionLibrary
			// The bValid and TrackingStatus above are also valid for this pose.
			if (xrSpaceType == EXRSpaceType::UnrealWorldSpace && xrControllerPoseType == EXRControllerPoseType::Grip)
			{
				motionControllerState.GripUnrealSpaceLocation = motionControllerState.ControllerLocation;
				motionControllerState.GripUnrealSpaceRotation = motionControllerState.ControllerRotation;

				return;
			}

			motionSource = ToMotionSourceName(hand, EXRControllerPoseType::Grip);
			trackingToWorld = GetTrackingToWorldTransform();
			success = motionController->GetControllerOrientationAndPosition(0, motionSource, rotation, position,
			                                                                GetWorldToMetersScale());
			if (success)
			{
				motionControllerState.GripUnrealSpaceLocation = trackingToWorld.TransformPosition(position);
				motionControllerState.GripUnrealSpaceRotation = trackingToWorld.TransformRotation(FQuat(rotation));
			}
		}
	}
}

void FStreamHMD::GetHandTrackingState(UObject* worldContext, const EXRSpaceType xrSpaceType, const EControllerHand hand,
                                      FXRHandTrackingState& handTrackingState)
{
	handTrackingState.DeviceName = FName(m_getDeviceInfoCallback(hand).deviceName.c_str());
	handTrackingState.ApplicationInstanceID = FApp::GetInstanceId();
	handTrackingState.TrackingStatus = ETrackingStatus::NotTracked;
	handTrackingState.Hand = hand;
	handTrackingState.XRSpaceType = xrSpaceType;
	handTrackingState.bValid = false;

	FName handTrackerName("Stream");
	TArray<IHandTracker*> handTrackers =
	    IModularFeatures::Get().GetModularFeatureImplementations<IHandTracker>(IHandTracker::GetModularFeatureName());
	IHandTracker* handTracker = nullptr;
	for (auto itr : handTrackers)
	{
		if (itr->GetHandTrackerDeviceTypeName() == handTrackerName)
		{
			handTracker = itr;
			break;
		}
	}

	if (((hand == EControllerHand::Left) || (hand == EControllerHand::Right)) && handTracker &&
	    handTracker->IsHandTrackingStateValid())
	{
		bool isTracked = false;
		handTrackingState.bValid = handTracker->GetAllKeypointStates(hand, handTrackingState.HandKeyLocations,
		                                                             handTrackingState.HandKeyRotations,
		                                                             handTrackingState.HandKeyRadii, isTracked);

		if (handTrackingState.bValid)
		{
			handTrackingState.TrackingStatus = isTracked ? ETrackingStatus::Tracked : ETrackingStatus::NotTracked;
		}
		check(!handTrackingState.bValid ||
		      (handTrackingState.HandKeyLocations.Num() == EHandKeypointCount &&
		       handTrackingState.HandKeyRotations.Num() == EHandKeypointCount &&
		       handTrackingState.HandKeyRadii.Num() == EHandKeypointCount));

		FTransform trackingToWorld =
		    xrSpaceType == EXRSpaceType::UnrealWorldSpace ? GetTrackingToWorldTransform() : FTransform::Identity;
		const float worldToMeters = xrSpaceType == EXRSpaceType::UnrealWorldSpace ? GetWorldToMetersScale() : 100.0f;

		// Above check succeeds when the data is not valid, so we check the array size here in case it is empty
		for (int i = 0; i < handTrackingState.HandKeyLocations.Num(); i++)
		{
			handTrackingState.HandKeyLocations[i] =
			    trackingToWorld.TransformPosition(handTrackingState.HandKeyLocations[i] * worldToMeters);
			handTrackingState.HandKeyRotations[i] =
			    trackingToWorld.TransformRotation(handTrackingState.HandKeyRotations[i]);
			handTrackingState.HandKeyRadii[i] *= worldToMeters;
		}
	}
}
#endif

bool FStreamHMD::EnumerateTrackedDevices(TArray<int32>& outDevices, EXRTrackedDeviceType type)
{
	if (type == EXRTrackedDeviceType::Any || type == EXRTrackedDeviceType::HeadMountedDisplay)
	{
		outDevices.Add(IXRTrackingSystem::HMDDeviceId);
	}
	if (type == EXRTrackedDeviceType::Any || type == EXRTrackedDeviceType::Controller)
	{
		auto deviceInfo = m_getDeviceInfoCallback(EControllerHand::Left);
		if (deviceInfo.deviceId != -1)
		{
			outDevices.Add(deviceInfo.deviceId);
		}

		deviceInfo = m_getDeviceInfoCallback(EControllerHand::Right);
		if (deviceInfo.deviceId != -1)
		{
			outDevices.Add(deviceInfo.deviceId);
		}
	}
	return outDevices.Num() > 0;
}

bool FStreamHMD::GetCurrentPose(int32 deviceId, FQuat& currentOrientation, FVector& currentPosition)
{
	if (deviceId == IXRTrackingSystem::HMDDeviceId)
	{
		const FPipelinedFrameState& pipelineState = GetPipelinedFrameStateForThread();
		if (m_connected && pipelineState.views.Num() > 0)
		{
			GetPositionRotation(pipelineState.views[0].pose.position, pipelineState.views[0].pose.orientation,
								currentPosition, currentOrientation);
		}
		else
		{
			currentOrientation = FQuat::Identity;
			currentPosition = FVector::ZeroVector;
		}
		return true;
	}

	auto deviceInfo = m_getDeviceInfoCallback(EControllerHand::Left);
	if (deviceInfo.deviceId != -1 && deviceInfo.deviceId == deviceId)
	{
		currentPosition = deviceInfo.position * m_worldToMeters;
		currentOrientation = deviceInfo.orientation;
		return true;
	}

	deviceInfo = m_getDeviceInfoCallback(EControllerHand::Right);
	if (deviceInfo.deviceId != -1 && deviceInfo.deviceId == deviceId)
	{
		currentPosition = deviceInfo.position * m_worldToMeters;
		currentOrientation = deviceInfo.orientation;
		return true;
	}

	return false;
}

void FStreamHMD::SetBaseRotation(const FRotator& baseRotation)
{
}

void FStreamHMD::SetBaseOrientation(const FQuat& baseOrientation)
{
}

void FStreamHMD::ResetOrientationAndPosition(float yaw)
{
	ResetOrientation(yaw);
	ResetPosition();
}

bool FStreamHMD::IsHMDEnabled() const
{
	return true;
}

void FStreamHMD::EnableHMD(bool enable)
{
}

void FStreamHMD::OnBeginPlay(FWorldContext& inWorldContext)
{
}

void FStreamHMD::OnEndPlay(FWorldContext& inWorldContext)
{
	m_connectionStateHandlers.Empty();

	if (!m_connectionCreated)
	{
		return;
	}

	StopAudio();
	m_shouldEnableAudio = false;
	m_inputModule->Stop();
	if (m_microphoneCaptureStream) // Microphone Capture Stream is not guaranteed to be available
	{
		m_microphoneCaptureStream->Stop();
		m_microphoneCaptureStream = nullptr;
	}

	auto err = m_serverApi.closeConnection(m_streamConnection);
	if (err != IsarError::eNone)
	{
		// Write error to output
		UE_LOG(LogHMD, Error, TEXT("Error in Close Connection, Status: %d"), err);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	err = m_serverApi.destroyConnection(&m_streamConnection);
	if (err != IsarError::eNone || m_streamConnection)
	{
		// Write error to output
		UE_LOG(LogHMD, Error, TEXT("Error in Destroy Connection, Status: %d"), err);
	}
	m_connectionCreated = false;

	// Reset views
	UE_LOG(LogHMD, Log, TEXT("Reset Views"));
	m_pipelinedFrameStateGame.viewConfigs.Empty();
	m_pipelinedFrameStateGame.views.Empty();
	
}

bool FStreamHMD::GetHMDMonitorInfo(MonitorInfo& monitorInfo)
{
	monitorInfo.MonitorName = "StreamHMD";
	monitorInfo.MonitorId = 0;
	monitorInfo.DesktopX = monitorInfo.DesktopY = monitorInfo.ResolutionX = monitorInfo.ResolutionY = 0;

	return true;
}

void FStreamHMD::GetFieldOfView(float& outHFOVInDegrees, float& outVFOVInDegrees) const
{
	const FPipelinedFrameState& frameState = GetPipelinedFrameStateForThread();

	XrFovf unifiedFov = {0.0f};
	for (const XrView& view : frameState.views)
	{
		unifiedFov.angleLeft = FMath::Min(unifiedFov.angleLeft, view.fov.angleLeft);
		unifiedFov.angleRight = FMath::Max(unifiedFov.angleRight, view.fov.angleRight);
		unifiedFov.angleUp = FMath::Max(unifiedFov.angleUp, view.fov.angleUp);
		unifiedFov.angleDown = FMath::Min(unifiedFov.angleDown, view.fov.angleDown);
	}
	outHFOVInDegrees = FMath::RadiansToDegrees(unifiedFov.angleRight - unifiedFov.angleLeft);
	outVFOVInDegrees = FMath::RadiansToDegrees(unifiedFov.angleUp - unifiedFov.angleDown);
}

void FStreamHMD::SetInterpupillaryDistance(float newInterpupillaryDistance)
{
}

float FStreamHMD::GetInterpupillaryDistance() const
{
	const FPipelinedFrameState& frameState = GetPipelinedFrameStateForThread();
	if (frameState.views.Num() < 2)
	{
		return 0.064f;
	}

	FVector leftPos = ToFVector(frameState.views[0].pose.position);
	FVector rightPos = ToFVector(frameState.views[1].pose.position);

	return FVector::Dist(leftPos, rightPos);
}

bool FStreamHMD::IsChromaAbCorrectionEnabled() const
{
	return false;
}

bool FStreamHMD::IsStereoEnabled() const
{
	return m_stereoEnabled;
}

EHMDTrackingOrigin::Type FStreamHMD::GetTrackingOrigin() const
{
	return EHMDTrackingOrigin::Stage;
}

void FStreamHMD::SetTrackingOrigin(EHMDTrackingOrigin::Type newOriginType)
{
}

bool FStreamHMD::EnableStereo(bool iStereo)
{
	// Workaround to the issue where StreamInput module is not loaded on package build
	IPluginManager::Get().LoadModulesForEnabledPlugins(ELoadingPhase::PostEngineInit);

	if (iStereo == m_stereoEnabled)
	{
		return true;
	}
	m_isMobileMultiViewEnabled = false;
	m_stereoEnabled = iStereo;
	if (iStereo)
	{
		UE_LOG(LogHMD, Display, TEXT("Start Connection"));
	}
	else
	{
		if (m_connectionCreated)
		{
			auto err = m_serverApi.closeConnection(m_streamConnection);
			if (err != IsarError::eNone)
			{
				// write error to output or so (if there is one)
				UE_LOG(LogHMD, Error, TEXT("Error in Close Connection"));
			}
			UE_LOG(LogHMD, Display, TEXT("Close Connection"));
		}
		return true;
	}

	if (!m_connectionCreated)
	{
		isar::RemotingConfig remotingConfig;
		std::vector<IsarIceServerConfig> iceServerSettings;
		if (!InitConnectionConfig(iceServerSettings))
		{
			UE_LOG(LogHMD, Error, TEXT("Error : Failed to setup Config settings"));
			return false;
		}

		remotingConfig.diagnosticOptions = m_diagnosticOptions;
		remotingConfig.encoderBitrateKbps = m_encoderBandwidth;

		IsarSignalingConfig signalingConfig;
		signalingConfig.port = m_streamPort;
		std::string streamIpStr(TCHAR_TO_UTF8(*m_streamIp));
		signalingConfig.suggestedIpv4 = streamIpStr.c_str();

		IsarPortRange portRange;
		portRange.minPort = m_minPort;
		portRange.maxPort = m_maxPort;

		std::string appName(TCHAR_TO_UTF8(FApp::GetProjectName()));

		IsarGraphicsApiConfig gfxConfig;

		if (m_gfxApiType == IsarGraphicsApiType_D3D12)
		{
			gfxConfig.graphicsApiType = isar::IsarGraphicsApiType_D3D12;

			gfxConfig.d3d12.device = m_pD3D12Device;
			gfxConfig.d3d12.commandQueue = m_pD3D12CommandQueue;
			gfxConfig.d3d12.fence = m_pD3D12Fence;

			auto err = CreateConnection(appName,
										gfxConfig,
										remotingConfig,
										iceServerSettings,
										signalingConfig,
										portRange,
										&m_streamConnection);

			if (err != IsarError::eNone || !m_streamConnection)
			{
				// Write error to output
				UE_LOG(LogHMD, Error, TEXT("Error in Create Connection (D3D12), Status: %d"), err);
				FMessageDialog::Open(EAppMsgType::Ok,
									 LOCTEXT("StreamCreateConnectionErrorD12",
											 "Error in Stream Create Connection (D3D12)"));
				return false;
			}
		}
		else
		{
			gfxConfig.graphicsApiType = isar::IsarGraphicsApiType_D3D11;
			gfxConfig.d3d11.device = m_pD3D11Device;

			auto err = CreateConnection(appName,
										gfxConfig,
										remotingConfig,
										iceServerSettings,
										signalingConfig,
										portRange,
										&m_streamConnection);

			if (err != IsarError::eNone || !m_streamConnection)
			{
				// Write error to output
				UE_LOG(LogHMD, Error, TEXT("Error in Create Connection (D3D11), Status: %d"), err);
				FMessageDialog::Open(EAppMsgType::Ok,
									 LOCTEXT("StreamCreateConnectionErrorD11",
											 "Error in Stream Create Connection (D3D11)"));
				return false;
			}
		}
		m_connectionCreated = true;

		check(m_inputModule);
		m_inputModule->SetStreamApi(m_streamConnection, &m_serverApi);
		m_audioListener->SetStreamApi(m_streamConnection, &m_serverApi);

		if (m_microphoneCaptureStream)
		{
			// If the microphone stream is created and set before the stereo is enabled,
			// update it here
			m_microphoneCaptureStream->SetStreamApi(m_streamConnection, &m_serverApi);
			m_microphoneCaptureStream->SetConnected(m_connected);
		}

		m_serverApi.registerConnectionStateHandler(m_streamConnection,
												   [](IsarConnectionState newState, void* userData)
												   {
													   reinterpret_cast<FStreamHMD*>(userData)->
														   OnConnectionStateChanged(newState);
												   },
												   this);

		// Init Video track
		auto err = m_serverApi.initVideoTrack(m_streamConnection, gfxConfig);
		if (err != IsarError::eNone)
		{
			// Write error to output
			UE_LOG(LogHMD, Error, TEXT("Error in InitVideoTrack, Status: %d"), err);
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("StreamInitVideoError", "Error in Stream Init Video Track"));
			return false;
		}

		FApp::SetUseVRFocus(true);
		FApp::SetHasVRFocus(true);
		constexpr float targetFrameRate = 90.0f;
		GEngine->FixedFrameRate = targetFrameRate;
		GEngine->bUseFixedFrameRate = true;
		if (iStereo)
		{
			GEngine->bForceDisableFrameRateSmoothing = true;
			GEngine->MinDesiredFrameRate = 0;
			if (OnStereoStartup())
			{
				m_inputModule->Start();
				if (!GIsEditor)
				{
					GEngine->SetMaxFPS(0);
				}
				FApp::SetUseVRFocus(true);
				FApp::SetHasVRFocus(true);
				if (auto* sceneVp = static_cast<FSceneViewport*>(FindSceneViewport()))
				{
					TSharedPtr<SWindow> window = sceneVp->FindWindow();
					if (window.IsValid())
					{
						uint32 sizeX = 0;
						uint32 sizeY = 0;
						CalculateRenderTargetSize(*sceneVp, sizeX, sizeY);
						// Window continues to be processed when PIE spectator window is minimized
						window->SetIndependentViewportSize(FVector2D(sizeX, sizeY));
					}
				}
			}
		}
	}

	auto err = m_serverApi.openConnection(m_streamConnection);
	if (err != IsarError::eNone || !m_streamConnection)
	{
		// Write error to output
		UE_LOG(LogHMD, Error, TEXT("Error in Open Connection, Status: %d "), err);
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("StreamOpenConnectionError", "Error in Stream Open Connection"));
		return false;
	}

	return true;
}

bool FStreamHMD::OnStartGameFrame(FWorldContext& worldContext)
{
#if WITH_EDITOR
	// In the editor there can be multiple worlds.  An editor world, pie worlds, other viewport worlds for editor pages.
	// XR hardware can only be running with one of them.
	if (GIsEditor && GEditor && GEditor->GetPIEWorldContext() != nullptr && !worldContext.bIsPrimaryPIEInstance)
	{
		return false;
	}
#endif // WITH_EDITOR

	const AWorldSettings* const pWorldSettings = worldContext.World()
		? worldContext.World()->GetWorldSettings()
		: nullptr;
	if (pWorldSettings)
	{
		m_worldToMeters = pWorldSettings->WorldToMeters;
	}

	RefreshTrackingToWorldTransform(worldContext);
	FCoreDelegates::VRHeadsetReconnected.Broadcast();
	UpdateDeviceLocations();
	return true;
}

bool FStreamHMD::OnStereoStartup()
{
	// Enumerate the views we will be simulating with.
	EnumerateViews(m_pipelinedFrameStateGame);
	m_pipelinedFrameStateRHI = m_pipelinedFrameStateRendering = m_pipelinedFrameStateGame;

	// Create initial tracking space
	m_baseOrientation = FQuat::Identity;
	m_basePosition = FVector::ZeroVector;
	if (m_renderBridge.IsValid())
	{
		m_renderBridge->SetStreamHMD(this);
	}
	else
	{
		UE_LOG(LogHMD, Error, TEXT("Error: Invalid RenderBridge"));
		return false;
	}

	// Grab a pointer to the renderer module for displaying our mirror window
	static const FName RENDERER_MODULE_NAME("Renderer");
	m_rendererModule = FModuleManager::GetModulePtr<IRendererModule>(RENDERER_MODULE_NAME);
	SpectatorScreenController = MakeUnique<FDefaultSpectatorScreenController>(this);

	return true;
}

bool FStreamHMD::InitConnectionConfig(std::vector<IsarIceServerConfig>& serverConfigArray)
{
#if WITH_EDITOR
	FString filePath = FPaths::Combine(FPaths::ProjectPluginsDir(),
									   TEXT("HololightStream/Resources/remoting-config.cfg"));
#else
	FString filePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/remoting-config.cfg"));
#endif

	// Load JSON content
	FString jsonContent;
	if (FFileHelper::LoadFileToString(jsonContent, *filePath))
	{
		TSharedPtr<FJsonObject> jsonObject;
		TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(jsonContent);

		if (FJsonSerializer::Deserialize(jsonReader, jsonObject) && jsonObject.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* iceServersArray;
			if (jsonObject->TryGetArrayField(TEXT("ice-servers"), iceServersArray))
			{
				if (iceServersArray->Num() > 0)
				{
					for (const TSharedPtr<FJsonValue>& serverValue : *iceServersArray)
					{
						TSharedPtr<FJsonObject> iceServerObject = serverValue->AsObject();
						if (iceServerObject.IsValid())
						{
							IsarIceServerConfig serverConfigObj;
							FString iUrl = iceServerObject->GetStringField(TEXT("url"));
							serverConfigObj.url = _wcsdup(*iUrl);
							FString iUsername = iceServerObject->GetStringField(TEXT("username"));
							serverConfigObj.username = _wcsdup(*iUsername);;
							FString iPasswd = iceServerObject->GetStringField(TEXT("credential"));
							serverConfigObj.password = _wcsdup(*iPasswd);;
							serverConfigArray.push_back(serverConfigObj);
						}
					}
				}
			}
			else
			{
				UE_LOG(LogHMD, Error, TEXT("Failed to find 'ice-servers' array in JSON!"));
			}

			m_diagnosticOptions = IsarDiagnosticOptions_DISABLED;
			const TArray<TSharedPtr<FJsonValue>>* diagnosticsArray;
			if (jsonObject->TryGetArrayField(TEXT("diagnostic-options"), diagnosticsArray))
			{
				if (diagnosticsArray->Num() > 0)
				{
					for (const TSharedPtr<FJsonValue>& diagnosticsValue : *diagnosticsArray)
					{
						auto diagnosticString = diagnosticsValue->AsString();
						if (diagnosticString == TEXT("tracing"))
						{
							m_diagnosticOptions = (IsarDiagnosticOptions)(m_diagnosticOptions |
								IsarDiagnosticOptions_ENABLE_TRACING);
						}
						if (diagnosticString == TEXT("event-log"))
						{
							m_diagnosticOptions = (IsarDiagnosticOptions)(m_diagnosticOptions |
								IsarDiagnosticOptions_ENABLE_EVENT_LOG);
						}
						if (diagnosticString == TEXT("stats-collector"))
						{
							m_diagnosticOptions = (IsarDiagnosticOptions)(m_diagnosticOptions |
								IsarDiagnosticOptions_ENABLE_STATS_COLLECTOR);
						}
					}
				}
			}

			m_streamIp = jsonObject->GetObjectField(TEXT("signaling"))->GetStringField(TEXT("ip"));
			m_streamPort = jsonObject->GetObjectField(TEXT("signaling"))->GetIntegerField(TEXT("port"));
			m_encoderBandwidth = jsonObject->GetIntegerField(TEXT("encoder-bandwidth-kbps"));
			m_minPort = jsonObject->GetObjectField(TEXT("port-range"))->GetIntegerField(TEXT("min-port"));
			m_maxPort = jsonObject->GetObjectField(TEXT("port-range"))->GetIntegerField(TEXT("max-port"));
			UE_LOG(LogHMD, Log, TEXT("Signaling IP: %s, Port: %d, Max Port: %d,, Min Port: %d"), *m_streamIp,
				   m_streamPort, m_maxPort, m_minPort);
		}
		else
		{
			UE_LOG(LogHMD, Error, TEXT("Failed to parse Remote-Config file: %s"), *filePath);
			return false;
		}
	}
	else
	{
		UE_LOG(LogHMD, Error, TEXT("Failed to load Remote-Config file: %s"), *filePath);
		return false;
	}
	return true;
}

void FStreamHMD::AdjustViewRect(int32 viewIndex, int32& x, int32& y, uint32& sizeX, uint32& sizeY) const
{
	const FPipelinedFrameState& pipelineState = GetPipelinedFrameStateForThread();
	if (viewIndex == INDEX_NONE || !pipelineState.viewConfigs.IsValidIndex(viewIndex))
	{
		return;
	}

	const XrViewConfigurationView& vConfig = pipelineState.viewConfigs[viewIndex];
	FIntPoint viewRectMin(EForceInit::ForceInitToZero);

	// If Mobile Multi-View is active the first two views will share the same position
	// Thus the start index should be the second view if enabled
	for (int32 i = m_isMobileMultiViewEnabled ? 1 : 0; i < viewIndex; ++i)
	{
		viewRectMin.X += FMath::CeilToInt(
			pipelineState.viewConfigs[i].recommendedImageRectWidth * pipelineState.pixelDensity);
		QuantizeSceneBufferSize(viewRectMin, viewRectMin);
	}

	x = viewRectMin.X;
	y = viewRectMin.Y;
	const FIntPoint densityAdjustedSize = GeneratePixelDensitySize(vConfig, pipelineState.pixelDensity);
	sizeX = densityAdjustedSize.X;
	sizeY = densityAdjustedSize.Y;
}

void FStreamHMD::SetFinalViewRect(FRHICommandListImmediate& rhiCmdList, const int32 stereoViewIndex,
								  const FIntRect& finalViewRect)
{
	check(IsInRenderingThread());

	if (stereoViewIndex == INDEX_NONE || !m_pipelinedLayerStateRendering.colorImages.IsValidIndex(stereoViewIndex))
	{
		return;
	}

	XrSwapchainSubImage& colorImage = m_pipelinedLayerStateRendering.colorImages[stereoViewIndex];
	colorImage.imageArrayIndex = m_isMobileMultiViewEnabled && stereoViewIndex < 2 ? stereoViewIndex : 0;
	colorImage.imageRect = {
		{finalViewRect.Min.X, finalViewRect.Min.Y},
		{finalViewRect.Width(), finalViewRect.Height()}
	};
}

void FStreamHMD::OnLateUpdateApplied_RenderThread(FRHICommandListImmediate& rhiCmdList,
												  const FTransform& newRelativeTransform)
{
	FHeadMountedDisplayBase::OnLateUpdateApplied_RenderThread(rhiCmdList, newRelativeTransform);
	if (m_pipelinedFrameStateRendering.views.Num() == m_pipelinedLayerStateRendering.projectionLayers.Num())
	{
		for (int32 viewIndex = 0; viewIndex < m_pipelinedLayerStateRendering.projectionLayers.Num(); viewIndex++)
		{
			const XrView& projView = m_pipelinedFrameStateRendering.views[viewIndex];
			XrCompositionLayerProjectionView& projection = m_pipelinedLayerStateRendering.projectionLayers[viewIndex];
			FTransform eyePose = ToFTransform(projView.pose, GetWorldToMetersScale());
			FTransform newRelativePoseTransform = eyePose * newRelativeTransform;
			newRelativePoseTransform.NormalizeRotation();
			projection.pose = ToXrPose(newRelativePoseTransform, GetWorldToMetersScale());
			projection.fov = projView.fov;
		}
	}
}

FMatrix FStreamHMD::GetStereoProjectionMatrix(const int32 viewIndex) const
{
	const FPipelinedFrameState& frameState = GetPipelinedFrameStateForThread();
	XrFovf fov = {};
	if (viewIndex == eSSE_MONOSCOPIC)
	{
		// The monoscopic projection matrix uses the combined field-of-view of both eyes

		for (int32 indexV = 0; indexV < frameState.views.Num(); indexV++)
		{
			const XrFovf& viewFov = frameState.views[indexV].fov;
			fov.angleUp = FMath::Max(fov.angleUp, viewFov.angleUp);
			fov.angleDown = FMath::Min(fov.angleDown, viewFov.angleDown);
			fov.angleLeft = FMath::Min(fov.angleLeft, viewFov.angleLeft);
			fov.angleRight = FMath::Max(fov.angleRight, viewFov.angleRight);
		}
	}
	else
	{
		fov = (viewIndex < frameState.views.Num())
			? frameState.views[viewIndex].fov
			: XrFovf{-PI / 4.0f, PI / 4.0f, PI / 4.0f, -PI / 4.0f};
	}

	fov.angleUp = tan(fov.angleUp);
	fov.angleDown = tan(fov.angleDown);
	fov.angleLeft = tan(fov.angleLeft);
	fov.angleRight = tan(fov.angleRight);

	float zNear = GNearClippingPlane_RenderThread;
	float sumRL = (fov.angleRight + fov.angleLeft);
	float sumTB = (fov.angleUp + fov.angleDown);
	float invRL = (1.0f / (fov.angleRight - fov.angleLeft));
	float invTB = (1.0f / (fov.angleUp - fov.angleDown));

	FMatrix mat = FMatrix(
		FPlane((2.0f * invRL), 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, (2.0f * invTB), 0.0f, 0.0f),
		FPlane((sumRL * -invRL), (sumTB * -invTB), 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, zNear, 0.0f)
		);

	return mat;
}

FIntPoint FStreamHMD::GetIdealRenderTargetSize() const
{
	const FPipelinedFrameState& pipelineState = GetPipelinedFrameStateForThread();
	FIntPoint pointSize(EForceInit::ForceInitToZero);
	for (int32 viewIndex = 0; viewIndex < pipelineState.views.Num(); viewIndex++)
	{
		const XrViewConfigurationView& configV = pipelineState.viewConfigs[viewIndex];

		// If Mobile Multi-View is active the first two views will share the same position
		pointSize.X = m_isMobileMultiViewEnabled && viewIndex < 2
			? FMath::Max(pointSize.X, (int)configV.recommendedImageRectWidth)
			: pointSize.X + (int)configV.recommendedImageRectWidth;
		pointSize.Y = FMath::Max(pointSize.Y, (int)configV.recommendedImageRectHeight);

		// Make sure we quantize in order to be consistent with the rest of the engine in creating our buffers.
		QuantizeSceneBufferSize(pointSize, pointSize);
	}

	return pointSize;
}

bool FStreamHMD::AllocateRenderTargetTextures(uint32 sizeX, uint32 sizeY, uint8 format, uint32 numLayers,
											  ETextureCreateFlags flags, ETextureCreateFlags targetableTextureFlags,
											  TArray<FTextureRHIRef>& outTargetableTextures,
											  TArray<FTextureRHIRef>& outShaderResourceTextures,
											  uint32 numSamples)
{
	ETextureCreateFlags unifiedCreateFlags = flags | targetableTextureFlags;

	// This is not a static swapchain
	unifiedCreateFlags |= TexCreate_Dynamic;

	// We need to ensure we can sample from the texture in CopyTexture
	unifiedCreateFlags |= TexCreate_ShaderResource;

	// We assume this could be used as a resolve target
	unifiedCreateFlags |= TexCreate_ResolveTargetable;

	// Some render APIs require us to present in RT layouts/configs,
	// so even if app won't use this texture as RT, we need the flag.
	unifiedCreateFlags |= TexCreate_RenderTargetable;

	FClearValueBinding valueBindings = FClearValueBinding::Transparent;
	UE_LOG(LogHMD, Verbose, TEXT("AllocateRenderTargetTextures"));
	int numViews = 2;
	if (m_connected)
	{
		m_serverApi.getConnectionInfo(m_streamConnection, &m_connectionInfo);
		sizeX = m_connectionInfo.renderConfig.width * m_connectionInfo.renderConfig.numViews;
		sizeY = m_connectionInfo.renderConfig.height;
		numViews = m_connectionInfo.renderConfig.numViews;
		UE_LOG(LogHMD, Log, TEXT("AllocateRenderTargetTextures  width: %d, height: %d"), sizeX, sizeY);
	}

	m_streamSwapchain.Reset();

	{
		uint8 unusedActualFormat = 0;
		m_streamSwapchain = m_renderBridge->CreateSwapchain(IStereoRenderTargetManager::GetStereoLayerPixelFormat(),
															unusedActualFormat,
															sizeX,
															sizeY,
															1,
															1,
															1,
															unifiedCreateFlags,
															valueBindings);
		if (!m_streamSwapchain)
		{
			UE_LOG(LogHMD, Error, TEXT("Error: Failed to create SwapChain with width %d height %d"), sizeX, sizeY);
			return false;
		}
	}

	outTargetableTextures = m_streamSwapchain->GetSwapChain();
	outShaderResourceTextures = outTargetableTextures;

	m_width = sizeX;
	m_height = sizeY;
	m_nViews = numViews;
	UE_LOG(LogHMD, Log, TEXT("Creating new StreamSwapchain width: %d, height: %d"), sizeX, sizeY);
	m_needsReallocation = false;

	return true;
}

// Ensure we always use the left eye when selecting LODs to avoid divergent selections in stereo
uint32 FStreamHMD::GetLODViewIndex() const
{
	return IStereoRendering::GetLODViewIndex();
}

EStereoscopicPass FStreamHMD::GetViewPassForIndex(bool stereoRequested, int32 viewIndex) const
{
	if (!stereoRequested)
	{
		return EStereoscopicPass::eSSP_FULL;
	}

	if (viewIndex == 0)
	{
		return EStereoscopicPass::eSSP_PRIMARY;
	}

	if (viewIndex == 1)
	{
		return EStereoscopicPass::eSSP_SECONDARY;
	}

	return EStereoscopicPass::eSSP_FULL;
}


void FStreamHMD::GetEyeRenderParams_RenderThread(const FHeadMountedDisplayPassContext& context,
												 FVector2D& eyeToSrcUVScaleValue,
												 FVector2D& eyeToSrcUVOffsetValue) const
{
	eyeToSrcUVOffsetValue = FVector2D::ZeroVector;
	eyeToSrcUVScaleValue = FVector2D(1.0f, 1.0f);
}

bool FStreamHMD::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& context) const
{
	// Don't activate the SVE if xr is being used for tracking only purposes
	static const bool XR_TRACKING_ONLY = FParse::Param(FCommandLine::Get(), TEXT("xrtrackingonly"));

	return FHMDSceneViewExtension::IsActiveThisFrame_Internal(context) && !XR_TRACKING_ONLY;
}

FIntRect FStreamHMD::GetFullFlatEyeRect_RenderThread(FTextureRHIRef eyeTexture) const
{
	FVector2D srcNormRectMin(0.0f, 0.0f);
	// with MMV, each eye occupies the whole RT layer, so we don't need to limit the source rect to the left half of the RT.
	FVector2D srcNormRectMax(1.0f, 1.0f);
	if (m_nViews > 1)
	{
		srcNormRectMin.X /= 2;
		srcNormRectMax.X /= 2;
	}

	return FIntRect(eyeTexture->GetSizeX() * srcNormRectMin.X, eyeTexture->GetSizeY() * srcNormRectMin.Y,
					eyeTexture->GetSizeX() * srcNormRectMax.X, eyeTexture->GetSizeY() * srcNormRectMax.Y);
}

int32 FStreamHMD::GetDesiredNumberOfViews(bool stereoRequested) const
{
	//Always instanced stereo
	return 2;
}

void FStreamHMD::OnBeginRendering_RHIThread(const FPipelinedFrameState& inFrameState, FXRSwapChainPtr swapchainPtr)
{
	ensure(IsInRenderingThread() || IsInRHIThread());
	m_pipelinedFrameStateRHI = inFrameState;
}

void FStreamHMD::OnBeginRendering_RenderThread(FRHICommandListImmediate& rhiCmdList, FSceneViewFamily& viewFamily)
{
	ensure(IsInRenderingThread());
	if (!m_renderBridge)
	{
		// Frame submission is not necessary in a headless session.
		return;
	}

	// Create the SwapChain here
	if (m_connected)
	{
		UpdateDeviceLocations();
	}

	rhiCmdList.EnqueueLambda(
		[this, FrameState = m_pipelinedFrameStateRendering](FRHICommandListImmediate& inRHICmdList)
		{
			OnBeginRendering_RHIThread(FrameState, m_streamSwapchain);
		});
}

int32 FStreamHMD::AcquireColorTexture()
{
	check(IsInGameThread());
	if (const FXRSwapChainPtr& swapchain = m_streamSwapchain)
	{
		return swapchain->GetSwapChainIndex_RHIThread();
	}
	return 0;
}

void FStreamHMD::PostRenderViewFamily_RenderThread(FRDGBuilder& graphBuilder, FSceneViewFamily& inViewFamily)
{
	if (m_streamSwapchain && m_connected)
	{
		for (int32 viewIndex = 0; viewIndex < m_pipelinedLayerStateRendering.colorImages.Num(); viewIndex++)
		{
			if (!m_pipelinedLayerStateRendering.colorImages.IsValidIndex(viewIndex))
			{
				continue;
			}

			// Update SubImages with latest swapchain
			XrSwapchainSubImage& colorImage = m_pipelinedLayerStateRendering.colorImages[viewIndex];
			colorImage.swapchain = m_pipelinedLayerStateRendering.colorSwapchain.IsValid()
				? static_cast<FStreamXRSwapchain*>(m_pipelinedLayerStateRendering.colorSwapchain
					.Get())->GetHandle()
				: nullptr;
			XrCompositionLayerProjectionView& projection = m_pipelinedLayerStateRendering.projectionLayers[viewIndex];
			projection.subImage = colorImage;
		}

		AddPass(graphBuilder, RDG_EVENT_NAME("StreamHMDCorrection"), [this](FRHICommandListImmediate& rhiCmdList)
		{
			auto* texture = m_streamSwapchain->GetTexture2D();
			const uint32 width = texture->GetSizeX();
			const uint32 height = texture->GetSizeY();
			const FIntPoint targetSize(width, height);

			FTextureRHIRef stagingTexture = m_stagingBufferPool.CreateStagingBuffer_RenderThread(
				rhiCmdList, width, height, texture->GetFormat());
			TransitionAndCopyTexture(rhiCmdList, texture, stagingTexture, {});

			rhiCmdList.Transition(FRHITransitionInfo(texture, ERHIAccess::Unknown, ERHIAccess::RTV));

			FRHITexture* colorRT = texture->GetTexture2DArray()
				? texture->GetTexture2DArray()
				: texture->GetTexture2D();
			FRHIRenderPassInfo renderPassInfo(colorRT, ERenderTargetActions::Load_Store);

			rhiCmdList.BeginRenderPass(renderPassInfo, TEXT("StreamHMDCorrection"));
			{
				DrawClearQuadAlpha(rhiCmdList, 0.0f);

				rhiCmdList.SetViewport(0, 0, 0, width, height, 1.0f);

				FGraphicsPipelineStateInitializer graphicsPSOInit;
				rhiCmdList.ApplyCachedRenderTargets(graphicsPSOInit);

				graphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA>::GetRHI();

				graphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				graphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
				graphicsPSOInit.PrimitiveType = PT_TriangleList;

				FGlobalShaderMap* pShaderMap = GetGlobalShaderMap(GetConfiguredShaderPlatform());;

				TShaderMapRef<FScreenVS> mapVertexShader(pShaderMap);

				TShaderRef<FGlobalShader> pixelShader;
				TShaderRef<FStreamCorrectionPS> streamCorrectionPS;

				TShaderMapRef<FStreamCorrectionPS> streamCorrectionPSRef(pShaderMap);

				streamCorrectionPS = streamCorrectionPSRef;
				pixelShader = streamCorrectionPSRef;

				graphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
				graphicsPSOInit.BoundShaderState.VertexShaderRHI = mapVertexShader.GetVertexShader();
				graphicsPSOInit.BoundShaderState.PixelShaderRHI = pixelShader.GetPixelShader();

				SetGraphicsPipelineState(rhiCmdList, graphicsPSOInit, 0);

				rhiCmdList.Transition(FRHITransitionInfo(stagingTexture, ERHIAccess::Unknown, ERHIAccess::SRVMask));

				SetShaderParametersLegacyPS(rhiCmdList, streamCorrectionPS, stagingTexture);

				m_rendererModule->DrawRectangle(
					rhiCmdList,
					0, 0,
					width, height,
					0.0f, 0.0f,
					1.0f, 1.0f,
					targetSize,
					FIntPoint(1, 1),
					mapVertexShader,
					EDRF_Default);
			}

			rhiCmdList.EndRenderPass();

			rhiCmdList.Transition(FRHITransitionInfo(texture, ERHIAccess::RTV, ERHIAccess::Present));

			m_stagingBufferPool.ReleaseStagingBufferForUnmap_AnyThread(stagingTexture);
		});
	}
}

void FStreamHMD::PostRenderView_RenderThread(FRDGBuilder& graphBuilder, FSceneView& inView)
{
}

inline IsarPose GetHeadToRightEyeTransform(const IsarXrPose& pose)
{
	using namespace Windows::Foundation::Numerics;

	IsarPose headToEyePose = {};

	float3 positionLeft = reinterpret_cast<const float3&>(pose.poseLeft.position);
	quaternion orientationLeft = reinterpret_cast<const quaternion&>(pose.poseLeft.orientation);
	float3 positionRight = reinterpret_cast<const float3&>(pose.poseRight.position);
	quaternion orientationRight = reinterpret_cast<const quaternion&>(pose.poseRight.orientation);

	auto leftTranslationMatrix = make_float4x4_translation(positionLeft);
	auto leftRotationMatrix = make_float4x4_from_quaternion(orientationLeft);

	auto leftWorldMatrix = leftRotationMatrix * leftTranslationMatrix;

	float4x4 leftViewMatrix{};
	invert(leftWorldMatrix, &leftViewMatrix);

	auto rightTranslationMatrix = make_float4x4_translation(positionRight);
	auto rightRotationMatrix = make_float4x4_from_quaternion(orientationRight);

	auto rightWorldMatrix = rightRotationMatrix * rightTranslationMatrix;

	float4x4 rightViewMatrix{};
	invert(rightWorldMatrix, &rightViewMatrix);

	auto t1 = translation(leftWorldMatrix);
	auto t2 = translation(rightWorldMatrix);
	auto pos = t2 - t1;

	// https://stackoverflow.com/a/22167097
	auto left = make_quaternion_from_rotation_matrix(leftViewMatrix);
	auto right = make_quaternion_from_rotation_matrix(rightViewMatrix);
	quaternion q = left * inverse(right);

	auto p = transform4(pos, left);

	// Right to left hand coordinate conversion
	headToEyePose.orientation = {q.x, q.y, q.z, q.w};
	headToEyePose.position = {p.x, p.y, p.z};

	return headToEyePose;
}

bool FStreamHMD::GetRelativeEyePose(int32 inDeviceId, int32 inViewIndex, FQuat& outOrientation, FVector& outPosition)
{
	if (inDeviceId != IXRTrackingSystem::HMDDeviceId)
	{
		return false;
	}

	IsarXrPose inputPose;
	if (m_connected)
	{
		const FPipelinedFrameState& frameState = GetPipelinedFrameStateForThread();;
		if (!frameState.views.IsValidIndex(inViewIndex))
		{
			return false;
		}

		if (inViewIndex <= 0) // Left eye
		{
			outPosition = FVector(0, 0, 0);
			outOrientation = FQuat::Identity;
		}
		else
		{
			inputPose.poseLeft.orientation.w = frameState.views[0].pose.orientation.w;
			inputPose.poseLeft.orientation.x = frameState.views[0].pose.orientation.x;
			inputPose.poseLeft.orientation.y = frameState.views[0].pose.orientation.y;
			inputPose.poseLeft.orientation.z = frameState.views[0].pose.orientation.z;
			inputPose.poseLeft.position.x = frameState.views[0].pose.position.x;
			inputPose.poseLeft.position.y = frameState.views[0].pose.position.y;
			inputPose.poseLeft.position.z = frameState.views[0].pose.position.z;
			inputPose.poseRight.position.x = frameState.views[1].pose.position.x;
			inputPose.poseRight.position.y = frameState.views[1].pose.position.y;
			inputPose.poseRight.position.z = frameState.views[1].pose.position.z;
			inputPose.poseRight.orientation.w = frameState.views[1].pose.orientation.w;
			inputPose.poseRight.orientation.x = frameState.views[1].pose.orientation.x;
			inputPose.poseRight.orientation.y = frameState.views[1].pose.orientation.y;
			inputPose.poseRight.orientation.z = frameState.views[1].pose.orientation.z;
			IsarPose rightRelativePose = GetHeadToRightEyeTransform(inputPose);

			outPosition = FVector(-rightRelativePose.position.z * m_worldToMeters,
								  rightRelativePose.position.x * m_worldToMeters,
								  rightRelativePose.position.y * m_worldToMeters);
			outOrientation = FQuat(-rightRelativePose.orientation.z, rightRelativePose.orientation.x,
								   rightRelativePose.orientation.y, -rightRelativePose.orientation.w);
		}
	}
	else
	{
		outPosition = FVector(0.0f, 0.0f, 0.0f);
		outOrientation = FQuat::Identity;
	}

	return true;
}

void FStreamHMD::GetPositionRotation(const XrVector3f& position, const XrQuaternionf& orientation, FVector& oPosition,
									 FQuat& pOrientation)
{
	oPosition = FVector(-position.z * m_worldToMeters, position.x * m_worldToMeters, position.y * m_worldToMeters);
	pOrientation = FQuat(-orientation.z, orientation.x, orientation.y, -orientation.w);
}

bool FStreamHMD::GetPlayAreaRect(FTransform& outTransform, FVector2D& outExtent) const
{
	return true;
}

void FStreamHMD::OnFinishRendering_RHIThread()
{
	ensure(IsInRenderingThread() || IsInRHIThread());
	if (!m_renderBridge || !m_streamSwapchain || m_needsReallocation)
	{
		return;
	}

	if (m_connected && m_streamConnection)
	{
		const FRHITexture* pRenderedTexture = m_streamSwapchain->GetTexture2D();
		const float nearZ = GNearClippingPlane_RenderThread / GetWorldToMetersScale();
		const float farZ = 5000.0f / GetWorldToMetersScale();
		const FPipelinedFrameState& pipelineState = m_pipelinedFrameStateRHI;
		IsarFrameInfo frameInfo;
		frameInfo.hasFocusPlane = 0;
		frameInfo.zFar = farZ;
		frameInfo.zNear = nearZ;
		frameInfo.textureFormat = IsarTextureFormat_RGBA32;
		frameInfo.hasFocusPlane = 0;

		m_serverApi.getConnectionInfo(m_streamConnection, &m_connectionInfo);
		if (m_connectionInfo.renderConfig.numViews == 1 && !pipelineState.views.IsEmpty())
		{
			frameInfo.pose.poseLeft.orientation.x = pipelineState.views[0].pose.orientation.x;
			frameInfo.pose.poseLeft.orientation.y = pipelineState.views[0].pose.orientation.y;
			frameInfo.pose.poseLeft.orientation.z = pipelineState.views[0].pose.orientation.z;
			frameInfo.pose.poseLeft.orientation.w = pipelineState.views[0].pose.orientation.w;
			frameInfo.pose.poseLeft.position.x = pipelineState.views[0].pose.position.x;
			frameInfo.pose.poseLeft.position.y = pipelineState.views[0].pose.position.y;
			frameInfo.pose.poseLeft.position.z = pipelineState.views[0].pose.position.z;

			frameInfo.pose.fovLeft.left = pipelineState.views[0].fov.angleLeft;
			frameInfo.pose.fovLeft.right = pipelineState.views[0].fov.angleRight;
			frameInfo.pose.fovLeft.down = pipelineState.views[0].fov.angleDown;
			frameInfo.pose.fovLeft.up = pipelineState.views[0].fov.angleUp;
		}
		else
		{
			if (!pipelineState.views.IsEmpty())
			{
				frameInfo.pose.poseLeft.orientation.x = pipelineState.views[0].pose.orientation.x;
				frameInfo.pose.poseLeft.orientation.y = pipelineState.views[0].pose.orientation.y;
				frameInfo.pose.poseLeft.orientation.z = pipelineState.views[0].pose.orientation.z;
				frameInfo.pose.poseLeft.orientation.w = pipelineState.views[0].pose.orientation.w;
				frameInfo.pose.poseLeft.position.x = pipelineState.views[0].pose.position.x;
				frameInfo.pose.poseLeft.position.y = pipelineState.views[0].pose.position.y;
				frameInfo.pose.poseLeft.position.z = pipelineState.views[0].pose.position.z;

				frameInfo.pose.poseRight.orientation.x = pipelineState.views[1].pose.orientation.x;
				frameInfo.pose.poseRight.orientation.y = pipelineState.views[1].pose.orientation.y;
				frameInfo.pose.poseRight.orientation.z = pipelineState.views[1].pose.orientation.z;
				frameInfo.pose.poseRight.orientation.w = pipelineState.views[1].pose.orientation.w;
				frameInfo.pose.poseRight.position.x = pipelineState.views[1].pose.position.x;
				frameInfo.pose.poseRight.position.y = pipelineState.views[1].pose.position.y;
				frameInfo.pose.poseRight.position.z = pipelineState.views[1].pose.position.z;

				frameInfo.pose.fovLeft.left = pipelineState.views[0].fov.angleLeft;
				frameInfo.pose.fovLeft.right = pipelineState.views[0].fov.angleRight;
				frameInfo.pose.fovLeft.down = pipelineState.views[0].fov.angleDown;
				frameInfo.pose.fovLeft.up = pipelineState.views[0].fov.angleUp;

				frameInfo.pose.fovRight.left = pipelineState.views[1].fov.angleLeft;
				frameInfo.pose.fovRight.right = pipelineState.views[1].fov.angleRight;
				frameInfo.pose.fovRight.down = pipelineState.views[1].fov.angleDown;
				frameInfo.pose.fovRight.up = pipelineState.views[1].fov.angleUp;
			}
		}
		frameInfo.pose.frameTimestamp = pipelineState.frameTimestamp;
		frameInfo.pose.poseTimestamp = pipelineState.poseTimestamp;

		IsarGraphicsApiFrame frame;
		frame.info = frameInfo;
		if (IsRHID3D11())
		{
			frame.graphicsApiType = IsarGraphicsApiType_D3D11;
			frame.d3d11.frame = reinterpret_cast<ID3D11Texture2D*>(pRenderedTexture->GetNativeResource());
			frame.d3d11.depthFrame = nullptr;
		}
		else
		{
			frame.graphicsApiType = IsarGraphicsApiType_D3D12;
			frame.d3d12.frame = reinterpret_cast<ID3D12Resource*>(pRenderedTexture->GetNativeResource());
			frame.d3d12.depthFrame = nullptr;
			frame.d3d12.frameFenceValue = m_pD3D12Fence->GetCompletedValue();
			frame.d3d12.subresourceIndex = 0;
		}

		if (m_connected && m_streamConnection)
		{
			FReadScopeLock lock(m_frameHandleMutex);
			auto err = m_serverApi.pushFrame(m_streamConnection, frame);
			if (err != IsarError::eNone)
			{
				// write error to output or so (if there is one)
				UE_LOG(LogHMD, Error, TEXT("Error in PushFrame "));
			}
		}
	}
}

bool FStreamHMD::OnEndGameFrame(FWorldContext& worldContext)
{
	return true;
}

void FStreamHMD::EnumerateViews(FPipelinedFrameState& pipelineState)
{
	SCOPED_NAMED_EVENT(EnumerateViews, FColor::Red);
	uint32 viewConfigCount = 2;
	uint32_t configWidth = 2064; // Recommended default;
	uint32_t configHeight = 2208; // Recommended default;
	if (m_streamConnection && m_connected)
	{
		m_serverApi.getConnectionInfo(m_streamConnection, &m_connectionInfo);
		configWidth = m_connectionInfo.renderConfig.width;
		configHeight = m_connectionInfo.renderConfig.height;
		pipelineState.viewConfigs.SetNum(viewConfigCount);
		pipelineState.viewConfigs[0].recommendedImageRectHeight = configHeight;
		pipelineState.viewConfigs[0].recommendedImageRectWidth = configWidth;
		pipelineState.viewConfigs[1].recommendedImageRectHeight = configHeight;
		pipelineState.viewConfigs[1].recommendedImageRectWidth = configWidth;
		pipelineState.views.SetNum(viewConfigCount);
		for (XrView& view : pipelineState.views)
		{
			view.fov = XrFovf{-PI / 4.0f, PI / 4.0f, PI / 4.0f, -PI / 4.0f};
			view.pose = ToXrPose(FTransform::Identity);
		}
		m_pipelinedLayerStateRendering.colorImages.SetNum(viewConfigCount);
		m_pipelinedLayerStateRendering.projectionLayers.SetNum(viewConfigCount);
		return;
	}

	// Enumerate the viewport configuration views
	m_pipelinedLayerStateRendering.colorImages.SetNum(viewConfigCount);

	for (uint32 viewIndex = 0; viewIndex < viewConfigCount; viewIndex++)
	{
		XrViewConfigurationView view;

		pipelineState.viewConfigs.Add(view);
		pipelineState.viewConfigs[viewIndex].recommendedImageRectHeight = configHeight;
		pipelineState.viewConfigs[viewIndex].recommendedImageRectWidth = configWidth;
		pipelineState.viewConfigs[viewIndex].maxImageRectHeight = configHeight;
		pipelineState.viewConfigs[viewIndex].maxImageRectWidth = configWidth;
		pipelineState.viewConfigs[viewIndex].recommendedSwapchainSampleCount = 1;
		pipelineState.viewConfigs[viewIndex].maxSwapchainSampleCount = 1;
	}
	{
		// Ensure the views have sane values before we locate them
		pipelineState.views.SetNum(pipelineState.viewConfigs.Num());
		for (XrView& view : pipelineState.views)
		{
			view.fov = XrFovf{-PI / 4.0f, PI / 4.0f, PI / 4.0f, -PI / 4.0f};
			view.pose = ToXrPose(FTransform::Identity);
		}
	}
}

void FStreamHMD::CalculateRenderTargetSize(const FViewport& viewport, uint32& inOutSizeX, uint32& inOutSizeY)
{
	check(IsInGameThread() || IsInRenderingThread());

	const FPipelinedFrameState& pipelineState = GetPipelinedFrameStateForThread();
	const float pixelDensity = pipelineState.pixelDensity;

	if (!pipelineState.viewConfigs.IsEmpty())
	{
		FIntPoint size(EForceInit::ForceInitToZero);
		for (int32 viewIndex = 0; viewIndex < pipelineState.viewConfigs.Num(); viewIndex++)
		{
			const XrViewConfigurationView& config = pipelineState.viewConfigs[viewIndex];

			// If Mobile Multi-View is active the first two views will share the same position
			const bool mmvView = m_isMobileMultiViewEnabled && viewIndex < 2;

			const FIntPoint densityAdjustedSize = GeneratePixelDensitySize(config, pipelineState.pixelDensity);
			size.X = mmvView ? FMath::Max(size.X, densityAdjustedSize.X) : size.X + densityAdjustedSize.X;
			size.Y = FMath::Max(size.Y, densityAdjustedSize.Y);
		}

		if (size.X == 0 && size.Y == 0)
		{
			UE_LOG(LogHMD, Log, TEXT("Width == 0 Height == 0"));
		}
		inOutSizeX = size.X;
		inOutSizeY = size.Y;
		check(inOutSizeX != 0 && inOutSizeY != 0);
	}
}

const FStreamHMD::FPipelinedFrameState& FStreamHMD::GetPipelinedFrameStateForThread() const
{
	// Relying on implicit selection of the RHI struct is hazardous since the RHI thread isn't always present
	check(!IsInRHIThread());

	if (IsInActualRenderingThread())
	{
		return m_pipelinedFrameStateRendering;
	}
	else
	{
		check(IsInGameThread());
		return m_pipelinedFrameStateGame;
	}
}

FStreamHMD::FPipelinedFrameState& FStreamHMD::GetPipelinedFrameStateForThread()
{
	// Relying on implicit selection of the RHI struct is hazardous since the RHI thread isn't always present
	check(!IsInRHIThread());

	if (IsInActualRenderingThread())
	{
		return m_pipelinedFrameStateRendering;
	}
	else
	{
		check(IsInGameThread());
		return m_pipelinedFrameStateGame;
	}
}

void FStreamHMD::CopyTexture_RenderThread(FRHICommandListImmediate& rhiCmdList, FRHITexture* srcTexture,
										  FIntRect srcRect, FRHITexture* dstTexture, FIntRect dstRect,
										  bool clearBlack, ERenderTargetActions rtAction, ERHIAccess finalDstAccess,
										  ETextureCopyModifier srcTextureCopyModifier) const
{
	check(IsInRenderingThread());

	const uint32 viewportWidth = dstRect.Width();
	const uint32 viewportHeight = dstRect.Height();
	const FIntPoint targetSize(viewportWidth, viewportHeight);

	const float srcTextureWidth = srcTexture->GetSizeX();
	const float srcTextureHeight = srcTexture->GetSizeY();
	float u = 0.f, v = 0.f, uSize = 1.f, vSize = 1.f;
	if (srcRect.IsEmpty())
	{
		srcRect.Min.X = 0;
		srcRect.Min.Y = 0;
		srcRect.Max.X = srcTextureWidth;
		srcRect.Max.Y = srcTextureHeight;
	}
	else
	{
		u = srcRect.Min.X / srcTextureWidth;
		v = srcRect.Min.Y / srcTextureHeight;
		uSize = srcRect.Width() / srcTextureWidth;
		vSize = srcRect.Height() / srcTextureHeight;
	}

	rhiCmdList.Transition(FRHITransitionInfo(dstTexture, ERHIAccess::Unknown, ERHIAccess::RTV));

	FRHITexture* colorRT = dstTexture->GetTexture2DArray()
		? dstTexture->GetTexture2DArray()
		: dstTexture->GetTexture2D();
	FRHIRenderPassInfo renderPassInfo(colorRT, rtAction);
	//RenderPassInfo.ShadingRateTextureCombiner = VRSRB_Passthrough;
	rhiCmdList.BeginRenderPass(renderPassInfo, TEXT("StreamHMD_CopyTexture"));
	{
		if (clearBlack || srcTextureCopyModifier == ETextureCopyModifier::Opaque)
		{
			const FIntRect clearRect(0, 0, dstTexture->GetSizeX(), dstTexture->GetSizeY());
			rhiCmdList.SetViewport(clearRect.Min.X, clearRect.Min.Y, 0, clearRect.Max.X, clearRect.Max.Y, 1.0f);

			if (clearBlack)
			{
				DrawClearQuad(rhiCmdList, FLinearColor::Black);
			}
			else
			{
				// For opaque texture copies, we want to make sure alpha is initialized to 1.0f
				DrawClearQuadAlpha(rhiCmdList, 1.0f);
			}
		}

		rhiCmdList.SetViewport(dstRect.Min.X, dstRect.Min.Y, 0, dstRect.Max.X, dstRect.Max.Y, 1.0f);

		FGraphicsPipelineStateInitializer graphicsPSOInit;
		rhiCmdList.ApplyCachedRenderTargets(graphicsPSOInit);

		// We need to differentiate between types of layers: opaque, unpremultiplied alpha (regular texture copy) and premultiplied alpha (emulation texture)
		switch (srcTextureCopyModifier)
		{
			case ETextureCopyModifier::Opaque: graphicsPSOInit.BlendState = TStaticBlendState<CW_RGB>::GetRHI();
				break;
			case ETextureCopyModifier::TransparentAlphaPassthrough
			: graphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA>::GetRHI();
				break;
			case ETextureCopyModifier::PremultipliedAlphaBlend:
				// Because StereoLayerRender actually enables alpha blending as it composites the layers into the emulation texture
				// the color values for the emulation swapchain are PREMULTIPLIED ALPHA. That means we don't want to multiply alpha again!
				// So we can just do SourceColor * 1.0f + DestColor (1 - SourceAlpha)
				graphicsPSOInit.BlendState = TStaticBlendState<
					CW_RGBA, BO_Add, BF_One, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
				break;
			default:
				check(!"Unsupported copy modifier");
				graphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
				break;
		}

		graphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		graphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		graphicsPSOInit.PrimitiveType = PT_TriangleList;

		FGlobalShaderMap* pShaderMap = GetGlobalShaderMap(GetConfiguredShaderPlatform());;

		TShaderMapRef<FScreenVS> mapVertexShader(pShaderMap);

		TShaderRef<FGlobalShader> pixelShader;
		TShaderRef<FScreenPS> screenPS;

		bool isArraySource = srcTexture->GetDesc().IsTextureArray();

		if (LIKELY(!isArraySource))
		{
			TShaderMapRef<FScreenPS> screenPSRef(pShaderMap);
			screenPS = screenPSRef;
			pixelShader = screenPSRef;
		}
		else
		{
			TShaderMapRef<FScreenFromSlice0PS> screenPSRef(pShaderMap);
			screenPS = screenPSRef;
			pixelShader = screenPSRef;
		}

		graphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		graphicsPSOInit.BoundShaderState.VertexShaderRHI = mapVertexShader.GetVertexShader();
		graphicsPSOInit.BoundShaderState.PixelShaderRHI = pixelShader.GetPixelShader();

		SetGraphicsPipelineState(rhiCmdList, graphicsPSOInit, 0);

		rhiCmdList.Transition(FRHITransitionInfo(srcTexture, ERHIAccess::Unknown, ERHIAccess::SRVMask));

		const bool sameSize = dstRect.Size() == srcRect.Size();
		if (screenPS.IsValid())
		{
			if (screenPS.GetPixelShader()->IsValid())
			{
				if (srcTexture->GetNativeResource())
				{
					FRHISamplerState* pixelSampler = sameSize
						? TStaticSamplerState<SF_Point>::GetRHI()
						: TStaticSamplerState<SF_Bilinear>::GetRHI();
					SetShaderParametersLegacyPS(rhiCmdList, screenPS, pixelSampler, srcTexture);
				}
			}
		}

		m_rendererModule->DrawRectangle(
			rhiCmdList,
			0, 0,
			viewportWidth, viewportHeight,
			u, v,
			uSize, vSize,
			targetSize,
			FIntPoint(1, 1),
			mapVertexShader,
			EDRF_Default);
	}

	rhiCmdList.EndRenderPass();
	rhiCmdList.Transition(FRHITransitionInfo(dstTexture, ERHIAccess::RTV, finalDstAccess));
}

bool FStreamHMD::HDRGetMetaDataForStereo(EDisplayOutputFormat& outDisplayOutputFormat,
										 EDisplayColorGamut& outDisplayColorGamut, bool& outHDRSupported)
{
	if (m_renderBridge == nullptr)
	{
		return false;
	}

	return m_renderBridge->HDRGetMetaDataForStereo(outDisplayOutputFormat, outDisplayColorGamut, outHDRSupported);
}

void FStreamHMD::CopyTexture_RenderThread(FRHICommandListImmediate& rhiCmdList, FRHITexture* srcTexture,
										  FIntRect srcRect, FRHITexture* dstTexture, FIntRect dstRect, bool clearBlack,
										  bool noAlpha) const
{
	// This call only comes from the spectator screen so we expect alpha to be premultiplied.
	//const ETextureCopyModifier SrcTextureCopyModifier = noAlpha ? ETextureCopyModifier::Opaque : ETextureCopyModifier::PremultipliedAlphaBlend;
	// We always need the Alpha channel
	const ETextureCopyModifier srcTextureCopyModifier = ETextureCopyModifier::TransparentAlphaPassthrough;
	CopyTexture_RenderThread(rhiCmdList, srcTexture, srcRect, dstTexture, dstRect, clearBlack,
							 ERenderTargetActions::Load_Store, ERHIAccess::Present, srcTextureCopyModifier);
}

void FStreamHMD::RenderTexture_RenderThread(class FRHICommandListImmediate& rhiCmdList, class FRHITexture* backBuffer,
											class FRHITexture* srcTexture, FVector2D windowSize) const
{
	if (SpectatorScreenController)
	{
		const FTextureRHIRef layersTexture = nullptr;
		SpectatorScreenController->RenderSpectatorScreen_RenderThread(rhiCmdList, backBuffer, srcTexture, layersTexture,
																	  windowSize);
	}
}

IsarError FStreamHMD::CreateConnection(const std::string& applicationName,
									   const IsarGraphicsApiConfig& gfxConfig,
									   const RemotingConfig remotingConfig,
									   std::vector<IsarIceServerConfig> iceServerSettings,
									   IsarSignalingConfig signalingConfig,
									   IsarPortRange portRange,
									   IsarConnection* connection)
{
	IsarConfig config{};
	config.friendlyName = applicationName.c_str();
	config.renderConfig.encoderBitrateKbps = remotingConfig.encoderBitrateKbps;
	config.renderConfig.width = 2064;
	config.renderConfig.height = 2208;
	config.renderConfig.framerate = 90;
	config.renderConfig.numViews = 2;

	config.diagnosticOptions = remotingConfig.diagnosticOptions;
	config.numIceServers = iceServerSettings.size();
	config.iceServers = iceServerSettings.data();
	config.signalingConfig.suggestedIpv4 = signalingConfig.suggestedIpv4;
	config.signalingConfig.port = signalingConfig.port;
	config.deviceType = IsarDeviceType_PC;
	config.portRange.minPort = portRange.minPort;
	config.portRange.maxPort = portRange.maxPort;

	return m_serverApi.createConnection(&config, gfxConfig, connection);
}

bool FStreamHMD::StartAudio()
{
	FAudioDeviceHandle audioDevice = GEngine ? GEngine->GetActiveAudioDevice() : FAudioDeviceHandle();
	if (!audioDevice)
	{
		UE_LOG(LogHMD, Display, TEXT("Could not find Audio Device."));
		return false;
	}

	if (audioDevice->SampleRate != FStreamAudioListener::SAMPLE_RATE)
	{
		UE_LOG(LogHMD, Display, TEXT("Sample rate for Stream must be %d, can not start audio stream."),
			   FStreamAudioListener::SAMPLE_RATE);
		return false;
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
	audioDevice->RegisterSubmixBufferListener(m_audioListener->AsShared(), audioDevice->GetMainSubmixObject());
#else
	audioDevice->RegisterSubmixBufferListener(m_audioListener.Get());
#endif
	auto err = m_serverApi.setAudioTrackEnabled(m_streamConnection, true);
	if (err != IsarError::eNone)
	{
		UE_LOG(LogHMD, Display, TEXT("Could not enable Stream audio track."));
		return false;
	}

	m_audioEnabled = true;
	return true;
}

void FStreamHMD::StopAudio()
{
	if (!m_connectionCreated || !m_audioEnabled)
	{
		return;
	}

	m_serverApi.setAudioTrackEnabled(m_streamConnection, false);
	FAudioDeviceHandle audioDevice = GEngine ? GEngine->GetActiveAudioDevice() : FAudioDeviceHandle();
	if (audioDevice)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
		audioDevice->UnregisterSubmixBufferListener(m_audioListener->AsShared(), audioDevice->GetMainSubmixObject());
#else
		audioDevice->UnregisterSubmixBufferListener(m_audioListener.Get());
#endif
	}
	m_audioEnabled = false;
}

bool FStreamHMD::ToggleAudio(bool enable)
{
	if (!enable)
	{
		StopAudio();
		m_shouldEnableAudio = false;
		return true;
	}

	if (!m_connected)
	{
		m_shouldEnableAudio = true;
		return true;
	}

	return StartAudio();
}

void FStreamHMD::OnConnectionStateChanged(IsarConnectionState newState)
{
	FString typeString;
	FString codecString;
	switch (newState)
	{
		case IsarConnectionState_CONNECTED:
			m_serverApi.getConnectionInfo(m_streamConnection, &m_connectionInfo);
			if (m_connectionInfo.renderConfig.depthEnabled)
			{
				UE_LOG(LogHMD, Warning, TEXT("Depth not supported ....Resetting the connection"));
				if (!RestartConnection())
				{
					UE_LOG(LogHMD, Log, TEXT("Error: Failed to Reset Connection "));
				}
				return;
			}
			m_connected = true;
			
			if (m_width != (m_connectionInfo.renderConfig.width * m_connectionInfo.renderConfig.numViews) ||
				m_height != m_connectionInfo.renderConfig.height ||
				m_nViews != m_connectionInfo.renderConfig.numViews || 
				!m_needsReallocation )
			{
				UE_LOG(LogHMD, Log, TEXT("Reset Config Views"));
				GEngine->FixedFrameRate = m_connectionInfo.renderConfig.framerate;
				m_needsReallocation = true;
				m_pipelinedLayerStateRendering.colorImages.Empty();
				
				m_pipelinedFrameStateGame.viewConfigs.Empty();
				m_pipelinedFrameStateGame.views.Empty();
				

				EnumerateViews(m_pipelinedFrameStateGame);
				m_pipelinedFrameStateRendering.viewConfigs.Empty();
				m_pipelinedFrameStateRendering.views.Empty();
				
				EnumerateViews(m_pipelinedFrameStateRendering);
				m_pipelinedFrameStateRHI.viewConfigs.Empty();
				m_pipelinedFrameStateRHI.views.Empty();
				
				EnumerateViews(m_pipelinedFrameStateRHI);
			}
			UE_LOG(LogHMD, Display, TEXT("Stream Connection State: CONNECTED"));

			switch (m_connectionInfo.remoteDeviceType)
			{
				case IsarDeviceType_AR: typeString = "AR";
					break;
				case IsarDeviceType_VR: typeString = "VR";
					break;
				case IsarDeviceType_MR: typeString = "MR";
					break;
				case IsarDeviceType_PC: typeString = "PC";
					break;
				default: typeString = "Undefined";
					break;
			}

			switch (m_connectionInfo.codecInUse)
			{
				case IsarCodecType_H264: codecString = "H.264";
					break;
				case IsarCodecType_H265: codecString = "H.265";
					break;
				case IsarCodecType_AV1: codecString = "AV1";
					break;
				case IsarCodecType_H265_10Bit: codecString = "H.265 10-bit";
					break;
				case IsarCodecType_AV1_10Bit: codecString = "AV1 10-bit";
					break;
				default: codecString = "Undefined";
					break;
			}

			UE_LOG(LogHMD, Display, TEXT("Connection Info:\n"
					   "Name: %s\n"
					   "Version: %d.%d.%d\n"
					   "Type: %s\n\n"
					   "Network Configuration\n"
					   "Codec in Use: %s\n"
					   "Bandwidth: %d Kbps\n\n"
					   "Render Configuration\n"
					   "Resolution: %dx%d\n"
					   "Number of Views: %d\n"
					   "Frame Rate: %d FPS\n"
					   "Depth Buffer Enabled: %s"),
				   *FString(m_connectionInfo.remoteName),
				   ISAR_GET_VERSION_MAJOR(m_connectionInfo.remoteVersion),
				   ISAR_GET_VERSION_MINOR(m_connectionInfo.remoteVersion),
				   ISAR_GET_VERSION_PATCH(m_connectionInfo.remoteVersion),
				   *typeString,
				   *codecString,
				   m_connectionInfo.renderConfig.encoderBitrateKbps,
				   m_connectionInfo.renderConfig.width,
				   m_connectionInfo.renderConfig.height,
				   m_connectionInfo.renderConfig.numViews,
				   m_connectionInfo.renderConfig.framerate,
				   m_connectionInfo.renderConfig.depthEnabled ? *FString("True") : *FString("False"));

			if (m_shouldEnableAudio)
			{
				StartAudio();
			}
			break;
		case IsarConnectionState_CONNECTING: m_connected = false;
			UE_LOG(LogHMD, Display, TEXT("Stream Connection State: CONNECTING"));
			break;
		case IsarConnectionState_DISCONNECTED: m_connected = false;
			UE_LOG(LogHMD, Display, TEXT("Stream Connection State: DISCONNECTED"));
			break;
		case IsarConnectionState_CLOSING: m_connected = false;
			UE_LOG(LogHMD, Display, TEXT("Stream Connection State: CLOSING"));
			break;
		case IsarConnectionState_FAILED: m_connected = false;
			UE_LOG(LogHMD, Display, TEXT("Stream Connection State: FAILED"));
			break;
		default: m_connected = false;
			UE_LOG(LogHMD, Display, TEXT("Unknown State"));
			break;
	}

	// Actors can have functionalities that need to be done on the Game Thread
	AsyncTask(ENamedThreads::GameThread,
	          [newState, connectionStateHandlers = m_connectionStateHandlers]()
	          {
		          for (auto connectionStateHandler : connectionStateHandlers)
		          {
			          connectionStateHandler.GetInterface()->Execute_OnConnectionStateChanged(
			              connectionStateHandler.GetObject(), (EStreamConnectionState)newState);
		          }
	          });

	m_audioListener->SetConnected(m_connected);
}


bool FStreamHMD::RestartConnection() const
{
	bool connectStatus = true;
	UE_LOG(LogHMD, Log, TEXT("Close Connection"));
	auto err = m_serverApi.closeConnection(m_streamConnection);
	if (err != IsarError::eNone)
	{
		// Write error to output
		UE_LOG(LogHMD, Error, TEXT("Error in Close Connection, Status: %d"), err);
		connectStatus = false;
	}
	UE_LOG(LogHMD, Log, TEXT("Open Connection"));
	err = m_serverApi.openConnection(m_streamConnection);
	if (err != IsarError::eNone || !m_streamConnection)
	{
		// Write error to output
		UE_LOG(LogHMD, Error, TEXT("Error in Open Connection, Status: %d "), err);
		connectStatus = false;
	}
	return connectStatus;
}
void FStreamHMD::UpdateDeviceLocations()
{
	if (!m_connected)
	{
		return;
	}

	FPipelinedFrameState& pipelineState = GetPipelinedFrameStateForThread();
	m_serverApi.getConnectionInfo(m_streamConnection, &m_connectionInfo);

	IsarXrPose inputPose;
	auto err = m_serverApi.pullViewPose(m_streamConnection, &inputPose);
	if (err == IsarError::eNone && !pipelineState.views.IsEmpty())
	{
		pipelineState.poseTimestamp = inputPose.poseTimestamp;
		pipelineState.frameTimestamp = inputPose.frameTimestamp;

		IsarVector3 position = inputPose.poseLeft.position;
		if (m_connectionInfo.renderConfig.numViews == 1 && !pipelineState.views.IsEmpty())
		{
			pipelineState.views[0].pose.position.x = position.x;
			pipelineState.views[0].pose.position.y = position.y;
			pipelineState.views[0].pose.position.z = position.z;

			pipelineState.views[0].pose.orientation.x = inputPose.poseLeft.orientation.x;
			pipelineState.views[0].pose.orientation.y = inputPose.poseLeft.orientation.y;
			pipelineState.views[0].pose.orientation.z = inputPose.poseLeft.orientation.z;
			pipelineState.views[0].pose.orientation.w = inputPose.poseLeft.orientation.w;
			pipelineState.views[0].fov.angleDown = inputPose.fovLeft.down;
			pipelineState.views[0].fov.angleLeft = inputPose.fovLeft.left;
			pipelineState.views[0].fov.angleRight = inputPose.fovLeft.right;
			pipelineState.views[0].fov.angleUp = inputPose.fovLeft.up;
		}
		else
		{
			pipelineState.views[0].pose.position.x = position.x;
			pipelineState.views[0].pose.position.y = position.y;
			pipelineState.views[0].pose.position.z = position.z;

			pipelineState.views[0].pose.orientation.x = inputPose.poseLeft.orientation.x;
			pipelineState.views[0].pose.orientation.y = inputPose.poseLeft.orientation.y;
			pipelineState.views[0].pose.orientation.z = inputPose.poseLeft.orientation.z;
			pipelineState.views[0].pose.orientation.w = inputPose.poseLeft.orientation.w;
			pipelineState.views[0].fov.angleDown = inputPose.fovLeft.down;
			pipelineState.views[0].fov.angleLeft = inputPose.fovLeft.left;
			pipelineState.views[0].fov.angleRight = inputPose.fovLeft.right;
			pipelineState.views[0].fov.angleUp = inputPose.fovLeft.up;

			position = inputPose.poseRight.position;
			pipelineState.views[1].pose.position.x = inputPose.poseRight.position.x;
			pipelineState.views[1].pose.position.y = inputPose.poseRight.position.y;
			pipelineState.views[1].pose.position.z = inputPose.poseRight.position.z;

			pipelineState.views[1].pose.orientation.x = inputPose.poseRight.orientation.x;
			pipelineState.views[1].pose.orientation.y = inputPose.poseRight.orientation.y;
			pipelineState.views[1].pose.orientation.z = inputPose.poseRight.orientation.z;
			pipelineState.views[1].pose.orientation.w = inputPose.poseRight.orientation.w;
			pipelineState.views[1].fov.angleDown = inputPose.fovRight.down;
			pipelineState.views[1].fov.angleLeft = inputPose.fovRight.left;
			pipelineState.views[1].fov.angleRight = inputPose.fovRight.right;
			pipelineState.views[1].fov.angleUp = inputPose.fovRight.up;
		}
	}
}

void FStreamHMD::SetMicrophoneCaptureStream(IStreamExtension* streamMicrophone)
{
	if (!streamMicrophone)
	{
		m_microphoneCaptureStream = streamMicrophone;
		return;
	}

	// If the stream is already set, since the last one will be used, stop the earlier one
	// This happens because the engine constructs a static class separate from the component in the level
	if (m_microphoneCaptureStream)
	{
		m_microphoneCaptureStream->Stop();
	}

	m_microphoneCaptureStream = streamMicrophone;

	if (!m_connectionCreated)
	{
		return;
	}

	m_microphoneCaptureStream->SetStreamApi(m_streamConnection, &m_serverApi);
	m_microphoneCaptureStream->SetConnected(m_connected);
}

bool FStreamHMD::GetPassthrough()
{
	return m_serverApi.getPassthroughMode(m_streamConnection);
}

bool FStreamHMD::TrySetPassthrough(bool enable)
{
	return m_serverApi.trySetPassthroughMode(m_streamConnection, enable);
}

void FStreamHMD::RegisterConnectionStateHandler(TScriptInterface<IStreamConnectionStateHandler> connectionStateHandler)
{
	m_connectionStateHandlers.Add(connectionStateHandler);
}

void FStreamHMD::UnregisterConnectionStateHandler(TScriptInterface<IStreamConnectionStateHandler> connectionStateHandler)
{
	m_connectionStateHandlers.Remove(connectionStateHandler);
}

bool FStreamHMD::GetConnectionInfo(FStreamConnectionInfo& ConnectionInfo)
{
	if (!m_connected)
	{
		return false;
	}

	m_serverApi.getConnectionInfo(m_streamConnection, &m_connectionInfo);

	ConnectionInfo.RemoteName = FString(m_connectionInfo.remoteName);
	ConnectionInfo.RemoteVersion = FString::Printf(
	    TEXT("%d.%d.%d"), ISAR_GET_VERSION_MAJOR(m_connectionInfo.remoteVersion),
	    ISAR_GET_VERSION_MINOR(m_connectionInfo.remoteVersion), ISAR_GET_VERSION_PATCH(m_connectionInfo.remoteVersion));

	ConnectionInfo.RenderConfig = FStreamRenderConfig
	{
		.Width = (int32)m_connectionInfo.renderConfig.width,
	    .Height = (int32)m_connectionInfo.renderConfig.height,
	    .NumViews = (int32)m_connectionInfo.renderConfig.numViews,
	    .EncoderBitrateKbps = m_connectionInfo.renderConfig.encoderBitrateKbps,
	    .Framerate = (int32)m_connectionInfo.renderConfig.framerate,
	    .bDepthEnabled = m_connectionInfo.renderConfig.depthEnabled == 1,
	    .bPosePredictionEnabled = m_connectionInfo.renderConfig.posePredictionEnabled == 1
	};

	ConnectionInfo.RemoteDeviceType = (EStreamDeviceType)m_connectionInfo.remoteDeviceType;
	ConnectionInfo.CodecInUse = (EStreamCodecType)m_connectionInfo.codecInUse;

	return true;
}

FTextureRHIRef FStagingBufferPool::CreateStagingBuffer_RenderThread(FRHICommandListImmediate& rhiCmdList, int32 width,
																	int32 height, EPixelFormat format)
{
	auto stagingBufferPredicate =
		[width, height, format](const FTextureRHIRef& texture2DrhiRef)
	{
		return texture2DrhiRef->GetSizeX() == width && texture2DrhiRef->GetSizeY() == height && texture2DrhiRef->
			GetFormat() == format;
	};

	TArray<FTextureRHIRef> localPool;
	{
		FScopeLock lock(&m_poolLock);
		localPool = MoveTemp(m_pool);
	}

	// Find any pooled staging buffer with suitable properties.
	int32 index = localPool.IndexOfByPredicate(stagingBufferPredicate);

	if (index != -1)
	{
		FTextureRHIRef stagingBuffer = MoveTemp(localPool[index]);
		localPool.RemoveAtSwap(index);
		return stagingBuffer;
	}

	FRHITextureCreateDesc desc =
		FRHITextureCreateDesc::Create2D(TEXT("FStagingBufferPool_StagingBuffer"), width, height, format);

	return RHICreateTexture(desc);
}

void FStagingBufferPool::ReleaseStagingBufferForUnmap_AnyThread(FTextureRHIRef& texture2DRHIRef)
{
	FScopeLock lock(&m_poolLock);
	m_pool.Emplace(MoveTemp(texture2DRHIRef));
}

FStagingBufferPool::~FStagingBufferPool()
{
	FScopeLock lock(&m_poolLock);
	m_pool.Empty();
}

#undef LOCTEXT_NAMESPACE