#include <functional>

#include <unity/lib/annotation/object_detection.hpp>
#include <unity/lib/annotation/utils.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <sframe/group_aggregate_value.hpp>
#include <sframe/groupby_aggregate_operators.hpp>

#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/image_util.hpp>
#include <unity/lib/unity_sarray_builder.hpp>

namespace turi {
namespace annotate {

ObjectDetection::ObjectDetection(const std::shared_ptr<unity_sframe> &data,
                                 const std::vector<std::string> &data_columns,
                                 const std::string &annotation_column)
    : AnnotationBase(data, data_columns, annotation_column) {}

annotate_spec::MetaData ObjectDetection::metaData() {
  annotate_spec::MetaData meta_data;

  meta_data.set_type(annotate_spec::MetaData_AnnotationType::
                         MetaData_AnnotationType_OBJECT_DETECTION);
  meta_data.set_num_examples(m_data->size());

  annotate_spec::ObjectDetectionMeta *object_detection_meta =
      meta_data.mutable_object_detection();

  gl_sframe gl_data(m_data);
  gl_sframe stacked_annotations =
      gl_data.stack(m_annotation_column, "annotations", true);
  gl_sframe unpacked_annotations = stacked_annotations.unpack("annotations");

  gl_sarray labels = unpacked_annotations["annotations.label"].dropna();
  gl_sarray counts =
      labels.cumulative_aggregate(std::make_shared<groupby_operators::count>());

  flex_type_enum array_type = labels.dtype();

  DASSERT_TRUE(labels.size() == labels.size());

  gl_sframe label_counts({{"labels", labels}, {"count", counts}});

  for (const auto &row : label_counts.range_iterator()) {
    if (array_type == flex_type_enum::STRING) {
      annotate_spec::MetaLabel *labels_meta =
          object_detection_meta->add_label();
      labels_meta->set_stringlabel(row[0].get<flex_string>());
      labels_meta->set_elementcount(row[1].get<flex_int>());
    }

    if (array_type == flex_type_enum::INTEGER) {
      annotate_spec::MetaLabel *labels_meta =
          object_detection_meta->add_label();
      labels_meta->set_intlabel(row[0].get<flex_int>());
      labels_meta->set_elementcount(row[1].get<flex_int>());
    }
  }

  return meta_data;
}

annotate_spec::Data ObjectDetection::getItems(size_t start, size_t end) {
  annotate_spec::Data data;

  gl_sframe gl_data(m_data);

  gl_sframe filtered_data =
      gl_data[{static_cast<long long>(start), static_cast<long long>(end)}];

  gl_sarray filtered_images = filtered_data[m_data_columns.at(0)].dropna();

  DASSERT_EQ(filtered_images.dtype(), flex_type_enum::IMAGE);

  size_t i = 0;
  for (const auto &image : filtered_images.range_iterator()) {
    flex_image img = image.get<flex_image>();
    img = turi::image_util::encode_image(img);

    size_t img_width = img.m_width;
    size_t img_height = img.m_height;
    size_t img_channels = img.m_channels;

    annotate_spec::Datum *datum = data.add_data();
    annotate_spec::ImageDatum *img_datum = datum->add_images();

    img_datum->set_width(img_width);
    img_datum->set_height(img_height);
    img_datum->set_channels(img_channels);

    const unsigned char *img_bytes = img.get_image_data();
    size_t img_data_size = img.m_image_data_size;

    std::string img_base64(
        boost::archive::iterators::base64_from_binary<
            boost::archive::iterators::transform_width<const unsigned char *, 6,
                                                       8>>(img_bytes),
        boost::archive::iterators::base64_from_binary<
            boost::archive::iterators::transform_width<const unsigned char *, 6,
                                                       8>>(img_bytes +
                                                           img_data_size));

    img_datum->set_type((annotate_spec::ImageDatum_Format)img.m_format);
    img_datum->set_imgdata(img_base64);

    datum->set_rowindex(start + i);
    i++;
  }

  return data;
}

annotate_spec::Annotations ObjectDetection::getAnnotations(size_t start,
                                                           size_t end) {
  annotate_spec::Annotations annotations;

  gl_sframe gl_data(m_data);

  gl_sframe filtered_data =
      gl_data[{static_cast<long long>(start), static_cast<long long>(end)}];

  gl_sarray filtered_images = filtered_data[m_annotation_column].dropna();

  DASSERT_EQ(filtered_images.dtype(), flex_type_enum::LIST);

  size_t i = 0;
  for (const auto &image : filtered_images.range_iterator()) {
    if (image.get_type() == flex_type_enum::UNDEFINED) {
      continue;
    }

    annotate_spec::Annotation *annotation = annotations.add_annotation();
    const flex_list &vectors = image.get<flex_list>();

    for (size_t i = 0; i < vectors.size(); ++i) {
      annotate_spec::Label *label = annotation->add_labels();

      flex_dict object_detection_label = vectors.at(i).get<flex_dict>();

      for (const auto &pair : object_detection_label) {
        const auto &dict_key = pair.first;
        const auto &dict_value = pair.second;
        if (std::string(dict_key) == "coordinates") {
          annotate_spec::ObjectDetectionLabel *od_label =
              label->mutable_objectdetectionlabel();

          flex_dict od_box = dict_value.get<flex_dict>();
          for (const auto &box : od_box) {
            const auto &box_key = box.first;
            const auto &box_value = box.second;
            if (std::string(box_key) == "height") {
              od_label->set_height(box_value.get<flex_float>());
            }

            if (std::string(box_key) == "width") {
              od_label->set_width(box_value.get<flex_float>());
            }

            if (std::string(box_key) == "x") {
              od_label->set_x(box_value.get<flex_float>());
            }

            if (std::string(box_key) == "y") {
              od_label->set_y(box_value.get<flex_float>());
            }
          }
        }
        if (std::string(dict_key) == "label") {
          if (dict_value.get_type() == flex_type_enum::STRING) {
            label->set_stringlabel(dict_value.get<flex_string>());
          } else if (dict_value.get_type() == flex_type_enum::INTEGER) {
            label->set_intlabel(dict_value.get<flex_int>());
          }
        }
      }
    }

    annotation->add_rowindex(start + i);
    i++;
  }

  return annotations;
}

bool ObjectDetection::setAnnotations(
    const annotate_spec::Annotations &annotations) {

  // TODO: implement `setAnnotations`
  return true;
}

void ObjectDetection::cast_annotations() {
  // TODO: implement `cast_annotations` used with Image Saliency for OD
}

void ObjectDetection::background_work() {
  // TODO: implement `background_work` used with Image Saliency for OD
}

} // namespace annotate
} // namespace turi