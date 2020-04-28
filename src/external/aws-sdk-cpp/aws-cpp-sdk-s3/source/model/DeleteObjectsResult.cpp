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

#include <aws/s3/model/DeleteObjectsResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

DeleteObjectsResult::DeleteObjectsResult() : 
    m_requestCharged(RequestCharged::NOT_SET)
{
}

DeleteObjectsResult::DeleteObjectsResult(const Aws::AmazonWebServiceResult<XmlDocument>& result) : 
    m_requestCharged(RequestCharged::NOT_SET)
{
  *this = result;
}

DeleteObjectsResult& DeleteObjectsResult::operator =(const Aws::AmazonWebServiceResult<XmlDocument>& result)
{
  const XmlDocument& xmlDocument = result.GetPayload();
  XmlNode resultNode = xmlDocument.GetRootElement();

  if(!resultNode.IsNull())
  {
    XmlNode deletedNode = resultNode.FirstChild("Deleted");
    if(!deletedNode.IsNull())
    {
      XmlNode deletedMember = deletedNode;
      while(!deletedMember.IsNull())
      {
        m_deleted.push_back(deletedMember);
        deletedMember = deletedMember.NextNode("Deleted");
      }

    }
    XmlNode errorsNode = resultNode.FirstChild("Error");
    if(!errorsNode.IsNull())
    {
      XmlNode errorMember = errorsNode;
      while(!errorMember.IsNull())
      {
        m_errors.push_back(errorMember);
        errorMember = errorMember.NextNode("Error");
      }

    }
  }

  const auto& headers = result.GetHeaderValueCollection();
  const auto& requestChargedIter = headers.find("x-amz-request-charged");
  if(requestChargedIter != headers.end())
  {
    m_requestCharged = RequestChargedMapper::GetRequestChargedForName(requestChargedIter->second);
  }

  return *this;
}
