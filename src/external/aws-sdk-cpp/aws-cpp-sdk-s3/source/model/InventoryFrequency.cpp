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

#include <aws/s3/model/InventoryFrequency.h>
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
      namespace InventoryFrequencyMapper
      {

        static const int Daily_HASH = HashingUtils::HashString("Daily");
        static const int Weekly_HASH = HashingUtils::HashString("Weekly");


        InventoryFrequency GetInventoryFrequencyForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == Daily_HASH)
          {
            return InventoryFrequency::Daily;
          }
          else if (hashCode == Weekly_HASH)
          {
            return InventoryFrequency::Weekly;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<InventoryFrequency>(hashCode);
          }

          return InventoryFrequency::NOT_SET;
        }

        Aws::String GetNameForInventoryFrequency(InventoryFrequency enumValue)
        {
          switch(enumValue)
          {
          case InventoryFrequency::Daily:
            return "Daily";
          case InventoryFrequency::Weekly:
            return "Weekly";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace InventoryFrequencyMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
