#ifndef TURI_ANNOTATIONS_OBJECT_DETECTION_HPP
#define TURI_ANNOTATIONS_OBJECT_DETECTION_HPP

#include <chrono>
#include <future>
#include <iostream>
#include <thread>

#include <visualization/annotation/annotation_base.hpp>
#include <model_server/lib/variant.hpp>
#include <toolkits/nearest_neighbors/unity_nearest_neighbors.hpp>

#include "build/format/cpp/annotate.pb.h"
#include "build/format/cpp/data.pb.h"

namespace turi {
namespace annotate {

class ObjectDetection : public AnnotationBase {
public:
  ObjectDetection() : AnnotationBase(){};
  ObjectDetection(const std::shared_ptr<unity_sframe> &data,
                  const std::vector<std::string> &data_columns,
                  const std::string &annotation_column);

  ~ObjectDetection(){};

  annotate_spec::MetaData metaData() override;

  annotate_spec::Data getItems(size_t start, size_t end) override;

  annotate_spec::Annotations getAnnotations(size_t start, size_t end) override;

  bool setAnnotations(const annotate_spec::Annotations &annotations) override;

  void cast_annotations() override;

  void background_work() override;

  void addAnnotationColumn() override;

  void checkDataSet() override;

  annotate_spec::Similarity get_similar_items(size_t index,
                                              size_t k = 7) override;

  BEGIN_CLASS_MEMBER_REGISTRATION("ObjectDetection");
  IMPORT_BASE_CLASS_REGISTRATION(AnnotationBase);
  END_CLASS_MEMBER_REGISTRATION
private:
  flex_dict _parse_bounding_boxes(annotate_spec::Label label);
  void _addAnnotationToSFrame(size_t index, flex_list label);
};

std::shared_ptr<ObjectDetection>
create_object_detection_annotation(const std::shared_ptr<unity_sframe> &data,
                                   const std::vector<std::string> &data_columns,
                                   const std::string &annotation_column);

} // namespace annotate
} // namespace turi

#endif