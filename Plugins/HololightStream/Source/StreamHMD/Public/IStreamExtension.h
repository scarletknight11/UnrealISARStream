/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef HOLOLIGHT_UNREAL_ISTREAMEXTENSION_H
#define HOLOLIGHT_UNREAL_ISTREAMEXTENSION_H

namespace isar
{
typedef void* IsarConnection;
struct IsarServerApi;
}

class STREAMHMD_API IStreamExtension
{
public:
	virtual void SetStreamApi(isar::IsarConnection connection, isar::IsarServerApi* serverApi) = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual void SetConnected(bool connected) = 0;
};

#endif // HOLOLIGHT_UNREAL_ISTREAMEXTENSION_H