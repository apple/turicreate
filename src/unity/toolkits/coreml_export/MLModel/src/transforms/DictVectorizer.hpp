//
//  DictVectorizer.h
//  libmlmodelspec
//
//  Created by Hoyt Koepke on 2/7/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#ifndef DictVectorizer_h
#define DictVectorizer_h

#include "../Result.hpp"
#include "../Model.hpp"

#include "unity/toolkits/coreml_export/protobuf_include_internal.hpp"

namespace CoreML {
    

class DictVectorizer : public Model {
    
public:
    
    explicit DictVectorizer(const std::string& description = "");
    
    Result addInput(const std::string& featureName, FeatureType featureType) override;
    
    Result setHandleUnknown(MLHandleUnknown state);
    
    Result setFeatureEncoding(const std::vector<int64_t>& container);
    
    Result setFeatureEncoding(const std::vector<std::string>& container);
};

}
#endif /* DictVectorizer_h */
