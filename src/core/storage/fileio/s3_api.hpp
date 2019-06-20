/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_S3_UPLOADER_HPP
#define TURI_S3_UPLOADER_HPP
#include <string>
#include <fstream>
#include <future>
#include <memory>
#include <vector>

namespace turi {


/**
 * \ingroup fileio
 * \internal
 * A complete specification of an S3 bucket and object,
 * including all authentication required.
 */
struct s3url {
  std::string access_key_id;
  std::string secret_key;
  std::string bucket;
  std::string object_name;
  std::string endpoint;

  bool operator==(const s3url& other) const {
    return access_key_id == other.access_key_id &&
           secret_key == other.secret_key &&
           bucket == other.bucket &&
           object_name == other.object_name &&
           endpoint == other.endpoint;
  }
};


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
  /// A list of all the "sub-directories" found
  std::vector<std::string> directories;

  /// A list of all the objects found.
  std::vector<std::string> objects;
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
list_objects_response list_objects(std::string s3_url,
                                   std::string proxy = "");


/**
 * \ingroup fileio
 * \internal
 * Lists all objects prefixed by a give s3 url.
 *
 * if s3_url points to a valid prefix, it will return the prefix's contents
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
std::pair<bool, bool> is_directory(std::string s3_url,
                                   std::string proxy = "");



/**
 * \ingroup fileio
 * \internal
 * Where url points to a single object, this deletes the object.
 * Returns an empty string on success, and an error string on failure.
 */
std::string delete_object(std::string s3_url,
                          std::string proxy = "");

/**
 * \ingroup fileio
 * \internal
 * Where url points to a prefix, this deletes all objects with the
 * specified prefix.
 * Returns an empty string on success, and an error string on failure.
 */
std::string delete_prefix(std::string s3_url,
                          std::string proxy = "");

/**
 * \ingroup fileio
 * \internal
 * Given an S3 URL of the form expected by parse_s3url,
 * this function drops the access_key_id and the secret_key from the string returning
 * s3://[bucket]/[object_name]
 *
 * If the url cannot be parsed, we try the best to remove information associated with ':'.
 *
 * If the url does not begin with s3://, return as is.
 */
std::string sanitize_s3_url(const std::string& url);

/**
 * \ingroup fileio
 * \internal
 * This splits a URL of the form
 * s3://[access_key_id]:[secret_key]:[endpoint][/bucket]/[object_name]
 * into several pieces.
 *
 * endpoint and object_name are optional.
 *
 * Returns true on success, false on failure.
 */
bool parse_s3url(std::string url, s3url& ret);

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

/**
 * \ingroup fileio
 * \internal
 * Return the S3 error code contains in the message.
 * If the message does not contain error code, return the message itself.
 */
std::string get_s3_error_code(const std::string& msg);


} // namespace turi


#endif
