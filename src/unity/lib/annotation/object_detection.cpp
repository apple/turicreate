#include <functional>

#include <unity/lib/annotation/object_detection.hpp>
#include <unity/lib/annotation/utils.hpp>

#include <sframe/group_aggregate_value.hpp>
#include <sframe/groupby_aggregate_operators.hpp>
#include <unity/lib/gl_sframe.hpp>

namespace turi {
namespace annotate {

ObjectDetection::ObjectDetection(const std::shared_ptr<unity_sframe> &data,
                                 const std::vector<std::string> &data_columns,
                                 const std::string &annotation_column)
    : AnnotationBase(data, data_columns, annotation_column) {}

annotate_spec::MetaData ObjectDetection::metaData() {
  annotate_spec::MetaData meta_data;

  meta_data.set_type(annotate_spec::MetaData_AnnotationType::MetaData_AnnotationType_OBJECT_DETECTION);
  meta_data.set_num_examples(m_data->size());
  
  annotate_spec::ObjectDetectionMeta *object_detection_meta = meta_data.mutable_object_detection();

  gl_sframe gl_data(m_data);
  gl_sframe stacked_annotations = gl_data.stack(m_annotation_column, "annotations", true);
  gl_sframe unpacked_annotations = stacked_annotations.unpack("annotations");

  gl_sarray labels = unpacked_annotations["annotations.label"].dropna();
  gl_sarray counts = labels.cumulative_aggregate(std::make_shared<groupby_operators::count>());

  flex_type_enum array_type = labels.dtype();

  DASSERT_TRUE(labels.size() == labels.size());

  gl_sframe label_counts({{"labels", labels}, {"count", counts}});

  for(const auto& row: label_counts.range_iterator()) {
    if (array_type == flex_type_enum::STRING) {
      annotate_spec::MetaLabel *labels_meta = object_detection_meta->add_label();
      labels_meta->set_stringlabel(row[0].get<flex_string>());
      labels_meta->set_elementcount(row[1].get<flex_int>());
    }

    if (array_type == flex_type_enum::INTEGER) {
      annotate_spec::MetaLabel *labels_meta = object_detection_meta->add_label();
      labels_meta->set_intlabel(row[0].get<flex_int>());
      labels_meta->set_elementcount(row[1].get<flex_int>());
    }
  }

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