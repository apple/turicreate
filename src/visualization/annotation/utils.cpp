#include "utils.hpp"
#include <core/logging/assertions.hpp>

namespace turi {
namespace annotate {

bool is_integer(std::string s) {
  if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
    return false;

  char *p;
  strtol(s.c_str(), &p, 10);

  return (*p == 0);
}

float vectors_distance(const std::vector<double> &a,
                       const std::vector<double> &b) {
  DASSERT_EQ(a.size(), b.size());
  double acc = 0;
  for (size_t i = 0; i < a.size(); ++i) {
    acc += std::pow(a[i] - b[i], 2);
  }
  return std::sqrt(acc);
}

std::vector<flexible_type> similar_items(const gl_sarray &distances,
                                         size_t index, size_t k) {
  DASSERT_EQ(distances.dtype(), flex_type_enum::VECTOR);
  flex_vec target_vector = distances[index].get<flex_vec>();

  gl_sarray calculated_distances = distances.apply(
      [=](const flexible_type &a) {
        return flexible_type(
            vectors_distance(target_vector, a.get<flex_vec>()));
      },
      flex_type_enum::FLOAT);

  calculated_distances.materialize();
  std::vector<flexible_type> indicies(distances.size());
  std::iota(indicies.begin(), indicies.end(), 0);

  gl_sarray gl_index = gl_sarray(indicies, flex_type_enum::INTEGER);
  gl_sframe sortableSFrame =
      gl_sframe({{"features", calculated_distances}, {"idx", gl_index}});
  gl_sframe sortedFrame = sortableSFrame.sort("features", true);
  gl_sarray gl_sorted = sortedFrame["idx"];
  gl_sarray head_gl_sorted = gl_sorted.head(k);

  return std::shared_ptr<unity_sarray>(head_gl_sorted)->to_vector();
}

#ifdef __APPLE__

image_deep_feature_extractor::image_deep_feature_extractor_toolkit
create_feature_extractor(std::string base_directory) {
  image_deep_feature_extractor::image_deep_feature_extractor_toolkit
      feature_extractor =
          image_deep_feature_extractor::image_deep_feature_extractor_toolkit();

  std::map<std::string, flexible_type> options = {
      {"model_name", "squeezenet_v1.1"}, {"download_path", base_directory}};

  feature_extractor.init_options(options);
  return feature_extractor;
}

gl_sarray featurize_images(const gl_sarray &images,
                           std::string base_directory) {
  DASSERT_EQ(images.dtype(), flex_type_enum::IMAGE);

  image_deep_feature_extractor::image_deep_feature_extractor_toolkit
      feature_extractor =
          image_deep_feature_extractor::image_deep_feature_extractor_toolkit();

  std::map<std::string, flexible_type> options = {
      {"model_name", "squeezenet_v1.1"}, {"download_path", base_directory}};

  feature_extractor.init_options(options);

  return feature_extractor.sarray_extract_features(images, false, 6);
}
#endif

} // namespace annotate
} // namespace turi
