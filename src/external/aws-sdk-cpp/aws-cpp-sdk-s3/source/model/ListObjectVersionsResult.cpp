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
#include <aws/s3/model/ListObjectVersionsResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

ListObjectVersionsResult::ListObjectVersionsResult() : 
    m_isTruncated(false),
    m_maxKeys(0)
{
}

ListObjectVersionsResult::ListObjectVersionsResult(const AmazonWebServiceResult<XmlDocument>& result) : 
    m_isTruncated(false),
    m_maxKeys(0)
{
  *this = result;
}

ListObjectVersionsResult& ListObjectVersionsResult::operator =(const AmazonWebServiceResult<XmlDocument>& result)
{
  const XmlDocument& xmlDocument = result.GetPayload();
  XmlNode resultNode = xmlDocument.GetRootElement();

  if(!resultNode.IsNull())
  {
    XmlNode isTruncatedNode = resultNode.FirstChild("IsTruncated");
    if(!isTruncatedNode.IsNull())
    {
      m_isTruncated = StringUtils::ConvertToBool(StringUtils::Trim(isTruncatedNode.GetText().c_str()).c_str());
    }
    XmlNode keyMarkerNode = resultNode.FirstChild("KeyMarker");
    if(!keyMarkerNode.IsNull())
    {
      m_keyMarker = StringUtils::Trim(keyMarkerNode.GetText().c_str());
    }
    XmlNode versionIdMarkerNode = resultNode.FirstChild("VersionIdMarker");
    if(!versionIdMarkerNode.IsNull())
    {
      m_versionIdMarker = StringUtils::Trim(versionIdMarkerNode.GetText().c_str());
    }
    XmlNode nextKeyMarkerNode = resultNode.FirstChild("NextKeyMarker");
    if(!nextKeyMarkerNode.IsNull())
    {
      m_nextKeyMarker = StringUtils::Trim(nextKeyMarkerNode.GetText().c_str());
    }
    XmlNode nextVersionIdMarkerNode = resultNode.FirstChild("NextVersionIdMarker");
    if(!nextVersionIdMarkerNode.IsNull())
    {
      m_nextVersionIdMarker = StringUtils::Trim(nextVersionIdMarkerNode.GetText().c_str());
    }
    XmlNode versionsNode = resultNode.FirstChild("Version");
    if(!versionsNode.IsNull())
    {
      XmlNode versionMember = versionsNode;
      while(!versionMember.IsNull())
      {
        m_versions.push_back(versionMember);
        versionMember = versionMember.NextNode("Version");
      }

    }
    XmlNode deleteMarkersNode = resultNode.FirstChild("DeleteMarker");
    if(!deleteMarkersNode.IsNull())
    {
      XmlNode deleteMarkerMember = deleteMarkersNode;
      while(!deleteMarkerMember.IsNull())
      {
        m_deleteMarkers.push_back(deleteMarkerMember);
        deleteMarkerMember = deleteMarkerMember.NextNode("DeleteMarker");
      }

    }
    XmlNode nameNode = resultNode.FirstChild("Name");
    if(!nameNode.IsNull())
    {
      m_name = StringUtils::Trim(nameNode.GetText().c_str());
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
    XmlNode maxKeysNode = resultNode.FirstChild("MaxKeys");
    if(!maxKeysNode.IsNull())
    {
      m_maxKeys = StringUtils::ConvertToInt32(StringUtils::Trim(maxKeysNode.GetText().c_str()).c_str());
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
