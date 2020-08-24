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

#include <aws/s3/model/Permission.h>
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
      namespace PermissionMapper
      {

        static const int FULL_CONTROL_HASH = HashingUtils::HashString("FULL_CONTROL");
        static const int WRITE_HASH = HashingUtils::HashString("WRITE");
        static const int WRITE_ACP_HASH = HashingUtils::HashString("WRITE_ACP");
        static const int READ_HASH = HashingUtils::HashString("READ");
        static const int READ_ACP_HASH = HashingUtils::HashString("READ_ACP");


        Permission GetPermissionForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == FULL_CONTROL_HASH)
          {
            return Permission::FULL_CONTROL;
          }
          else if (hashCode == WRITE_HASH)
          {
            return Permission::WRITE;
          }
          else if (hashCode == WRITE_ACP_HASH)
          {
            return Permission::WRITE_ACP;
          }
          else if (hashCode == READ_HASH)
          {
            return Permission::READ;
          }
          else if (hashCode == READ_ACP_HASH)
          {
            return Permission::READ_ACP;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<Permission>(hashCode);
          }

          return Permission::NOT_SET;
        }

        Aws::String GetNameForPermission(Permission enumValue)
        {
          switch(enumValue)
          {
          case Permission::FULL_CONTROL:
            return "FULL_CONTROL";
          case Permission::WRITE:
            return "WRITE";
          case Permission::WRITE_ACP:
            return "WRITE_ACP";
          case Permission::READ:
            return "READ";
          case Permission::READ_ACP:
            return "READ_ACP";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace PermissionMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
