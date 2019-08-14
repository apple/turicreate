/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_GENERAL_TESTING_UTILS_H_
#define TURI_GENERAL_TESTING_UTILS_H_

#include <core/parallel/pthread_tools.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/util/try_finally.hpp>
#include <core/random/random.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <vector>
#include <string>
#include <locale>

#include <core/parallel/mutex.hpp>
#include <sys/types.h>
#include <unistd.h>
#include <boost/filesystem.hpp>

namespace turi {

/** The directories we use for our temporary archives should be unique
 *  and everything, but we don't want hundreds of these lying around.
 *  Thus add them to a list with which we delete when the program
 *  exits; this function does that.
 */
void _add_directory_to_deleter(const std::string& name);

/** Make a unique directory name.
 */
std::string _get_unique_directory(const std::string& file, size_t line);


/**
 * \ingroup util
 * Serializes and deserializes a model, making sure that the
 *  model leaves the stream iterator in the appropriate place.
 */
template <typename T, typename U>
void _save_and_load_object(T& dest, const U& src, std::string dir) {

  // Create the directory
  boost::filesystem::create_directory(dir);
  _add_directory_to_deleter(dir);

  std::string arc_name = dir + "/test_archive";

  uint64_t random_number = hash64(random::fast_uniform<size_t>(0,size_t(-1)));

  // Save it
  dir_archive archive_write;
  archive_write.open_directory_for_write(arc_name);

  turi::oarchive oarc(archive_write);

  oarc << src << random_number;

  archive_write.close();

  // Load it
  dir_archive archive_read;
  archive_read.open_directory_for_read(arc_name);

  turi::iarchive iarc(archive_read);

  iarc >> dest;

  uint64_t test_number;

  iarc >> test_number;

  archive_read.close();

  ASSERT_EQ(test_number, random_number);
}

#define save_and_load_object(dest, src)                         \
  do{                                                           \
    _save_and_load_object(                                      \
        dest, src, _get_unique_directory(__FILE__, __LINE__));  \
  } while(false)

}

#endif /* _TESTING_UTILS_H_ */
