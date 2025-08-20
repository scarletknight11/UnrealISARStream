/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "StreamAudioCaptureExtensionComponent.h"
#include "StreamMicrophoneCaptureModule.h"

UStreamAudioCaptureExtensionComponent::UStreamAudioCaptureExtensionComponent(
	const FObjectInitializer& ObjectInitializer)
	: USceneComponent(ObjectInitializer),
	  m_captureStream(nullptr)
{
	// Early return for construction during Engine initialization, where there is no owner
	if (!GetOwner())
		return;

	// Only work if there is also an Audio Component attached
	if (!GetOwner()->FindComponentByClass<UAudioCaptureComponent>())
	{
		UE_LOG(LogHLSMicrophoneCapture, Warning,
			   TEXT(
				   "Stream Audio Capture Extension Component is attached to %s. It is meant to be used together with Audio Capture Component."
			   ), *GetOwner()->GetName());
		return;
	}

	TArray<Audio::IAudioCaptureFactory*> audioCaptureStreamFactories = IModularFeatures::Get().
		GetModularFeatureImplementations<Audio::IAudioCaptureFactory>(
			Audio::IAudioCaptureFactory::GetModularFeatureName());

	for (auto& factory : audioCaptureStreamFactories)
	{
		try
		{
			if (FStreamMicrophoneCaptureFactory* streamFactory = dynamic_cast<FStreamMicrophoneCaptureFactory*>(factory))
			{
				m_captureStream = streamFactory->GetLastCreatedStream();
				return;
			}
		}
		catch (...)
		{
		}
	}

	UE_LOG(LogHLSMicrophoneCapture, Warning, TEXT("Hololight Stream Microphone Stream could not be found."));
}

void UStreamAudioCaptureExtensionComponent::Start()
{
	if (m_captureStream)
	{
		m_captureStream->OpenTrack();
	}
}

void UStreamAudioCaptureExtensionComponent::Stop()
{
	if (m_captureStream)
	{
		m_captureStream->CloseTrack();
	}
}