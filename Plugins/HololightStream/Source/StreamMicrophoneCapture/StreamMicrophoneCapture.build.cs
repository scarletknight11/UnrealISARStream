/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

using System.IO;
using UnrealBuildTool;

public class StreamMicrophoneCapture : ModuleRules
{
	public StreamMicrophoneCapture(ReadOnlyTargetRules Target) : base(Target)
	{
		
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		string DllDirectory = Path.Combine(PluginDirectory, "Binaries", "Win64", "isar.dll");
		string LibDirectory = Path.Combine(PluginDirectory, "Binaries", "Win64", "isar.lib");

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"AudioCaptureCore",
				"AudioCapture",
				"AudioMixer",
				"StreamHMD"
			}
		);

		bUseRTTI = true;
		bEnableExceptions = true;

		PrivateIncludePaths.Add(Path.Combine(PluginDirectory, "Source", "Include"));

		PublicAdditionalLibraries.Add(LibDirectory);
		RuntimeDependencies.Add("$(ProjectDir)/Binaries/Win64/isar.dll", DllDirectory);
		PublicDefinitions.Add("WITH_AUDIOCAPTURE=1");
		PublicDefinitions.Add("STREAM_MIC_EXPORTS=1");
		PublicDefinitions.Add("PLATFORM_WINDOWS=1");
	}
}
