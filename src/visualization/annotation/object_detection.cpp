#include <functional>

#include <visualization/annotation/object_detection.hpp>
#include <visualization/annotation/utils.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <core/storage/sframe_data/group_aggregate_value.hpp>
#include <core/storage/sframe_data/groupby_aggregate_operators.hpp>

#include <core/data/sframe/gl_sframe.hpp>
#include <model_server/lib/image_util.hpp>
#include <core/storage/sframe_interface/unity_sarray_builder.hpp>

namespace turi {
namespace annotate {

ObjectDetection::ObjectDetection(const std::shared_ptr<unity_sframe> &data,
                                 const std::vector<std::string> &data_columns,
                                 const std::string &annotation_column)
    : AnnotationBase(data, data_columns, annotation_column) {
  this->addAnnotationColumn();
  this->checkDataSet();
}

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
  gl_sframe labels({{"labels", unpacked_annotations["X.label"].dropna()}});
  gl_sframe label_counts = labels.groupby({"labels"}, {{"count", aggregate::COUNT()}});
  flex_type_enum array_type = labels["labels"].dtype();

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
  bool error = true;
  for (int a_idx = 0; a_idx < annotations.annotation_size(); a_idx++) {
    annotate_spec::Annotation annotation = annotations.annotation(a_idx);

    if (annotation.labels_size() < 1) {
      std::cerr << "No Labels present in the Annotation" << std::endl;
      error = false;
      continue;
    }

    size_t sf_idx = annotation.rowindex(0);
    if (sf_idx >= m_data->size()) {
      std::cerr << "Out of range error: Annotation rowIndex exceeds the "
                   "acceptable range"
                << std::endl;
      error = false;
      continue;
    }

    flex_list annotation_list;
    for (int l_idx = 0; l_idx < annotation.labels_size(); l_idx++) {
      annotate_spec::Label label = annotation.labels(l_idx);
      switch (label.labelIdentifier_case()) {
      case annotate_spec::Label::LabelIdentifierCase::kIntLabel:
        switch (label.labelType_case()) {
        case annotate_spec::Label::LabelTypeCase::kObjectDetectionLabel: {
          flex_dict bounds = _parse_bounding_boxes(label);
          int tag = label.intlabel();

          flex_dict annotation_dict = {{"label", tag}, {"coordinates", bounds}};

          annotation_list.push_back(annotation_dict);
        } break;
        default:
          std::cerr << "Unexpected label type type. Expected `ObjectDetection`"
                    << std::endl;
          error = false;
        }
        break;
      case annotate_spec::Label::LabelIdentifierCase::kStringLabel:
        switch (label.labelType_case()) {
        case annotate_spec::Label::LabelTypeCase::kObjectDetectionLabel: {
          flex_dict bounds = _parse_bounding_boxes(label);
          std::string tag = label.stringlabel();

          flex_dict annotation_dict = {{"label", tag}, {"coordinates", bounds}};

          annotation_list.push_back(annotation_dict);
        } break;
        default:
          std::cerr << "Unexpected label type type. Expected `ObjectDetection`"
                    << std::endl;
          error = false;
        }
        break;
      default:
        std::cerr
            << "Unexpected label identifier type. Expected INTEGER or STRING."
            << std::endl;
        error = false;
      }
    }

    _addAnnotationToSFrame(sf_idx, annotation_list);
  }

  return error;
}

void ObjectDetection::addAnnotationColumn() {
  std::vector<std::string> column_names = m_data->column_names();

  if (m_annotation_column == "") {
    m_annotation_column = "annotations";
  }

  if (!(std::find(column_names.begin(), column_names.end(),
                  m_annotation_column) != column_names.end())) {
    std::shared_ptr<unity_sarray> empty_annotation_sarray =
        std::make_shared<unity_sarray>();

    empty_annotation_sarray->construct_from_const(
        FLEX_UNDEFINED, m_data->size(), flex_type_enum::LIST);

    m_data->add_column(empty_annotation_sarray, m_annotation_column);
  }
}

void ObjectDetection::checkDataSet() {
  size_t image_column_index = m_data->column_index(m_data_columns.at(0));
  flex_type_enum image_column_dtype = m_data->dtype().at(image_column_index);

  if (image_column_dtype != flex_type_enum::IMAGE) {
    std_log_and_throw(std::invalid_argument, "Image column \"" +
                                                 m_data_columns.at(0) +
                                                 "\" not of image type.");
  }

  size_t annotation_column_index = m_data->column_index(m_annotation_column);
  flex_type_enum annotation_column_dtype =
      m_data->dtype().at(annotation_column_index);

  if (!(annotation_column_dtype == flex_type_enum::LIST)) {
    std_log_and_throw(std::invalid_argument,
                      "Annotation column \"" + m_data_columns.at(0) +
                          "\" of type \'" +
                          flex_type_enum_to_name(annotation_column_dtype) +
                          "\' not of 'list' type.");
  }
}

void ObjectDetection::_addAnnotationToSFrame(size_t index, flex_list label) {
  /* Assert that the column type is indeed of type flex_enum::LIST */
  size_t annotation_column_index = m_data->column_index(m_annotation_column);
  DASSERT_EQ(m_data->dtype().at(annotation_column_index), flex_type_enum::LIST);

  std::shared_ptr<unity_sarray> data_sarray =
      std::static_pointer_cast<unity_sarray>(
          m_data->select_column(m_annotation_column));

  m_data->remove_column(annotation_column_index);

  std::shared_ptr<unity_sarray> place_holder = std::make_shared<unity_sarray>();

  place_holder->construct_from_const(label, 1, flex_type_enum::LIST);

  /* if index is not equal to the first index */
  if (index != 0) {
    std::shared_ptr<unity_sarray> top_sarray =
        std::static_pointer_cast<unity_sarray>(
            data_sarray->copy_range(0, 1, index));
    place_holder = std::static_pointer_cast<unity_sarray>(
        top_sarray->append(place_holder));
  }

  /* if index is not equal to the last index */
  if (index != (m_data->size() - 1)) {
    std::shared_ptr<unity_sarray_base> bottom_sarray =
        data_sarray->copy_range((index + 1), 1, m_data->size());
    place_holder = std::static_pointer_cast<unity_sarray>(
        place_holder->append(bottom_sarray));
  }

  /* Assert that the sarray we just created and the sframe are the same size. */
  DASSERT_EQ(place_holder->size(), m_data->size());

  m_data->add_column(place_holder, m_annotation_column);
}

flex_dict ObjectDetection::_parse_bounding_boxes(annotate_spec::Label label) {
  annotate_spec::ObjectDetectionLabel od_label = label.objectdetectionlabel();
  flex_dict bounds = {{"height", od_label.height()},
                      {"width", od_label.width()},
                      {"x", od_label.x()},
                      {"y", od_label.y()}};
  return bounds;
}

void ObjectDetection::cast_annotations() {
  // TODO: implement `cast_annotations` used with Image Saliency for OD
}

void ObjectDetection::background_work() {
  // TODO: implement `background_work` used with Image Saliency for OD
}

annotate_spec::Similarity ObjectDetection::get_similar_items(size_t index,
                                                             size_t k) {
  annotate_spec::Similarity similar;
  // TODO: implement `get_similar_items` used with Image Saliency for OD
  return similar;
}

std::shared_ptr<ObjectDetection>
create_object_detection_annotation(const std::shared_ptr<unity_sframe> &data,
                                   const std::vector<std::string> &data_columns,
                                   const std::string &annotation_column) {
  return std::make_shared<ObjectDetection>(data, data_columns,
                                           annotation_column);
}

} // namespace annotate
} // namespace turi