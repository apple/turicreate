#ifndef TURI_ANNOTATIONS_ANNOTATION_BASE_HPP
#define TURI_ANNOTATIONS_ANNOTATION_BASE_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <export.hpp>

#include <flexible_type/flexible_type.hpp>

#include <unity/lib/extensions/ml_model.hpp>

#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/unity_sframe.hpp>

#include "build/format/cpp/annotate.pb.h"
#include "build/format/cpp/data.pb.h"
#include "build/format/cpp/meta.pb.h"

namespace annotate_spec = TuriCreate::Annotation::Specification;

namespace turi {
namespace annotate {

/**
 *
 * Fallback
 *
 * If the user forgets to assign a return variable in their Python script this
 * global will hold the last annotated sframe
 */
struct EXPORT annotation_global : public ml_model_base {
  std::shared_ptr<unity_sframe> annotation_sframe;

  std::shared_ptr<unity_sframe> get_value() { return annotation_sframe; }

  BEGIN_CLASS_MEMBER_REGISTRATION("annotation_global")
  REGISTER_GETTER("annotation_sframe", annotation_global::get_value)
  END_CLASS_MEMBER_REGISTRATION
};

/**
 * Every annotation backend extends from this class. This forces the annotation
 * api to remain consistent across all implementations. The reason the virtual
 * methods exist rather than a switch statement in the annotate method is to
 * expose this functionality to the capi so that other developers have the
 * ability to tie their own annotations UI's to use this api.
 */
class EXPORT AnnotationBase : public ml_model_base {
public:
  AnnotationBase(){};
  AnnotationBase(const std::shared_ptr<unity_sframe> &data,
                 const std::vector<std::string> &data_columns,
                 const std::string &annotation_column);

  virtual ~AnnotationBase(){};

  void annotate(const std::string &path_to_client);

  size_t size();

  std::shared_ptr<unity_sframe> returnAnnotations(bool drop_null = false);

  std::shared_ptr<annotation_global> get_annotation_registry();

  virtual annotate_spec::MetaData metaData() = 0;

  virtual annotate_spec::Data getItems(size_t start, size_t end) = 0;

  virtual annotate_spec::Annotations getAnnotations(size_t start,
                                                    size_t end) = 0;

  virtual bool
  setAnnotations(const annotate_spec::Annotations &annotations) = 0;

  BEGIN_BASE_CLASS_MEMBER_REGISTRATION()

  IMPORT_BASE_CLASS_REGISTRATION(ml_model_base);

  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("annotate", AnnotationBase::annotate,
                                       "path_to_client");

  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("returnAnnotations",
                                       AnnotationBase::returnAnnotations,
                                       "drop_null");
  register_defaults("returnAnnotations", {{"drop_null", false}});

  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get_annotation_registry",
                                       AnnotationBase::get_annotation_registry);

  // TODO: Figure out how to plumb `::google::protobuf::MessageLite` to variant
  //       type.

  END_CLASS_MEMBER_REGISTRATION

protected:
  std::shared_ptr<unity_sframe> m_data;
  const std::vector<std::string> m_data_columns;
  std::string m_annotation_column;

  void _addAnnotationColumn();
  void _addIndexColumn();
  void _checkDataSet();
  void _reshapeIndicies(size_t &start, size_t &end);
};

} // namespace annotate
} // namespace turi

#endif
