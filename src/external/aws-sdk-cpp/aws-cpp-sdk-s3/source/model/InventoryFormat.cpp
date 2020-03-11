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

#include <aws/s3/model/InventoryFormat.h>
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
      namespace InventoryFormatMapper
      {

        static const int CSV_HASH = HashingUtils::HashString("CSV");
        static const int ORC_HASH = HashingUtils::HashString("ORC");
        static const int Parquet_HASH = HashingUtils::HashString("Parquet");


        InventoryFormat GetInventoryFormatForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == CSV_HASH)
          {
            return InventoryFormat::CSV;
          }
          else if (hashCode == ORC_HASH)
          {
            return InventoryFormat::ORC;
          }
          else if (hashCode == Parquet_HASH)
          {
            return InventoryFormat::Parquet;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<InventoryFormat>(hashCode);
          }

          return InventoryFormat::NOT_SET;
        }

        Aws::String GetNameForInventoryFormat(InventoryFormat enumValue)
        {
          switch(enumValue)
          {
          case InventoryFormat::CSV:
            return "CSV";
          case InventoryFormat::ORC:
            return "ORC";
          case InventoryFormat::Parquet:
            return "Parquet";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace InventoryFormatMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
