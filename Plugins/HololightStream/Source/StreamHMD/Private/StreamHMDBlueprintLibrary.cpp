/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "StreamHMDBlueprintLibrary.h"

#include "FStreamHMD.h"

inline static FStreamHMD* GetStreamHMD()
{
	if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == STREAM_HMD_SYSTEM_NAME))
	{
		return static_cast<FStreamHMD*>(GEngine->XRSystem.Get());
	}

	return nullptr;
}

bool UStreamHMDBlueprintLibrary::IsAudioEnabled()
{
	if (auto* streamHMD = GetStreamHMD())
	{
		return streamHMD->IsAudioEnabled();
	}

	return false;
}

bool UStreamHMDBlueprintLibrary::ToggleAudio(bool enable)
{
	if (auto* streamHMD = GetStreamHMD())
	{
		return streamHMD->ToggleAudio(enable);
	}

	return false;
}

bool UStreamHMDBlueprintLibrary::GetPassthrough()
{
	if (auto* streamHMD = GetStreamHMD())
	{
		return streamHMD->GetPassthrough();
	}

	return false;
}

bool UStreamHMDBlueprintLibrary::TrySetPassthrough(bool enable)
{
	if (auto* streamHMD = GetStreamHMD())
	{
		return streamHMD->TrySetPassthrough(enable);
	}

	return false;
}

void UStreamHMDBlueprintLibrary::RegisterConnectionStateHandler(
    TScriptInterface<IStreamConnectionStateHandler> connectionStateHandler)
{
	if (auto* streamHMD = GetStreamHMD())
	{
		streamHMD->RegisterConnectionStateHandler(connectionStateHandler);
	}
}

void UStreamHMDBlueprintLibrary::UnregisterConnectionStateHandler(
    TScriptInterface<IStreamConnectionStateHandler> connectionStateHandler)
{
	if (auto* streamHMD = GetStreamHMD())
	{
		streamHMD->UnregisterConnectionStateHandler(connectionStateHandler);
	}
}

bool UStreamHMDBlueprintLibrary::GetConnectionInfo(FStreamConnectionInfo& ConnectionInfo)
{
	if (auto* streamHMD = GetStreamHMD())
	{
		return streamHMD->GetConnectionInfo(ConnectionInfo);
	}

	return false;
}