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

#include <aws/s3/model/BucketLogsPermission.h>
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
      namespace BucketLogsPermissionMapper
      {

        static const int FULL_CONTROL_HASH = HashingUtils::HashString("FULL_CONTROL");
        static const int READ_HASH = HashingUtils::HashString("READ");
        static const int WRITE_HASH = HashingUtils::HashString("WRITE");


        BucketLogsPermission GetBucketLogsPermissionForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == FULL_CONTROL_HASH)
          {
            return BucketLogsPermission::FULL_CONTROL;
          }
          else if (hashCode == READ_HASH)
          {
            return BucketLogsPermission::READ;
          }
          else if (hashCode == WRITE_HASH)
          {
            return BucketLogsPermission::WRITE;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<BucketLogsPermission>(hashCode);
          }

          return BucketLogsPermission::NOT_SET;
        }

        Aws::String GetNameForBucketLogsPermission(BucketLogsPermission enumValue)
        {
          switch(enumValue)
          {
          case BucketLogsPermission::FULL_CONTROL:
            return "FULL_CONTROL";
          case BucketLogsPermission::READ:
            return "READ";
          case BucketLogsPermission::WRITE:
            return "WRITE";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace BucketLogsPermissionMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
