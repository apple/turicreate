#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>


#include <unistd.h>

#include <image/image_type.hpp>
#include <unity/lib/image_util.hpp>

using namespace turi;
using namespace turi::image_util;

struct image_util_test {
 public:
  void test_encode_decode() {
    image_type image_raw = make_raw_image(8, 6, 3);
    flexible_type image_wrapped(image_raw);
    {
      // decode raw array should be the same
      flexible_type decoded = decode_image(image_wrapped);
      auto& decoded_image = decoded.get<flex_image>();
      TS_ASSERT_EQUALS(decoded.get_type(), flex_type_enum::IMAGE);
      TS_ASSERT(decoded_image.is_decoded());
      TS_ASSERT_EQUALS(decoded_image.m_width, image_raw.m_width);
      TS_ASSERT_EQUALS(decoded_image.m_height, image_raw.m_height);
      TS_ASSERT_EQUALS(decoded_image.m_channels, image_raw.m_channels);
      TS_ASSERT_EQUALS(decoded_image.m_image_data_size, image_raw.m_image_data_size);
      for (size_t i = 0 ; i < image_raw.m_image_data_size; ++i) {
        TS_ASSERT_EQUALS(decoded_image.get_image_data()[i],
                         image_raw.get_image_data()[i]);
      }
    }

    {
      // encode decode should be lossless
      flexible_type encoded = encode_image(image_wrapped);
      image_type& encoded_image = encoded.mutable_get<flex_image>();
      TS_ASSERT(!encoded_image.is_decoded());

      flexible_type decoded = decode_image(encoded);
      auto& decoded_image = decoded.get<flex_image>();
      TS_ASSERT_EQUALS(decoded.get_type(), flex_type_enum::IMAGE);
      TS_ASSERT(decoded_image.is_decoded());
      TS_ASSERT_EQUALS(decoded_image.m_width, image_raw.m_width);
      TS_ASSERT_EQUALS(decoded_image.m_height, image_raw.m_height);
      TS_ASSERT_EQUALS(decoded_image.m_channels, image_raw.m_channels);
      TS_ASSERT_EQUALS(decoded_image.m_image_data_size, image_raw.m_image_data_size);
      for (size_t i = 0 ; i < image_raw.m_image_data_size; ++i) {
        TS_ASSERT_EQUALS(decoded_image.get_image_data()[i],
                         image_raw.get_image_data()[i]);
      }
    }
  }

  void test_resize() {
    size_t height = 8;
    size_t width = 6;
    size_t channels = 3;

    image_type image_raw = make_raw_image(height, width, channels);
    flexible_type image_wrapped(image_raw);

    // Test upsample
    _test_resize_impl(image_wrapped, height * 2, width * 2, channels, true);
    _test_resize_impl(image_wrapped, height * 2, width * 2, channels, false);
    // Test down sample
    _test_resize_impl(image_wrapped, height * .5, width * .5, channels, true);
    _test_resize_impl(image_wrapped, height * .5, width * .5, channels, false);
    // Test same size
    _test_resize_impl(image_wrapped, height, width, channels, true);
    _test_resize_impl(image_wrapped, height, width, channels, false);

    // Test compressed input
    flexible_type image_encoded = encode_image(image_wrapped);
    // Test upsample
    _test_resize_impl(image_encoded, height * 2, width * 2, channels, true);
    _test_resize_impl(image_encoded, height * 2, width * 2, channels, false);
    // Test down sample
    _test_resize_impl(image_encoded, height * .5, width * .5, channels, true);
    _test_resize_impl(image_encoded, height * .5, width * .5, channels, false);
    // Test same size
    _test_resize_impl(image_wrapped, height, width, channels, true);
    _test_resize_impl(image_wrapped, height, width, channels, false);
  }

  image_type make_raw_image(size_t height, size_t width, size_t channels) {
    int format = (int)(Format::RAW_ARRAY);
    int version = IMAGE_TYPE_CURRENT_VERSION;
    size_t image_data_size = width * height * channels;
    char* buf = new char[image_data_size];
    image_type image_raw(buf, height, width, channels, image_data_size, version, format);
    return image_raw;
  }

  void _test_resize_impl(const flexible_type& image, size_t new_height, size_t new_width, size_t new_channels,
                         bool save_as_decoded) {
    flexible_type resized = resize_image(image, new_width, new_height, new_channels, save_as_decoded);
    const image_type& resized_image = resized.get<flex_image>();
    TS_ASSERT_EQUALS(resized_image.is_decoded(), save_as_decoded);
    TS_ASSERT_EQUALS(resized_image.m_width, new_width);
    TS_ASSERT_EQUALS(resized_image.m_height, new_height);
    TS_ASSERT_EQUALS(resized_image.m_channels, new_channels);
  }
};

BOOST_FIXTURE_TEST_SUITE(_image_util_test, image_util_test)
BOOST_AUTO_TEST_CASE(test_encode_decode) {
  image_util_test::test_encode_decode();
}
BOOST_AUTO_TEST_CASE(test_resize) {
  image_util_test::test_resize();
}
BOOST_AUTO_TEST_SUITE_END()
