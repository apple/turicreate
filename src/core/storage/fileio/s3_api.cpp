/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef _WIN32
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif
#include <memory>
#include <fstream>
#include <string>
#include <thread>
#include <future>
#include <regex>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/tokenizer.hpp>
#include <core/logging/logger.hpp>
#include <core/storage/fileio/s3_api.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/fileio/get_s3_endpoint.hpp>
#include <core/system/cppipc/server/cancel_ops.hpp>

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/ListObjectsV2Result.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <aws/s3/model/Delete.h>

using namespace Aws;
using namespace Aws::S3;
using namespace turi::fileio;
namespace turi {
namespace {

/**
 * Check the string is a valid s3 bucket name using the following criteria from
 * http://docs.aws.amazon.com/AmazonS3/latest/dev/BucketRestrictions.html:
 *
 * 1 Bucket names must be at least 3 and no more than 63 characters long.
 * 2 Bucket names must be a series of one or more labels.
 * 3 Adjacent labels are separated by a single period (.).
 * 4 Bucket names can contain lowercase letters, numbers, and hyphens.
 * 5 Each label must start and end with a lowercase letter or a number.
 * 6 Bucket names must not be formatted as an IP address (e.g., 192.168.5.4).
 *
 * Amendment 1:
 *   Uppercase letters are in fact fine... And it is in fact case sensitive.
 *   Our test bucket Turi-Datasets breaks a couple of the rules above.
 *   Tweaked to accept capital letters.
 *
 * Amendment 2:
 *   underscores are fine too
 */
bool bucket_name_valid(const std::string& bucket_name) {

  // rule 1
  if (bucket_name.size() < 3 || bucket_name.size() > 63) {
    return false;
  }

  // rule 2, 3
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  boost::char_separator<char> sep(".");
  tokenizer labels(bucket_name, sep);
  tokenizer::iterator iter = labels.begin();
  if (iter == labels.end()) {
    return false;
  }

  // rule 4, 5
  auto label_valid = [](const std::string& label) {
    if (label.empty()) return false;
    using namespace std::regex_constants;

    auto alnum = [=](char x) { return (x <= 'Z' && x >= 'A') || (x <= 'z' && x >= 'a') || (x <= '9' && x >= '0'); };
    auto alnum_underscore_or_hypen = [=](char x) { return x == '-' || x == '_' || alnum(x); };

    // begin
    if (!alnum(*label.begin())) return false;
    // end
    if (!alnum(*(label.end()-1))) return false;
    // everything in between
    for (size_t i = 1; i < label.size() - 1; ++i) {
      if (!alnum_underscore_or_hypen(label[i])) return false;
    }
    return true;
  };
  while (iter != labels.end()) {
    if (!label_valid(*iter)) return false;
    ++iter;
  }

  // rule 6, to validate, let's try creating an ip address from the bucket name.
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, bucket_name.c_str(), &(sa.sin_addr));
  if (result != 0) return false;

  return true;
}


std::string string_from_s3url(const s3url& parsed_url) {
  std::string ret = "s3://" + parsed_url.access_key_id + ":"
      + parsed_url.secret_key + ":";
  if (!parsed_url.endpoint.empty()) {
    ret += parsed_url.endpoint + "/";
  }
  ret += parsed_url.bucket;
  if (!parsed_url.object_name.empty()) {
    ret += "/" + parsed_url.object_name;
  }
  return ret;
}

} // anonymous namespace

/**
 * This splits a URL of the form
 * s3://[access_key_id]:[secret_key]:[endpoint/][bucket]/[object_name]
 * into several pieces.
 *
 * Returns true on success, false on failure.
 */
bool parse_s3url(std::string url, s3url& ret) {
  // must begin with s3://
  if (!boost::algorithm::starts_with(url, "s3://")) {
    return false;
  }
  // strip the s3://
  url = url.substr(5);

  // Extract the access key ID and secret key
  size_t splitpos = url.find(':');
  if (splitpos == std::string::npos) {
    logstream(LOG_WARNING) << "Cannot find AWS_ACCESS_KEY_ID in the s3 url." << std::endl;
    return false;
  } else {
    ret.access_key_id = url.substr(0, splitpos);
    url= url.substr(splitpos + 1);
  }
  // Extract the secret key
  splitpos = url.find(':');
  if (splitpos == std::string::npos) {
    logstream(LOG_WARNING) << "Cannot find SECRET_AWS_ACCESS_KEY in the s3 url." << std::endl;
    return false;
  } else {
    ret.secret_key= url.substr(0, splitpos);
    url= url.substr(splitpos + 1);
  }

  // The rest is parsed using boost::tokenizer
  typedef boost::tokenizer<boost::char_separator<char> >
    tokenizer;
  boost::char_separator<char> sep("/");
  tokenizer tokens(url, sep);
  tokenizer::iterator iter = tokens.begin();
  if (iter == tokens.end()) {
    return false;
  }

  // Parse endpoints
  if (std::regex_match (*iter, std::regex("(.*)(com)"))) {
    ret.endpoint = *iter;
    ++iter;
  }

  // Parse bucket name
  if (iter == tokens.end()) {
    return false;
  }
  if (!bucket_name_valid(*iter)) {
    logstream(LOG_WARNING) << "Invalid bucket name: " << *iter << std::endl;
    return false;
  }
  ret.bucket = *iter;
  ++iter;

  // The rest part is the object key
  if (iter == tokens.end()) {
    // no object key
    return true;
  }
  ret.object_name = *iter;
  ++iter;
  while(iter != tokens.end()) {
    ret.object_name += "/" + *iter;
    ++iter;
  }

  // std::cout << "Access Key: " << ret.access_key_id << "\n"
  //           << "Secret Key: " << ret.secret_key<< "\n"
  //           << "Bucket: " << ret.bucket<< "\n"
  //           << "Object: " << ret.object_name<< "\n"
  //           << "Endpoint: " << ret.endpoint << "\n";
  return true;
}


// The options we pass to aws cli for s3 commands
// "us-east-1" is the us-standard and it works with buckets from all regions
// "acl" grants the bucket owner full permission regardless of the uploader's account
static const std::string S3_COMMAND_OPTION = "--region us-east-1 --acl bucket-owner-full-control";


std::string validate_input_file(const std::string& local_file) {
  // Try to open the input file
  std::shared_ptr<turi::general_ifstream> fin(
      new turi::general_ifstream(local_file.c_str(),
                                     false)); // gzip_compressed.
                                             // We avoid decompressing the file
                                             // on transfer. i.e. if the file is
                                             // compressed/uncompressed to begin
                                             // with, lets  keep it that way.

  // file cannot be opened
  if (!fin->good()) {
    return std::string("File ") + local_file + " cannot be opened.";
  }

  // get the file size. Return failure on failure.
  size_t file_size = fin->file_size();
  if (file_size == (size_t)(-1)) {
    return std::string("Size of file ") + local_file + " cannot be obtained.";
  }
  return "";
}

std::string validate_output_file(const std::string& local_file) {
  // Try to open the output file
  std::shared_ptr<turi::general_ofstream> fout(
      new turi::general_ofstream(local_file.c_str(),
                                     false));// gzip_compressed.
                                             // We avoid recompressing the file
                                             // on transfer. i.e. if the file is
                                             // compressed/uncompressed to begin
                                             // with, lets  keep it that way.
  // file cannot be opened
  if (!fout->good()) {
    // return a failure immediately.
    return std::string("File ") + local_file + " cannot be opened.";
  }
  return "";
}

/**
 * Adding single quote around the path, and escape all single quotes inside the path.
 */
std::string quote_and_escape_path(const std::string& path) {
  // s3 keys are at most 1024 bytes,
  // we make the buffer three times bigger
  // and it should be enough to conver the length of escaped path s3://bucket_name/key
  const size_t BUF_SIZE = 1024 * 3;
  char* buf = new char[BUF_SIZE];
  size_t current_pos = 0;
  buf[current_pos++] = '\"'; // begin quote
  for (const auto& c : path) {
    if (c == '\'') {
      buf[current_pos++] = '\\'; // escape quote
      if (current_pos >= BUF_SIZE) {
        delete[] buf;
        throw("Invalid path: exceed length limit");
      }
    }
    buf[current_pos++] = c;
    if (current_pos >= BUF_SIZE) {
      delete[] buf;
      throw("Invalid path: exceed length limit");
    }
  }
  buf[current_pos++] = '\"'; // end quote
  std::string ret(buf, current_pos);
  delete[] buf;
  return ret;
}


list_objects_response list_objects_impl(s3url parsed_url,
                                        std::string proxy,
                                        std::string endpoint)
{
    // initialization
    Aws::SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    Aws::InitAPI(options);

    // credentials
    Aws::Auth::AWSCredentials credentials(parsed_url.access_key_id.c_str(), parsed_url.secret_key.c_str());


    // s3 client config
    Aws::Client::ClientConfiguration clientConfiguration;
    if (turi::fileio::insecure_ssl_cert_checks()) {
      clientConfiguration.verifySSL = false;
    }
    if (parsed_url.endpoint.empty()) {
      clientConfiguration.endpointOverride = endpoint.c_str();
    } else {
      clientConfiguration.endpointOverride = parsed_url.endpoint.c_str();
    }
    clientConfiguration.proxyHost = proxy.c_str();
    clientConfiguration.requestTimeoutMs = 5 * 60000;
    clientConfiguration.connectTimeoutMs = 20000;
    std::string region = fileio::get_region_name_from_endpoint(clientConfiguration.endpointOverride.c_str());
    clientConfiguration.region = region.c_str();

    S3Client client(credentials, clientConfiguration);

    list_objects_response ret;

    Aws::S3::Model::ListObjectsV2Request request;
    request.WithBucket(parsed_url.bucket.c_str());
    request.WithPrefix(parsed_url.object_name.c_str());
    request.WithDelimiter("/"); // seperate objects from directories

    bool moreResults = false;
    do
    {
        auto outcome = client.ListObjectsV2(request);
        if(outcome.IsSuccess())
        {
            auto result = outcome.GetResult();

            // now iterate through found objects - these are files
            Aws::Vector<Aws::S3::Model::Object> objects;
            objects = result.GetContents();
            for (auto const& o : objects)
            {
                // std::cout << "object: " << objects[i].GetKey() << ", size: " << objects[i].GetSize() << ", last_modified: " << objects[i].GetLastModified().Millis() << std::endl;
                ret.objects.push_back(std::string(o.GetKey().c_str()));
                std::stringstream stream;
                stream << o.GetLastModified().Millis();
                ret.objects_last_modified.push_back(stream.str());
            }

            // now iterate through common prefixes - these are directories
            Aws::Vector<Aws::S3::Model::CommonPrefix> prefixes;
            prefixes = result.GetCommonPrefixes();
            for (auto const& p : prefixes)
            {
                // std::cout << "directory: " << prefixes[i].GetPrefix() << std::endl;
                std::string key = std::string(p.GetPrefix().c_str());
                // strip the ending "/" on a directory
                if (boost::ends_with(key, "/")) key = key.substr(0, key.length() - 1);
                ret.directories.push_back(key);
            }

            // more results to retrieve
            moreResults = result.GetIsTruncated();
            if (moreResults)
            {
                // add to the request object with continuation token
                request.WithContinuationToken(result.GetContinuationToken());
            }

        }
        else
        {
            std::stringstream stream;
            stream << "Error while listing Objects, exception: " <<
                    outcome.GetError().GetExceptionName() <<
                    ", msg: " << outcome.GetError().GetMessage() << std::endl;
            ret.error = stream.str();
        }

    } while (moreResults);

    Aws::ShutdownAPI(options);
    for (auto& dir : ret.directories) {
      s3url dirurl = parsed_url;
      dirurl.object_name = dir;
      dir = string_from_s3url(dirurl);
    }
    for (auto& object: ret.objects) {
      s3url objurl = parsed_url;
      objurl.object_name = object;
      object = string_from_s3url(objurl);
    }
    return ret;
}

/// returns an error string on failure. Empty string on success
std::string delete_object_impl(s3url parsed_url,
                               std::string proxy,
                               std::string endpoint) {
    std::string ret;

    // initialization
    Aws::SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    Aws::InitAPI(options);

    // credentials
    Aws::Auth::AWSCredentials credentials(parsed_url.access_key_id.c_str(), parsed_url.secret_key.c_str());


    // s3 client config
    Aws::Client::ClientConfiguration clientConfiguration;
    if (turi::fileio::insecure_ssl_cert_checks()) {
      clientConfiguration.verifySSL = false;
    }

    if (parsed_url.endpoint.empty()) {
      clientConfiguration.endpointOverride = endpoint.c_str();
    } else {
      clientConfiguration.endpointOverride = parsed_url.endpoint.c_str();
    }
    clientConfiguration.proxyHost = proxy.c_str();
    clientConfiguration.requestTimeoutMs = 5 * 60000;
    clientConfiguration.connectTimeoutMs = 20000;
    std::string region = fileio::get_region_name_from_endpoint(clientConfiguration.endpointOverride.c_str());
    clientConfiguration.region = region.c_str();

    S3Client client(credentials, clientConfiguration);

    Aws::S3::Model::DeleteObjectRequest request;
    request.WithBucket(parsed_url.bucket.c_str());
    request.WithKey(parsed_url.object_name.c_str());

    auto outcome = client.DeleteObject(request);
    if(!outcome.IsSuccess())
    {
        std::stringstream stream;
        stream << "Error while deleting object, exception: " <<
                outcome.GetError().GetExceptionName() <<
                ", msg: " << outcome.GetError().GetMessage() << std::endl;
        ret = stream.str();
    }

    Aws::ShutdownAPI(options);
    return ret;
}

/// returns an error string on failure. Empty string on success
std::string delete_prefix_impl(s3url parsed_url,
                               std::string proxy,
                               std::string endpoint) {

    // List objects and then create a DeleteObjects request from the resulting list

    std::string ret;

    // initialization
    Aws::SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    Aws::InitAPI(options);

    // credentials
    Aws::Auth::AWSCredentials credentials(parsed_url.access_key_id.c_str(), parsed_url.secret_key.c_str());


    // s3 client config
    Aws::Client::ClientConfiguration clientConfiguration;
    if (turi::fileio::insecure_ssl_cert_checks()) {
      clientConfiguration.verifySSL = false;
    }

    if (parsed_url.endpoint.empty()) {
      clientConfiguration.endpointOverride = endpoint.c_str();
    } else {
      clientConfiguration.endpointOverride = parsed_url.endpoint.c_str();
    }

    clientConfiguration.proxyHost = proxy.c_str();
    clientConfiguration.requestTimeoutMs = 5 * 60000;
    clientConfiguration.connectTimeoutMs = 20000;
    std::string region = fileio::get_region_name_from_endpoint(clientConfiguration.endpointOverride.c_str());
    clientConfiguration.region = region.c_str();

    S3Client client(credentials, clientConfiguration);

    Aws::S3::Model::ListObjectsV2Request request;
    request.WithBucket(parsed_url.bucket.c_str());
    request.WithPrefix(parsed_url.object_name.c_str());

    // keep retrieving objects until no more objects match query
    bool moreResults = false;
    Aws::S3::Model::Delete deleteObjects;
    do
    {
        auto outcome = client.ListObjectsV2(request);
        if(outcome.IsSuccess())
        {
            auto result = outcome.GetResult();

            // now iterate through found objects and construct DeleteObjects request with them
            auto objects = result.GetContents();
            for (auto const& o : objects)
            {
                Aws::S3::Model::ObjectIdentifier key;
                deleteObjects.AddObjects(key.WithKey(o.GetKey()));
            }

            // more results to retrieve
            moreResults = result.GetIsTruncated();
            if (moreResults)
            {
                // add to the request object with continuation token
                request.WithContinuationToken(result.GetContinuationToken());
            }

        }
        else
        {
            std::stringstream stream;
            stream << "Error while listing Objects, exception: " <<
                    outcome.GetError().GetExceptionName() <<
                    ", msg: " << outcome.GetError().GetMessage() << std::endl;
            ret = stream.str();
        }

    } while (moreResults);

    if (deleteObjects.GetObjects().size() > 0)
    {
        Aws::S3::Model::DeleteObjectsRequest delRequest;
        delRequest.WithBucket(parsed_url.bucket.c_str());
        delRequest.WithDelete(deleteObjects);

        auto outcome = client.DeleteObjects(delRequest);
        if (!outcome.IsSuccess())
        {
            std::stringstream stream;
            stream << "Error while deleting Objects, exception: " <<
                    outcome.GetError().GetExceptionName() <<
                    ", msg: " << outcome.GetError().GetMessage() << std::endl;
            ret = stream.str();
        }
    }

    Aws::ShutdownAPI(options);
    return ret;
}

list_objects_response list_objects(std::string url,
                                   std::string proxy) {
  s3url parsed_url;
  list_objects_response ret;
  bool success = parse_s3url(url, parsed_url);
  if (!success) {
    ret.error = "Malformed URL";
    return ret;
  }

  size_t current_endpoint = 0;
  // try all endpoints
  auto endpoints = get_s3_endpoints();
  do {
    ret = list_objects_impl(parsed_url, proxy, endpoints[current_endpoint]);
    ++current_endpoint;
  } while (boost::algorithm::icontains(ret.error, "PermanentRedirect") &&
         current_endpoint < endpoints.size());
  return ret;
}


std::pair<bool, bool> is_directory(std::string url,
                                   std::string proxy) {
  s3url parsed_url;
  list_objects_response ret;
  bool success = parse_s3url(url, parsed_url);
  if (!success) {
    return {false, false};
  }
  // if there are no "/"'s it is just a top level bucket

  list_objects_response response = list_objects(url, proxy);
  // an error occured
  if (!response.error.empty()) {
    return {false, false};
  }
  // its a top level bucket name
  if (parsed_url.object_name.empty()) {
    return {true, true};
  }
  // is a directory
  for (auto dir: response.directories) {
    if (dir == url) {
      return {true, true};
    }
  }
  // is an object
  for (auto object: response.objects) {
    if (object == url) {
      return {true, false};
    }
  }
  // is not found
  return {false, false};
}


list_objects_response list_directory(std::string url,
                                     std::string proxy) {
  s3url parsed_url;
  list_objects_response ret;
  bool success = parse_s3url(url, parsed_url);
  if (!success) {
    ret.error = "Malformed URL";
    return ret;
  }
  // normalize the URL so it doesn't matter if you put strange "/"s at the end
  url = string_from_s3url(parsed_url);
  std::pair<bool, bool> isdir = is_directory(url, proxy);
  // if not found.
  if (isdir.first == false) return ret;
  // if its a directory
  if (isdir.second) {
    // if there are no "/"'s it is a top level bucket and we don't need
    // to mess with prefixes to get the contents
    if (!parsed_url.object_name.empty()) {
      parsed_url.object_name = parsed_url.object_name + "/";
    }
    size_t current_endpoint = 0;

    // try all endpoints
    auto endpoints = get_s3_endpoints();
    do {
      ret = list_objects_impl(parsed_url, proxy, endpoints[current_endpoint]);
      ++current_endpoint;
    } while (boost::algorithm::icontains(ret.error, "PermanentRedirect") &&
           current_endpoint < endpoints.size());
  } else {
    ret.objects.push_back(url);
  }
  return ret;
}


std::string delete_object(std::string url,
                          std::string proxy) {
  s3url parsed_url;
  std::string ret;
  bool success = parse_s3url(url, parsed_url);
  if (!success) {
    ret = "Malformed URL";
    return ret;
  }
  // try all endpoints
  size_t current_endpoint = 0;
  auto endpoints = get_s3_endpoints();
  do {
    ret = delete_object_impl(parsed_url, proxy, endpoints[current_endpoint]);
    ++current_endpoint;
  } while (boost::algorithm::icontains(ret , "PermanentRedirect") &&
         current_endpoint < endpoints.size());
  return ret;
}

std::string delete_prefix(std::string url,
                          std::string proxy) {
  s3url parsed_url;
  std::string ret;
  bool success = parse_s3url(url, parsed_url);
  if (!success) {
    ret = "Malformed URL";
    return ret;
  }
  // try all endpoints
  size_t current_endpoint = 0;
  auto endpoints = get_s3_endpoints();
  do {
    ret = delete_prefix_impl(parsed_url, proxy, endpoints[current_endpoint]);
    ++current_endpoint;
  } while (boost::algorithm::icontains(ret , "PermanentRedirect") &&
         current_endpoint < endpoints.size());
  return ret;
}


std::string sanitize_s3_url_aggressive(std::string url) {
  // must begin with s3://
  if (!boost::algorithm::starts_with(url, "s3://")) {
    return url;
  }
  // strip the s3://
  url = url.substr(5);

  // strip the secret key and the access key following the usual rules.
  size_t splitpos = url.find(':');
  if (splitpos != std::string::npos) url = url.substr(splitpos + 1);
  splitpos = url.find(':');
  if (splitpos != std::string::npos) url = url.substr(splitpos + 1);

  // now, a user error is possible where ":" shows up inside the
  // secret key / access key thus leaking part of a key in the logs.
  // so we also perform a more aggressive truncation.
  // find the first "/" and delete everything up to the last ":"
  // before the first "/"
  size_t bucketend = url.find('/');
  if (bucketend == std::string::npos) bucketend = url.length();
  size_t last_colon = url.find_last_of(':', bucketend);
  if (last_colon != std::string::npos) url = url.substr(last_colon + 1);
  return "s3://" + url;
}

std::string sanitize_s3_url(const std::string& url) {
  s3url parsed_url;
  if ( parse_s3url(url, parsed_url) ) {
    if (parsed_url.endpoint.empty())
      return "s3://" + parsed_url.bucket + "/" + parsed_url.object_name;
    else
      return "s3://" + parsed_url.endpoint + "/" + parsed_url.bucket + "/" + parsed_url.object_name;
  } else {
    return sanitize_s3_url_aggressive(url);
  };
}

std::string get_s3_error_code(const std::string& msg) {
  static const std::vector<std::string> errorcodes {
    "AccessDenied", "NoSuchBucket", "InvalidAccessKeyId",
    "InvalidBucketName", "KeyTooLong", "NoSuchKey", "RequestTimeout"
  };

  // User friendly error codes, return immediately.
  for (const auto& ec : errorcodes) {
    if (boost::algorithm::icontains(msg, ec)) {
      return ec;
    }
  }

  // Error code that may need some explanation.
  // Add messages for error code below:
  // ...
  // best guess for 403 error
  if (boost::algorithm::icontains(msg, "forbidden")) {
    return "403 Forbidden. Please check your AWS credentials and permission to the file.";
  }

  return msg;
}

std::string get_s3_file_last_modified(const std::string& url){
  list_objects_response response = list_objects(url);
  if (response.error.empty() && response.objects_last_modified.size() == 1) {
    return response.objects_last_modified[0];
  } else if (!response.error.empty()) {
    logstream(LOG_WARNING) << "List object error: " << response.error << std::endl;
    throw(response.error);
  }
  return "";
}
}
