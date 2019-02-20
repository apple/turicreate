#include "utils.hpp"

namespace annotation_testing {

char *generate_data(size_t data_size) {
  char *data_array = (char *)malloc(data_size * sizeof(char));
  for (size_t x = 0; x < data_size; x++) {
    data_array[x] = static_cast<char>((uint8_t)(rand() % 256));
  }
  return data_array;
}

turi::flex_image *random_image() {
  size_t height = 25;
  size_t width = 25;
  size_t channels = 3;

  size_t data_size = height * width * channels;

  size_t image_type_version = IMAGE_TYPE_CURRENT_VERSION;
  size_t format = 2;

  char *img_data = generate_data(data_size);

  return new turi::image_type(img_data, height, width, channels, data_size,
                              image_type_version, format);
}

turi::flex_string random_string() {
  static const char allowedcharacters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                          "abcdefghijklmnopqrstuvwxyz"
                                          "0123456789";

  std::string randomString("");
  size_t lengthString = rand() % MAX_LENGTH_STRING;

  for (int i = 0; i < lengthString; ++i) {
    randomString += allowedcharacters[rand() % (sizeof(allowedcharacters) - 1)];
  }

  return randomString;
}

std::shared_ptr<turi::unity_sarray> random_image_sarray(size_t length) {
  std::vector<turi::flexible_type> image_column_data;
  for (int x = 0; x < length; x++) {
    image_column_data.push_back(*random_image());
  }

  std::shared_ptr<turi::unity_sarray> sa =
      std::make_shared<turi::unity_sarray>();
  sa->construct_from_vector(image_column_data, turi::flex_type_enum::IMAGE);
  return sa;
}

std::shared_ptr<turi::unity_sarray> random_string_sarray(size_t length) {
  std::vector<turi::flexible_type> annotation_column_data;
  for (int x = 0; x < length; x++) {
    annotation_column_data.push_back(random_string());
  }

  std::shared_ptr<turi::unity_sarray> sa =
      std::make_shared<turi::unity_sarray>();
  sa->construct_from_vector(annotation_column_data,
                            turi::flex_type_enum::STRING);
  return sa;
}

std::shared_ptr<turi::unity_sframe>
random_sframe(size_t length, std::string image_column_name,
              std::string annotation_column_name) {
  std::shared_ptr<turi::unity_sarray> image_sa = random_image_sarray(length);
  std::shared_ptr<turi::unity_sarray> string_sa = random_string_sarray(length);

  std::shared_ptr<turi::unity_sframe> annotate_sf =
      std::make_shared<turi::unity_sframe>();

  annotate_sf->add_column(image_sa, image_column_name);
  annotate_sf->add_column(string_sa, annotation_column_name);

  return annotate_sf;
}

bool check_equality(std::shared_ptr<turi::unity_sframe> first,
                    std::shared_ptr<turi::unity_sframe> second) {

  // TODO: check for equality
  return true;
}

} // namespace annotation_testing