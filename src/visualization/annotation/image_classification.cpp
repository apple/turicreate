#include <core/data/sframe/gl_sarray.hpp>

#include <functional>
#include <visualization/annotation/image_classification.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>

#include <boost/range/combine.hpp>

#include <core/storage/sframe_data/groupby_aggregate.hpp>
#include <core/storage/sframe_data/groupby_aggregate_operators.hpp>

#include <model_server/lib/image_util.hpp>
#include <core/storage/sframe_interface/unity_sarray_builder.hpp>

#include "utils.hpp"

namespace turi {
namespace annotate {

ImageClassification::ImageClassification(
    const std::shared_ptr<unity_sframe> &data,
    const std::vector<std::string> &data_columns,
    const std::string &annotation_column)
    : AnnotationBase(data, data_columns, annotation_column) {
  this->addAnnotationColumn();
  this->checkDataSet();
  this->_createFeaturesExtractor();
}

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
    const flexible_type& flex_label = flex_data.at(i);
    if (flex_label.get_type() == flex_type_enum::UNDEFINED) {
      // skip unlabeled items
      continue;
    }

    annotate_spec::Annotation *annotation = annotations.add_annotation();
    annotate_spec::Label *label = annotation->add_labels();

    // initialize the label as an image classification label
    label->mutable_imageclassificationlabel();

    if (flex_label.get_type() == flex_type_enum::STRING) {
      const flex_string& label_value = flex_label.get<flex_string>();
      label->set_stringlabel(label_value);
    } else if (flex_label.get_type() == flex_type_enum::INTEGER) {
      flex_int label_value = flex_label.get<flex_int>();
      label->set_intlabel(label_value);
    }

    annotation->add_rowindex(start + i);
  }

  return annotations;
}

void ImageClassification::background_work() {
#ifdef __APPLE__
  if(m_nn_model.size() == 0) {
    if (!this->_stepFeaturesExtractor()) {
      this->_sendProgress(1);
      this->m_feature_sarray = m_writer->close();
      this->_create_nearest_neighbors_model();
      this->_sendProgress(2);
    }
  }else{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
#endif
}

void ImageClassification::_create_nearest_neighbors_model() {
#ifdef __APPLE__
  std::shared_ptr<unity_sarray> ref_labels =
      std::static_pointer_cast<unity_sarray>(m_data->select_column("__idx"));

  std::map<std::string, gl_sarray> feature_map;
  feature_map["features"] = this->m_feature_sarray;
  gl_sframe feature_sf(feature_map);

  std::vector<std::string> features = {"features"};

  auto fn = function_closure_info();
  fn.native_fn_name = "_distances.euclidean";

  std::vector<nearest_neighbors::dist_component_type> p = {
      std::make_tuple(features, fn, 1.0)};

  variant_map_type opts;

  opts["model_name"] = "nearest_neighbors_ball_tree";
  opts["ref_labels"] = to_variant(ref_labels);
  opts["sf_features"] = to_variant(feature_sf);
  opts["composite_params"] = to_variant(p);

  this->m_nn_model = turi::nearest_neighbors::train(opts);
#endif
}

annotate_spec::Similarity ImageClassification::get_similar_items(size_t index,
                                                                 size_t k) {
  gl_sframe sf({{"features", {this->m_feature_sarray[index]}}});

  const std::vector<flexible_type> query_labels = {0};
  gl_sarray sa(query_labels, flex_type_enum::INTEGER);

  variant_map_type opts;

  opts["model"] = this->m_nn_model["model"];
  opts["model_name"] = "nearest_neighbors";
  opts["features"] = to_variant(sf);
  opts["query_labels"] = to_variant(sa);
  opts["k"] = k;
  opts["radius"] = -1.0;

  auto ret_neighbors = turi::nearest_neighbors::query(opts);
  gl_sframe neighbors = safe_varmap_get<gl_sframe>(ret_neighbors, "neighbors");
  gl_sarray ref_label = neighbors["reference_label"];

  annotate_spec::Similarity similar;
  similar.set_rowindex(index);

  for (const auto &idx : ref_label.range_iterator()) {
    std::shared_ptr<unity_sarray> data_sarray =
        std::static_pointer_cast<unity_sarray>(
            m_data->select_column(m_data_columns.at(0)));

    gl_sarray data_sa(data_sarray);

    flex_image img = data_sa[idx];
    img = turi::image_util::encode_image(img);

    size_t img_width = img.m_width;
    size_t img_height = img.m_height;
    size_t img_channels = img.m_channels;

    annotate_spec::Datum *datum = similar.add_data();
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

    datum->set_rowindex(idx);
  }

  return similar;
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
  if (m_data->dtype().at(annotation_column_index) == flex_type_enum::INTEGER) {
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
    if (!is_integer(label_value)) {
      castable = false;
      break;
    }
  }

  if (castable) {
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

  annotate_spec::ImageClassificationMeta *image_classification_meta =
      meta_data.mutable_image_classification();

  for (size_t x = 0; x < label_vector.size(); x++) {

    const flexible_type& label = label_vector.at(x);
    if (label.get_type() == flex_type_enum::UNDEFINED) {
      // skip unlabeled items
      continue;
    }

    if (array_type == flex_type_enum::STRING) {
      annotate_spec::MetaLabel *labels_meta =
          image_classification_meta->add_label();
      labels_meta->set_stringlabel(label.get<flex_string>());
      labels_meta->set_elementcount(count_vector.at(x).get<flex_int>());
    }

    if (array_type == flex_type_enum::INTEGER) {
      annotate_spec::MetaLabel *labels_meta =
          image_classification_meta->add_label();
      labels_meta->set_intlabel(label.to<flex_int>());
      labels_meta->set_elementcount(count_vector.at(x).get<flex_int>());
    }
  }

  return meta_data;
}

void ImageClassification::addAnnotationColumn() {
  std::vector<std::string> column_names = m_data->column_names();

  if (m_annotation_column == "") {
    m_annotation_column = "annotations";
  }

  if (!(std::find(column_names.begin(), column_names.end(),
                  m_annotation_column) != column_names.end())) {
    std::shared_ptr<unity_sarray> empty_annotation_sarray =
        std::make_shared<unity_sarray>();

    empty_annotation_sarray->construct_from_const(
        FLEX_UNDEFINED, m_data->size(), flex_type_enum::STRING);

    m_data->add_column(empty_annotation_sarray, m_annotation_column);
  }
}

void ImageClassification::checkDataSet() {
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

  if (!(annotation_column_dtype == flex_type_enum::STRING ||
        annotation_column_dtype == flex_type_enum::INTEGER)) {
    std_log_and_throw(std::invalid_argument,
                      "Annotation column \"" + m_data_columns.at(0) +
                          "\" of type \'" +
                          flex_type_enum_to_name(annotation_column_dtype) +
                          "\' not of 'string' or 'integer' type.");
  }
}

void ImageClassification::_createFeaturesExtractor() {
#ifdef __APPLE__
  m_extractor = create_feature_extractor();
  auto data_sarray = std::static_pointer_cast<unity_sarray>(m_data->select_column(m_data_columns.at(0)));
  m_image_feature_extraction_sarray = gl_sarray(data_sarray);
  m_writer = std::make_shared<gl_sarray_writer>(flex_type_enum::VECTOR, 1);
#endif
}

bool ImageClassification::_stepFeaturesExtractor() {
#ifdef __APPLE__
  size_t sa_size = m_image_feature_extraction_sarray.size();
  size_t index = 0;
  size_t endIdx = ((index + this->m_feature_batch_size) > sa_size)
                      ? sa_size
                      : (index + this->m_feature_batch_size);
  auto img_batch = m_image_feature_extraction_sarray[{static_cast<long long>(index),
                                                      static_cast<long long>(endIdx)}];
  auto extracted_features =
      m_extractor.sarray_extract_features(img_batch, false, 6);

  for (const auto &row : extracted_features.range_iterator()) {
    m_writer->write(row, 0);
  }

  this->_sendProgress(1.0 - ((float)m_image_feature_extraction_sarray.size() / m_data->size()));

  // slice off the rows we already featurized
  m_image_feature_extraction_sarray = m_image_feature_extraction_sarray[{static_cast<long long>(endIdx),
                                                                         static_cast<long long>(m_image_feature_extraction_sarray.size())}];

  // if more remain, return true
  return m_image_feature_extraction_sarray.size() > 0;
#endif
  return false;
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
