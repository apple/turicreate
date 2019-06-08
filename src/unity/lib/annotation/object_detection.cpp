#include <functional>
#include <unity/lib/annotation/object_detection.hpp>
#include <unity/lib/annotation/utils.hpp>

namespace turi {
namespace annotate {

ObjectDetection::ObjectDetection(const std::shared_ptr<unity_sframe> &data,
                                 const std::vector<std::string> &data_columns,
                                 const std::string &annotation_column)
    : AnnotationBase(data, data_columns, annotation_column) {}

annotate_spec::MetaData ObjectDetection::metaData() {
  annotate_spec::MetaData meta_data;
  // TODO: implement `metaData`
  return meta_data;
}

annotate_spec::Data ObjectDetection::getItems(size_t start, size_t end) {
  annotate_spec::Data data;
  // TODO: Implement `getItems`
  return data;
}

annotate_spec::Annotations ObjectDetection::getAnnotations(size_t start,
                                                           size_t end) {
  annotate_spec::Annotations annotations;
  // TODO: Implement `getAnnotations`
  return annotations;
}

bool ObjectDetection::setAnnotations(
    const annotate_spec::Annotations &annotations) {
  // TODO: implement `setAnnotations`
  return true;
}

void ObjectDetection::cast_annotations() {
  // TODO: implement `cast_annotations`
}

void ObjectDetection::background_work() {
  // TODO: implement `background_work`
}

} // namespace annotate
} // namespace turi