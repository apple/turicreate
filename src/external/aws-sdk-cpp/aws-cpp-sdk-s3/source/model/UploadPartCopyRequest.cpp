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

#include <aws/s3/model/UploadPartCopyRequest.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/http/URI.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws::Http;

UploadPartCopyRequest::UploadPartCopyRequest() : 
    m_bucketHasBeenSet(false),
    m_copySourceHasBeenSet(false),
    m_copySourceIfMatchHasBeenSet(false),
    m_copySourceIfModifiedSinceHasBeenSet(false),
    m_copySourceIfNoneMatchHasBeenSet(false),
    m_copySourceIfUnmodifiedSinceHasBeenSet(false),
    m_copySourceRangeHasBeenSet(false),
    m_keyHasBeenSet(false),
    m_partNumber(0),
    m_partNumberHasBeenSet(false),
    m_uploadIdHasBeenSet(false),
    m_sSECustomerAlgorithmHasBeenSet(false),
    m_sSECustomerKeyHasBeenSet(false),
    m_sSECustomerKeyMD5HasBeenSet(false),
    m_copySourceSSECustomerAlgorithmHasBeenSet(false),
    m_copySourceSSECustomerKeyHasBeenSet(false),
    m_copySourceSSECustomerKeyMD5HasBeenSet(false),
    m_requestPayer(RequestPayer::NOT_SET),
    m_requestPayerHasBeenSet(false),
    m_customizedAccessLogTagHasBeenSet(false)
{
}

Aws::String UploadPartCopyRequest::SerializePayload() const
{
  return {};
}

void UploadPartCopyRequest::AddQueryStringParameters(URI& uri) const
{
    Aws::StringStream ss;
    if(m_partNumberHasBeenSet)
    {
      ss << m_partNumber;
      uri.AddQueryStringParameter("partNumber", ss.str());
      ss.str("");
    }

    if(m_uploadIdHasBeenSet)
    {
      ss << m_uploadId;
      uri.AddQueryStringParameter("uploadId", ss.str());
      ss.str("");
    }

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

Aws::Http::HeaderValueCollection UploadPartCopyRequest::GetRequestSpecificHeaders() const
{
  Aws::Http::HeaderValueCollection headers;
  Aws::StringStream ss;
  if(m_copySourceHasBeenSet)
  {
    ss << m_copySource;
    headers.emplace("x-amz-copy-source", URI::URLEncodePath(ss.str()));
    ss.str("");
  }

  if(m_copySourceIfMatchHasBeenSet)
  {
    ss << m_copySourceIfMatch;
    headers.emplace("x-amz-copy-source-if-match",  ss.str());
    ss.str("");
  }

  if(m_copySourceIfModifiedSinceHasBeenSet)
  {
    headers.emplace("x-amz-copy-source-if-modified-since", m_copySourceIfModifiedSince.ToGmtString(DateFormat::RFC822));
  }

  if(m_copySourceIfNoneMatchHasBeenSet)
  {
    ss << m_copySourceIfNoneMatch;
    headers.emplace("x-amz-copy-source-if-none-match",  ss.str());
    ss.str("");
  }

  if(m_copySourceIfUnmodifiedSinceHasBeenSet)
  {
    headers.emplace("x-amz-copy-source-if-unmodified-since", m_copySourceIfUnmodifiedSince.ToGmtString(DateFormat::RFC822));
  }

  if(m_copySourceRangeHasBeenSet)
  {
    ss << m_copySourceRange;
    headers.emplace("x-amz-copy-source-range",  ss.str());
    ss.str("");
  }

  if(m_sSECustomerAlgorithmHasBeenSet)
  {
    ss << m_sSECustomerAlgorithm;
    headers.emplace("x-amz-server-side-encryption-customer-algorithm",  ss.str());
    ss.str("");
  }

  if(m_sSECustomerKeyHasBeenSet)
  {
    ss << m_sSECustomerKey;
    headers.emplace("x-amz-server-side-encryption-customer-key",  ss.str());
    ss.str("");
  }

  if(m_sSECustomerKeyMD5HasBeenSet)
  {
    ss << m_sSECustomerKeyMD5;
    headers.emplace("x-amz-server-side-encryption-customer-key-md5",  ss.str());
    ss.str("");
  }

  if(m_copySourceSSECustomerAlgorithmHasBeenSet)
  {
    ss << m_copySourceSSECustomerAlgorithm;
    headers.emplace("x-amz-copy-source-server-side-encryption-customer-algorithm",  ss.str());
    ss.str("");
  }

  if(m_copySourceSSECustomerKeyHasBeenSet)
  {
    ss << m_copySourceSSECustomerKey;
    headers.emplace("x-amz-copy-source-server-side-encryption-customer-key",  ss.str());
    ss.str("");
  }

  if(m_copySourceSSECustomerKeyMD5HasBeenSet)
  {
    ss << m_copySourceSSECustomerKeyMD5;
    headers.emplace("x-amz-copy-source-server-side-encryption-customer-key-md5",  ss.str());
    ss.str("");
  }

  if(m_requestPayerHasBeenSet)
  {
    headers.emplace("x-amz-request-payer", RequestPayerMapper::GetNameForRequestPayer(m_requestPayer));
  }

  return headers;
}
