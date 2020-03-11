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

#include <aws/s3/model/GetBucketAclResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

GetBucketAclResult::GetBucketAclResult()
{
}

GetBucketAclResult::GetBucketAclResult(const Aws::AmazonWebServiceResult<XmlDocument>& result)
{
  *this = result;
}

GetBucketAclResult& GetBucketAclResult::operator =(const Aws::AmazonWebServiceResult<XmlDocument>& result)
{
  const XmlDocument& xmlDocument = result.GetPayload();
  XmlNode resultNode = xmlDocument.GetRootElement();

  if(!resultNode.IsNull())
  {
    XmlNode ownerNode = resultNode.FirstChild("Owner");
    if(!ownerNode.IsNull())
    {
      m_owner = ownerNode;
    }
    XmlNode grantsNode = resultNode.FirstChild("AccessControlList");
    if(!grantsNode.IsNull())
    {
      XmlNode grantsMember = grantsNode.FirstChild("Grant");
      while(!grantsMember.IsNull())
      {
        m_grants.push_back(grantsMember);
        grantsMember = grantsMember.NextNode("Grant");
      }

    }
  }

  return *this;
}
