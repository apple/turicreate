/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE image_util
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>


#include <unistd.h>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <fileio/fs_utils.hpp>
#include <fileio/temp_files.hpp>
#include <image/image_type.hpp>
#include <image/io.hpp>
#include <unity/lib/image_util.hpp>

using namespace turi;
using namespace turi::image_util;

namespace {

// Typedef for a callback that takes an image and a path.
using image_row_handler =
    std::function<void(const image_type& img, const std::string& path)>;

struct image_descriptor {
  size_t height;
  size_t width;
  size_t channels;
  Format format;
};

image_type make_raw_image(size_t height, size_t width, size_t channels) {
  int format = static_cast<int>(Format::RAW_ARRAY);
  int version = IMAGE_TYPE_CURRENT_VERSION;
  size_t image_data_size = width * height * channels;
  std::unique_ptr<char[]> buf(new char[image_data_size]);
  image_type image_raw(buf.get(), height, width, channels, image_data_size,
                       version, format);
  return image_raw;
}

void write_test_images(
    const std::map<std::string, image_descriptor>& descriptors_by_path) {

  for (const auto& path_and_descriptor : descriptors_by_path) {
    const std::string& path = path_and_descriptor.first;
    const image_descriptor& desc = path_and_descriptor.second;
    const image_type test_image =
        make_raw_image(desc.height, desc.width, desc.channels);
    write_image(path, test_image.m_image_data.get(), desc.width, desc.height,
                desc.channels, desc.format);
  }
}

template <class T>
std::set<std::string> get_keys(const std::map<std::string, T>& map) {

  std::set<std::string> keys;
  for (const auto& key_value : map) {
    keys.insert(key_value.first);
  }
  return keys;
}

void enumerate_rows(const std::shared_ptr<unity_sframe>& sf,
                    const image_row_handler& row_handler) {

  // Determine layout of each row.
  const size_t path_column_index = sf->column_index("path");
  const size_t image_column_index = sf->column_index("image");

  // Iterate through the SFrame.
  sf->begin_iterator();
  const std::vector<std::vector<flexible_type>> rows =
      sf->iterator_get_next(sf->size());
  for (const std::vector<flexible_type>& row : rows) {
    const std::string& path = row[path_column_index].to<flex_string>();
    const image_type& img = row[image_column_index].to<flex_image>();
    row_handler(img, path);
  }
}

void _test_resize_impl(
    const flexible_type& image, size_t new_height, size_t new_width,
    size_t new_channels, bool save_as_decoded) {

  flexible_type resized = resize_image(image, new_width, new_height,
                                       new_channels, save_as_decoded);
  const image_type& resized_image = resized.get<flex_image>();

  TS_ASSERT_EQUALS(resized_image.is_decoded(), save_as_decoded);
  TS_ASSERT_EQUALS(resized_image.m_width, new_width);
  TS_ASSERT_EQUALS(resized_image.m_height, new_height);
  TS_ASSERT_EQUALS(resized_image.m_channels, new_channels);

  // other parts of the code depend on the output being specifically
  // encoded in PNG format when resized with decode=False
  if (!save_as_decoded) {
    TS_ASSERT_EQUALS(static_cast<size_t>(resized_image.m_format), static_cast<size_t>(Format::PNG));
  }
}

}  // namespace

BOOST_AUTO_TEST_CASE(test_encode_decode) {
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

BOOST_AUTO_TEST_CASE(test_resize) {
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

BOOST_AUTO_TEST_CASE(test_load_images) {

  // Create a new temporary directory and an image subdirectory, to exercise
  // recursive directory traversal.
  const std::string temp_dir = get_temp_name();
  const std::string image_dir = temp_dir + "/images";
  TS_ASSERT(fileio::create_directory_or_throw(image_dir));

  // Define the images we'll create and then load, mapping paths to
  // {height, width, channels, format}, and including each supported extension.
  std::map<std::string, image_descriptor> descriptors_by_path;
  descriptors_by_path[image_dir + "/image.jpg"] = {10, 20, 3, Format::JPG};
  descriptors_by_path[image_dir + "/image.JPEG"] = {20, 30, 3, Format::JPG};
  descriptors_by_path[image_dir + "/image.png"] = {30, 40, 3, Format::PNG};

  // For each image descriptor, write an arbitrary image into the temporary
  // directory.
  write_test_images(descriptors_by_path);

  // Write some non-image files. The load_images call below should ignore these.
  {
    general_ofstream ds_store_stream(image_dir + "/.DS_Store");
    ds_store_stream << "Not an image.\n";
  }

  // Invoke load_images on the temporary directory. This call uses the default
  // options, except it uses ignore_failure = false so that an attempt to load
  // the non-image file above would throw an exception.
  auto sf = load_images(temp_dir, /* format */ "auto", /* with_path */ true,
                        /* recursive */ true, /* ignore_failure */ false,
                        /* random_order */ false);

  // Iterate through the resulting SFrame...
  std::set<std::string> loaded_paths;
  enumerate_rows(sf, [&](const image_type& img, const std::string& path) {
    // Keep track of which paths were actually loaded.
    loaded_paths.insert(path);

    // Check that each loaded image matches the descriptor written earlier.
    const auto desc_iter = descriptors_by_path.find(path);
    if (desc_iter == descriptors_by_path.end()) {
      TS_FAIL(std::string("Unexpected loaded path: ") + path);
    } else {
      TS_ASSERT_EQUALS(img.m_height, desc_iter->second.height);
      TS_ASSERT_EQUALS(img.m_width, desc_iter->second.width);
      TS_ASSERT_EQUALS(img.m_channels, desc_iter->second.channels);
      TS_ASSERT_EQUALS(static_cast<size_t>(img.m_format),
                       static_cast<size_t>(desc_iter->second.format));
    }
  });

  // Verify that all the written images were found.
  TS_ASSERT_EQUALS(get_keys(descriptors_by_path), loaded_paths);
  TS_ASSERT_EQUALS(descriptors_by_path.size(), sf->size());

  // Clean up.
  TS_ASSERT(fileio::delete_path_recursive(temp_dir));
}

BOOST_AUTO_TEST_CASE(test_load_images_with_nonexistent_file) {

  // Create a new temporary directory.
  const std::string temp_dir = get_temp_name();
  TS_ASSERT(fileio::create_directory_or_throw(temp_dir));

  TS_ASSERT_THROWS_ANYTHING(load_images(
      temp_dir + "/notfound", /* format */ "auto", /* with_path */ true,
      /* recursive */ true, /* ignore_failure */ true,
      /* random_order */ false));

  // Clean up.
  TS_ASSERT(fileio::delete_path_recursive(temp_dir));
}

BOOST_AUTO_TEST_CASE(test_load_images_with_unsupported_file) {

  // Create a new temporary directory.
  const std::string temp_dir = get_temp_name();
  TS_ASSERT(fileio::create_directory_or_throw(temp_dir));

  // Write some non-image file.
  const std::string path = temp_dir + "/image.unsupported";
  {
    general_ofstream ds_store_stream(path);
    ds_store_stream << "Not an image.\n";
  }

  // Loading with ignore_failure = true returns an empty SFrame.
  auto sf = load_images(
      path, /* format */ "auto", /* with_path */ true, /* recursive */ true,
      /* ignore_failure */ true, /* random_order */ false);
  TS_ASSERT_EQUALS(sf->size(), 0);

  // Loading with ignore_failure = false throws.
  TS_ASSERT_THROWS_ANYTHING(load_images(
      path, /* format */ "auto", /* with_path */ true, /* recursive */ true,
      /* ignore_failure */ false, /* random_order */ false));

  // Clean up.
  TS_ASSERT(fileio::delete_path_recursive(temp_dir));
}

BOOST_AUTO_TEST_CASE(test_load_images_with_specified_format) {

  // Create a new temporary directory.
  const std::string temp_dir = get_temp_name();
  TS_ASSERT(fileio::create_directory_or_throw(temp_dir));

  // Define the images we'll create and then load, mapping paths to
  // {height, width, channels, format}, and including each supported extension.
  // Note that the extensions here are unimportant, since we specify format
  // explicitly.
  std::map<std::string, image_descriptor> descriptors_by_path;
  descriptors_by_path[temp_dir + "/image.jpg"] = {10, 20, 3, Format::JPG};
  descriptors_by_path[temp_dir + "/image.png"] = {20, 30, 3, Format::JPG};
  descriptors_by_path[temp_dir + "/.DS_Store"] = {30, 40, 3, Format::JPG};

  // For each image descriptor, write an arbitrary image into the temporary
  // directory.
  write_test_images(descriptors_by_path);

  // Invoke load_images on the temporary directory.
  auto sf = load_images(temp_dir, /* format */ "JPG", /* with_path */ true,
                        /* recursive */ true, /* ignore_failure */ false,
                        /* random_order */ false);

  // Iterate through the resulting SFrame...
  std::set<std::string> loaded_paths;
  enumerate_rows(sf, [&](const image_type& img, const std::string& path) {
    // Keep track of which paths were actually loaded.
    loaded_paths.insert(path);

    // Check that each loaded image matches the descriptor written earlier.
    const auto desc_iter = descriptors_by_path.find(path);
    if (desc_iter == descriptors_by_path.end()) {
      TS_FAIL(std::string("Unexpected loaded path: ") + path);
    } else {
      TS_ASSERT_EQUALS(img.m_height, desc_iter->second.height);
      TS_ASSERT_EQUALS(img.m_width, desc_iter->second.width);
      TS_ASSERT_EQUALS(img.m_channels, desc_iter->second.channels);
      TS_ASSERT_EQUALS(static_cast<size_t>(img.m_format),
                       static_cast<size_t>(desc_iter->second.format));
    }
  });

  // Verify that all the written images were found.
  TS_ASSERT_EQUALS(get_keys(descriptors_by_path), loaded_paths);
  TS_ASSERT_EQUALS(descriptors_by_path.size(), sf->size());

  // Clean up.
  TS_ASSERT(fileio::delete_path_recursive(temp_dir));
}

BOOST_AUTO_TEST_CASE(test_load_images_without_paths) {

  // Create a new temporary directory.
  const std::string temp_dir = get_temp_name();
  TS_ASSERT(fileio::create_directory_or_throw(temp_dir));

  // Define the images we'll create and then load, mapping paths to
  // {height, width, channels, format}, and including each supported extension.
  std::map<std::string, image_descriptor> descriptors_by_path;
  descriptors_by_path[temp_dir + "/image.jpg"] = {10, 20, 3, Format::JPG};
  descriptors_by_path[temp_dir + "/image.png"] = {20, 30, 3, Format::PNG};

  // For each image descriptor, write an arbitrary image into the temporary
  // directory.
  write_test_images(descriptors_by_path);

  // Invoke load_images on the temporary directory.
  auto sf = load_images(temp_dir, /* format */ "auto", /* with_path */ false,
                        /* recursive */ true, /* ignore_failure */ true,
                        /* random_order */ false);

  TS_ASSERT_THROWS_ANYTHING(sf->column_index("path"));

  // Clean up.
  TS_ASSERT(fileio::delete_path_recursive(temp_dir));
}

BOOST_AUTO_TEST_CASE(test_load_images_nonrecursive) {

  // Create a new temporary directory and an image subdirectory, to exercise
  // recursive directory traversal.
  const std::string temp_dir = get_temp_name();
  const std::string image_dir = temp_dir + "/images";
  TS_ASSERT(fileio::create_directory_or_throw(image_dir));

  // Define the images we'll create and then load, mapping paths to
  // {height, width, channels, format}, and including each supported extension.
  std::map<std::string, image_descriptor> descriptors_by_path;
  descriptors_by_path[temp_dir + "/image.jpg"] = {10, 20, 3, Format::JPG};
  descriptors_by_path[temp_dir + "/image.png"] = {20, 30, 3, Format::PNG};

  // Save off the paths written to temp_dir. Below we'll add some more images to
  // the image_dir subdirectory.
  const std::set<std::string> top_level_images = get_keys(descriptors_by_path);

  descriptors_by_path[image_dir + "/image.jpg"] = {30, 40, 3, Format::JPG};

  // For each image descriptor, write an arbitrary image into the temporary
  // directory.
  write_test_images(descriptors_by_path);

  // Invoke load_images on the temporary directory.
  auto sf = load_images(temp_dir, /* format */ "auto", /* with_path */ true,
                        /* recursive */ false, /* ignore_failure */ false,
                        /* random_order */ false);

  // Iterate through the resulting SFrame...
  std::set<std::string> loaded_paths;
  enumerate_rows(sf, [&](const image_type& img, const std::string& path) {
    // Keep track of which paths were actually loaded.
    loaded_paths.insert(path);

    // Check that each loaded image matches the descriptor written earlier.
    const auto desc_iter = descriptors_by_path.find(path);
    if (desc_iter == descriptors_by_path.end()) {
      TS_FAIL(std::string("Unexpected loaded path: ") + path);
    } else {
      TS_ASSERT_EQUALS(img.m_height, desc_iter->second.height);
      TS_ASSERT_EQUALS(img.m_width, desc_iter->second.width);
      TS_ASSERT_EQUALS(img.m_channels, desc_iter->second.channels);
      TS_ASSERT_EQUALS(static_cast<size_t>(img.m_format),
                       static_cast<size_t>(desc_iter->second.format));
    }
  });

  // Verify that only the top-level images were loaded.
  TS_ASSERT_EQUALS(top_level_images, loaded_paths);
  TS_ASSERT_EQUALS(top_level_images.size(), sf->size());

  // Clean up.
  TS_ASSERT(fileio::delete_path_recursive(temp_dir));
}
