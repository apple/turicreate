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
#include <aws/s3/model/ListMultipartUploadsResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

ListMultipartUploadsResult::ListMultipartUploadsResult() : 
    m_maxUploads(0),
    m_isTruncated(false)
{
}

ListMultipartUploadsResult::ListMultipartUploadsResult(const AmazonWebServiceResult<XmlDocument>& result) : 
    m_maxUploads(0),
    m_isTruncated(false)
{
  *this = result;
}

ListMultipartUploadsResult& ListMultipartUploadsResult::operator =(const AmazonWebServiceResult<XmlDocument>& result)
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
    XmlNode keyMarkerNode = resultNode.FirstChild("KeyMarker");
    if(!keyMarkerNode.IsNull())
    {
      m_keyMarker = StringUtils::Trim(keyMarkerNode.GetText().c_str());
    }
    XmlNode uploadIdMarkerNode = resultNode.FirstChild("UploadIdMarker");
    if(!uploadIdMarkerNode.IsNull())
    {
      m_uploadIdMarker = StringUtils::Trim(uploadIdMarkerNode.GetText().c_str());
    }
    XmlNode nextKeyMarkerNode = resultNode.FirstChild("NextKeyMarker");
    if(!nextKeyMarkerNode.IsNull())
    {
      m_nextKeyMarker = StringUtils::Trim(nextKeyMarkerNode.GetText().c_str());
    }
    XmlNode prefixNode = resultNode.FirstChild("Prefix");
    if(!prefixNode.IsNull())
    {
      m_prefix = StringUtils::Trim(prefixNode.GetText().c_str());
    }
    XmlNode delimiterNode = resultNode.FirstChild("Delimiter");
    if(!delimiterNode.IsNull())
    {
      m_delimiter = StringUtils::Trim(delimiterNode.GetText().c_str());
    }
    XmlNode nextUploadIdMarkerNode = resultNode.FirstChild("NextUploadIdMarker");
    if(!nextUploadIdMarkerNode.IsNull())
    {
      m_nextUploadIdMarker = StringUtils::Trim(nextUploadIdMarkerNode.GetText().c_str());
    }
    XmlNode maxUploadsNode = resultNode.FirstChild("MaxUploads");
    if(!maxUploadsNode.IsNull())
    {
      m_maxUploads = StringUtils::ConvertToInt32(StringUtils::Trim(maxUploadsNode.GetText().c_str()).c_str());
    }
    XmlNode isTruncatedNode = resultNode.FirstChild("IsTruncated");
    if(!isTruncatedNode.IsNull())
    {
      m_isTruncated = StringUtils::ConvertToBool(StringUtils::Trim(isTruncatedNode.GetText().c_str()).c_str());
    }
    XmlNode uploadsNode = resultNode.FirstChild("Upload");
    if(!uploadsNode.IsNull())
    {
      XmlNode uploadMember = uploadsNode;
      while(!uploadMember.IsNull())
      {
        m_uploads.push_back(uploadMember);
        uploadMember = uploadMember.NextNode("Upload");
      }

    }
    XmlNode commonPrefixesNode = resultNode.FirstChild("CommonPrefixes");
    if(!commonPrefixesNode.IsNull())
    {
      XmlNode commonPrefixesMember = commonPrefixesNode;
      while(!commonPrefixesMember.IsNull())
      {
        m_commonPrefixes.push_back(commonPrefixesMember);
        commonPrefixesMember = commonPrefixesMember.NextNode("CommonPrefixes");
      }

    }
    XmlNode encodingTypeNode = resultNode.FirstChild("EncodingType");
    if(!encodingTypeNode.IsNull())
    {
      m_encodingType = EncodingTypeMapper::GetEncodingTypeForName(StringUtils::Trim(encodingTypeNode.GetText().c_str()).c_str());
    }
  }

  return *this;
}
