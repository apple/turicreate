/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef IMAGE_FEATURE_EXTRACTOR_HPP
#define IMAGE_FEATURE_EXTRACTOR_HPP

#include <core/data/sframe/gl_sarray.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>

namespace turi {
namespace image_deep_feature_extractor {

// Interface for classes that can transform an image into a vector of feature
// values for training purposes, as well as exporting this transformation to
// CoreML.
class image_feature_extractor {
public:
  virtual ~image_feature_extractor() = default;

  // Returns a model specification that performs an equivalent computation when
  // compiled by CoreML. The model must accept an image as input and produce a
  // floating-point vector as output.
  virtual const CoreML::Specification::Model& coreml_spec() const = 0;

  // Returns an SArray of flex_vec values, representing the features extracted
  // from each corresponding flex_image in `images`. The extracted features must
  // match what the compiled CoreML model would produce, but implementations are
  // free to perform this computation in a more optimized fashion. The input
  // SArray may also contain flex_string values, in which case each string is
  // interpreted as a URL from which the image can be loaded.
  virtual gl_sarray extract_features(gl_sarray images, bool verbose, size_t batch_size) const = 0;
};

}  // image_deep_feature_extractor
}  // turi

#endif  // IMAGE_FEATURE_EXTRACTOR_HPP
