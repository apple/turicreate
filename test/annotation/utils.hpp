#ifndef __TC_TEST_ANNOTATION_UTILS
#define __TC_TEST_ANNOTATION_UTILS

#include <core/data/image/image_type.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>

#include <core/data/flexible_type/flexible_type.hpp>

#include <core/storage/sframe_interface/unity_sarray.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>

#include <core/data/sframe/gl_sarray.hpp>

namespace annotation_testing {

unsigned const MAX_LENGTH_STRING = 60;

char *generate_data(size_t data_size);
turi::flex_image *random_image();
turi::flex_string random_string();
turi::flex_list random_bounding_box();

std::shared_ptr<turi::unity_sarray> random_image_sarray(size_t length);
std::shared_ptr<turi::unity_sarray> random_string_sarray(size_t length,
                                                         bool fill_na = false);

std::shared_ptr<turi::unity_sarray>
random_bounding_box_sarray(size_t length, bool fill_na = false);

std::shared_ptr<turi::unity_sframe>
random_sframe(size_t length, std::string image_column_name = "image",
              std::string annotation_column_name = "annotation",
              bool fill_na = false);

std::shared_ptr<turi::unity_sframe>
random_od_sframe(size_t length, std::string image_column_name = "image",
                 std::string annotation_column_name = "annotation",
                 bool fill_na = false);

bool check_equality(std::shared_ptr<turi::unity_sframe> first,
                    std::shared_ptr<turi::unity_sframe> second);
} // namespace annotation_testing

#endif