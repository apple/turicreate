#include <unity/lib/visualization/process_wrapper.hpp>
#include <unity/lib/visualization/thread.hpp>

#include <unity/lib/annotation/annotation_base.hpp>

namespace turi {
namespace annotate {

AnnotationBase::AnnotationBase(const std::shared_ptr<unity_sframe> &data,
                               const std::vector<std::string> &data_columns,
                               const std::string &annotation_column)
    : m_data(data), m_data_columns(data_columns),
      m_annotation_column(annotation_column) {
  if (annotation_column == "" || false) {
    /* TODO: if annotation column isn't present create it and fill it with all
   * null values initially. */
  }
}

void AnnotationBase::annotate(const std::string &path_to_client) {
  std::shared_ptr<AnnotationBase> self = this->m_self;
  ::turi::visualization::run_thread([self, path_to_client]() {
    visualization::process_wrapper aw(path_to_client);
    // TODO: handle the messages to and from the client app.
    while (aw.good()) {
      break;
    }
  });
}

size_t AnnotationBase::size() { return m_data->size(); }

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