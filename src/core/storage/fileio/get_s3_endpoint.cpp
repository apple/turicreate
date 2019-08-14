/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <map>
#include <boost/algorithm/string/predicate.hpp>
#include <core/storage/fileio/get_s3_endpoint.hpp>
#include <core/storage/fileio/get_s3_endpoint.hpp>
#include <core/storage/fileio/fileio_constants.hpp>
#include <core/system/platform/process/process_util.hpp>

namespace turi {
namespace fileio {
// Endpoint addresses specified on http://docs.aws.amazon.com/general/latest/gr/rande.html
const std::vector<std::string> AWS_S3_END_POINTS {
  "s3.amazonaws.com",
  "s3-us-west-1.amazonaws.com",
  "s3-us-west-2.amazonaws.com",
  "s3-eu-west-1.amazonaws.com",
  "s3-eu-central-1.amazonaws.com",
  "s3-ap-southeast-1.amazonaws.com",
  "s3-ap-southeast-2.amazonaws.com",
  "s3-ap-northeast-1.amazonaws.com",
  "s3-ap-northeast-2.amazonaws.com",
  "s3-sa-east-1.amazonaws.com",
  "s3-ap-south-1.amazonaws.com",
};

const std::map<std::string, std::string> AWS_S3_ENDPOINT_TO_REGION {
  {"s3.amazonaws.com","us-east-1"},
  {"s3-us-west-1.amazonaws.com","us-west-1"},
  {"s3-us-west-2.amazonaws.com","us-west-2"},
  {"s3-eu-west-1.amazonaws.com","eu-west-1"},
  {"s3-eu-central-1.amazonaws.com","eu-central-1"},
  {"s3-ap-southeast-1.amazonaws.com","ap-southeast-1"},
  {"s3-ap-southeast-2.amazonaws.com","ap-southeast-2"},
  {"s3-ap-northeast-1.amazonaws.com","ap-northeast-1"},
  {"s3-ap-northeast-2.amazonaws.com","ap-northeast-2"},
  {"s3-sa-east-1.amazonaws.com","sa-east-1"},
  {"s3-ap-south-1.amazonaws.com","ap-south-1"}};


std::vector<std::string> get_s3_endpoints() {
  if (S3_ENDPOINT.empty()) {
    return AWS_S3_END_POINTS;
  } else {
    // we need to trim the https:// from the front
    auto ret = S3_ENDPOINT;
    if (boost::algorithm::starts_with(ret, "https://")) {
      ret = ret.substr(8);
    }
    return {ret};
  }
}

std::string get_region_name_from_endpoint(std::string endpoint) {
  if (S3_REGION.size())
    return S3_REGION;

  boost::optional<std::string> aws_default_region = getenv_str("AWS_DEFAULT_REGION");
  if (aws_default_region) {
    return *std::move(aws_default_region);
  }
  // try to infer from endpoint
  auto iter = AWS_S3_ENDPOINT_TO_REGION.find(endpoint);
  if (iter != AWS_S3_ENDPOINT_TO_REGION.end()) {
    return iter->second;
  } else {
    // use default region which aws provides with
    return "";
  }
}

std::string get_bucket_path(const std::string& bucket) {
  if (S3_ENDPOINT.empty()) {
    return "https://" + bucket + ".s3.amazonaws.com" + '/';
  } else {
    if (boost::algorithm::ends_with(S3_ENDPOINT, "/")) {
      return S3_ENDPOINT + bucket + "/";
    } else {
      return S3_ENDPOINT + "/" + bucket + "/";
    }
  }
}
} // fileio
} // turi
