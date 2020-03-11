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

#include <aws/s3/model/InventoryIncludedObjectVersions.h>
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
      namespace InventoryIncludedObjectVersionsMapper
      {

        static const int All_HASH = HashingUtils::HashString("All");
        static const int Current_HASH = HashingUtils::HashString("Current");


        InventoryIncludedObjectVersions GetInventoryIncludedObjectVersionsForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == All_HASH)
          {
            return InventoryIncludedObjectVersions::All;
          }
          else if (hashCode == Current_HASH)
          {
            return InventoryIncludedObjectVersions::Current;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<InventoryIncludedObjectVersions>(hashCode);
          }

          return InventoryIncludedObjectVersions::NOT_SET;
        }

        Aws::String GetNameForInventoryIncludedObjectVersions(InventoryIncludedObjectVersions enumValue)
        {
          switch(enumValue)
          {
          case InventoryIncludedObjectVersions::All:
            return "All";
          case InventoryIncludedObjectVersions::Current:
            return "Current";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace InventoryIncludedObjectVersionsMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
