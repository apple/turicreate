/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
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
