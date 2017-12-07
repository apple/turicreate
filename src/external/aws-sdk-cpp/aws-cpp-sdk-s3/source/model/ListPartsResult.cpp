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
#include <aws/s3/model/ListPartsResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

ListPartsResult::ListPartsResult() : 
    m_partNumberMarker(0),
    m_nextPartNumberMarker(0),
    m_maxParts(0),
    m_isTruncated(false)
{
}

ListPartsResult::ListPartsResult(const AmazonWebServiceResult<XmlDocument>& result) : 
    m_partNumberMarker(0),
    m_nextPartNumberMarker(0),
    m_maxParts(0),
    m_isTruncated(false)
{
  *this = result;
}

ListPartsResult& ListPartsResult::operator =(const AmazonWebServiceResult<XmlDocument>& result)
{
  const XmlDocument& xmlDocument = result.GetPayload();
  XmlNode resultNode = xmlDocument.GetRootElement();

  if(!resultNode.IsNull())
  {
    XmlNode bucketNode = resultNode.FirstChild("Bucket");
    if(!bucketNode.IsNull())
    {
      m_bucket = StringUtils::Trim(bucketNode.GetText().c_str());
    }
    XmlNode keyNode = resultNode.FirstChild("Key");
    if(!keyNode.IsNull())
    {
      m_key = StringUtils::Trim(keyNode.GetText().c_str());
    }
    XmlNode uploadIdNode = resultNode.FirstChild("UploadId");
    if(!uploadIdNode.IsNull())
    {
      m_uploadId = StringUtils::Trim(uploadIdNode.GetText().c_str());
    }
    XmlNode partNumberMarkerNode = resultNode.FirstChild("PartNumberMarker");
    if(!partNumberMarkerNode.IsNull())
    {
      m_partNumberMarker = StringUtils::ConvertToInt32(StringUtils::Trim(partNumberMarkerNode.GetText().c_str()).c_str());
    }
    XmlNode nextPartNumberMarkerNode = resultNode.FirstChild("NextPartNumberMarker");
    if(!nextPartNumberMarkerNode.IsNull())
    {
      m_nextPartNumberMarker = StringUtils::ConvertToInt32(StringUtils::Trim(nextPartNumberMarkerNode.GetText().c_str()).c_str());
    }
    XmlNode maxPartsNode = resultNode.FirstChild("MaxParts");
    if(!maxPartsNode.IsNull())
    {
      m_maxParts = StringUtils::ConvertToInt32(StringUtils::Trim(maxPartsNode.GetText().c_str()).c_str());
    }
    XmlNode isTruncatedNode = resultNode.FirstChild("IsTruncated");
    if(!isTruncatedNode.IsNull())
    {
      m_isTruncated = StringUtils::ConvertToBool(StringUtils::Trim(isTruncatedNode.GetText().c_str()).c_str());
    }
    XmlNode partsNode = resultNode.FirstChild("Part");
    if(!partsNode.IsNull())
    {
      XmlNode partMember = partsNode;
      while(!partMember.IsNull())
      {
        m_parts.push_back(partMember);
        partMember = partMember.NextNode("Part");
      }

    }
    XmlNode initiatorNode = resultNode.FirstChild("Initiator");
    if(!initiatorNode.IsNull())
    {
      m_initiator = initiatorNode;
    }
    XmlNode ownerNode = resultNode.FirstChild("Owner");
    if(!ownerNode.IsNull())
    {
      m_owner = ownerNode;
    }
    XmlNode storageClassNode = resultNode.FirstChild("StorageClass");
    if(!storageClassNode.IsNull())
    {
      m_storageClass = StorageClassMapper::GetStorageClassForName(StringUtils::Trim(storageClassNode.GetText().c_str()).c_str());
    }
  }

  const auto& headers = result.GetHeaderValueCollection();
  const auto& abortDateIter = headers.find("x-amz-abort-date");
  if(abortDateIter != headers.end())
  {
    m_abortDate = DateTime(abortDateIter->second, DateFormat::RFC822);
  }

  const auto& abortRuleIdIter = headers.find("x-amz-abort-rule-id");
  if(abortRuleIdIter != headers.end())
  {
    m_abortRuleId = abortRuleIdIter->second;
  }

  const auto& requestChargedIter = headers.find("x-amz-request-charged");
  if(requestChargedIter != headers.end())
  {
    m_requestCharged = RequestChargedMapper::GetRequestChargedForName(requestChargedIter->second);
  }

  return *this;
}
