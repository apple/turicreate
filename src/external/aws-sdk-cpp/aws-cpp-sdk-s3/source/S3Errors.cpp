/*
* Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/client/AWSError.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/s3/S3Errors.h>

using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::Utils;

namespace Aws
{
namespace S3
{
namespace S3ErrorMapper
{

static const int NO_SUCH_UPLOAD_HASH = HashingUtils::HashString("NoSuchUpload");
static const int BUCKET_ALREADY_OWNED_BY_YOU_HASH = HashingUtils::HashString("BucketAlreadyOwnedByYou");
static const int OBJECT_ALREADY_IN_ACTIVE_TIER_HASH = HashingUtils::HashString("ObjectAlreadyInActiveTierError");
static const int NO_SUCH_BUCKET_HASH = HashingUtils::HashString("NoSuchBucket");
static const int NO_SUCH_KEY_HASH = HashingUtils::HashString("NoSuchKey");
static const int OBJECT_NOT_IN_ACTIVE_TIER_HASH = HashingUtils::HashString("ObjectNotInActiveTierError");
static const int BUCKET_ALREADY_EXISTS_HASH = HashingUtils::HashString("BucketAlreadyExists");


AWSError<CoreErrors> GetErrorForName(const char* errorName)
{
  int hashCode = HashingUtils::HashString(errorName);

  if (hashCode == NO_SUCH_UPLOAD_HASH)
  {
    return AWSError<CoreErrors>(static_cast<CoreErrors>(S3Errors::NO_SUCH_UPLOAD), false);
  }
  else if (hashCode == BUCKET_ALREADY_OWNED_BY_YOU_HASH)
  {
    return AWSError<CoreErrors>(static_cast<CoreErrors>(S3Errors::BUCKET_ALREADY_OWNED_BY_YOU), false);
  }
  else if (hashCode == OBJECT_ALREADY_IN_ACTIVE_TIER_HASH)
  {
    return AWSError<CoreErrors>(static_cast<CoreErrors>(S3Errors::OBJECT_ALREADY_IN_ACTIVE_TIER), false);
  }
  else if (hashCode == NO_SUCH_BUCKET_HASH)
  {
    return AWSError<CoreErrors>(static_cast<CoreErrors>(S3Errors::NO_SUCH_BUCKET), false);
  }
  else if (hashCode == NO_SUCH_KEY_HASH)
  {
    return AWSError<CoreErrors>(static_cast<CoreErrors>(S3Errors::NO_SUCH_KEY), false);
  }
  else if (hashCode == OBJECT_NOT_IN_ACTIVE_TIER_HASH)
  {
    return AWSError<CoreErrors>(static_cast<CoreErrors>(S3Errors::OBJECT_NOT_IN_ACTIVE_TIER), false);
  }
  else if (hashCode == BUCKET_ALREADY_EXISTS_HASH)
  {
    return AWSError<CoreErrors>(static_cast<CoreErrors>(S3Errors::BUCKET_ALREADY_EXISTS), false);
  }
  return AWSError<CoreErrors>(CoreErrors::UNKNOWN, false);
}

} // namespace S3ErrorMapper
} // namespace S3
} // namespace Aws
