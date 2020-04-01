#pragma once

// Adapted from the gcc wiki (by Niall Douglas originally)
// This allows for better code optimisation through PLT indirections. Also reduces the size and load times of DSO.

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_DLL
#ifdef __GNUC__
#define OE_PUBLIC __attribute__((dllexport))
#else
#define OE_PUBLIC __declspec(dllexport)    // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define OE_PUBLIC __attribute__((dllimport))
#else
#define OE_PUBLIC __declspec(dllimport)    // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#else
#if __GNUC__ >= 4
#define OE_PUBLIC __attribute__((visibility("default")))
#else
#define OE_PUBLIC
#endif
#endif

// used mainly for silencing warnings
#define OE_UNUSED(var) \
static_cast<void>(var)

#ifdef __GNUC__
#define OE_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define OE_FORCE_INLINE inline
#endif
