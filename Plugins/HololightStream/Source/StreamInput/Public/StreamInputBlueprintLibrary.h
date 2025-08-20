/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "StreamControllerStateHandler.h"

#include "StreamInputBlueprintLibrary.generated.h"

UCLASS()
class STREAMINPUT_API UStreamInputBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hololight Stream|Input")
	static void RegisterControllerStateHandler(TScriptInterface<IStreamControllerStateHandler> ControllerStateHandler);

	UFUNCTION(BlueprintCallable, Category = "Hololight Stream|Input")
	static void UnregisterControllerStateHandler(TScriptInterface<IStreamControllerStateHandler> ControllerStateHandler);
};