#include <visualization/server/thread.hpp>

#include <core/logging/assertions.hpp>

#include <visualization/annotation/annotation_base.hpp>

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
  _addIndexColumn();
}

void AnnotationBase::annotate(const std::string &path_to_client) {
  m_aw = std::make_shared<visualization::process_wrapper>(path_to_client);
  *m_aw << this->_serialize_proto<annotate_spec::MetaData>(this->metaData())
               .c_str();

  while (m_aw->good()) {
    std::string input;
    *m_aw >> input;

    if (input.empty()) {
      this->background_work();
      continue;
    }

    std::string response = this->__parse_proto_and_respond(input).c_str();

    if (!response.empty()) {
      *m_aw << response;
    }
  }
}

void AnnotationBase::_sendProgress(double value) {
  if(m_aw->good()){
    annotate_spec::ProgressMeta progress;
    progress.set_percentage(value);
    *m_aw
        << this->_serialize_proto<annotate_spec::ProgressMeta>(progress).c_str();
  }
}

std::shared_ptr<unity_sframe>
AnnotationBase::returnAnnotations(bool drop_null) {
  cast_annotations();

  std::shared_ptr<unity_sframe> copy_data;
  {
    // append undefined values
    if (m_data_na && m_data_na->size()) {
      auto cast_type = m_data->select_column(m_annotation_column)->dtype();
      auto na_type = m_data_na->select_column(m_annotation_column)->dtype();
      if (cast_type != na_type) {
        auto data_sarray = std::static_pointer_cast<unity_sarray>(
            m_data_na->select_column(m_annotation_column));
        auto integer_annotations =
            data_sarray->astype(cast_type, true);
        m_data_na->remove_column(m_data_na->column_index(m_annotation_column));
        m_data_na->add_column(integer_annotations, m_annotation_column);
      }
      // temporary solution using sort to reconstruct
      copy_data = std::static_pointer_cast<unity_sframe>(
          m_data->append(m_data_na)->sort({"__idx"}, {true}));
    } else {
      copy_data = std::static_pointer_cast<unity_sframe>(
          m_data->copy_range(0, 1, m_data->size()));
    }
  }

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
        copy_data->drop_missing_values(annotation_column_name, false, false,
                                       false);

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

/* must be called after _addIndexColumn */
void AnnotationBase::_splitUndefined(const std::string& column_names, bool how, bool recursive) {
  DASSERT_TRUE(m_data->contains_column("__idx"));
  const auto &image_column_name = m_data_columns.at(0);
  auto split_data_set =
      m_data->drop_missing_values({image_column_name}, how, true, recursive);
  m_data = std::static_pointer_cast<unity_sframe>(split_data_set.front());
  m_data_na = std::static_pointer_cast<unity_sframe>(split_data_set.back());
}

void AnnotationBase::_addIndexColumn() {
  m_data->add_column(unity_sarray::create_sequential_sarray(m_data->size(), 0, false), "__idx");
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
std::string AnnotationBase::_serialize_proto(T message) {
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
      return this->_serialize_proto<annotate_spec::Data>(
          this->getItems(data_getter.start(), data_getter.end()));
    case annotate_spec::DataGetter_GetterType::
        DataGetter_GetterType_ANNOTATIONS:
      return this->_serialize_proto<annotate_spec::Annotations>(
          this->getAnnotations(data_getter.start(), data_getter.end()));
    case annotate_spec::DataGetter_GetterType::DataGetter_GetterType_SIMILARITY:
      return this->_serialize_proto<annotate_spec::Similarity>(
          this->get_similar_items(data_getter.start()));
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
