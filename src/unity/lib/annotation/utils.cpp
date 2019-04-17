#include "utils.hpp"
#include <logger/assertions.hpp>
#include <toolkits/image_deep_feature_extractor/image_deep_feature_extractor_toolkit.hpp>

namespace turi {
namespace annotate {

bool is_integer(std::string s) {
  if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
    return false;

  char *p;
  strtol(s.c_str(), &p, 10);

  return (*p == 0);
}

gl_sarray
featurize_images(const std::shared_ptr<turi::gl_sarray> &images) {
  DASSERT_EQ(images->dtype(), flex_type_enum::IMAGE);

  image_deep_feature_extractor::image_deep_feature_extractor_toolkit
      feature_extractor =
          image_deep_feature_extractor::image_deep_feature_extractor_toolkit();

  std::map<std::string, flexible_type> options = {
      {"model_name", "squeezenet_v1.1"},
      {"download_path",
       "./"} // TODO: figure out the cache directory in TuriCreate
  };

  feature_extractor.init_options(options);

  return feature_extractor.sarray_extract_features(*images.get(), false, 6);
}

float vectors_distance(const std::vector<double> &a,
                       const std::vector<double> &b) {
  std::vector<double> auxiliary;
  std::transform(a.begin(), a.end(), b.begin(), std::back_inserter(auxiliary),
                 [](double element1, double element2) {
                   return pow((element1 - element2), 2);
                 });
  auxiliary.shrink_to_fit();
  return ((float)(std::sqrt(std::accumulate(auxiliary.begin(), auxiliary.end(), 0.0))));
}

/**
 * Note: very inefficient way of calculating the distances.
 */
std::vector<flexible_type>
similar_items(const gl_sarray& distances, size_t index,
              size_t k) {
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
  
  gl_sframe sortableSFrame = gl_sframe({{"features", calculated_distances}, {"idx", gl_index}}); 

  gl_sframe sortedFrame = sortableSFrame.sort("features", true);

  gl_sarray gl_sorted = sortedFrame["idx"];
  gl_sarray head_gl_sorted = gl_sorted.head(k);

  return std::shared_ptr<unity_sarray>(head_gl_sorted)->to_vector();
}

} // namespace annotate
} // namespace turi
