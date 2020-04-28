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

#include <aws/s3/model/ExistingObjectReplicationStatus.h>
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
      namespace ExistingObjectReplicationStatusMapper
      {

        static const int Enabled_HASH = HashingUtils::HashString("Enabled");
        static const int Disabled_HASH = HashingUtils::HashString("Disabled");


        ExistingObjectReplicationStatus GetExistingObjectReplicationStatusForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == Enabled_HASH)
          {
            return ExistingObjectReplicationStatus::Enabled;
          }
          else if (hashCode == Disabled_HASH)
          {
            return ExistingObjectReplicationStatus::Disabled;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<ExistingObjectReplicationStatus>(hashCode);
          }

          return ExistingObjectReplicationStatus::NOT_SET;
        }

        Aws::String GetNameForExistingObjectReplicationStatus(ExistingObjectReplicationStatus enumValue)
        {
          switch(enumValue)
          {
          case ExistingObjectReplicationStatus::Enabled:
            return "Enabled";
          case ExistingObjectReplicationStatus::Disabled:
            return "Disabled";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace ExistingObjectReplicationStatusMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
