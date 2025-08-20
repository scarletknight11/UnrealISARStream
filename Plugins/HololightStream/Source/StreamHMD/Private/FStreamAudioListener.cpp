/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "FStreamAudioListener.h"

#include "Runtime/Launch/Resources/Version.h"

#include <algorithm>
#include <execution>

FStreamAudioListener::FStreamAudioListener()
	: m_connected(false),
	  m_streamConnection(nullptr),
	  m_serverApi(nullptr),
	  m_isRunning(true)
{
	m_pushThread = std::thread(&FStreamAudioListener::PushCarryBuffer, this);
}

FStreamAudioListener::~FStreamAudioListener()
{
	if (m_pushThread.joinable())
	{
		m_isRunning = false;
		// Conditional variable might be waiting while we are not pushing any data
		m_newDataCv.notify_all();
		m_pushThread.join();
	}
}

void FStreamAudioListener::SetStreamApi(isar::IsarConnection connection, isar::IsarServerApi* serverApi)
{
	m_streamConnection = connection;
	m_serverApi = serverApi;
}

void FStreamAudioListener::SetConnected(bool connected)
{
	m_connected = connected;
}

void FStreamAudioListener::OnNewSubmixBuffer(const USoundSubmix* owningSubmix, float* audioData, int32 numSamples,
											 int32 inNumChannels, const int32 inSampleRate, double audioClock)
{
	if (!m_connected)
	{
		return;
	}

	if (m_numChannels != inNumChannels)
	{
		if (inNumChannels > MAX_NUM_CHANNELS)
		{
			UE_LOG(LogHMD, Display, TEXT("Only mono or stereo audio is supported, will not stream audio."));
			return;
		}
		m_numChannels = inNumChannels;
	}

	TArray<int16_t> pcmData;
	pcmData.Reset(numSamples);
	pcmData.AddZeroed(numSamples);

	for (int i = 0; i < numSamples; i++)
	{
		int32 value = audioData[i] >= 0 ? audioData[i] * int32(MAX_int16) : audioData[i] * (int32(MAX_int16) + 1);
		pcmData[i] = static_cast<int16_t>(FMath::Clamp(value, int32(MIN_int16), int32(MAX_int16)));
	}

	m_carryBuffer.Append(reinterpret_cast<const int16_t*>(pcmData.GetData()), numSamples);
	m_newDataCv.notify_all();
}

void FStreamAudioListener::PushCarryBuffer()
{
	while (m_isRunning)
	{
		if (m_carryBuffer.Num() < BUFFER_SIZE * m_numChannels)
		{
			std::unique_lock lock(m_newDataMutex);
			m_newDataCv.wait(lock);
			continue;
		}

		isar::IsarAudioData audioData{
		    .data = (void*)m_carryBuffer.GetData(),
		    .bitsPerSample = BITS_PER_SAMPLE,
		    .sampleRate = SAMPLE_RATE,
		    .numberOfChannels = (size_t)m_numChannels,
		    .samplesPerChannel = BUFFER_SIZE
		};

		auto err = m_serverApi->pushAudioData(m_streamConnection, audioData);
		if (err != isar::IsarError::eNone) UE_LOG(LogTemp, Display, TEXT("Could not push audio data."));
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
		m_carryBuffer.RemoveAt(0, BUFFER_SIZE * m_numChannels, EAllowShrinking::No);
#else
		m_carryBuffer.RemoveAt(0, BUFFER_SIZE * m_numChannels, false);
#endif
	}
}