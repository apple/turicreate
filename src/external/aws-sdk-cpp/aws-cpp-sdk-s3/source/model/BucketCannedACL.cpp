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

#include <aws/s3/model/BucketCannedACL.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/Globals.h>
#include <aws/core/utils/EnumParseOverflowContainer.h>

using namespace Aws::Utils;


namespace Aws
{
  namespace S3
  {
    namespace Model
    {
      namespace BucketCannedACLMapper
      {

        static const int private__HASH = HashingUtils::HashString("private");
        static const int public_read_HASH = HashingUtils::HashString("public-read");
        static const int public_read_write_HASH = HashingUtils::HashString("public-read-write");
        static const int authenticated_read_HASH = HashingUtils::HashString("authenticated-read");


        BucketCannedACL GetBucketCannedACLForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == private__HASH)
          {
            return BucketCannedACL::private_;
          }
          else if (hashCode == public_read_HASH)
          {
            return BucketCannedACL::public_read;
          }
          else if (hashCode == public_read_write_HASH)
          {
            return BucketCannedACL::public_read_write;
          }
          else if (hashCode == authenticated_read_HASH)
          {
            return BucketCannedACL::authenticated_read;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<BucketCannedACL>(hashCode);
          }

          return BucketCannedACL::NOT_SET;
        }

        Aws::String GetNameForBucketCannedACL(BucketCannedACL enumValue)
        {
          switch(enumValue)
          {
          case BucketCannedACL::private_:
            return "private";
          case BucketCannedACL::public_read:
            return "public-read";
          case BucketCannedACL::public_read_write:
            return "public-read-write";
          case BucketCannedACL::authenticated_read:
            return "authenticated-read";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace BucketCannedACLMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
