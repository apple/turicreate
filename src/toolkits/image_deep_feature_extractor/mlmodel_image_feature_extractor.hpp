/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef MLMODEL_IMAGE_FEATURE_EXTRACTOR_HPP
#define MLMODEL_IMAGE_FEATURE_EXTRACTOR_HPP

#include <toolkits/image_deep_feature_extractor/image_feature_extractor.hpp>

#include <memory>
#include <string>

namespace turi {
namespace image_deep_feature_extractor {

// Implementation of image_feature_extractor that instantiates an actual MLModel
// to process each image.
class mlmodel_image_feature_extractor: public image_feature_extractor {
public:
  // Constructs an instance produced by downloading a known MLModel and using
  // the appropriate layer for the feature values. If necessary, the model will
  // be downloaded to `download_path`. Supported `model_name` values are:
  // "resnet-50", "squeezenet_v1.1"
  mlmodel_image_feature_extractor(const std::string& model_name,
				  const std::string& download_path);

  // image_feature_extractor interface
  const CoreML::Specification::Model& coreml_spec() const override;
  gl_sarray extract_features(gl_sarray images, bool verbose, size_t batch_size) const override;

private:
  // Use PIMPL pattern to hide Objective C from this C++ header.
  struct impl;

  std::unique_ptr<impl> m_impl;
};

}  // image_deep_feature_extractor
}  // turi

#endif  // MLMODEL_IMAGE_FEATURE_EXTRACTOR_HPP
