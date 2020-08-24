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

#include <aws/s3/model/LifecycleExpiration.h>
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

LifecycleExpiration::LifecycleExpiration() : 
    m_dateHasBeenSet(false),
    m_days(0),
    m_daysHasBeenSet(false),
    m_expiredObjectDeleteMarker(false),
    m_expiredObjectDeleteMarkerHasBeenSet(false)
{
}

LifecycleExpiration::LifecycleExpiration(const XmlNode& xmlNode) : 
    m_dateHasBeenSet(false),
    m_days(0),
    m_daysHasBeenSet(false),
    m_expiredObjectDeleteMarker(false),
    m_expiredObjectDeleteMarkerHasBeenSet(false)
{
  *this = xmlNode;
}

LifecycleExpiration& LifecycleExpiration::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode dateNode = resultNode.FirstChild("Date");
    if(!dateNode.IsNull())
    {
      m_date = DateTime(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(dateNode.GetText()).c_str()).c_str(), DateFormat::ISO_8601);
      m_dateHasBeenSet = true;
    }
    XmlNode daysNode = resultNode.FirstChild("Days");
    if(!daysNode.IsNull())
    {
      m_days = StringUtils::ConvertToInt32(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(daysNode.GetText()).c_str()).c_str());
      m_daysHasBeenSet = true;
    }
    XmlNode expiredObjectDeleteMarkerNode = resultNode.FirstChild("ExpiredObjectDeleteMarker");
    if(!expiredObjectDeleteMarkerNode.IsNull())
    {
      m_expiredObjectDeleteMarker = StringUtils::ConvertToBool(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(expiredObjectDeleteMarkerNode.GetText()).c_str()).c_str());
      m_expiredObjectDeleteMarkerHasBeenSet = true;
    }
  }

  return *this;
}

void LifecycleExpiration::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_dateHasBeenSet)
  {
   XmlNode dateNode = parentNode.CreateChildElement("Date");
   dateNode.SetText(m_date.ToGmtString(DateFormat::ISO_8601));
  }

  if(m_daysHasBeenSet)
  {
   XmlNode daysNode = parentNode.CreateChildElement("Days");
   ss << m_days;
   daysNode.SetText(ss.str());
   ss.str("");
  }

  if(m_expiredObjectDeleteMarkerHasBeenSet)
  {
   XmlNode expiredObjectDeleteMarkerNode = parentNode.CreateChildElement("ExpiredObjectDeleteMarker");
   ss << std::boolalpha << m_expiredObjectDeleteMarker;
   expiredObjectDeleteMarkerNode.SetText(ss.str());
   ss.str("");
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
