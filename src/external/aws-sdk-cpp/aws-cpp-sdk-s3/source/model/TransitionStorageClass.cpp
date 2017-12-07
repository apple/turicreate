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
#include <aws/s3/model/TransitionStorageClass.h>
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
      namespace TransitionStorageClassMapper
      {

        static const int GLACIER_HASH = HashingUtils::HashString("GLACIER");
        static const int STANDARD_IA_HASH = HashingUtils::HashString("STANDARD_IA");


        TransitionStorageClass GetTransitionStorageClassForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == GLACIER_HASH)
          {
            return TransitionStorageClass::GLACIER;
          }
          else if (hashCode == STANDARD_IA_HASH)
          {
            return TransitionStorageClass::STANDARD_IA;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<TransitionStorageClass>(hashCode);
          }

          return TransitionStorageClass::NOT_SET;
        }

        Aws::String GetNameForTransitionStorageClass(TransitionStorageClass enumValue)
        {
          switch(enumValue)
          {
          case TransitionStorageClass::GLACIER:
            return "GLACIER";
          case TransitionStorageClass::STANDARD_IA:
            return "STANDARD_IA";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return "";
          }
        }

      } // namespace TransitionStorageClassMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
