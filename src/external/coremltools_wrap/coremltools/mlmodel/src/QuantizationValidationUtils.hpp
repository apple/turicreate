//
//  QuantizationValidationUtils.hpp
//  CoreML_framework
//
//  Created by Shuoxin Lin on 2/18/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

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
