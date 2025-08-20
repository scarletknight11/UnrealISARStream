/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "StreamMicrophoneCaptureModule.h"

TUniquePtr<Audio::IAudioCaptureStream> FStreamMicrophoneCaptureFactory::CreateNewAudioCaptureStream()
{
	m_lastCaptureStream = new FStreamMicrophoneCaptureStream();
	return TUniquePtr<Audio::IAudioCaptureStream>(m_lastCaptureStream);
}

FStreamMicrophoneCaptureStream* FStreamMicrophoneCaptureFactory::GetLastCreatedStream()
{
	return m_lastCaptureStream;
}

IMPLEMENT_MODULE(FStreamMicrophoneCaptureModule, StreamMicrophoneCapture)