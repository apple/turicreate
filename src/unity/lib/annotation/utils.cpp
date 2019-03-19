#include "utils.hpp"
#include <logger/assertions.hpp>
#include <toolkits/image_deep_feature_extractor/image_deep_feature_extractor_toolkit.hpp>

namespace turi {
namespace annotate {

bool isInteger(std::string s) {
  if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
    return false;

  char *p;
  strtol(s.c_str(), &p, 10);

  return (*p == 0);
}

std::shared_ptr<unity_sarray>
featurize_images(std::shared_ptr<unity_sarray> images) {
  DASSERT_EQ(images->dtype(), flex_type_enum::IMAGE);

  image_deep_feature_extractor::image_deep_feature_extractor_toolkit
      feature_extractor =
          image_deep_feature_extractor::image_deep_feature_extractor_toolkit();

  std::map<std::string, flexible_type> options = {
      {"model_name", "squeezenet_v1.1"},
      {"download_path", ""} // TODO: URL path to the mlmodel of squeeze-net
  };

  gl_sarray gl_images = gl_sarray(images);
  feature_extractor.init_options(options);

  gl_sarray gl_features =
      feature_extractor.sarray_extract_features(gl_images, false, 6);

  return std::shared_ptr<unity_sarray>(gl_features);
}

std::vector<size_t> similar_items(std::shared_ptr<unity_sarray> distances, size_t index, size_t k) {
  DASSERT_EQ(distances->dtype(), flex_type_enum::VECTOR);
  // TODO: calculate eigen distance
  // TODO: return top k lowest distances calculated
  return {1, 2, 3, 4};
}

} // namespace annotate
} // namespace turi
