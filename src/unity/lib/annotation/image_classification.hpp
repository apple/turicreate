#ifndef TURI_ANNOTATIONS_IMAGE_CLASSIFICATION_HPP
#define TURI_ANNOTATIONS_IMAGE_CLASSIFICATION_HPP

#include <export.hpp>

#include <chrono>
#include <future>
#include <iostream>
#include <thread>

#include <unity/lib/annotation/annotation_base.hpp>

#include "build/format/cpp/annotate.pb.h"
#include "build/format/cpp/data.pb.h"

namespace turi {
namespace annotate {

class ImageClassification : public AnnotationBase {
public:
  ImageClassification() : AnnotationBase(){};
  ImageClassification(const std::shared_ptr<unity_sframe> &data,
                      const std::vector<std::string> &data_columns,
                      const std::string &annotation_column);

  ~ImageClassification(){};

  annotate_spec::MetaData metaData() override;

  annotate_spec::Data getItems(size_t start, size_t end) override;

  annotate_spec::Annotations getAnnotations(size_t start, size_t end) override;

  bool setAnnotations(const annotate_spec::Annotations &annotations) override;

  void cast_annotations() override;

  void add_image_features(const std::shared_ptr<unity_sarray> &features);

  BEGIN_CLASS_MEMBER_REGISTRATION("ImageClassification");
  IMPORT_BASE_CLASS_REGISTRATION(AnnotationBase);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("add_image_features",
                                       ImageClassification::add_image_features,
                                       "features");
  END_CLASS_MEMBER_REGISTRATION
private:
  /*
  std::future<std::shared_ptr<unity_sarray>> m_image_features;
  */
  void _addAnnotationToSFrame(size_t index, std::string label);
  void _addAnnotationToSFrame(size_t index, int label);

  std::shared_ptr<unity_sarray> _calculateFeatureSimilarity();

  std::shared_ptr<unity_sarray> _filterDataSFrame(size_t &start, size_t &end);
  std::shared_ptr<unity_sarray> _filterAnnotationSFrame(size_t &start,
                                                        size_t &end);
};

std::shared_ptr<ImageClassification> create_image_classification_annotation(
    const std::shared_ptr<unity_sframe> &data,
    const std::vector<std::string> &data_columns,
    const std::string &annotation_column);

} // namespace annotate
} // namespace turi
#endif