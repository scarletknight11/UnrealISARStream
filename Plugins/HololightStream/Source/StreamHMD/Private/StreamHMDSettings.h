/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "StreamHMDSettings.generated.h"

UCLASS(config = Game, defaultconfig)
class STREAMHMD_API UStreamHMDSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category = "Misc",
	          meta = (DisplayName = "Signaling Port", ToolTip = "Port to be used for Signaling",
	                  ClampMin = 1024, ClampMax = 65535))
	int Port;

	UPROPERTY(config, EditAnywhere, Category = "Connection Port Range",
	          meta = (DisplayName = "Minimum Port", ToolTip = "Minimum port number for connection",
					  ClampMin = 1024, ClampMax = 65535))
	int MinPort;

	UPROPERTY(config, EditAnywhere, Category = "Connection Port Range",
	          meta = (DisplayName = "Maximum Port", ToolTip = "Maximum port number for connection",
					  ClampMin = 1024, ClampMax = 65535))
	int MaxPort;

	UPROPERTY(config, EditAnywhere, Category = "Misc",
	          meta = (DisplayName = "Encoder Bandwidth (Kbps)",
	                  ToolTip =
					  "Bandwidth used for encoding. Should be between 1-100000. Use -1 for bandwidth value set by the client.",
	                  ClampMin = -1, ClampMax = 100000))
	int EncoderBandwidth;

	UPROPERTY(config, EditAnywhere, Category = "Misc",
	          meta = (DisplayName = "Statistics Logging Enabled (Experimental, see tooltip.)",
					  ToolTip = "Log files are currently saved in the same directory as the executable: UnrealEditor.exe for the editor, and the packaged executable located in the Binaries/Win64 folder for builds. Due to potential write permission issues on some systems, this functionality is currently considered experimental."))
	bool bEnableStatsLogging;

	UStreamHMDSettings(const FObjectInitializer& ObjectInitializer);
	void SaveSettingsToConfig();
	bool LoadSettingsFromConfig();
#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};