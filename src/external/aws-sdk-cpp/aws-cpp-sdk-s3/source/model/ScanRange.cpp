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

#include <aws/s3/model/ScanRange.h>
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

ScanRange::ScanRange() : 
    m_start(0),
    m_startHasBeenSet(false),
    m_end(0),
    m_endHasBeenSet(false)
{
}

ScanRange::ScanRange(const XmlNode& xmlNode) : 
    m_start(0),
    m_startHasBeenSet(false),
    m_end(0),
    m_endHasBeenSet(false)
{
  *this = xmlNode;
}

ScanRange& ScanRange::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode startNode = resultNode.FirstChild("Start");
    if(!startNode.IsNull())
    {
      m_start = StringUtils::ConvertToInt64(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(startNode.GetText()).c_str()).c_str());
      m_startHasBeenSet = true;
    }
    XmlNode endNode = resultNode.FirstChild("End");
    if(!endNode.IsNull())
    {
      m_end = StringUtils::ConvertToInt64(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(endNode.GetText()).c_str()).c_str());
      m_endHasBeenSet = true;
    }
  }

  return *this;
}

void ScanRange::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_startHasBeenSet)
  {
   XmlNode startNode = parentNode.CreateChildElement("Start");
   ss << m_start;
   startNode.SetText(ss.str());
   ss.str("");
  }

  if(m_endHasBeenSet)
  {
   XmlNode endNode = parentNode.CreateChildElement("End");
   ss << m_end;
   endNode.SetText(ss.str());
   ss.str("");
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
