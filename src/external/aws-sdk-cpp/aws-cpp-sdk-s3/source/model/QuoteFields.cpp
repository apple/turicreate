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

#include <aws/s3/model/QuoteFields.h>
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
      namespace QuoteFieldsMapper
      {

        static const int ALWAYS_HASH = HashingUtils::HashString("ALWAYS");
        static const int ASNEEDED_HASH = HashingUtils::HashString("ASNEEDED");


        QuoteFields GetQuoteFieldsForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == ALWAYS_HASH)
          {
            return QuoteFields::ALWAYS;
          }
          else if (hashCode == ASNEEDED_HASH)
          {
            return QuoteFields::ASNEEDED;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<QuoteFields>(hashCode);
          }

          return QuoteFields::NOT_SET;
        }

        Aws::String GetNameForQuoteFields(QuoteFields enumValue)
        {
          switch(enumValue)
          {
          case QuoteFields::ALWAYS:
            return "ALWAYS";
          case QuoteFields::ASNEEDED:
            return "ASNEEDED";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace QuoteFieldsMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
