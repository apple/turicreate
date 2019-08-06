/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FILEIO_FILEIO_CONSTANTS_HPP
#define TURI_FILEIO_FILEIO_CONSTANTS_HPP
#include <string>

namespace turi {
namespace fileio {

/**
 * \ingroup fileio
 * Returns the system temporary directory
 */
std::string get_system_temp_directory();

/**
 * \ingroup fileio
 * The protocol prefix cache:// to identify a cached file.
 */
std::string get_cache_prefix();

/**
 * \ingroup fileio
 * The "directory" (cache://tmp/) which all cached files are located in
 */
std::string get_temp_cache_prefix();

/**
 * \ingroup fileio
 * Gets the physical directory (/var/tmp) which all cached files are located in .
 * colon seperated.
 */
std::string get_cache_file_locations();

/**
 * \ingroup fileio
 * Sets the physical directory (/var/tmp) which all cached files are located in .
 * colon seperated.
 */
void set_cache_file_locations(std::string);

/**
 * \ingroup fileio
 * Additional HDFS location for storing large temp files.
 */
std::string get_cache_file_hdfs_location();

/**
 * \ingroup fileio
 * The initial memory capacity assigned to caches
 */
extern const size_t FILEIO_INITIAL_CAPACITY_PER_FILE;

/**
 * \ingroup fileio
 * The maximum memory capacity assigned to a cached file until it has to
 * be flushed.
 */
extern size_t FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE;

/**
 * \ingroup fileio
 * The maximum memory capacity used by all cached files be flushed.
 */
extern size_t FILEIO_MAXIMUM_CACHE_CAPACITY;

/**
 * \ingroup fileio
 * The default fileio reader buffer size
 */
extern size_t FILEIO_READER_BUFFER_SIZE;

/**
 * \ingroup fileio
 * The default fileio writer buffer size
 */
extern size_t FILEIO_WRITER_BUFFER_SIZE;

/**
 * \ingroup fileio
 * The S3 connection endpoint; if empty string, S3 is assumed.
 */
extern std::string S3_ENDPOINT;

/**
 * \ingroup fileio
 * The S3 connection region; if empty string, region will be guessed by:
 * 1. TURI_S3_REGION environment variable
 * 2. AWS_DEFAULT_REGION environment variable
 * 3. known region to endpoint mappings
 * if none of above works, empty region string will be set and AWS will
 * guess bucket region from endpoint.
 */
extern std::string S3_REGION;

/**
 * \ingroup fileio
 * The number of GPUs.
 */
extern int64_t NUM_GPUS;

/**
 * \ingroup fileio
 * Gets the alternative ssl certificate file and directory.
 */
const std::string& get_alternative_ssl_cert_dir();
/**
 * \ingroup fileio
 * Sets the alternative ssl certificate file and directory.
 */
const std::string& get_alternative_ssl_cert_file();
/**
 * \ingroup fileio
 * If true, ssl certificate checks are disabled.
 */
const bool insecure_ssl_cert_checks();

}
}
#endif
