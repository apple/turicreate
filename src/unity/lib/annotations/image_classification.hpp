#ifndef TURI_ANNOTATIONS_IMAGE_CLASSIFICATION_HPP
#define TURI_ANNOTATIONS_IMAGE_CLASSIFICATION_HPP

#include "build/format/cpp/annotate.pb.h"
#include "build/format/cpp/data.pb.h"

#include "annotation_base.hpp"

namespace turi {
namespace annotate {

class ImageClassification : public AnnotationBase {
public:
  ~ImageClassification(){};

  annotate_spec::Data getItems(size_t start, size_t end);

  annotate_spec::Annotations getAnnotations(size_t start, size_t end);

  bool setAnnotations(const annotate_spec::Annotations &annotation);

  std::shared_ptr<unity_sframe> returnAnnotations(bool drop_null);

private:
  std::shared_ptr<unity_sarray> _filterDataSFrame(size_t &start, size_t &end);
  std::shared_ptr<unity_sarray> _filterAnnotationSFrame(size_t &start,
                                                        size_t &end);
};

} // namespace annotate
} // namespace turi
#endif