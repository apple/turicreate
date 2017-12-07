/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef MLMODEL_ONE_HOT_ENCODER_SPEC_HPP
#define MLMODEL_ONE_HOT_ENCODER_SPEC_HPP

#include "../Result.hpp"
#include "../Model.hpp"
#include "../../build/format/OneHotEncoder_enums.h"


namespace CoreML {
    
    class EXPORT OneHotEncoder : public Model {
    
    public:

        explicit OneHotEncoder(const std::string& description = "");
        
        Result addInput(const std::string& featureName, FeatureType featureType) override;

        Result setHandleUnknown(MLHandleUnknown state);

        Result setUseSparse(bool state);
    
        Result setFeatureEncoding(const std::vector<int64_t>& container);
        
        Result setFeatureEncoding(const std::vector<std::string>& container);

        Result getHandleUnknown(MLHandleUnknown *state);
    
        Result getUseSparse(bool *state);
    
        Result getFeatureEncoding(std::vector<int64_t>& container);
        
        Result getFeatureEncoding(std::vector<std::string>& container);
    };
}

#endif
