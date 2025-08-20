/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_SIGNALING_API_H
#define ISAR_SIGNALING_API_H

#include "isar/isar_api.h"
#include "isar/types.h"

ISAR_CPP_NS_BEGIN

typedef void* IsarSignaling;

// Provider
// -----------------------------------------------------------------------------

/// @brief Implement this interface for your signaling provider.
/// @details Function pointers to be populated by your signaling provider to be called from within ISAR
typedef struct IsarSignalingProvider
{
  /// @brief User data which will be passed on each function call
  void* userData;

  /// @brief Call to start signaling with provided ip and port.
  /// @param suggestedIPv4 Suggested IP Version 4 address for signaling
  /// @param suggestedPort Suggested port for signaling
  /// @param userData User data
  void (*Start)(const char* suggestedIPv4, uint32_t suggestedPort, void* userData);

  /// @brief Call to stop signaling.
  /// @param userData User data
  void (*Stop)(void* userData);

  /// @brief Call to indicate changed connection state.
  /// @param userData User data
  void (*ConnectionChanged)(IsarConnectionState state, void* userData);

  /// @brief Call to send SDP information to other end.
  /// @param sdp SDP string
  /// @param userData User data
  void (*SendSdp)(const char* sdp, void* userData);

  /// @brief Call to send ICE candidate information to other end.
  /// @param id Id of ICE candidate
  /// @param lineIndex Line index of ICE candidate
  /// @param candidate String representation of ICE candidate
  /// @param userData User data
  void (*SendIceCandidate)(const char* id, int lineIndex, const char* candidate, void* userData);
} IsarSignalingProvider;

// Functions
// -----------------------------------------------------------------------------

/// @brief Register signaling provider that will be used internally in ISAR during WebRTC signaling.
/// @param connection           [in]:  ISAR connection pointer
/// @param provider             [in]:  Reference to a struct of functions which will be used by ISAR to interact provider
/// @param signaling            [out]: Isar signaling pointer. To be passed back into signaling functions.
/// @return eNone - Successfully registered signaling provider.    \n
///         eInvalidArgument - connection, provider or isarSignaling has not been instantiated.  \n
typedef IsarError (*IsarRegisterProvider)(IsarConnection connection, IsarSignalingProvider *const provider, OUT IsarSignaling* signaling);

/// @brief Set SDP information coming from remote end.
/// @param signaling            [in]: Isar signaling pointer.
/// @param sdpDesc              [in]: String representation of remote SDP.
/// @return eNone - Successfully set remote SDP.    \n
///         eInvalidArgument - isarSignaling or sdpDesc has not been instantiated.  \n
///         eSdpParse - Error occured during parsing SDP string.
///         eUnsupportedVersion - Unsupported server version.
typedef IsarError (*IsarSetRemoteSdp)(IsarSignaling signaling, const char* sdpDesc);

/// @brief Set ICE candidate information coming from remote end.
/// @param signaling           [in]: Isar signaling pointer.
/// @param id                      [in]: id of ICE candidate.
/// @param lineIndex               [in]: line index of ICE candidate.
/// @param iceCandidate            [in]: String representation of ICE candidate.
/// @return eNone - Successfully set remote ICE candidate.    \n
///         eInvalidArgument - isarSignaling, id or iceCandidate has not been instantiated.  \n
///         eSdpParse - Error occured during parsing SDP string.
typedef IsarError (*IsarSetRemoteIceCandidate)(IsarSignaling signaling, const char* id, int32_t lineIndex, const char* iceCandidate);

/// @brief Set signaling connection state.
/// @param signaling        [in]: Isar signaling pointer. To be passed back into signaling functions.
/// @param connected            [in]: Signaling connection state.
/// @return eNone - Successfully set signaling connection state.    \n
///         eInvalidArgument - connection has not been instantiated.  \n
typedef IsarError (*IsarSetConnectionState)(IsarSignaling signaling, bool connected);

// API
// -----------------------------------------------------------------------------

typedef struct IsarSignalingApi
{
  IsarRegisterProvider registerProvider;
  IsarSetRemoteSdp  setRemoteSdp;
  IsarSetRemoteIceCandidate setRemoteIceCandidate;
  IsarSetConnectionState setConnectionState;
} IsarSignalingApi;

/// @brief Instantiate the signaling api for managing external signaling provider.
/// @param api  [in/out]: The API function pointers to populate
/// @return eNone - Successfully populated the api.    \n
///         eInvalidArgument - api has not been instantiated.  \n
#pragma comment(linker, "/include:Isar_Signaling_CreateApi")
ISAR_API(IsarError) Isar_Signaling_CreateApi(IsarSignalingApi* api);

ISAR_CPP_NS_END

#endif //ISAR_SIGNALING_API_H
