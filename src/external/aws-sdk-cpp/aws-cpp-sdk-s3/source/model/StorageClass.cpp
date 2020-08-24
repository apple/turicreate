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

#include <aws/s3/model/StorageClass.h>
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
      namespace StorageClassMapper
      {

        static const int STANDARD_HASH = HashingUtils::HashString("STANDARD");
        static const int REDUCED_REDUNDANCY_HASH = HashingUtils::HashString("REDUCED_REDUNDANCY");
        static const int STANDARD_IA_HASH = HashingUtils::HashString("STANDARD_IA");
        static const int ONEZONE_IA_HASH = HashingUtils::HashString("ONEZONE_IA");
        static const int INTELLIGENT_TIERING_HASH = HashingUtils::HashString("INTELLIGENT_TIERING");
        static const int GLACIER_HASH = HashingUtils::HashString("GLACIER");
        static const int DEEP_ARCHIVE_HASH = HashingUtils::HashString("DEEP_ARCHIVE");


        StorageClass GetStorageClassForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == STANDARD_HASH)
          {
            return StorageClass::STANDARD;
          }
          else if (hashCode == REDUCED_REDUNDANCY_HASH)
          {
            return StorageClass::REDUCED_REDUNDANCY;
          }
          else if (hashCode == STANDARD_IA_HASH)
          {
            return StorageClass::STANDARD_IA;
          }
          else if (hashCode == ONEZONE_IA_HASH)
          {
            return StorageClass::ONEZONE_IA;
          }
          else if (hashCode == INTELLIGENT_TIERING_HASH)
          {
            return StorageClass::INTELLIGENT_TIERING;
          }
          else if (hashCode == GLACIER_HASH)
          {
            return StorageClass::GLACIER;
          }
          else if (hashCode == DEEP_ARCHIVE_HASH)
          {
            return StorageClass::DEEP_ARCHIVE;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<StorageClass>(hashCode);
          }

          return StorageClass::NOT_SET;
        }

        Aws::String GetNameForStorageClass(StorageClass enumValue)
        {
          switch(enumValue)
          {
          case StorageClass::STANDARD:
            return "STANDARD";
          case StorageClass::REDUCED_REDUNDANCY:
            return "REDUCED_REDUNDANCY";
          case StorageClass::STANDARD_IA:
            return "STANDARD_IA";
          case StorageClass::ONEZONE_IA:
            return "ONEZONE_IA";
          case StorageClass::INTELLIGENT_TIERING:
            return "INTELLIGENT_TIERING";
          case StorageClass::GLACIER:
            return "GLACIER";
          case StorageClass::DEEP_ARCHIVE:
            return "DEEP_ARCHIVE";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace StorageClassMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
