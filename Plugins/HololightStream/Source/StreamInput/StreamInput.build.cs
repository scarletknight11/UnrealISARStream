/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

using System.IO;
using UnrealBuildTool;

public class StreamInput : ModuleRules
{
	public StreamInput(ReadOnlyTargetRules Target) : base(Target)
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
				"ApplicationCore",
				"Engine",
				"InputDevice",
				"InputCore",
				"HeadMountedDisplay",
				"XRBase",
				"StreamHMD"
			}
		);

		PublicDependencyModuleNames.Add("EnhancedInput");

		bEnableExceptions = true;

		PrivateIncludePaths.Add(Path.Combine(PluginDirectory, "Source", "Include"));

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("EditorFramework");
			PrivateDependencyModuleNames.Add("UnrealEd");
			PrivateDependencyModuleNames.Add("InputEditor");
			PrivateDependencyModuleNames.Add("SourceControl");
		}

		PublicAdditionalLibraries.Add(LibDirectory);
		RuntimeDependencies.Add("$(ProjectDir)/Binaries/Win64/isar.dll", DllDirectory);
		PublicDefinitions.Add("STREAM_INPUT_EXPORTS=1");
		PublicDefinitions.Add("PLATFORM_WINDOWS=1");
	}
}
