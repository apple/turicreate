/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#ifdef __APPLE__

#define TURI_USE_FLOAT16

#endif

/*
 Source of the numeric limits -
 https://github.com/apple/coremltools/blob/a63319be4c70271f229c9f5c3423a97c7535f08b/coremltools/models/utils.py#L172
 */
#ifdef TURI_USE_FLOAT16

const constexpr __fp16 FLOAT16_NUMERIC_LIMIT_MIN = 0.000061035;
const constexpr __fp16 FLOAT16_NUMERIC_LIMIT_MAX = 65504;

#endif
