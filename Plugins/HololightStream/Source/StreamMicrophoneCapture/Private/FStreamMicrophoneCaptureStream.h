/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_FSTREAMMICROPHONECAPTURESTREAM_H
#define HOLOLIGHT_UNREAL_FSTREAMMICROPHONECAPTURESTREAM_H

#include "StreamMicrophoneCaptureCommon.h"

#include "AudioCaptureDeviceInterface.h"
#include "Logging/LogMacros.h"

#include "IStreamExtension.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHLSMicrophoneCapture, Log, All);

namespace isar
{
struct IsarAudioData;
enum IsarConnectionState;
}


/// <summary>
/// Implements the Unreal Engine Audio Capture Stream interface, which is created by the Audio Capture Factory
/// implementations when a Audio Capture Component is created on the game. Audio Capture Component is responsible for
/// using the capture data of the stream and playing it into a audio source. This class is created by the factory
/// implementation: <see cref="FStreamMicrophoneCaptureFactory"/>
/// </summary>
class STREAMMICROPHONECAPTURE_API FStreamMicrophoneCaptureStream : public Audio::IAudioCaptureStream, public IStreamExtension
{
public:
	FStreamMicrophoneCaptureStream();
	~FStreamMicrophoneCaptureStream() override;

	// IStreamExtension
	void SetStreamApi(isar::IsarConnection connection, isar::IsarServerApi* serverApi) override;
	// Does not do anything, unused interface function
	void Start() override
	{
	}
	void Stop() override;
	void SetConnected(bool connected) override;

	// IAudioCaptureStream
	bool GetCaptureDeviceInfo(Audio::FCaptureDeviceInfo& outInfo, int32 deviceIndex) override;
	bool OpenAudioCaptureStream(const Audio::FAudioCaptureDeviceParams& inParams,
								Audio::FOnAudioCaptureFunction inOnCapture, uint32 numFramesDesired) override;
	bool CloseStream() override;
	bool StartStream() override;
	bool StopStream() override;
	bool AbortStream() override;
	bool GetStreamTime(double& outStreamTime) override;
	int32 GetSampleRate() const override { return SAMPLE_RATE; }
	bool IsStreamOpen() const override { return m_isStreamOpen; }
	bool IsCapturing() const override { return m_isCapturing; }
	void OnAudioCapture(void* inBuffer, uint32 inBufferFrames, double streamTime, bool overflow) override;
	bool GetInputDevicesAvailable(TArray<Audio::FCaptureDeviceInfo>& outDevices) override;

	bool OpenTrack();
	bool CloseTrack();

private:
	static constexpr int32 SAMPLE_RATE = 48000;
	static constexpr int32 NUM_CHANNELS = 1;
	static const FString DEVICE_NAME;
	static const FString DEVICE_ID;
	static constexpr bool SUPPORTS_HARDWARE_AEC = false;

	bool m_connected;
	isar::IsarConnection m_streamConnection;
	isar::IsarServerApi* m_serverApi;

	bool m_isStreamOpen;
	// Engine has checks for IsCapturing, where the wrong value crashes the Engine.
	// So m_isCapturing keeps the engine state, whereas m_isTrackOpen keeps the track state, which is necessary for us
	bool m_isCapturing;
	bool m_isTrackOpen;
	bool m_startMicrophoneOnConnection;

	double m_streamTime;

	// Audio callback which will be called during audio capture.
	Audio::FOnAudioCaptureFunction m_onCaptureCallback;

	void RegisterCallbacks();
	void UnregisterCallbacks();

	static void ConnectionStateChangedHandler(isar::IsarConnectionState newState, void* userData);
	static void MicrophoneCaptureHandler(isar::IsarAudioData const* audioData, void* userData);

	void OnConnectionStateChanged(isar::IsarConnectionState newState);
	void OnMicrophoneCapture(isar::IsarAudioData const* audioData);
};

#endif // HOLOLIGHT_UNREAL_FSTREAMMICROPHONECAPTURESTREAM_H