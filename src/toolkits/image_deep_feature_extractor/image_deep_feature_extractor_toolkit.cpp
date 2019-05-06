/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/image_deep_feature_extractor/image_deep_feature_extractor_toolkit.hpp>

#include <toolkits/image_deep_feature_extractor/mlmodel_image_feature_extractor.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>

namespace turi {
namespace image_deep_feature_extractor {


void image_deep_feature_extractor_toolkit::init_options(const std::map<std::string, flexible_type>& options) {
  const auto& model_name = options.find("model_name");
  DASSERT_TRUE(model_name != options.end());

  const auto& download_path = options.find("download_path");
  DASSERT_TRUE(download_path != options.end());

  m_feature_extractor.reset(new mlmodel_image_feature_extractor(model_name->second, download_path->second));
}

gl_sarray image_deep_feature_extractor_toolkit::extract_features(gl_sframe data,
                                                                 const std::string& column_name,
                                                                 bool verbose, size_t batch_size) const {
  return m_feature_extractor->extract_features(data[column_name], verbose, batch_size);
}


gl_sarray image_deep_feature_extractor_toolkit::sarray_extract_features(gl_sarray data,
                                                                 bool verbose, size_t batch_size) const {
  return m_feature_extractor->extract_features(data, verbose, batch_size);
}


}  // image_deep_feature_extractor
}  // turi
