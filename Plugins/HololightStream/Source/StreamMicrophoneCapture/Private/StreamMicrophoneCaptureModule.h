/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_STREAMMICROPHONECAPTUREMODULE_H
#define HOLOLIGHT_UNREAL_STREAMMICROPHONECAPTUREMODULE_H

#include "StreamMicrophoneCaptureCommon.h"

#include "Features/IModularFeatures.h"
#include "Modules/ModuleManager.h"

#include "FStreamMicrophoneCaptureStream.h"

class FStreamMicrophoneCaptureFactory : public Audio::IAudioCaptureFactory
{
public:
	// IAudioCaptureFactory
	TUniquePtr<Audio::IAudioCaptureStream> CreateNewAudioCaptureStream() override;

	FStreamMicrophoneCaptureStream* GetLastCreatedStream();

private:
	FStreamMicrophoneCaptureStream* m_lastCaptureStream;
};

class FStreamMicrophoneCaptureModule : public IModuleInterface
{
private:
	FStreamMicrophoneCaptureFactory m_streamMicrophoneCaptureFactory;

public:
	void StartupModule() override
	{
		IModularFeatures::Get().RegisterModularFeature(Audio::IAudioCaptureFactory::GetModularFeatureName(),
													   &m_streamMicrophoneCaptureFactory);
	}

	void ShutdownModule() override
	{
		IModularFeatures::Get().UnregisterModularFeature(Audio::IAudioCaptureFactory::GetModularFeatureName(),
														 &m_streamMicrophoneCaptureFactory);
	}
};

#endif // HOLOLIGHT_UNREAL_STREAMMICROPHONECAPTUREMODULE_H