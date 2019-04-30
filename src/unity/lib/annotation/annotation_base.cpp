#include <unity/lib/visualization/process_wrapper.hpp>
#include <unity/lib/visualization/thread.hpp>

#include <logger/assertions.hpp>

#include <unity/lib/annotation/annotation_base.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <sstream>

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

  aw << this->__serialize_proto<annotate_spec::MetaData>(this->metaData())
            .c_str();

  while (aw.good()) {
    std::string input;
    aw >> input;

    if (input.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    std::string response = this->__parse_proto_and_respond(input).c_str();

    if (!response.empty()) {
      aw << response;
    }
  }
}

std::shared_ptr<unity_sframe>
AnnotationBase::returnAnnotations(bool drop_null) {
  this->cast_annotations();

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
                          "\" not of string or integer type.");
  }
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

  if (start >= data_size) {
    start = data_size;
  }

  if (end >= data_size) {
    end = data_size - 1;
  }
}

template <typename T, typename>
std::string AnnotationBase::__serialize_proto(T message) {
  std::stringstream ss;
  ss << "{\"protobuf\": \"";

  annotate_spec::Parcel parcel;

  populate_parcel<T>(parcel, message);

  std::string proto_intermediary;
  parcel.SerializeToString(&proto_intermediary);
  const char *parcel_data = proto_intermediary.c_str();

  size_t parcel_size = parcel.ByteSizeLong();

  std::copy(
      boost::archive::iterators::base64_from_binary<
          boost::archive::iterators::transform_width<const unsigned char *, 6,
                                                     8>>(parcel_data),
      boost::archive::iterators::base64_from_binary<
          boost::archive::iterators::transform_width<const unsigned char *, 6,
                                                     8>>(parcel_data +
                                                         parcel_size),
      std::ostream_iterator<char>(ss));

  ss << "\"}\n";

  return ss.str();
}

std::string AnnotationBase::__parse_proto_and_respond(std::string &input) {
  std::string result(boost::archive::iterators::transform_width<
                         boost::archive::iterators::binary_from_base64<
                             boost::archive::iterators::remove_whitespace<
                                 std::string::const_iterator>>,
                         8, 6>(input.begin()),
                     boost::archive::iterators::transform_width<
                         boost::archive::iterators::binary_from_base64<
                             boost::archive::iterators::remove_whitespace<
                                 std::string::const_iterator>>,
                         8, 6>(input.end()));

  annotate_spec::ClientRequest request;
  request.ParseFromString(result);

  if (request.has_getter()) {
    annotate_spec::DataGetter data_getter = request.getter();
    switch (data_getter.type()) {
    case annotate_spec::DataGetter_GetterType::DataGetter_GetterType_DATA:
      return this->__serialize_proto<annotate_spec::Data>(
          this->getItems(data_getter.start(), data_getter.end()));
    case annotate_spec::DataGetter_GetterType::
        DataGetter_GetterType_ANNOTATIONS:
      return this->__serialize_proto<annotate_spec::Annotations>(
          this->getAnnotations(data_getter.start(), data_getter.end()));
    default:
      break;
    }
  } else if (request.has_annotations()) {
    this->setAnnotations(request.annotations());
  }
  return "";
}

} // namespace annotate
} // namespace turi
