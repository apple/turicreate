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

#include <aws/s3/model/PutBucketPolicyRequest.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/http/URI.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Stream;
using namespace Aws::Utils;
using namespace Aws::Http;
using namespace Aws;

PutBucketPolicyRequest::PutBucketPolicyRequest() : 
    m_bucketHasBeenSet(false),
    m_contentMD5HasBeenSet(false),
    m_confirmRemoveSelfBucketAccess(false),
    m_confirmRemoveSelfBucketAccessHasBeenSet(false),
    m_customizedAccessLogTagHasBeenSet(false)
{
}


void PutBucketPolicyRequest::AddQueryStringParameters(URI& uri) const
{
    Aws::StringStream ss;
    if(!m_customizedAccessLogTag.empty())
    {
        // only accept customized LogTag which starts with "x-"
        Aws::Map<Aws::String, Aws::String> collectedLogTags;
        for(const auto& entry: m_customizedAccessLogTag)
        {
            if (!entry.first.empty() && !entry.second.empty() && entry.first.substr(0, 2) == "x-")
            {
                collectedLogTags.emplace(entry.first, entry.second);
            }
        }

        if (!collectedLogTags.empty())
        {
            uri.AddQueryStringParameter(collectedLogTags);
        }
    }
}
Aws::Http::HeaderValueCollection PutBucketPolicyRequest::GetRequestSpecificHeaders() const
{
  Aws::Http::HeaderValueCollection headers;
  Aws::StringStream ss;
  if(m_contentMD5HasBeenSet)
  {
    ss << m_contentMD5;
    headers.emplace("content-md5",  ss.str());
    ss.str("");
  }

  if(m_confirmRemoveSelfBucketAccessHasBeenSet)
  {
    ss << m_confirmRemoveSelfBucketAccess;
    headers.emplace("x-amz-confirm-remove-self-bucket-access",  ss.str());
    ss.str("");
  }

  return headers;

}
