/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <stdio.h>
#include <string>
#include <vector>
#include "ValidatorUtils-inl.hpp"
#include "QuantizationValidationUtils.hpp"

namespace CoreML {

    bool hasSufficientBytesInQuantizedWeightParam(const CoreML::Specification::WeightParams& weight,
                                                         const uint64_t units){
        // Checks whether the quantized weight params have sufficiently large byte array
        const uint64_t bitsPerUnit = weight.quantization().numberofbits();
        const uint64_t numBytesNeeded = bitsToBytesCeil(bitsPerUnit * units);
        uint64_t weightSizeInBytes = (uint64_t) CoreML::getWeightParamSizeInBytes(weight);
        return (weightSizeInBytes >= numBytesNeeded);
    }

    bool hasValidQuantizationParams(const CoreML::Specification::WeightParams& weight,
                                    const int expectSize){
        if (!weight.has_quantization()){
            return false;
        }
        const CoreML::Specification::QuantizationParams& quant = weight.quantization();
        const uint64_t nbits = quant.numberofbits();
        if (!(nbits >= 1 && nbits <= 8)){
            return false;
        }
        if (quant.has_linearquantization()){
            // Acceptable linear quantization cases:
            // scale can be vector of size 1 or expectSize (output channels in most cases)
            // bias can be nothing, or having the same length as scale
            int scaleLength = quant.linearquantization().scale_size();
            if (!(scaleLength == 1 || scaleLength == expectSize)){
                return false;
            }
            int biasLength = quant.linearquantization().bias_size();
            if (!(biasLength == 0 || biasLength == scaleLength)){
                return false;
            }
        } else if (quant.has_lookuptablequantization()){
            int tableLength = quant.lookuptablequantization().floatvalue_size();
            if (tableLength != 2 << (nbits-1)){
                return false;
            }
        } else { // invalid type
            return false;
        }
        return true;
    }

}
