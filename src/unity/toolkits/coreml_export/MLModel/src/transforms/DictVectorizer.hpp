/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef DictVectorizer_h
#define DictVectorizer_h

#include "../Result.hpp"
#include "../Model.hpp"
#include "../../build/format/OneHotEncoder_enums.h"


namespace CoreML {
    

class EXPORT DictVectorizer : public Model {
    
public:
    
    explicit DictVectorizer(const std::string& description = "");
    
    Result addInput(const std::string& featureName, FeatureType featureType) override;
    
    Result setHandleUnknown(MLHandleUnknown state);
    
    Result setFeatureEncoding(const std::vector<int64_t>& container);
    
    Result setFeatureEncoding(const std::vector<std::string>& container);
};

}
#endif /* DictVectorizer_h */
