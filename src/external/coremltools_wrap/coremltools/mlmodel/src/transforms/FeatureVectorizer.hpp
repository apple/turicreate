//
//  FeatureVectorizer.hpp
//  libmlmodelspec
//
//  Created by Hoyt Koepke on 11/24/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#ifndef FeatureVectorizer_hpp
#define FeatureVectorizer_hpp

#include "../Model.hpp"

namespace CoreML {

  class FeatureVectorizer : public Model {
  public:

    /*  Initialize as a generic transform.
     */
    explicit FeatureVectorizer(const std::string& description);

    /**
     * Construct from proto.
     */
    explicit FeatureVectorizer(const Specification::Model &modelSpec);

    // Destructor.
    virtual ~FeatureVectorizer();

    /** Adds in a transform MLModel.
     */
    Result add(const std::string& input_feature, size_t input_dimension);

    std::vector<std::pair<std::string, size_t> > get_inputs() const;
  };
}


#endif /* FeatureVectorizer_hpp */
