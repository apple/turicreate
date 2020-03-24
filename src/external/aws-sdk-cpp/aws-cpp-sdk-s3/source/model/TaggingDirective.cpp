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

#include <aws/s3/model/TaggingDirective.h>
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
      namespace TaggingDirectiveMapper
      {

        static const int COPY_HASH = HashingUtils::HashString("COPY");
        static const int REPLACE_HASH = HashingUtils::HashString("REPLACE");


        TaggingDirective GetTaggingDirectiveForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == COPY_HASH)
          {
            return TaggingDirective::COPY;
          }
          else if (hashCode == REPLACE_HASH)
          {
            return TaggingDirective::REPLACE;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<TaggingDirective>(hashCode);
          }

          return TaggingDirective::NOT_SET;
        }

        Aws::String GetNameForTaggingDirective(TaggingDirective enumValue)
        {
          switch(enumValue)
          {
          case TaggingDirective::COPY:
            return "COPY";
          case TaggingDirective::REPLACE:
            return "REPLACE";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace TaggingDirectiveMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
