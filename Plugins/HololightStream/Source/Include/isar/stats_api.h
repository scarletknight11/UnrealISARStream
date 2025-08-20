/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_STATS_API_H
#define ISAR_STATS_API_H

#include "isar/isar_api.h"

#include <stdint.h>

ISAR_CPP_NS_BEGIN

#pragma comment(linker, "/include:Isar_Stats_GetStatsList")
/// @brief Get the list of stats and stat types from the generated report
/// @param report [in]: The report to get the stats from
/// @param length [out]: The number of stats in the report
/// @param types [out]: The types of the stats in the array
/// @return An array of stats
ISAR_API(const void**) Isar_Stats_GetStatsList(const void* report, size_t* length, uint32_t** types);

#pragma comment(linker, "/include:Isar_Stats_GetJson")
/// @brief Get the stat as a json formated string
/// @param stats [in]: The stat to convert to json
/// @return A null terminated string in json format
ISAR_API(const char*) Isar_Stats_GetJson(const void* stats);

#pragma comment(linker, "/include:Isar_Stats_GetId")
/// @brief Get the ID of the stats
/// @param stats [in]: The stats to get the ID for
/// @return A null terminated string of the ID
ISAR_API(const char*) Isar_Stats_GetId(const void* stats);

#pragma comment(linker, "/include:Isar_Stats_GetType")
/// @brief Get the type of the specified stats
/// @param stats [in]: The stats to get the type for
/// @return The type of the stats
ISAR_API(uint32_t) Isar_Stats_GetType(const void* stats);

#pragma comment(linker, "/include:Isar_Stats_GetTimestamp")
/// @brief Get the timestamp of the generated stats
/// @param stats [in]: The stats to get the timestamp for
/// @return The timestamp for the generated stats
ISAR_API(int64_t) Isar_Stats_GetTimestamp(const void* stats);

#pragma comment(linker, "/include:Isar_Stats_GetMembers")
/// @brief Get an array of the members
/// @param stats [in]: The stats to get the members of
/// @param length [out]: The length of the memebers array
/// @return The members array
ISAR_API(void*) Isar_Stats_GetMembers(const void* stats, size_t* length);

#pragma comment(linker, "/include:Isar_Stats_GetMemberName")
/// @brief Get the name of a specified member
/// @param member [in]: The member to get the name for
/// @return A null terminated string with the member name
ISAR_API(const char*) Isar_Stats_GetMemberName(const void* member);

#pragma comment(linker, "/include:Isar_Stats_GetMemberType")
/// @brief Get the type of the specified member
/// @param member [in]: The member to get the type from
/// @return The type of the member
ISAR_API(uint32_t) Isar_Stats_GetMemberType(const void* member);

#pragma comment(linker, "/include:Isar_Stats_IsMemberDefined")
/// @brief Query whether the member is defined
/// @param member [in]: The member to query
/// @return True if defined, false if not
ISAR_API(bool) Isar_Stats_IsMemberDefined(const void* member);

#pragma comment(linker, "/include:Isar_Stats_MemberGetBool")
/// @brief If the member is type bool, get the value
/// @param member [in]: The member to get the value for
/// @return The value
ISAR_API(bool) Isar_Stats_MemberGetBool(const void* member);

#pragma comment(linker, "/include:Isar_Stats_MemberGetInt")
/// @brief If the member is type int, get the value
/// @param member [in]: The member to get the value for
/// @return The value
ISAR_API(int32_t) Isar_Stats_MemberGetInt(const void* member);

#pragma comment(linker, "/include:Isar_Stats_MemberGetUint")
/// @brief If the member is type uint, get the value
/// @param member [in]: The member to get the value for
/// @return The value
ISAR_API(uint32_t) Isar_Stats_MemberGetUint(const void* member);

#pragma comment(linker, "/include:Isar_Stats_MemberGetLong")
/// @brief If the member is type long, get the value
/// @param member [in]: The member to get the value for
/// @return The value
ISAR_API(int64_t) Isar_Stats_MemberGetLong(const void* member);

#pragma comment(linker, "/include:Isar_Stats_MemberGetUlong")
/// @brief If the member is type ulong, get the value
/// @param member [in]: The member to get the value for
/// @return The value
ISAR_API(uint64_t) Isar_Stats_MemberGetUlong(const void* member);

#pragma comment(linker, "/include:Isar_Stats_MemberGetDouble")
/// @brief If the member is type double, get the value
/// @param member [in]: The member to get the value for
/// @return The value
ISAR_API(double) Isar_Stats_MemberGetDouble(const void* member);

#pragma comment(linker, "/include:Isar_Stats_MemberGetString")
/// @brief If the member is type string, get the value
/// @param member [in]: The member to get the value for
/// @return The value as a null terminated string
ISAR_API(const char *) Isar_Stats_MemberGetString(const void* member);

#pragma comment(linker, "/include:Isar_Stats_MemberGetBoolArray")
/// @brief If the member is type bool array, get the array and length
/// @param member [in]: The member to get the value for
/// @param length [out]: The length of the array
/// @return The array of bools
ISAR_API(bool*) Isar_Stats_MemberGetBoolArray(const void* member, size_t* length);

#pragma comment(linker, "/include:Isar_Stats_MemberGetIntArray")
/// @brief If the member is type int array, get the array and length
/// @param member [in]: The member to get the value for
/// @param length [out]: The length of the array
/// @return The array of ints
ISAR_API(int32_t*) Isar_Stats_MemberGetIntArray(const void* member, size_t* length);

#pragma comment(linker, "/include:Isar_Stats_MemberGetUintArray")
/// @brief If the member is type uint array, get the array and length
/// @param member [in]: The member to get the value for
/// @param length [out]: The length of the array
/// @return The array of uints
ISAR_API(uint32_t*) Isar_Stats_MemberGetUintArray(const void* member, size_t* length);

#pragma comment(linker, "/include:Isar_Stats_MemberGetLongArray")
/// @brief If the member is type long array, get the array and length
/// @param member [in]: The member to get the value for
/// @param length [out]: The length of the array
/// @return The array of longs
ISAR_API(int64_t*) Isar_Stats_MemberGetLongArray(const void* member, size_t* length);

#pragma comment(linker, "/include:Isar_Stats_MemberGetUlongArray")
/// @brief If the member is type ulong array, get the array and length
/// @param member [in]: The member to get the value for
/// @param length [out]: The length of the array
/// @return The array of ulongs
ISAR_API(uint64_t*) Isar_Stats_MemberGetUlongArray(const void* member, size_t* length);

#pragma comment(linker, "/include:Isar_Stats_MemberGetDoubleArray")
/// @brief If the member is type double array, get the array and length
/// @param member [in]: The member to get the value for
/// @param length [out]: The length of the array
/// @return The array of double
ISAR_API(double*) Isar_Stats_MemberGetDoubleArray(const void* member, size_t* length);

#pragma comment(linker, "/include:Isar_Stats_MemberGetStringArray")
/// @brief If the member is type string array, get the array and length
/// @param member [in]: The member to get the value for
/// @param length [out]: The length of the array
/// @return The array of strings, each being a null terminated string
ISAR_API(const char**) Isar_Stats_MemberGetStringArray(const void* member, size_t* length);

ISAR_CPP_NS_END

#endif  //ISAR_STATS_API_H
