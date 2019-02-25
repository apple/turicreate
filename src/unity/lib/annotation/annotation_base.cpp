#include <unity/lib/visualization/process_wrapper.hpp>
#include <unity/lib/visualization/thread.hpp>

#include <unity/lib/annotation/annotation_base.hpp>
#include <logger/assertions.hpp>

namespace turi {
namespace annotate {

AnnotationBase::AnnotationBase(const std::shared_ptr<unity_sframe> &data,
                               const std::vector<std::string> &data_columns,
                               const std::string &annotation_column)
    : m_data(data), m_data_columns(data_columns),
      m_annotation_column(annotation_column) {

  /* Copy as so not to mutate the sframe passed into the function */
  m_data = std::static_pointer_cast<unity_sframe>(
      m_data->copy_range(0, 1, m_data->size()));

  this->_addAnnotationColumn();
  this->_addIndexColumn();
  this->_checkDataSet();
}

void AnnotationBase::annotate(const std::string &path_to_client) {
  visualization::process_wrapper aw(path_to_client);
  /* TODO: Easier to Implement when working on the Front End. Hold off
   * implementation until front end PR. Will be more clear what to implement
   * there */
  while (aw.good()) {
    break;
  }
}

std::shared_ptr<unity_sframe>
AnnotationBase::returnAnnotations(bool drop_null) {
  std::shared_ptr<unity_sframe> copy_data =
      std::static_pointer_cast<unity_sframe>(
          m_data->copy_range(0, 1, m_data->size()));

  size_t id_column = copy_data->column_index("__idx");
  copy_data->remove_column(id_column);

  std::shared_ptr<annotation_global> annotation_global =
      this->get_annotation_registry();

  if (!drop_null) {
    annotation_global->annotation_sframe = copy_data;
    return copy_data;
  }

  std::vector<std::string> annotation_column_name = {m_annotation_column};
  std::list<std::shared_ptr<unity_sframe_base>> dropped_missing =
      copy_data->drop_missing_values(annotation_column_name, false, false);

  std::shared_ptr<unity_sframe> final_sf =
      std::static_pointer_cast<unity_sframe>(dropped_missing.front());

  annotation_global->annotation_sframe = final_sf;

  return final_sf;
}

std::shared_ptr<annotation_global> AnnotationBase::get_annotation_registry() {
  static std::shared_ptr<annotation_global> value =
      std::make_shared<annotation_global>();
  return value;
}

size_t AnnotationBase::size() { return m_data->size(); }

void AnnotationBase::_addAnnotationColumn() {
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

void AnnotationBase::_addIndexColumn() {
  std::vector<flexible_type> indicies;

  for (size_t x = 0; x < m_data->size(); x++) {
    indicies.push_back(x);
  }

  std::shared_ptr<unity_sarray> empty_annotation_sarray =
      std::make_shared<unity_sarray>();

  empty_annotation_sarray->construct_from_vector(indicies,
                                                 flex_type_enum::INTEGER);

  m_data->add_column(empty_annotation_sarray, "__idx");
}

void AnnotationBase::_checkDataSet() {

  size_t image_column_index = m_data->column_index(m_data_columns.at(0));
  flex_type_enum image_column_dtype = m_data->dtype().at(image_column_index);

  DASSERT_EQ(image_column_dtype, flex_type_enum::IMAGE);

  size_t annotation_column_index = m_data->column_index(m_annotation_column);
  flex_type_enum annotation_column_dtype =
      m_data->dtype().at(annotation_column_index);

  DASSERT_TRUE(annotation_column_dtype == flex_type_enum::STRING ||
               annotation_column_dtype == flex_type_enum::INTEGER);

#pragma unused(image_column_index, image_column_dtype)
#pragma unused(annotation_column_index, annotation_column_dtype)
}

void AnnotationBase::_reshapeIndicies(size_t &start, size_t &end) {
  size_t data_size = this->size();

  if (start > end) {
    size_t intermediate_store = start;
    start = end;
    end = intermediate_store;
  }

  if (start > data_size) {
    start = data_size;
  }

  if (start < 0) {
    start = 0;
  }

  if (end < 0) {
    end = 0;
  }

  if (start >= data_size) {
    start = data_size;
  }

  if (end >= data_size) {
    end = data_size - 1;
  }
}

} // namespace annotate
} // namespace turi
