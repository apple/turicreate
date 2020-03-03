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

#include <aws/s3/model/DeleteMarkerEntry.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::Utils::Xml;
using namespace Aws::Utils;

namespace Aws
{
namespace S3
{
namespace Model
{

DeleteMarkerEntry::DeleteMarkerEntry() : 
    m_ownerHasBeenSet(false),
    m_keyHasBeenSet(false),
    m_versionIdHasBeenSet(false),
    m_isLatest(false),
    m_isLatestHasBeenSet(false),
    m_lastModifiedHasBeenSet(false)
{
}

DeleteMarkerEntry::DeleteMarkerEntry(const XmlNode& xmlNode) : 
    m_ownerHasBeenSet(false),
    m_keyHasBeenSet(false),
    m_versionIdHasBeenSet(false),
    m_isLatest(false),
    m_isLatestHasBeenSet(false),
    m_lastModifiedHasBeenSet(false)
{
  *this = xmlNode;
}

DeleteMarkerEntry& DeleteMarkerEntry::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode ownerNode = resultNode.FirstChild("Owner");
    if(!ownerNode.IsNull())
    {
      m_owner = ownerNode;
      m_ownerHasBeenSet = true;
    }
    XmlNode keyNode = resultNode.FirstChild("Key");
    if(!keyNode.IsNull())
    {
      m_key = Aws::Utils::Xml::DecodeEscapedXmlText(keyNode.GetText());
      m_keyHasBeenSet = true;
    }
    XmlNode versionIdNode = resultNode.FirstChild("VersionId");
    if(!versionIdNode.IsNull())
    {
      m_versionId = Aws::Utils::Xml::DecodeEscapedXmlText(versionIdNode.GetText());
      m_versionIdHasBeenSet = true;
    }
    XmlNode isLatestNode = resultNode.FirstChild("IsLatest");
    if(!isLatestNode.IsNull())
    {
      m_isLatest = StringUtils::ConvertToBool(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(isLatestNode.GetText()).c_str()).c_str());
      m_isLatestHasBeenSet = true;
    }
    XmlNode lastModifiedNode = resultNode.FirstChild("LastModified");
    if(!lastModifiedNode.IsNull())
    {
      m_lastModified = DateTime(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(lastModifiedNode.GetText()).c_str()).c_str(), DateFormat::ISO_8601);
      m_lastModifiedHasBeenSet = true;
    }
  }

  return *this;
}

void DeleteMarkerEntry::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_ownerHasBeenSet)
  {
   XmlNode ownerNode = parentNode.CreateChildElement("Owner");
   m_owner.AddToNode(ownerNode);
  }

  if(m_keyHasBeenSet)
  {
   XmlNode keyNode = parentNode.CreateChildElement("Key");
   keyNode.SetText(m_key);
  }

  if(m_versionIdHasBeenSet)
  {
   XmlNode versionIdNode = parentNode.CreateChildElement("VersionId");
   versionIdNode.SetText(m_versionId);
  }

  if(m_isLatestHasBeenSet)
  {
   XmlNode isLatestNode = parentNode.CreateChildElement("IsLatest");
   ss << std::boolalpha << m_isLatest;
   isLatestNode.SetText(ss.str());
   ss.str("");
  }

  if(m_lastModifiedHasBeenSet)
  {
   XmlNode lastModifiedNode = parentNode.CreateChildElement("LastModified");
   lastModifiedNode.SetText(m_lastModified.ToGmtString(DateFormat::ISO_8601));
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
