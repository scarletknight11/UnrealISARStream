/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "StreamHMDModule.h"
#include "FStreamHMD.h"
#include "FStreamRenderBridge.h"

#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "IXRTrackingSystem.h"
#include "ShaderCore.h"

IMPLEMENT_MODULE(FStreamHMDModule, StreamHMD)

FStreamHMDModule::FStreamHMDModule() :
	m_renderBridge(nullptr)
{
}

/**
* IModuleInterface implementation
*/
void FStreamHMDModule::StartupModule()
{
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	FString shaderDirectory = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("HololightStream"))->GetBaseDir(),
											  TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/HololightStream"), shaderDirectory);
}

void FStreamHMDModule::ShutdownModule()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
}

/**
* End IModuleInterface implementation
*/

TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> FStreamHMDModule::CreateTrackingSystem()
{
	if (!m_renderBridge)
	{
		if (!InitRenderBridge())
		{
			return nullptr;
		}
	}
	return FSceneViewExtensions::NewExtension<FStreamHMD>(m_renderBridge);
}

FStreamHMD* FStreamHMDModule::GetStreamHMD() const
{
	static FName systemName(TEXT("StreamHMD"));
	if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == systemName))
	{
		return static_cast<FStreamHMD*>(GEngine->XRSystem.Get());
	}

	return nullptr;
}

bool FStreamHMDModule::InitRenderBridge()
{
	const ERHIInterfaceType rhiType = RHIGetInterfaceType();
	if (rhiType == ERHIInterfaceType::D3D11)
	{
		m_renderBridge = CreateRenderBridge_D3D11();
	}
	else if (rhiType == ERHIInterfaceType::D3D12)
	{
		m_renderBridge = CreateRenderBridge_D3D12();
	}

	if (!m_renderBridge)
	{
		FString rhiString = FApp::GetGraphicsRHI();
		UE_LOG(LogHMD, Warning, TEXT("%s is currently not supported by Hololight Stream"), *rhiString);
		return false;
	}
	return true;
}