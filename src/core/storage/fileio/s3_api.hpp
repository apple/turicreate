/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_S3_UPLOADER_HPP
#define TURI_S3_UPLOADER_HPP

#ifndef TC_DISABLE_REMOTEFS

#include <aws/s3/S3Client.h>

#include <core/logging/assertions.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <fstream>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace turi {

/**
 * \ingroup fileio
 * \internal
 *
 * constructed **only** from user provided url
 *
 * A complete specification of an S3 bucket and object,
 * including all authentication required.
 */
struct s3url {
  std::string access_key_id;
  std::string secret_key;
  std::string bucket;
  std::string object_name;
  // endpoint that embeded in the url
  std::string endpoint;

  // endpoint used by sdk, not in the url
  boost::optional<std::string> sdk_endpoint;
  boost::optional<std::string> sdk_region;
  boost::optional<std::string> sdk_proxy;

  // this call doesn't compare the optional members
  bool operator==(const s3url& other) const {
    return access_key_id == other.access_key_id &&
           secret_key == other.secret_key && bucket == other.bucket &&
           object_name == other.object_name && endpoint == other.endpoint;
  }

  /*
   * @param with_credentials: user should not see this
   *
   * reconstruct to url format,
   * s3://[access_key_id]:[secret_key]:[endpoint/][bucket]/[object_name],
   * which turi uses everywhere.
   */
  std::string string_from_s3url(bool with_credentials = true) const {
    std::string ret("s3://");
    ret.reserve(128);
    const size_t prot_len = ret.size();

    if (with_credentials && !access_key_id.empty()) {
      ASSERT_TRUE(!secret_key.empty());
      ret.append(access_key_id);
      ret.append(1, ':');
      ret.append(secret_key);
      ret.append(1, ':');
    }

    // this is embeded form
    // something like: s3://s3.amazonaws.com/bucket/object/name
    if (!endpoint.empty()) {
      ret.append(1, ':');
      ret.append(endpoint);
      ret.append(1, '/');
    }

    ASSERT_TRUE(!bucket.empty());
    ret.append(bucket);

    if (!object_name.empty()) {
      // s3://object_key is a valid case
      if (ret.size() > prot_len) ret.append(1, '/');
      ret.append(object_name);
    }

    return ret;
  }

  friend std::ostream& operator<<(std::ostream& os, const s3url& url) {
    if (url.sdk_endpoint)
      os << "endpoint used by sdk: " << *url.sdk_endpoint << "; ";
    if (url.sdk_region) os << "region used by sdk: " << *url.sdk_region << "; ";
    if (url.sdk_proxy) os << "proxy used by sdk: " << *url.sdk_proxy << "; ";
    return os << url.string_from_s3url(false);
  }
};

/**
 * \ingroup fileio
 * \internal
 *
 * initialize the sdk with TRUI constomized environment variable
 *
 * will set the endpoint/region that used to configure the client
 *
 * this call will modify optional sdk_* members
 */
Aws::S3::S3Client init_aws_sdk_with_turi_env(s3url& parsed_url);

/**
 * \ingroup fileio
 * \internal
 * Get the last modified time stamp of file.
 *
 * Throw exception if the url cannot be fetched.
 *
 * Return empty string if last modified is not available,
 * e.g. the url is a directory path or file does not exist.
 */
std::string get_s3_file_last_modified(const std::string& url);

/**
 * \ingroup fileio
 * \internal
 * Return type of list_objects;
 */
struct list_objects_response {
  /// Non-empty if there was an error
  std::string error;
  /// A list of all the "sub-directories" found. Encoded with url, see s3url.
  std::vector<std::string> directories;
  /// A list of all the objects found. Encoded with url, see s3url.
  /// this should be really called object_urls;
  std::vector<std::string> objects;
  /// A list of all the objects size.
  std::vector<size_t> objects_size;
  /// Last modified time for the objects.
  std::vector<std::string> objects_last_modified;
};

/**
 * \ingroup fileio
 * \internal
 * Lists objects or prefixes prefixed by a give s3 url.
 *
 * This is a thin wrapper around the S3 API
 * http://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketGET.html
 * and may not quite do what you think it does.
 *
 * if s3_url points to a valid prefix, it will return only the prefix
 * as a directory. For instance if I have an S3 bucket containing
 *
 * foo/hello.txt
 *
 * list_objects("s3://foo") will return simply "foo/" as a directory.
 *
 * See list_directory() and is_directory() for a more sensible implementation
 * which behaves somewhat more file system like.
 *
 * \returns A list_objects_response object.
 * If list_objects_response.error is an empty string, it indicates success.
 * Otherwise, it contains an error code. list_objects_response.directories
 * indicate all "directories" stored with the requested prefix. And
 * list_objects_response.objects indicates all regular objects stored with the
 * requested prefix.
 *
 */
list_objects_response list_objects(std::string s3_url, std::string proxy = "");

/**
 * \ingroup fileio
 * \internal
 * Lists all objects prefixed by a give s3 url.
 *
 * if s3_url points to a valid prefix, it  return the prefix's contents
 * like a directory.
 *
 * foo/hello.txt
 *
 * list_objects("s3://foo") will return "foo/hello.txt"
 *
 * If s3_url points to an object it will just return the object.
 *
 * \returns A list_objects_response object.
 * If list_objects_response.error is an empty string, it indicates success.
 * Otherwise, it contains an error code. list_objects_response.directories
 * indicate all "directories" stored with the requested prefix. And
 * list_objects_response.objects indicates all regular objects stored with the
 * requested prefix.
 *
 */
list_objects_response list_directory(std::string s3_url,
                                     std::string proxy = "");

/**
 * \ingroup fileio
 * \internal
 * Tests if url is a directory or a regular file.
 * Returns a pair of (exists, is_directory). If exists is false,
 * is_directory should be ignored
 */
using turi::fileio::file_status;
std::pair<file_status, list_objects_response> is_directory(
    std::string s3_url, std::string proxy = "");

/**
 * \ingroup fileio
 * \internal
 * Where url points to a single object, this deletes the object.
 * Returns an empty string on success, and an error string on failure.
 */
std::string delete_object(std::string s3_url, std::string proxy = "");

/**
 * \ingroup fileio
 * \internal
 * Where url points to a prefix, this deletes all objects with the
 * specified prefix.
 * Returns an empty string on success, and an error string on failure.
 */
std::string delete_prefix(std::string s3_url, std::string proxy = "");

/**
 * \ingroup fileio
 * \internal
 * Given an S3 URL of the form expected by parse_s3url,
 * this function drops the access_key_id and the secret_key from the string
 * returning s3://[bucket]/[object_name]
 *
 * If the url cannot be parsed, we try the best to remove information associated
 * with ':'.
 *
 * If the url does not begin with s3://, return as is.
 */
std::string sanitize_s3_url(const std::string& url);

/**
 * \ingroup fileio
 * \internal
 * This splits a URL of the form
 * s3://[access_key_id]:[secret_key]:[endpoint/][bucket]/[object_name]
 * into several pieces.
 *
 * endpoint and object_name are optional.
 *
 * Returns true on success, false on failure.
 */
bool parse_s3url(const std::string& url, s3url& ret, std::string& err_msg);

/**
 * \ingroup fileio
 * \internal
 * Set the timeout for S3 upload.
 * \param timeout Timeout value in secs.
 */
void set_upload_timeout(long timeout);

/**
 * \ingroup fileio
 * \internal
 * Set the timeout for S3 download.
 * \param timeout Timeout value in secs.
 */
void set_download_timeout(long timeout);

struct S3Operation {
  enum ops_enum {
    Delete,
    List,
    HEAD,
  };

  static std::string toString(ops_enum operation) {
    return _enum_to_str.at(operation);
  }

  static const std::vector<std::string> _enum_to_str;
};

template <class Response>
std::ostream& reportS3Error(std::ostream& ss, const s3url& parsed_url,
                            S3Operation::ops_enum operation,
                            const Aws::Client::ClientConfiguration& config,
                            const Response& outcome) {
  auto error = outcome.GetError();
  ss << "('" << parsed_url << ", proxy: '" << config.proxyHost << "', region: '"
     << config.region << "')"
     << " Error while performing " << S3Operation::toString(operation)
     << ". Error Name: " << error.GetExceptionName()
     << ". Error Message: " << error.GetMessage()
     << ". HTTP Error Code: " << static_cast<int>(error.GetResponseCode());

  return ss;
}

#define reportS3ErrorDetailed(ss, parsed_url, operation, config, outcome) \
  reportS3Error(ss, parsed_url, operation, config, outcome)               \
      << " in " << __FILE__ << " at " << __LINE__

}  // namespace turi

#endif  // End ifndef TC_DISABLE_REMOTEFS

#endif
