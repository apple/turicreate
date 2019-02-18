#ifndef __TC_TEST_ANNOTATION_UTILS
#define __TC_TEST_ANNOTATION_UTILS

#include <image/image_type.hpp>

char* generate_data(size_t data_size) {
  char* data_array = (char *)malloc(data_size * sizeof(char));
  
  for (size_t x = 0; x < data_size; x++) {
    data_array[0] = (uint8_t)(rand() % 256)
  }
}

flex_image random_image() {
  size_t height = 25;
  size_t width = 25;
  size_t channels = 3;  

  size_t data_size = height * width * channels;


}

#endif