#ifndef TURI_ANNOTATIONS_ANNOTATION_BASE_HPP
#define TURI_ANNOTATIONS_ANNOTATION_BASE_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <flexible_type/flexible_type.hpp>

#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/unity_sframe.hpp>

#include "build/format/cpp/annotate.pb.h"
#include "build/format/cpp/data.pb.h"

namespace annotate_spec = TuriCreate::Annotation::Specification;

namespace turi {
namespace annotate {

class AnnotationBase {
public:
  AnnotationBase(){};
  AnnotationBase(const std::shared_ptr<unity_sframe> &data,
                 const std::vector<std::string> &data_columns,
                 const std::string &annotation_column = "");

  virtual ~AnnotationBase(){};

  void show(const std::string &path_to_client);

  size_t size();

  virtual annotate_spec::Data getItems(size_t start, size_t end);

  virtual annotate_spec::Annotations getAnnotations(size_t start, size_t end);

  virtual bool setAnnotations(const annotate_spec::Annotations &annotations);

  virtual std::shared_ptr<unity_sframe>
  returnAnnotations(bool drop_null = true);

protected:
  std::shared_ptr<unity_sframe> m_data;
  std::vector<std::string> m_data_columns;
  std::string m_annotation_column;

  void _reshapeIndicies(size_t &start, size_t &end);
};

} // namespace annotate
} // namespace turi

#endif
