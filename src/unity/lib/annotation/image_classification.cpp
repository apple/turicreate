#include <unity/lib/gl_sarray.hpp>

#include <functional>
#include <unity/lib/annotation/image_classification.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <boost/range/combine.hpp>

#include <sframe/groupby_aggregate.hpp>
#include <sframe/groupby_aggregate_operators.hpp>

#include <unity/lib/image_util.hpp>

#include "utils.hpp"

namespace turi {
namespace annotate {

ImageClassification::ImageClassification(
    const std::shared_ptr<unity_sframe> &data,
    const std::vector<std::string> &data_columns,
    const std::string &annotation_column)
    : AnnotationBase(data, data_columns, annotation_column) {}

annotate_spec::Data ImageClassification::getItems(size_t start, size_t end) {
  annotate_spec::Data data;

  std::shared_ptr<unity_sarray> filtered_data =
      this->_filterDataSFrame(start, end);

  DASSERT_EQ(filtered_data->dtype(), flex_type_enum::IMAGE);

  std::vector<flexible_type> flex_data = filtered_data->to_vector();

  for (size_t i = 0; i < flex_data.size(); i++) {
    flex_image img = flex_data.at(i).get<flex_image>();
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
  }

  return data;
}

annotate_spec::Annotations ImageClassification::getAnnotations(size_t start,
                                                               size_t end) {
  annotate_spec::Annotations annotations;

  std::shared_ptr<unity_sarray> filtered_data =
      this->_filterAnnotationSFrame(start, end);

  DASSERT_TRUE((filtered_data->dtype() == flex_type_enum::STRING) ||
               (filtered_data->dtype() == flex_type_enum::INTEGER));

  std::vector<flexible_type> flex_data = filtered_data->to_vector();

  for (size_t i = 0; i < flex_data.size(); i++) {
    annotate_spec::Annotation *annotation = annotations.add_annotation();
    annotate_spec::Label *label = annotation->add_labels();

    label->imageclassificationlabel();

    if (flex_data.at(i).get_type() == flex_type_enum::STRING) {
      std::string label_value = flex_data.at(i).get<flex_string>();
      label->set_stringlabel(label_value);
    } else if (flex_data.at(i).get_type() == flex_type_enum::INTEGER) {
      int label_value = flex_data.at(i);
      label->set_intlabel(label_value);
    }

    annotation->add_rowindex(start + i);
  }

  return annotations;
}

bool ImageClassification::setAnnotations(
    const annotate_spec::Annotations &annotations) {

  /* For Image Classification a number of assumptions are made.
   *
   * - There can only be one label per image.
   * - There can only be one image per label.
   *
   * (Note: the future we may support multi-class labeling, multiple images per
   * label and this design supports it. To enable this feature refactor this
   * code.) */

  bool error = true;
  for (int a_idx = 0; a_idx < annotations.annotation_size(); a_idx++) {

    annotate_spec::Annotation annotation = annotations.annotation(a_idx);

    if (annotation.labels_size() < 1) {
      std::cerr << "No Labels present in the Annotation" << std::endl;
      error = false;
      continue;
    }

    annotate_spec::Label label = annotation.labels(0);
    size_t sf_idx = annotation.rowindex(0);

    if (sf_idx >= m_data->size()) {
      std::cerr << "Out of range error: Annotation rowIndex exceeds the "
                   "acceptable range"
                << std::endl;
      error = false;
      continue;
    }

    switch (label.labelIdentifier_case()) {
    case annotate_spec::Label::LabelIdentifierCase::kIntLabel:
      _addAnnotationToSFrame(sf_idx, label.intlabel());
      break;
    case annotate_spec::Label::LabelIdentifierCase::kStringLabel:
      _addAnnotationToSFrame(sf_idx, label.stringlabel());
      break;
    default:
      std::cerr << "Unexpected label type type. Expected INTEGER or STRING."
                << std::endl;
      error = false;
    }
  }

  m_data->materialize();

  return error;
}

void ImageClassification::_addAnnotationToSFrame(size_t index,
                                                 std::string label) {
  /* Assert that the column type is indeed of type flex_enum::STRING */
  size_t annotation_column_index = m_data->column_index(m_annotation_column);
  DASSERT_EQ(m_data->dtype().at(annotation_column_index),
             flex_type_enum::STRING);

  std::shared_ptr<unity_sarray> data_sarray =
      std::static_pointer_cast<unity_sarray>(
          m_data->select_column(m_annotation_column));

  m_data->remove_column(annotation_column_index);

  std::shared_ptr<unity_sarray> place_holder = std::make_shared<unity_sarray>();

  place_holder->construct_from_const(label, 1, flex_type_enum::STRING);

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

void ImageClassification::_addAnnotationToSFrame(size_t index, int label) {
  /* Assert that the column type is indeed of type flex_enum::INTEGER */
  size_t annotation_column_index = m_data->column_index(m_annotation_column);
  DASSERT_EQ(m_data->dtype().at(annotation_column_index),
             flex_type_enum::INTEGER);

  std::shared_ptr<unity_sarray> data_sarray =
      std::static_pointer_cast<unity_sarray>(
          m_data->select_column(m_annotation_column));

  m_data->remove_column(annotation_column_index);

  std::shared_ptr<unity_sarray> place_holder = std::make_shared<unity_sarray>();

  place_holder->construct_from_const(label, 1, flex_type_enum::INTEGER);

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

void ImageClassification::cast_annotations() {
  size_t annotation_column_index = m_data->column_index(m_annotation_column);
  if(m_data->dtype().at(annotation_column_index) == flex_type_enum::INTEGER) {
    return;
  }

  std::shared_ptr<unity_sframe> copy_data =
      std::static_pointer_cast<unity_sframe>(
          m_data->copy_range(0, 1, m_data->size()));

  std::vector<std::string> annotation_column_name = {m_annotation_column};
  std::list<std::shared_ptr<unity_sframe_base>> dropped_missing =
      copy_data->drop_missing_values(annotation_column_name, false, false);

  std::shared_ptr<unity_sframe> filtered_sframe =
      std::static_pointer_cast<unity_sframe>(dropped_missing.front());

  std::shared_ptr<unity_sarray> data_sarray =
      std::static_pointer_cast<unity_sarray>(
          filtered_sframe->select_column(m_annotation_column));

  std::vector<flexible_type> flex_data = data_sarray->to_vector();

  bool castable = true;
  for (size_t i = 0; i < flex_data.size(); i++) {
    std::string label_value = flex_data.at(i).get<flex_string>();
    if(!isInteger(label_value)){
      castable = false;
      break;
    }
  }

  if(castable){
    std::shared_ptr<unity_sarray> data_sarray =
      std::static_pointer_cast<unity_sarray>(
          m_data->select_column(m_annotation_column));

    std::shared_ptr<unity_sarray_base> integer_annotations =
        data_sarray->astype(flex_type_enum::INTEGER, true);

    m_data->remove_column(annotation_column_index);
    m_data->add_column(integer_annotations, m_annotation_column);
  }
}

annotate_spec::MetaData ImageClassification::metaData() {
  annotate_spec::MetaData meta_data;

  meta_data.set_type(annotate_spec::MetaData_AnnotationType::
                         MetaData_AnnotationType_IMAGE_CLASSIFICATION);

  meta_data.set_num_examples(m_data->size());

  std::shared_ptr<unity_sarray> data_sarray =
      std::static_pointer_cast<unity_sarray>(
          m_data->select_column(m_annotation_column));

  gl_sarray in(data_sarray);
  std::shared_ptr<unity_sarray> unity_sa =
      std::shared_ptr<unity_sarray>(in.unique());

  std::shared_ptr<unity_sframe> count_sf =
      std::static_pointer_cast<unity_sframe>(m_data->groupby_aggregate(
          {m_annotation_column}, {{}}, {"__count"}, {"__builtin__count__"}));

  std::shared_ptr<unity_sarray> label_sa =
      std::static_pointer_cast<unity_sarray>(
          count_sf->select_column(m_annotation_column));

  std::shared_ptr<unity_sarray> count_sa =
      std::static_pointer_cast<unity_sarray>(
          count_sf->select_column("__count"));

  flex_type_enum array_type = label_sa->dtype();

  DASSERT_TRUE(array_type == flex_type_enum::STRING ||
               array_type == flex_type_enum::INTEGER);

  auto label_vector = label_sa->to_vector();
  auto count_vector = count_sa->to_vector();

  DASSERT_TRUE(label_vector.size() == count_vector.size());

  annotate_spec::ImageClassificationMeta* image_classification_meta =
      meta_data.mutable_image_classification();

  for (size_t x = 0; x < label_vector.size(); x++) {

    if (array_type == flex_type_enum::STRING) {
      annotate_spec::MetaLabel *labels_meta =
          image_classification_meta->add_label();
      labels_meta->set_stringlabel(label_vector.at(x).to<std::string>());
      labels_meta->set_elementcount(count_vector.at(x));
    }

    if (array_type == flex_type_enum::INTEGER) {
      annotate_spec::MetaLabel *labels_meta =
          image_classification_meta->add_label();
      labels_meta->set_intlabel(label_vector.at(x));
      labels_meta->set_elementcount(count_vector.at(x));
    }
  }

  return meta_data;
}

std::shared_ptr<unity_sarray>
ImageClassification::_filterDataSFrame(size_t &start, size_t &end) {
  this->_reshapeIndicies(start, end);

  /* ASSUMPTION
   *
   * Assume for Image Classification that there's only one column in the
   * annotation and that the column is of Type Image */

  std::shared_ptr<unity_sarray> data_sarray =
      std::static_pointer_cast<unity_sarray>(
          m_data->select_column(m_data_columns.at(0)));

  return std::static_pointer_cast<unity_sarray>(
      data_sarray->copy_range(start, 1, end));
}

std::shared_ptr<unity_sarray>
ImageClassification::_filterAnnotationSFrame(size_t &start, size_t &end) {
  this->_reshapeIndicies(start, end);

  std::shared_ptr<unity_sarray> data_sarray =
      std::static_pointer_cast<unity_sarray>(
          m_data->select_column(m_annotation_column));

  return std::static_pointer_cast<unity_sarray>(
      data_sarray->copy_range(start, 1, end));
}

std::shared_ptr<ImageClassification> create_image_classification_annotation(
    const std::shared_ptr<unity_sframe> &data,
    const std::vector<std::string> &data_columns,
    const std::string &annotation_column) {
  return std::make_shared<ImageClassification>(data, data_columns,
                                               annotation_column);
}

} // namespace annotate
} // namespace turi
