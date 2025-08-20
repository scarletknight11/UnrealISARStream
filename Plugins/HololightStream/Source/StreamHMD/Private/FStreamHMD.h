/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_FSTREAMHMD_H
#define HOLOLIGHT_UNREAL_FSTREAMHMD_H

#include "StreamHMDCommon.h"

#include "SceneViewExtension.h"
#include "XRTrackingSystemBase.h"
#include "XRRenderTargetManager.h"
#include "Runtime/Launch/Resources/Version.h"

//Stream Headers
#include "IStreamHMD.h"
#include "FStreamRenderBridge.h"
#include "FStreamAudioListener.h"
#include "StreamHMDBlueprintLibrary.h"
#include "StreamConnectionStateHandler.h"

// std Library includes
#include <functional>

// Forward declaration
class FSceneView;
class FSceneViewFamily;
struct ID3D12DynamicRHI;
struct ID3D11DynamicRHI;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12Device;
struct ID3D11Device;

#ifndef IID_GRAPHICS_PPV_ARGS
#define IID_GRAPHICS_PPV_ARGS(x) IID_PPV_ARGS(x)
#endif

namespace isar
{
typedef void* IsarConnection;

typedef struct RemotingConfig
{
	IsarDiagnosticOptions diagnosticOptions;
	int32_t encoderBitrateKbps;
} RemotingConfig;
} // namespace isar

const FName STREAM_HMD_SYSTEM_NAME("StreamHMD");

using namespace isar;

class FStagingBufferPool
{
public:
	FTextureRHIRef CreateStagingBuffer_RenderThread(FRHICommandListImmediate& rhiCmdList, int32 width, int32 height,
													EPixelFormat format);
	void ReleaseStagingBufferForUnmap_AnyThread(FTextureRHIRef& texture2DRHIRef);
	~FStagingBufferPool();

private:
	FCriticalSection m_poolLock;
	TArray<FTextureRHIRef> m_pool;
};

class STREAMHMD_API FStreamHMD : public IStreamHMD
								 , public FHMDSceneViewExtension
								 , public FXRRenderTargetManager
{
public:
	struct FPipelinedFrameState
	{
		TArray<XrView> views;
		TArray<XrViewConfigurationView> viewConfigs;
		float worldToMetersScale = 100.0f;
		float pixelDensity = 1.0f;
		int64_t poseTimestamp = 0;
		int64_t frameTimestamp = 0;
	};

	struct FPipelinedLayerState
	{
		TArray<XrSwapchainSubImage> colorImages;
		FXRSwapChainPtr colorSwapchain;
		TArray<XrCompositionLayerProjectionView> projectionLayers;
	};

	/** IXRTrackingSystem interface */
	FName GetSystemName() const override
	{
		return STREAM_HMD_SYSTEM_NAME;
	}

	FName GetHMDName() const override;

	int32 GetXRSystemFlags() const override
	{
		return EXRSystemFlags::IsHeadMounted | EXRSystemFlags::SupportsHandTracking;
	}

	bool EnumerateTrackedDevices(TArray<int32>& outDevices,
								 EXRTrackedDeviceType type = EXRTrackedDeviceType::Any) override;
	class IHeadMountedDisplay* GetHMDDevice() override { return this; }

	class TSharedPtr<class IStereoRendering, ESPMode::ThreadSafe> GetStereoRenderingDevice() override
	{
		return SharedThis(this);
	}

	/** IStreamHMD interface */
	void SetInputModule(IStreamExtension* streamInput) override { m_inputModule = streamInput; }
	void SetMicrophoneCaptureStream(IStreamExtension* streamMicrophone) override;
	void SetDeviceInfoCallback(const std::function<DeviceInfo(EControllerHand)>& functionPtr) override
	{
		m_getDeviceInfoCallback = functionPtr;
	};

	float GetWorldToMetersScale() const override;

	/** IHeadMountedDisplay interface */
	bool IsHMDConnected() override { return true; }
	void OnBeginRendering_RenderThread(FRHICommandListImmediate& rhiCmdList, FSceneViewFamily& viewFamily) override;
	void PostRenderViewFamily_RenderThread(FRDGBuilder& graphBuilder, FSceneViewFamily& inViewFamily) override;
	void PostRenderView_RenderThread(FRDGBuilder& graphBuilder, FSceneView& inView) override;

	/** Constructor */
	FStreamHMD(const FAutoRegister&, TRefCountPtr<FStreamRenderBridge>& inRenderBridge);
	/** Destructor */
	~FStreamHMD() ;

	void OnBeginRendering_RHIThread(const FPipelinedFrameState& inFrameState, FXRSwapChainPtr swapchainPtr);
	void OnFinishRendering_RHIThread();

	/** IXRTrackingSystem */
	void OnBeginPlay(FWorldContext& inWorldContext) override;
	void OnEndPlay(FWorldContext& inWorldContext) override;
	bool GetPlayAreaRect(FTransform& outTransform, FVector2D& outExtent) const override;

	/** ISceneViewExtension interface */
	/** IStereoRenderTargetManager */
	bool ShouldUseSeparateRenderTarget() const override
	{
		return IsStereoEnabled() && m_renderBridge.IsValid();
	}

	void CalculateRenderTargetSize(const FViewport& viewport, uint32& inOutSizeX, uint32& inOutSizeY) override;
	void SetupView(FSceneViewFamily& inViewFamily, FSceneView& inView) override;
	void BeginRenderViewFamily(FSceneViewFamily& inViewFamily) override;
	void SetupViewFamily(FSceneViewFamily& inViewFamily) override;
	bool ReconfigureForShaderPlatform(EShaderPlatform newShaderPlatform) override;
	int32 AcquireColorTexture() override final;
	void PreRenderView_RenderThread(FRDGBuilder& graphBuilder, FSceneView& inView) override;
	void PreRenderViewFamily_RenderThread(FRDGBuilder& graphBuilder, FSceneViewFamily& inViewFamily) override;
	bool GetCurrentPose(int32 deviceId, FQuat& currentOrientation, FVector& currentPosition) override;
	void SetBaseRotation(const FRotator& baseRotation) override;
	void SetBaseOrientation(const FQuat& baseOrientation) override;
	void ResetOrientationAndPosition(float yaw = 0.f) override;
	bool IsHMDEnabled() const override;
	void OnLateUpdateApplied_RenderThread(FRHICommandListImmediate& rhiCmdList,
										  const FTransform& newRelativeTransform) override;
	FIntRect GetFullFlatEyeRect_RenderThread(FTextureRHIRef eyeTexture) const override;
	void GetEyeRenderParams_RenderThread(const struct FHeadMountedDisplayPassContext& context,
										 FVector2D& eyeToSrcUVScaleValue,
										 FVector2D& eyeToSrcUVOffsetValue) const override;
	EStereoscopicPass GetViewPassForIndex(bool stereoRequested, int32 viewIndex) const override;
	uint32 GetLODViewIndex() const override;
	bool IsStandaloneStereoOnlyDevice() const override { return false; }
	void EnableHMD(bool enable = true) override;
	IStereoRenderTargetManager* GetRenderTargetManager() override;
	bool DoesSupportLateProjectionUpdate() const override { return true; }
	EHMDWornState::Type GetHMDWornState() override { return EHMDWornState::Worn; }
	bool OnEndGameFrame(FWorldContext& worldContext) override;
	bool GetHMDMonitorInfo(MonitorInfo& monitorInfo) override;
	bool GetRelativeEyePose(int32 inDeviceId, int32 inViewIndex, FQuat& outOrientation,
							FVector& outPosition) override;
	void SetTrackingOrigin(EHMDTrackingOrigin::Type newOriginType) override;
	EHMDTrackingOrigin::Type GetTrackingOrigin() const override;
	bool OnStartGameFrame(FWorldContext& worldContext) override;
	void GetFieldOfView(float& outHFOVInDegrees, float& outVFOVInDegrees) const override;
	void SetInterpupillaryDistance(float newInterpupillaryDistance) override;
	float GetInterpupillaryDistance() const override;
	bool IsChromaAbCorrectionEnabled() const override;
	bool IsStereoEnabled() const override;
	bool EnableStereo(bool stereo = true) override;
	void AdjustViewRect(int32 viewIndex, int32& x, int32& y, uint32& sizeX, uint32& sizeY) const override;
	int32 GetDesiredNumberOfViews(bool stereoRequested) const override;
	void SetFinalViewRect(FRHICommandListImmediate& rhiCmdList, const int32 stereoViewIndex,
						  const FIntRect& finalViewRect) override;
	FMatrix GetStereoProjectionMatrix(const int32 viewIndex) const override;
	bool HDRGetMetaDataForStereo(EDisplayOutputFormat& outDisplayOutputFormat,
								 EDisplayColorGamut& outDisplayColorGamut, bool& outHDRSupported) override;
	bool AllocateRenderTargetTextures(uint32 sizeX, uint32 sizeY, uint8 format, uint32 numLayers,
									  ETextureCreateFlags flags, ETextureCreateFlags targetableTextureFlags,
									  TArray<FTextureRHIRef>& outTargetableTextures,
									  TArray<FTextureRHIRef>& outShaderResourceTextures,
									  uint32 numSamples = 1) override;

	/** FHMDSceneViewExtension interface */
	bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& context) const override;

	/** FXRRenderTargetManager */
	FXRRenderBridge* GetActiveRenderBridge_GameThread(bool useSeparateRenderTarget) override;
	void SetPixelDensity(const float newDensity) override;
	void GetMotionControllerData(UObject* worldContext, const EControllerHand hand,
								 FXRMotionControllerData& motionControllerData) override;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	void GetMotionControllerState(UObject* worldContext, const EXRSpaceType xrSpaceType,
                                  const EControllerHand hand, const EXRControllerPoseType xrControllerPoseType,
                                  FXRMotionControllerState& motionControllerState) override;
	void GetHandTrackingState(UObject* worldContext, const EXRSpaceType xrSpaceType, const EControllerHand hand,
                              FXRHandTrackingState& handTrackingState) override;
#endif
	FIntPoint GetIdealRenderTargetSize() const override;
	bool DoesSupportPositionalTracking() const override { return true; }
	void RenderTexture_RenderThread(class FRHICommandListImmediate& rhiCmdList, class FRHITexture* backBuffer,
									class FRHITexture* srcTexture, FVector2D windowSize) const override;
	void CopyTexture_RenderThread(FRHICommandListImmediate& rhiCmdList, FRHITexture* srcTexture,
								  FIntRect srcRect, FRHITexture* dstTexture, FIntRect dstRect, bool clearBlack,
								  bool noAlpha) const override;
	/** Returns shader platform the plugin is currently configured for, in the editor it can change due to preview platforms. */
	EShaderPlatform GetConfiguredShaderPlatform() const
	{
		check(m_configuredShaderPlatform != EShaderPlatform::SP_NumPlatforms);
		return m_configuredShaderPlatform;
	}

	bool NeedReAllocateViewportRenderTarget(const FViewport& viewport) override { return m_needsReallocation; }

	// Exposed from Blueprints
	bool GetPassthrough();
	bool TrySetPassthrough(bool enable);
	bool ToggleAudio(bool enable);
	bool IsAudioEnabled() { return m_audioEnabled; }
	void RegisterConnectionStateHandler(TScriptInterface<IStreamConnectionStateHandler> connectionStateHandler);
	void UnregisterConnectionStateHandler(TScriptInterface<IStreamConnectionStateHandler> connectionStateHandler);
	bool GetConnectionInfo(FStreamConnectionInfo& ConnectionInfo);

private:
	FStagingBufferPool m_stagingBufferPool;
	FQuat m_baseOrientation;
	FVector m_basePosition;
	float m_worldToMeters = 100.0f;
	float m_runtimePixelDensityMax = FHeadMountedDisplayBase::PixelDensityMax;
	TRefCountPtr<FStreamRenderBridge> m_renderBridge;
	IRendererModule* m_rendererModule;
	ID3D12CommandQueue* m_pD3D12CommandQueue;
	ID3D12Fence* m_pD3D12Fence;
	ID3D12Device* m_pD3D12Device;
	ID3D11Device* m_pD3D11Device;
	FRWLock m_frameHandleMutex;
	FPipelinedFrameState m_pipelinedFrameStateRendering;
	FPipelinedFrameState m_pipelinedFrameStateGame;
	FPipelinedFrameState m_pipelinedFrameStateRHI;
	bool m_isMobileMultiViewEnabled;
	IsarConnection m_streamConnection;
	IsarServerApi m_serverApi;
	IsarGraphicsApiType m_gfxApiType;
	int m_deviceType;
	bool m_stereoEnabled;
	FXRSwapChainPtr m_streamSwapchain;
	int m_width;
	int m_height;
	int m_nViews;
	FPipelinedLayerState m_pipelinedLayerStateRendering;
	EShaderPlatform m_configuredShaderPlatform = EShaderPlatform::SP_NumPlatforms;
	bool m_connected = false;
	isar::IsarConnectionInfo m_connectionInfo;
	IStreamExtension* m_inputModule;
	TSharedPtr<FStreamAudioListener, ESPMode::ThreadSafe> m_audioListener;
	IStreamExtension* m_microphoneCaptureStream = nullptr;
	FString m_streamIp;
	FString m_streamURL;
	FString m_userName;
	FString m_credential;
	int32 m_streamPort;
	int32 m_encoderBandwidth;
	int32 m_minPort;
	int32 m_maxPort;
	IsarDiagnosticOptions m_diagnosticOptions;
	bool m_connectionCreated;
	bool m_needsReallocation;

	bool m_shouldEnableAudio = false;
	bool m_audioEnabled = false;

	TArray<TScriptInterface<IStreamConnectionStateHandler>> m_connectionStateHandlers;

	std::function<DeviceInfo(EControllerHand)> m_getDeviceInfoCallback;

	bool InitConnectionConfig(std::vector<IsarIceServerConfig>& serverConfigArray);
	bool RestartConnection() const;
	IsarError CreateConnection(const std::string& applicationName,
							   const IsarGraphicsApiConfig& gfxConfig,
							   const RemotingConfig remotingConfig,
							   std::vector<IsarIceServerConfig> iceServerSettings,
							   IsarSignalingConfig signalingConfig,
							   IsarPortRange portRange,
							   IsarConnection* connection);
	void OnConnectionStateChanged(IsarConnectionState newState);
	void UpdateDeviceLocations();
	void GetPositionRotation(const XrVector3f& position, const XrQuaternionf& orientation, FVector& outPosition,
							 FQuat& outOrientation);
	bool OnStereoStartup();
	void EnumerateViews(FPipelinedFrameState& pipelineState);
	const FPipelinedFrameState& GetPipelinedFrameStateForThread() const;
	FPipelinedFrameState& GetPipelinedFrameStateForThread();
	enum ETextureCopyModifier : uint8;
	void CopyTexture_RenderThread(FRHICommandListImmediate& rhiCmdList, FRHITexture* srcTexture, FIntRect srcRect,
								  FRHITexture* dstTexture, FIntRect dstRect,
								  bool clearBlack, ERenderTargetActions rtAction, ERHIAccess finalDstAccess,
								  ETextureCopyModifier srcTextureCopyModifier) const;
	bool StartAudio();
	void StopAudio();
};

#endif // HOLOLIGHT_UNREAL_FSTREAMHMD_H