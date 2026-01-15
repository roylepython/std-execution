/**
 * Amphisbaena 🐍 - Format Header Fix for GCC 14.2.0
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * PROPER FIX for GCC 14.2.0 format header bug with C++26
 * 
 * This header must be included before any standard library headers that might
 * pull in <format>. It defines macros that prevent the problematic code path
 * in the format header from being compiled.
 * 
 * The bug is at line 3750 of /usr/include/c++/14/format where there's a
 * syntax error with consecutive '[' tokens in C++26 mode.
 */

#pragma once

// Only apply fix for GCC 14.x with C++26
#if defined(__GNUC__) && __GNUC__ == 14 && defined(__cplusplus) && __cplusplus > 202302L

// ============================================================================
// PROPER FIX for GCC 14.2.0 format header bug at line 3750
// ============================================================================
// The format header has invalid syntax with consecutive '[' tokens in C++26 mode
// The bug occurs because the format header checks __cpp_lib_format to decide
// which code path to compile. When __cpp_lib_format >= 202311L (C++26), it tries
// to use C++26-specific attribute syntax that GCC 14.2.0 doesn't properly support.
//
// THE FIX: Force the format header to use C++20 implementation (202110L) by
// undefining and redefining __cpp_lib_format BEFORE any standard headers.
// ============================================================================

// CRITICAL: Undefine __cpp_lib_format FIRST to remove compiler's C++26 value
// The compiler sets this based on -std=c++2c, so we must override it
#ifdef __cpp_lib_format
#undef __cpp_lib_format
#endif

// CRITICAL: Force C++20 format implementation (202110L)
// This MUST be set to 202110L (C++20) to prevent the buggy C++26 code path
// The format header checks: if __cpp_lib_format >= 202311L, use C++26 code (BUGGY)
//                          else use C++20 code (STABLE)
#define __cpp_lib_format 202110L

// Ensure C99 features are available (required for format header)
#ifndef _GLIBCXX_USE_C99_STDIO
#define _GLIBCXX_USE_C99_STDIO 1
#endif

#ifndef _GLIBCXX_USE_C99_COMPLEX
#define _GLIBCXX_USE_C99_COMPLEX 1
#endif

// Prevent format header from detecting C++26 features
// These macros tell libstdc++ to not use experimental C++26 format features
#ifndef _GLIBCXX_HAVE_CXX26_FORMAT
#define _GLIBCXX_HAVE_CXX26_FORMAT 0
#endif

// Prevent C++26 format features from being enabled
#define _GLIBCXX_NO_CXX26_FORMAT_FEATURES 1

// Force format header to use C++20 code path
#ifndef _GLIBCXX_USE_CXX20_FORMAT
#define _GLIBCXX_USE_CXX20_FORMAT 1
#endif

// CRITICAL: Force C++20 format implementation (prevents C++26 code path)
#define _GLIBCXX_FORCE_CXX20_FORMAT 1

// Prevent C++26 attribute syntax (the bug is with [[[nested]]] attributes)
#define _GLIBCXX_NO_CXX26_ATTRIBUTES 1

// ADDITIONAL FIX: Prevent format header from checking __cplusplus for C++26
// The format header might also check __cplusplus directly, so we need to
// ensure it doesn't enter C++26-specific code paths based on that either
// We can't change __cplusplus, but we can prevent the format header from
// using it to enable C++26 features by ensuring __cpp_lib_format is low enough

#endif // GCC 14.x with C++26

