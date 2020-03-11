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

#include <aws/s3/model/GetObjectTorrentResult.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Stream;
using namespace Aws::Utils;
using namespace Aws;

GetObjectTorrentResult::GetObjectTorrentResult() : 
    m_requestCharged(RequestCharged::NOT_SET)
{
}

GetObjectTorrentResult::GetObjectTorrentResult(GetObjectTorrentResult&& toMove) : 
    m_body(std::move(toMove.m_body)),
    m_requestCharged(toMove.m_requestCharged)
{
}

GetObjectTorrentResult& GetObjectTorrentResult::operator=(GetObjectTorrentResult&& toMove)
{
   if(this == &toMove)
   {
      return *this;
   }

   m_body = std::move(toMove.m_body);
   m_requestCharged = toMove.m_requestCharged;

   return *this;
}

GetObjectTorrentResult::GetObjectTorrentResult(Aws::AmazonWebServiceResult<ResponseStream>&& result) : 
    m_requestCharged(RequestCharged::NOT_SET)
{
  *this = std::move(result);
}

GetObjectTorrentResult& GetObjectTorrentResult::operator =(Aws::AmazonWebServiceResult<ResponseStream>&& result)
{
  m_body = result.TakeOwnershipOfPayload();

  const auto& headers = result.GetHeaderValueCollection();
  const auto& requestChargedIter = headers.find("x-amz-request-charged");
  if(requestChargedIter != headers.end())
  {
    m_requestCharged = RequestChargedMapper::GetRequestChargedForName(requestChargedIter->second);
  }

   return *this;
}
