/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef QuantizationValidationUtils_h
#define QuantizationValidationUtils_h

#include <iostream>
#include <string>
#include <vector>

#include "Format.hpp"

namespace CoreML {
    inline uint64_t bitsToBytesCeil(uint64_t nBits){
        return (nBits + 7) / 8;
    }
    bool hasSufficientBytesInQuantizedWeightParam(const CoreML::Specification::WeightParams& weight,
                                                  const uint64_t units);
    bool hasValidQuantizationParams(const CoreML::Specification::WeightParams& weight, const int expectSize);

}

#endif /* QuantizationValidationUtils_h */
