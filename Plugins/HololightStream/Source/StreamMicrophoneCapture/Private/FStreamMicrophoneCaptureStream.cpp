/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "FStreamMicrophoneCaptureStream.h"

#include "IStreamHMD.h"

#include <vector>

DEFINE_LOG_CATEGORY(LogHLSMicrophoneCapture);

const FString FStreamMicrophoneCaptureStream::DEVICE_NAME = FString("Hololight Stream Microphone");
const FString FStreamMicrophoneCaptureStream::DEVICE_ID = FString("HololightStreamMicrophone");

FStreamMicrophoneCaptureStream::FStreamMicrophoneCaptureStream() : m_connected(false),
																   m_streamConnection(nullptr),
																   m_serverApi(nullptr),
																   m_isStreamOpen(false),
																   m_isCapturing(false),
																   m_isTrackOpen(false),
																   m_startMicrophoneOnConnection(false),
																   m_streamTime(0.0),
																   m_onCaptureCallback(nullptr)
{
	if (auto* streamHMD = static_cast<IStreamHMD*>(GEngine->XRSystem.Get()))
	{
		streamHMD->SetMicrophoneCaptureStream(this);
	}
}

FStreamMicrophoneCaptureStream::~FStreamMicrophoneCaptureStream()
{
	if (IsCapturing())
	{
		StopStream();
	}
	if (IsStreamOpen())
	{
		CloseStream();
	}

	if (m_serverApi && m_streamConnection)
	{
		UnregisterCallbacks();
	}

	if (GEngine) // In case of destruction after the engine
	{
		if (auto* streamHMD = static_cast<IStreamHMD*>(GEngine->XRSystem.Get()))
		{
			streamHMD->SetMicrophoneCaptureStream(nullptr);
		}
	}
}

void FStreamMicrophoneCaptureStream::SetStreamApi(isar::IsarConnection connection, isar::IsarServerApi* serverApi)
{
	m_streamConnection = connection;
	m_serverApi = serverApi;

	if (!m_serverApi || !m_streamConnection)
	{
		UE_LOG(LogHLSMicrophoneCapture, Warning, TEXT("Hololight Stream connection is not initialized."));
		return;
	}

	RegisterCallbacks();
}

void FStreamMicrophoneCaptureStream::Stop()
{
	if (!m_streamConnection)
	{
		return;
	}

	UnregisterCallbacks();
	
	CloseTrack();

	m_streamConnection = nullptr;
}

void FStreamMicrophoneCaptureStream::SetConnected(bool connected)
{
	m_connected = connected;
}

bool FStreamMicrophoneCaptureStream::GetCaptureDeviceInfo(Audio::FCaptureDeviceInfo& outInfo, int32 deviceIndex)
{
	if (deviceIndex != Audio::DefaultDeviceIndex)
	{
		return false;
	}

	outInfo.DeviceName = DEVICE_NAME;
	outInfo.DeviceId = DEVICE_ID;
	outInfo.InputChannels = NUM_CHANNELS;
	outInfo.PreferredSampleRate = SAMPLE_RATE;
	outInfo.bSupportsHardwareAEC = SUPPORTS_HARDWARE_AEC;

	return true;
}

bool FStreamMicrophoneCaptureStream::OpenAudioCaptureStream(const Audio::FAudioCaptureDeviceParams& inParams,
															Audio::FOnAudioCaptureFunction inOnCapture,
															uint32 numFramesDesired)
{
	m_onCaptureCallback = MoveTemp(inOnCapture);
	m_isStreamOpen = true;
	return true;
}

bool FStreamMicrophoneCaptureStream::CloseStream()
{
	m_isStreamOpen = false;
	return true;
}

bool FStreamMicrophoneCaptureStream::StartStream()
{
	if (!OpenTrack())
		return false;
	m_isCapturing = true;
	return true;
}

bool FStreamMicrophoneCaptureStream::StopStream()
{
	if (!CloseTrack())
		return false;
	m_isCapturing = false;
	return true;
}

bool FStreamMicrophoneCaptureStream::AbortStream()
{
	return StopStream() && CloseStream();
}

bool FStreamMicrophoneCaptureStream::GetStreamTime(double& outStreamTime)
{
	outStreamTime = m_streamTime;
	return true;
}

void FStreamMicrophoneCaptureStream::OnAudioCapture(void* inBuffer, uint32 inBufferFrames, double streamTime,
													bool overflow)
{
	if (m_onCaptureCallback)
	{
		m_onCaptureCallback(inBuffer, inBufferFrames, NUM_CHANNELS, SAMPLE_RATE, streamTime, overflow);
	}
}

bool FStreamMicrophoneCaptureStream::GetInputDevicesAvailable(TArray<Audio::FCaptureDeviceInfo>& outDevices)
{
	Audio::FCaptureDeviceInfo captureDeviceInfo{};
	GetCaptureDeviceInfo(captureDeviceInfo, Audio::DefaultDeviceIndex);
	outDevices.Reset(0);
	outDevices.Add(captureDeviceInfo);

	return true;
}

bool FStreamMicrophoneCaptureStream::OpenTrack()
{
	if (m_isTrackOpen)
		return true;

	if (!m_connected)
	{
		m_startMicrophoneOnConnection = true;
		return true;
	}

	auto err = m_serverApi->setMicrophoneCaptureEnabled(m_streamConnection, true);
	if (err != isar::IsarError::eNone)
	{
		UE_LOG(LogHLSMicrophoneCapture, Display, TEXT("Failed to enable the microphone with error: %d"), err);
		return false;
	}
	m_isTrackOpen = true;

	return true;
}

void FStreamMicrophoneCaptureStream::RegisterCallbacks()
{
	m_serverApi->registerConnectionStateHandler(m_streamConnection, ConnectionStateChangedHandler, this);
	m_serverApi->registerMicrophoneCaptureHandler(m_streamConnection, MicrophoneCaptureHandler, this);
}

void FStreamMicrophoneCaptureStream::UnregisterCallbacks()
{
	m_serverApi->unregisterConnectionStateHandler(m_streamConnection, ConnectionStateChangedHandler, this);
	m_serverApi->unregisterMicrophoneCaptureHandler(m_streamConnection, MicrophoneCaptureHandler, this);
}

bool FStreamMicrophoneCaptureStream::CloseTrack()
{
	if (!m_isTrackOpen)
		return true;

	auto err = m_serverApi->setMicrophoneCaptureEnabled(m_streamConnection, false);
	if (err != isar::IsarError::eNone)
	{
		UE_LOG(LogHLSMicrophoneCapture, Display, TEXT("Failed to disable the microphone with error: %d"), err);
		return false;
	}
	m_isTrackOpen = false;
	m_startMicrophoneOnConnection = false;

	return true;
}

void FStreamMicrophoneCaptureStream::ConnectionStateChangedHandler(isar::IsarConnectionState newState, void* userData)
{
	reinterpret_cast<FStreamMicrophoneCaptureStream*>(userData)->OnConnectionStateChanged(newState);
}

void FStreamMicrophoneCaptureStream::MicrophoneCaptureHandler(isar::IsarAudioData const* audioData, void* userData)
{
	reinterpret_cast<FStreamMicrophoneCaptureStream*>(userData)->OnMicrophoneCapture(audioData);
}

void FStreamMicrophoneCaptureStream::OnConnectionStateChanged(isar::IsarConnectionState newState)
{
	if (newState == isar::IsarConnectionState_CONNECTED)
	{
		m_connected = true;
		if (!m_isTrackOpen && m_startMicrophoneOnConnection)
		{
			auto err = m_serverApi->setMicrophoneCaptureEnabled(m_streamConnection, true);
			if (err != isar::IsarError::eNone)
			{
				UE_LOG(LogHLSMicrophoneCapture, Display,
					   TEXT("Failed to enable the microphone when connected with error: %d"), err);
				return;
			}
			m_isTrackOpen = true;
		}
	}
	else
	{
		m_connected = false;
		if (newState == isar::IsarConnectionState_DISCONNECTED || newState == isar::IsarConnectionState_CLOSING)
		{
			if (m_isTrackOpen)
			{
				m_startMicrophoneOnConnection = true;
			}
			m_isTrackOpen = false;
		}
	}
}

void FStreamMicrophoneCaptureStream::OnMicrophoneCapture(isar::IsarAudioData const* audioData)
{
	// Check for the case where the class is deconstructed but the callback is called one last time
	if (!m_isTrackOpen)
		return;

	std::vector<float> data(audioData->samplesPerChannel * audioData->numberOfChannels);
	for (int i = 0; i < data.size(); i++)
	{
		data[i] = reinterpret_cast<const int16_t*>(audioData->data)[i] / (float)MAX_int16;
	}

	m_streamTime += 0.01;
	OnAudioCapture(data.data(), audioData->samplesPerChannel, m_streamTime, false);
}