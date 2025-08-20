/**
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#ifndef ISAR_API_H
#define ISAR_API_H

#pragma region // ISAR_EXPORT

# ifdef __cplusplus
#   define ISAR_EXTERN extern "C"
# else
#   define ISAR_EXTERN
# endif  // __cplusplus

// HACK: because we dont have defines set up in build.gn at the moment
#define ISAR_EXPORT_FUNCTIONS

// This will be defined on all versions of Windows we care about (ARM/ARM64/x86/x64)
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-160#microsoft-specific-predefined-macros
#if defined(_WIN32)

# if defined(ISAR_EXPORT_FUNCTIONS)
#   define ISAR_EXPORT ISAR_EXTERN __declspec(dllexport)
# else
// Ref: https://docs.microsoft.com/en-us/cpp/build/importing-function-calls-using-declspec-dllimport?view=vs-2019
#   define ISAR_EXPORT ISAR_EXTERN __declspec(dllimport)
# endif

#elif defined(WEBRTC_ANDROID)
# if defined(ISAR_EXPORT_FUNCTIONS)
#   define ISAR_EXPORT ISAR_EXTERN __attribute__((visibility("default")))
# else
// https://clang.llvm.org/docs/LTOVisibility.html
// dllexport & dllimport both expose symbols so... ¯\_(ツ)_/¯
// TODO(viktor): research dllimport, i guess
#   define ISAR_EXPORT ISAR_EXTERN __attribute__((visibility("default")))
// #define ISAR_EXPORT __attribute__((visibility("hidden")))
# endif
#endif // defined(WEBRTC_ANDROID)

#pragma endregion // ISAR_EXPORT

#ifndef ISAR_API
#define ISAR_API(_return_value) ISAR_EXPORT _return_value
#endif

#ifdef __cplusplus
#define ISAR_CPP_NS_BEGIN namespace isar {
#define ISAR_CPP_NS_END }
#else
#define ISAR_CPP_NS_BEGIN
#define ISAR_CPP_NS_END
#endif

// TODO(viktor): create core.h/lang.h/preprocessor.h or sth
#ifndef IN
# define IN
#endif // IN

#ifndef OUT
# define OUT
#endif // OUT

#ifndef IN_OUT
# define IN_OUT
#endif // IN_OUT

//#define ISAR_ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#ifndef ISAR_STRINGIFY
# define ISAR_STRINGIFY(value) #value
#endif // ISAR_STRINGIFY

#ifndef ISAR_STRINGIFY_DELAYED
# define ISAR_STRINGIFY_DELAYED(value) ISAR_STRINGIFY(value)
#endif // ISAR_STRINGIFY

// unicode string
#ifndef ISAR_WIDE
# define ISAR_WIDE(value) L##value
#endif // ISAR_WIDE

#ifndef ISAR_WIDE_DELAYED
# define ISAR_WIDE_DELAYED(value) ISAR_WIDE(value)
#endif // ISAR_WIDE_DELAYED

#ifndef ISAR_STRINGIFY_W
//# define ISAR_STRINGIFY_W(value) L#value // I think this is an msvc extension
//# define ISAR_STRINGIFY_W(value) TEXT(#value) // winnt.h (windows only)
# define ISAR_STRINGIFY_W(value) ISAR_WIDE(#value)
#endif // ISAR_STRINGIFY_W

#ifndef ISAR_STRINGIFY_DELAYED_W
# define ISAR_STRINGIFY_DELAYED_W(value) ISAR_STRINGIFY_W(value)
#endif // ISAR_STRINGIFY_DELAYED_W

#ifndef ISAR_STRINGIFY_ENUM_CASE
#define ISAR_STRINGIFY_ENUM_CASE(value) \
	case value:                     \
		return ISAR_STRINGIFY(value)
#endif

#ifndef ISAR_STRINGIFY_ENUM_CASE_W
#define ISAR_STRINGIFY_ENUM_CASE_W(value) \
	case value:                       \
		return ISAR_STRINGIFY_W(value)
#endif

#ifndef NDEBUG
#ifndef ISAR_DEBUG
#define ISAR_DEBUG
#endif  // ISAR_DEBUG
#endif  // NDEBUG

#endif  // ISAR_API_H
