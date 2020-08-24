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

#include <aws/s3/model/Part.h>
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

Part::Part() : 
    m_partNumber(0),
    m_partNumberHasBeenSet(false),
    m_lastModifiedHasBeenSet(false),
    m_eTagHasBeenSet(false),
    m_size(0),
    m_sizeHasBeenSet(false)
{
}

Part::Part(const XmlNode& xmlNode) : 
    m_partNumber(0),
    m_partNumberHasBeenSet(false),
    m_lastModifiedHasBeenSet(false),
    m_eTagHasBeenSet(false),
    m_size(0),
    m_sizeHasBeenSet(false)
{
  *this = xmlNode;
}

Part& Part::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode partNumberNode = resultNode.FirstChild("PartNumber");
    if(!partNumberNode.IsNull())
    {
      m_partNumber = StringUtils::ConvertToInt32(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(partNumberNode.GetText()).c_str()).c_str());
      m_partNumberHasBeenSet = true;
    }
    XmlNode lastModifiedNode = resultNode.FirstChild("LastModified");
    if(!lastModifiedNode.IsNull())
    {
      m_lastModified = DateTime(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(lastModifiedNode.GetText()).c_str()).c_str(), DateFormat::ISO_8601);
      m_lastModifiedHasBeenSet = true;
    }
    XmlNode eTagNode = resultNode.FirstChild("ETag");
    if(!eTagNode.IsNull())
    {
      m_eTag = Aws::Utils::Xml::DecodeEscapedXmlText(eTagNode.GetText());
      m_eTagHasBeenSet = true;
    }
    XmlNode sizeNode = resultNode.FirstChild("Size");
    if(!sizeNode.IsNull())
    {
      m_size = StringUtils::ConvertToInt64(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(sizeNode.GetText()).c_str()).c_str());
      m_sizeHasBeenSet = true;
    }
  }

  return *this;
}

void Part::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_partNumberHasBeenSet)
  {
   XmlNode partNumberNode = parentNode.CreateChildElement("PartNumber");
   ss << m_partNumber;
   partNumberNode.SetText(ss.str());
   ss.str("");
  }

  if(m_lastModifiedHasBeenSet)
  {
   XmlNode lastModifiedNode = parentNode.CreateChildElement("LastModified");
   lastModifiedNode.SetText(m_lastModified.ToGmtString(DateFormat::ISO_8601));
  }

  if(m_eTagHasBeenSet)
  {
   XmlNode eTagNode = parentNode.CreateChildElement("ETag");
   eTagNode.SetText(m_eTag);
  }

  if(m_sizeHasBeenSet)
  {
   XmlNode sizeNode = parentNode.CreateChildElement("Size");
   ss << m_size;
   sizeNode.SetText(ss.str());
   ss.str("");
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
