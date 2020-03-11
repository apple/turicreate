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

#include <aws/s3/model/GetBucketPolicyResult.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Stream;
using namespace Aws::Utils;
using namespace Aws;

GetBucketPolicyResult::GetBucketPolicyResult()
{
}

GetBucketPolicyResult::GetBucketPolicyResult(GetBucketPolicyResult&& toMove) : 
    m_policy(std::move(toMove.m_policy))
{
}

GetBucketPolicyResult& GetBucketPolicyResult::operator=(GetBucketPolicyResult&& toMove)
{
   if(this == &toMove)
   {
      return *this;
   }

   m_policy = std::move(toMove.m_policy);

   return *this;
}

GetBucketPolicyResult::GetBucketPolicyResult(Aws::AmazonWebServiceResult<ResponseStream>&& result)
{
  *this = std::move(result);
}

GetBucketPolicyResult& GetBucketPolicyResult::operator =(Aws::AmazonWebServiceResult<ResponseStream>&& result)
{
  m_policy = result.TakeOwnershipOfPayload();

   return *this;
}
