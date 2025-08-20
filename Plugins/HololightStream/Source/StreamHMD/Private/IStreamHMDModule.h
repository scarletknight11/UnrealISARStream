/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_ISTREAMHMDMODULE_H
#define HOLOLIGHT_UNREAL_ISTREAMHMDMODULE_H

#include "CoreMinimal.h"
#include "IHeadMountedDisplayModule.h"
#include "Modules/ModuleManager.h"
#include "FStreamHMD.h"

static const FString STREAM_HMD_MODULE_NAME(TEXT("StreamHMD"));

/**
 * The public interface of the HL Streaming HMD module.
 */
class STREAMHMD_API IStreamHMDModule : public IHeadMountedDisplayModule
{
public:
	/**
	 * Singleton-like access to this module's interface.
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IStreamHMDModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IStreamHMDModule>(*STREAM_HMD_MODULE_NAME);
	}

	/**
	 * Checks to see if this module is loaded.
	 *
	 * @return True if the module is loaded.
	 */
	static inline bool IsAvailable() { return FModuleManager::Get().IsModuleLoaded(*STREAM_HMD_MODULE_NAME); }

	/**
	 * @brief Get the Stream HMD object
	 *
	 * @return FStreamHMD*
	 */
	virtual FStreamHMD* GetStreamHMD() const = 0;
};

#endif // HOLOLIGHT_UNREAL_ISTREAMHMDMODULE_H