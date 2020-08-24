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

#include <aws/s3/model/ListObjectsResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

ListObjectsResult::ListObjectsResult() : 
    m_isTruncated(false),
    m_maxKeys(0),
    m_encodingType(EncodingType::NOT_SET)
{
}

ListObjectsResult::ListObjectsResult(const Aws::AmazonWebServiceResult<XmlDocument>& result) : 
    m_isTruncated(false),
    m_maxKeys(0),
    m_encodingType(EncodingType::NOT_SET)
{
  *this = result;
}

ListObjectsResult& ListObjectsResult::operator =(const Aws::AmazonWebServiceResult<XmlDocument>& result)
{
  const XmlDocument& xmlDocument = result.GetPayload();
  XmlNode resultNode = xmlDocument.GetRootElement();

  if(!resultNode.IsNull())
  {
    XmlNode isTruncatedNode = resultNode.FirstChild("IsTruncated");
    if(!isTruncatedNode.IsNull())
    {
      m_isTruncated = StringUtils::ConvertToBool(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(isTruncatedNode.GetText()).c_str()).c_str());
    }
    XmlNode markerNode = resultNode.FirstChild("Marker");
    if(!markerNode.IsNull())
    {
      m_marker = Aws::Utils::Xml::DecodeEscapedXmlText(markerNode.GetText());
    }
    XmlNode nextMarkerNode = resultNode.FirstChild("NextMarker");
    if(!nextMarkerNode.IsNull())
    {
      m_nextMarker = Aws::Utils::Xml::DecodeEscapedXmlText(nextMarkerNode.GetText());
    }
    XmlNode contentsNode = resultNode.FirstChild("Contents");
    if(!contentsNode.IsNull())
    {
      XmlNode contentsMember = contentsNode;
      while(!contentsMember.IsNull())
      {
        m_contents.push_back(contentsMember);
        contentsMember = contentsMember.NextNode("Contents");
      }

    }
    XmlNode nameNode = resultNode.FirstChild("Name");
    if(!nameNode.IsNull())
    {
      m_name = Aws::Utils::Xml::DecodeEscapedXmlText(nameNode.GetText());
    }
    XmlNode prefixNode = resultNode.FirstChild("Prefix");
    if(!prefixNode.IsNull())
    {
      m_prefix = Aws::Utils::Xml::DecodeEscapedXmlText(prefixNode.GetText());
    }
    XmlNode delimiterNode = resultNode.FirstChild("Delimiter");
    if(!delimiterNode.IsNull())
    {
      m_delimiter = Aws::Utils::Xml::DecodeEscapedXmlText(delimiterNode.GetText());
    }
    XmlNode maxKeysNode = resultNode.FirstChild("MaxKeys");
    if(!maxKeysNode.IsNull())
    {
      m_maxKeys = StringUtils::ConvertToInt32(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(maxKeysNode.GetText()).c_str()).c_str());
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
      m_encodingType = EncodingTypeMapper::GetEncodingTypeForName(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(encodingTypeNode.GetText()).c_str()).c_str());
    }
  }

  return *this;
}
