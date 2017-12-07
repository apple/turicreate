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
#include <aws/s3/model/Event.h>
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
      namespace EventMapper
      {

        static const int s3_ReducedRedundancyLostObject_HASH = HashingUtils::HashString("s3:ReducedRedundancyLostObject");
        static const int s3_ObjectCreated_HASH = HashingUtils::HashString("s3:ObjectCreated:*");
        static const int s3_ObjectCreated_Put_HASH = HashingUtils::HashString("s3:ObjectCreated:Put");
        static const int s3_ObjectCreated_Post_HASH = HashingUtils::HashString("s3:ObjectCreated:Post");
        static const int s3_ObjectCreated_Copy_HASH = HashingUtils::HashString("s3:ObjectCreated:Copy");
        static const int s3_ObjectCreated_CompleteMultipartUpload_HASH = HashingUtils::HashString("s3:ObjectCreated:CompleteMultipartUpload");
        static const int s3_ObjectRemoved_HASH = HashingUtils::HashString("s3:ObjectRemoved:*");
        static const int s3_ObjectRemoved_Delete_HASH = HashingUtils::HashString("s3:ObjectRemoved:Delete");
        static const int s3_ObjectRemoved_DeleteMarkerCreated_HASH = HashingUtils::HashString("s3:ObjectRemoved:DeleteMarkerCreated");


        Event GetEventForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == s3_ReducedRedundancyLostObject_HASH)
          {
            return Event::s3_ReducedRedundancyLostObject;
          }
          else if (hashCode == s3_ObjectCreated_HASH)
          {
            return Event::s3_ObjectCreated;
          }
          else if (hashCode == s3_ObjectCreated_Put_HASH)
          {
            return Event::s3_ObjectCreated_Put;
          }
          else if (hashCode == s3_ObjectCreated_Post_HASH)
          {
            return Event::s3_ObjectCreated_Post;
          }
          else if (hashCode == s3_ObjectCreated_Copy_HASH)
          {
            return Event::s3_ObjectCreated_Copy;
          }
          else if (hashCode == s3_ObjectCreated_CompleteMultipartUpload_HASH)
          {
            return Event::s3_ObjectCreated_CompleteMultipartUpload;
          }
          else if (hashCode == s3_ObjectRemoved_HASH)
          {
            return Event::s3_ObjectRemoved;
          }
          else if (hashCode == s3_ObjectRemoved_Delete_HASH)
          {
            return Event::s3_ObjectRemoved_Delete;
          }
          else if (hashCode == s3_ObjectRemoved_DeleteMarkerCreated_HASH)
          {
            return Event::s3_ObjectRemoved_DeleteMarkerCreated;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<Event>(hashCode);
          }

          return Event::NOT_SET;
        }

        Aws::String GetNameForEvent(Event enumValue)
        {
          switch(enumValue)
          {
          case Event::s3_ReducedRedundancyLostObject:
            return "s3:ReducedRedundancyLostObject";
          case Event::s3_ObjectCreated:
            return "s3:ObjectCreated:*";
          case Event::s3_ObjectCreated_Put:
            return "s3:ObjectCreated:Put";
          case Event::s3_ObjectCreated_Post:
            return "s3:ObjectCreated:Post";
          case Event::s3_ObjectCreated_Copy:
            return "s3:ObjectCreated:Copy";
          case Event::s3_ObjectCreated_CompleteMultipartUpload:
            return "s3:ObjectCreated:CompleteMultipartUpload";
          case Event::s3_ObjectRemoved:
            return "s3:ObjectRemoved:*";
          case Event::s3_ObjectRemoved_Delete:
            return "s3:ObjectRemoved:Delete";
          case Event::s3_ObjectRemoved_DeleteMarkerCreated:
            return "s3:ObjectRemoved:DeleteMarkerCreated";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return "";
          }
        }

      } // namespace EventMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
