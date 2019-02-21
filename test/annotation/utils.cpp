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
  size_t height = ((size_t)(rand() % 10) + 15);
  size_t width = ((size_t)(rand() % 10) + 15);
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

  for (size_t i = 0; i < lengthString; ++i) {
    randomString += allowedcharacters[rand() % (sizeof(allowedcharacters) - 1)];
  }

  return randomString;
}

std::shared_ptr<turi::unity_sarray> random_image_sarray(size_t length) {
  std::vector<turi::flexible_type> image_column_data;
  for (size_t x = 0; x < length; x++) {
    image_column_data.push_back(*random_image());
  }

  std::shared_ptr<turi::unity_sarray> sa =
      std::make_shared<turi::unity_sarray>();
  sa->construct_from_vector(image_column_data, turi::flex_type_enum::IMAGE);
  return sa;
}

std::shared_ptr<turi::unity_sarray> random_string_sarray(size_t length,
                                                         bool fill_na) {
  std::vector<turi::flexible_type> annotation_column_data;
  for (size_t x = 0; x < length; x++) {
    if (rand() % 20 > 15) {
      annotation_column_data.push_back("");
    } else {
      annotation_column_data.push_back(random_string());
    }
  }

  std::shared_ptr<turi::unity_sarray> sa =
      std::make_shared<turi::unity_sarray>();
  sa->construct_from_vector(annotation_column_data,
                            turi::flex_type_enum::STRING);
  return sa;
}

std::shared_ptr<turi::unity_sframe>
random_sframe(size_t length, std::string image_column_name,
              std::string annotation_column_name, bool fill_na) {
  std::shared_ptr<turi::unity_sarray> image_sa = random_image_sarray(length);
  std::shared_ptr<turi::unity_sarray> string_sa =
      random_string_sarray(length, fill_na);

  std::shared_ptr<turi::unity_sframe> annotate_sf =
      std::make_shared<turi::unity_sframe>();

  annotate_sf->add_column(image_sa, image_column_name);
  annotate_sf->add_column(string_sa, annotation_column_name);

  return annotate_sf;
}

bool check_equality(std::shared_ptr<turi::unity_sframe> first,
                    std::shared_ptr<turi::unity_sframe> second) {

  // Step one: check column names are the same
  std::vector<std::string> first_column_names = first->column_names();
  std::vector<std::string> second_column_names = second->column_names();

  for (std::vector<int>::size_type i = 0; i != first_column_names.size(); i++) {
    if (first_column_names[i] != second_column_names[i]) {
      printf("The SFrame column names don't match\n");
      return false;
    }
  }

  // Step two: check both sframes have the same length
  if (first->size() != second->size()) {
    printf("The SFrame's have different sizes\n");
    return false;
  }

  // Step three: check both sframes have the same type
  std::vector<turi::flex_type_enum> first_type = first->dtype();
  std::vector<turi::flex_type_enum> second_type = second->dtype();

  if (first_type.size() != second_type.size()) {
    printf("The SFrame type arrays are different sizes\n");
    return false;
  }

  for (std::vector<int>::size_type i = 0; i != first_type.size(); i++) {
    if (first_type[i] != second_type[i]) {
      printf("The SFrame type's are different\n");
      return false;
    }
  }

  // Step four: check both sarray's are the same
  for (std::vector<int>::size_type i = 0; i != first_column_names.size(); i++) {

    std::shared_ptr<turi::unity_sarray> first_sa =
        std::static_pointer_cast<turi::unity_sarray>(
            first->select_column(first_column_names[i]));

    std::shared_ptr<turi::unity_sarray> second_sa =
        std::static_pointer_cast<turi::unity_sarray>(
            second->select_column(second_column_names[i]));

    std::vector<turi::flexible_type> flex_data_first = first_sa->to_vector();
    std::vector<turi::flexible_type> flex_data_second = second_sa->to_vector();

    if (flex_data_first.size() != flex_data_second.size()) {
      printf("The SArray sizes are different\n");
      return false;
    }
    /**
     * ASSUMPTION
     *
     * Not checking image column elements since we don't modify them. Seems a
     * bit useless to check pixel by pixel if the values haven't changed.
     */
    if ((first_sa->dtype() == turi::flex_type_enum::STRING &&
         second_sa->dtype() == turi::flex_type_enum::STRING) ||
        (first_sa->dtype() == turi::flex_type_enum::INTEGER &&
         second_sa->dtype() == turi::flex_type_enum::INTEGER)) {
      for (std::vector<int>::size_type x = 0; x != flex_data_first.size();
           x++) {
        if (flex_data_first[x] != flex_data_second[x]) {
          printf("The SArray elements are different\n");
          return false;
        }
      }
    }
  }

  return true;
}

} // namespace annotation_testing