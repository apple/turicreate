/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/globals/globals.hpp>
#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <core/storage/fileio/block_cache.hpp>
#include <core/storage/fileio/file_download_cache.hpp>
#include <core/storage/fileio/fixed_size_cache_manager.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/fileio/s3_filesys.hpp>
#include <core/storage/serialization/dir_archive.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>

/**
 * example program to set up s3 read
 *
 * set AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY in environment variable.
 * set TURI_S3_REGION and TURI_S3_ENDPOINT for more control.
 */
int main(int argc, char** argv) {
  global_logger().set_log_level(LOG_DEBUG);

  if (argc != 3) {
    std::cerr << "Usage:\n"
              << "* set TURI_S3_REION TURI_S3_ENDPOIT, AWS_ACCESS_KEY_ID, and "
                 "AWS_SECRET_ACCESS_KEY environment variables.\n."
              << "./s3_filesys_test bucket key\n"
              << "Examples:\n./s3_filesys_test tc_qa "
                 "integration/manual/sframes/cats-dogs-images/"
              << std::endl;
    return 0;
  }

  turi::s3url url;
  url.bucket = argv[1];
  url.object_name = argv[2];

  auto aws_key_id = turi::getenv_str("AWS_ACCESS_KEY_ID");
  if (!aws_key_id) {
    std::cerr << "AWS_ACCESS_KEY_ID not set in environment variable"
              << std::endl;
    return -1;
  }

  url.access_key_id = aws_key_id.value();

  auto aws_key = turi::getenv_str("AWS_SECRET_ACCESS_KEY");

  if (!aws_key) {
    std::cerr << "AWS_SECRET_ACCESS_KEY not set in environment variable"
              << std::endl;
    return -1;
  }

  url.secret_key = aws_key.value();

  std::string url_read = url.string_from_s3url();
  logstream(LOG_DEBUG) << "read from url" << url_read << std::endl;

  __attribute__((unused)) auto& dummy =
      turi::fileio::s3::turi_global_AWS_SDK_setup();

  // TURI_S3_REGION and TURI_S3_ENDPOINT will be initialized
  turi::globals::initialize_globals_from_environment(".");

  try {
    turi::unity_sframe sf;
    sf.construct_from_sframe_index(url_read);

    auto sf_size = sf.size();
    sf.begin_iterator();

    // extract all
    auto ret = sf.iterator_get_next(sf_size);
    ASSERT_EQ(ret.size(), sf_size);

    auto column_names = sf.column_names();
    // touch all entries
    auto sa_ptr =
        sf.pack_columns(column_names, column_names, turi::flex_type_enum::LIST,
                        turi::flex_undefined());

    DASSERT_EQ(sa_ptr->size(), sf.size());

    // force write all entries
    sa_ptr->materialize();

  } catch (std::string& e) {
    std::cerr << "Exception: " << e << std::endl;
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  /* teardown manually */
  turi::file_download_cache::get_instance().clear();
  turi::block_cache::release_instance();

  return 0;
}
