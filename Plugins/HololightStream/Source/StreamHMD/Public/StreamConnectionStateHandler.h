/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StreamConnectionStateHandler.generated.h"

UENUM(BlueprintType)
enum class EStreamConnectionState : uint8
{
	Disconnected,
	Connecting,
	Connected,
	Closing,
	Failed,
};

UINTERFACE(BlueprintType)
class UStreamConnectionStateHandler : public UInterface
{
	GENERATED_BODY()
};

class STREAMHMD_API IStreamConnectionStateHandler
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hololight Stream")
	void OnConnectionStateChanged(const EStreamConnectionState& NewState);
};