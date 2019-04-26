/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_API_VISIBLE_ENUMS_HPP_
#define TURI_API_VISIBLE_ENUMS_HPP_

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifndef NS_OPTIONS
#define NS_OPTIONS(_type, _name) enum _name : _type _name; enum _name : _type
#endif // NS_OPTIONS

#ifdef __cplusplus
// In C++, NS_OPTIONS doesn't seem to be valid syntax; skip it and just use a C++ enum.
#define MAKE_NS_OPTIONS(_type, _name) enum _name : _type
#else
#ifdef __OBJC__
// In ObjC, define MAKE_NS_OPTIONS in terms of NS_OPTIONS
#define MAKE_NS_OPTIONS(_type, _name) typedef NS_OPTIONS(_type, _name)
#else
#define MAKE_NS_OPTIONS(_type, _name) typedef enum _name##_enum _name; enum _name##_enum
#endif
#endif // __cplusplus

MAKE_NS_OPTIONS(uint32_t, tc_log_level) {
  TURI_LOG_EVERYTHING = 0,
  TURI_LOG_DEBUG = 1,
  TURI_LOG_INFO = 2,
  TURI_LOC_EMPH = 4,
  TURI_LOG_PROGRESS = 4,
  TURI_LOG_WARNING = 5,
  TURI_LOG_ERROR = 6,
  TURI_LOG_FATAL = 7,
  TURI_LOG_NONE = 8
};


/*
 * Bit flags to configure plot variations.
 * The bit layout is as follows:
 * The first 4 bits (1 hex digit) represents size.
 * The next 4 bits (1 hex digit) represents color mode (light/dark).
 * Zeroes in any set of bits imply defaults should be used.
 * To apply multiple flags, simply OR them together.
 * (Note: only a single flag within each bit range should be used.)
 */
MAKE_NS_OPTIONS(uint64_t, tc_plot_variation){
    tc_plot_variation_default = 0x00,

    // Sizes (defaults to medium)
    tc_plot_size_small = 0x01,
    tc_plot_size_medium = 0x02,
    tc_plot_size_large = 0x03,

    // Color variations
    // default could be light/dark depending on OS settings
    tc_plot_color_light = 0x10,
    tc_plot_color_dark = 0x20,

};

MAKE_NS_OPTIONS(uint64_t, tc_ft_type_enum) {
  FT_TYPE_INTEGER = 0,
  FT_TYPE_FLOAT   = 1,
  FT_TYPE_STRING  = 2,
  FT_TYPE_ARRAY   = 3,
  FT_TYPE_LIST    = 4,
  FT_TYPE_DICT    = 5,
  FT_TYPE_DATETIME = 6,
  FT_TYPE_UNDEFINED = 7,
  FT_TYPE_IMAGE   = 8,
  FT_TYPE_NDARRAY = 9
};

#endif
