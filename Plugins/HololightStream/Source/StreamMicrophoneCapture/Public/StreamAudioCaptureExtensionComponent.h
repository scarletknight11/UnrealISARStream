/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "StreamAudioCaptureExtensionComponent.generated.h"

class FStreamMicrophoneCaptureStream;

/// <summary>
/// Extension component to be used together with Audio Capture Component. The extension ensures that the Hololight
/// Stream microphone streaming is also stopped when the audio capture is stopped. To use it, add the component to the
/// same owner with the Audio Capture component and append Start/Stop calls to Start/Stop calls of the Audio Capture
/// component
/// </summary>
UCLASS(ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class STREAMMICROPHONECAPTURE_API UStreamAudioCaptureExtensionComponent : public USceneComponent
{
	GENERATED_BODY()

protected:
	UStreamAudioCaptureExtensionComponent(const FObjectInitializer& ObjectInitializer);

public:
	/// <summary>
	/// Starts the Stream Microphone Stream.
	/// </summary>
	UFUNCTION(BlueprintCallable, Category = "Hololight Stream")
	void Start();

	/// <summary>
	/// Stops the Stream Microphone Stream.
	/// </summary>
	UFUNCTION(BlueprintCallable, Category = "Hololight Stream")
	void Stop();

private:
	FStreamMicrophoneCaptureStream* m_captureStream;
};