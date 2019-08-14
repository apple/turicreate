#include <visualization/annotation/class_registrations.hpp>

#include <visualization/annotation/annotation_base.hpp>
#include <visualization/annotation/image_classification.hpp>
#include <visualization/annotation/object_detection.hpp>

namespace turi {
namespace annotate {

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(ImageClassification)
REGISTER_CLASS(ObjectDetection)
REGISTER_CLASS(annotation_global)
END_CLASS_REGISTRATION

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(create_image_classification_annotation, "data",
                  "data_columns", "annotation_column");
REGISTER_FUNCTION(create_object_detection_annotation, "data",
                  "data_columns", "annotation_column");
END_FUNCTION_REGISTRATION

} // namespace annotate
} // namespace turi
