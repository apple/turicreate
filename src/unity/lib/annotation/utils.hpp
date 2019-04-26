#ifndef TURI_ANNOTATIONS_UTILS_HPP
#define TURI_ANNOTATIONS_UTILS_HPP

#include "build/format/cpp/annotate.pb.h"
#include "build/format/cpp/data.pb.h"
#include "build/format/cpp/message.pb.h"
#include "build/format/cpp/meta.pb.h"

#include <boost/regex.hpp>

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

bool isInteger(std::string s);


} // namespace annotate
} // namespace turi

#endif