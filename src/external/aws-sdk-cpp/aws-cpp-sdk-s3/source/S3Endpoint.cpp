/*
* Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
*  http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/
#include <aws/s3/S3Endpoint.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/HashingUtils.h>

using namespace Aws;
using namespace Aws::S3;

namespace Aws
{
namespace S3
{
namespace S3Endpoint
{

  static const int US_EAST_1_HASH = Aws::Utils::HashingUtils::HashString("us-east-1");
  static const int US_WEST_1_HASH = Aws::Utils::HashingUtils::HashString("us-west-1");
  static const int US_WEST_2_HASH = Aws::Utils::HashingUtils::HashString("us-west-2");
  static const int EU_WEST_1_HASH = Aws::Utils::HashingUtils::HashString("eu-west-1");
  static const int AP_SOUTHEAST_1_HASH = Aws::Utils::HashingUtils::HashString("ap-southeast-1");
  static const int AP_SOUTHEAST_2_HASH = Aws::Utils::HashingUtils::HashString("ap-southeast-2");
  static const int AP_NORTHEAST_1_HASH = Aws::Utils::HashingUtils::HashString("ap-northeast-1");
  static const int SA_EAST_1_HASH = Aws::Utils::HashingUtils::HashString("sa-east-1");

  Aws::String ForRegion(const Aws::String& regionName, bool useDualStack)
  {
    if(!useDualStack)
    {
      auto hash = Aws::Utils::HashingUtils::HashString(regionName.c_str());

      if(hash == US_EAST_1_HASH)
      {
        return "s3.amazonaws.com";
      }
      else if(hash == US_WEST_1_HASH)
      {
        return "s3-us-west-1.amazonaws.com";
      }
      else if(hash == US_WEST_2_HASH)
      {
        return "s3-us-west-2.amazonaws.com";
      }
      else if(hash == EU_WEST_1_HASH)
      {
        return "s3-eu-west-1.amazonaws.com";
      }
      else if(hash == AP_SOUTHEAST_1_HASH)
      {
        return "s3-ap-southeast-1.amazonaws.com";
      }
      else if(hash == AP_SOUTHEAST_2_HASH)
      {
        return "s3-ap-southeast-2.amazonaws.com";
      }
      else if(hash == AP_NORTHEAST_1_HASH)
      {
        return "s3-ap-northeast-1.amazonaws.com";
      }
      else if(hash == SA_EAST_1_HASH)
      {
        return "s3-sa-east-1.amazonaws.com";
      }
    }
    Aws::StringStream ss;
    ss << "s3" << ".";

    if(useDualStack)
    {
      ss << "dualstack.";
    }

    ss << regionName << ".amazonaws.com";
    return ss.str();
  }

} // namespace S3Endpoint
} // namespace S3
} // namespace Aws

