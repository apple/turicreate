/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef Pipeline_hpp
#define Pipeline_hpp

#include <stdio.h>
#include "../Model.hpp"

namespace CoreML {

// Forward declare structs to abstract way storage layouts.
namespace Specification {
    class PipelineParameters;
}

class EXPORT Pipeline : public Model {
private:
  Pipeline(const std::string& description);
  Pipeline(const std::string& a, const std::string& b, const std::string& description, bool isClassifier);

public:
  
  /**
   * Construct as a regressor.
  */
  static Pipeline Regressor(const std::string& predictedValueOutputName,
                            const std::string& description);

  /**
   * Construct as a classifier.
   */
  static Pipeline Classifier(const std::string& predictedClassName,
                             const std::string& probabilityName,
                             const std::string& description);


  static Pipeline Transformer(const std::string& description);
  /**
   * Construct from proto.
   */
  Pipeline(const Specification::Model &modelSpec);

  // Destructor.
  virtual ~Pipeline();

  /** Adds in a transform MLModel.
   */
  Result add(const Model& spec);

  /**  Returns the pipeline
   *
   */
  std::vector<Model> getPipeline() const;
};

}

#endif /* Pipeline_hpp */
