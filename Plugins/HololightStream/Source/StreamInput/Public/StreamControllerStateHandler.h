/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "HeadMountedDisplayTypes.h"
#include "IMotionController.h"
#include "UObject/Interface.h"

#include "StreamControllerStateHandler.generated.h"

USTRUCT(BlueprintType)
struct STREAMINPUT_API FStreamControllerStateInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hololight Stream|Input")
	FName ControllerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hololight Stream|Input")
	ETrackingStatus NewTrackingStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hololight Stream|Input")
	EXRVisualType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hololight Stream|Input")
	EControllerHand Hand;
};

UINTERFACE(BlueprintType)
class UStreamControllerStateHandler : public UInterface
{
	GENERATED_BODY()
};

class STREAMINPUT_API IStreamControllerStateHandler
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hololight Stream|Input")
	void OnControllerStateChanged(const FStreamControllerStateInfo& NewStateInfo);
};