#ifndef TURI_ANNOTATIONS_UTILS_HPP
#define TURI_ANNOTATIONS_UTILS_HPP

#include "build/format/cpp/annotate.pb.h"
#include "build/format/cpp/data.pb.h"
#include "build/format/cpp/message.pb.h"
#include "build/format/cpp/meta.pb.h"

#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <boost/regex.hpp>
#include <memory>
#include <vector>

namespace annotate_spec = TuriCreate::Annotation::Specification;

namespace turi {
namespace annotate {

template <typename T>
typename std::enable_if<
    std::is_same<T, annotate_spec::Annotations>::value>::type
populate_parcel(annotate_spec::Parcel &parcel, T message) {
  parcel.mutable_annotations()->CopyFrom(message);
}

template <typename T>
typename std::enable_if<std::is_same<T, annotate_spec::Data>::value>::type
populate_parcel(annotate_spec::Parcel &parcel, T message) {
  parcel.mutable_data()->CopyFrom(message);
}

template <typename T>
typename std::enable_if<std::is_same<T, annotate_spec::MetaData>::value>::type
populate_parcel(annotate_spec::Parcel &parcel, T message) {
  parcel.mutable_metadata()->CopyFrom(message);
}

float vectors_distance(const std::vector<double> &a,
                       const std::vector<double> &b);

bool is_integer(std::string s);

gl_sarray
featurize_images(const std::shared_ptr<turi::gl_sarray> &images);

std::vector<flexible_type>
similar_items(const gl_sarray& distances, size_t index,
              size_t k);

} // namespace annotate
} // namespace turi

#endif
