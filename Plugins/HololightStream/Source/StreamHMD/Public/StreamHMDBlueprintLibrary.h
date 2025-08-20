/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "StreamConnectionStateHandler.h"

#include "StreamHMDBlueprintLibrary.generated.h"

USTRUCT(BlueprintType)
struct FStreamRenderConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	int32 Width = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	int32 Height = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	int32 NumViews = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	int32 EncoderBitrateKbps = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	int32 Framerate = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	bool bDepthEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	bool bPosePredictionEnabled = false;
};

UENUM(BlueprintType)
enum class EStreamDeviceType : uint8
{
	AR,
	VR,
	MR,
	PC
};

UENUM(BlueprintType)
enum class EStreamCodecType : uint8
{
	Auto,
	H264,
	H265,
	VP8,
	VP9,
	AV1,
	H265_10Bit,
	AV1_10Bit
};

USTRUCT(BlueprintType)
struct FStreamConnectionInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	FString RemoteName = "";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	FString RemoteVersion = "";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	FStreamRenderConfig RenderConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	EStreamDeviceType RemoteDeviceType = (EStreamDeviceType)(-1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hololight Stream")
	EStreamCodecType CodecInUse = EStreamCodecType::Auto;
};


UCLASS()
class STREAMHMD_API UStreamHMDBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hololight Stream")
	static bool IsAudioEnabled();

	UFUNCTION(BlueprintCallable, Category = "Hololight Stream")
	static bool ToggleAudio(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Hololight Stream")
	static bool GetPassthrough();

	UFUNCTION(BlueprintCallable, Category = "Hololight Stream")
	static bool TrySetPassthrough(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Hololight Stream")
	static void RegisterConnectionStateHandler(TScriptInterface<IStreamConnectionStateHandler> ConnectionStateHandler);

	UFUNCTION(BlueprintCallable, Category = "Hololight Stream")
	static void UnregisterConnectionStateHandler(TScriptInterface<IStreamConnectionStateHandler> ConnectionStateHandler);

	UFUNCTION(BlueprintCallable, Category = "Hololight Stream")
	static bool GetConnectionInfo(FStreamConnectionInfo& ConnectionInfo);
};