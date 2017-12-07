/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <boost/algorithm/string/predicate.hpp>
#include <fileio/get_s3_endpoint.hpp>
#include <fileio/get_s3_endpoint.hpp>
#include <fileio/fileio_constants.hpp>

namespace turi {
namespace fileio {
// Endpoint addresses specified on http://docs.aws.amazon.com/general/latest/gr/rande.html
const std::vector<std::string> AWS_S3_END_POINTS {
  "s3.amazonaws.com",
  "s3-us-west-2.amazonaws.com",
  "s3-us-west-1.amazonaws.com",
  "s3-eu-west-1.amazonaws.com",
  "s3-ap-southeast-1.amazonaws.com",
  "s3-ap-southeast-2.amazonaws.com",
  "s3-ap-northeast-1.amazonaws.com",
  "s3-sa-east-1.amazonaws.com",
};

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
