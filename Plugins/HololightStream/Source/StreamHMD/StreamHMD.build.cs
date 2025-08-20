/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

using System.IO;
using UnrealBuildTool;

public class StreamHMD : ModuleRules
{
	public StreamHMD(ReadOnlyTargetRules Target) : base(Target)
	{
		
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		string DllDirectory = Path.Combine(PluginDirectory, "Binaries", "Win64", "isar.dll");
		string LibDirectory = Path.Combine(PluginDirectory, "Binaries", "Win64", "isar.lib");
		string ConfigDirectory = Path.Combine(PluginDirectory, "Resources", "remoting-config.cfg");
		PublicDependencyModuleNames.AddRange(
		new string[]
			{
				"HeadMountedDisplay",
				"XRBase"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"BuildSettings",
				"InputCore",
				"Json",
				"Projects",
				"RHI",
				"RenderCore",
				"Renderer",
				"AudioMixer",
				"AugmentedReality",
				"SignalProcessing",
				"HeadMountedDisplay",
				"XRBase",
				"RHICore"
			}
		);

		PrivateIncludePaths.Add(Path.Combine(PluginDirectory, "Source", "Include"));

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("EditorFramework");
			PrivateDependencyModuleNames.Add("UnrealEd");
			PrivateDependencyModuleNames.Add("Settings");
		}

		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Windows))
		{
			PrivateDependencyModuleNames.Add("D3D11RHI");
			PrivateDependencyModuleNames.Add("D3D12RHI");

			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11", "DX12");
		}

		PublicAdditionalLibraries.Add(LibDirectory);
		RuntimeDependencies.Add("$(ProjectDir)/Binaries/Win64/isar.dll", DllDirectory);
		RuntimeDependencies.Add("$(ProjectDir)/Config/remoting-config.cfg", ConfigDirectory);

		PublicDefinitions.Add("STREAM_HMD_EXPORTS=1");
		PublicDefinitions.Add("PLATFORM_WINDOWS=1");
		PublicDefinitions.Add("XR_USE_GRAPHICS_API_D3D11=1");
		PublicDefinitions.Add("XR_USE_GRAPHICS_API_D3D12=1");
		PublicDefinitions.Add("NOMINMAX");
		PublicDefinitions.Add("_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH");
		PublicDefinitions.Add("_ALLOW_RUNTIME_LIBRARY_MISMATCH");
	}
}
