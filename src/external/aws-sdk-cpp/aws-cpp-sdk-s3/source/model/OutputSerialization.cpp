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

#include <aws/s3/model/OutputSerialization.h>
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

OutputSerialization::OutputSerialization() : 
    m_cSVHasBeenSet(false),
    m_jSONHasBeenSet(false)
{
}

OutputSerialization::OutputSerialization(const XmlNode& xmlNode) : 
    m_cSVHasBeenSet(false),
    m_jSONHasBeenSet(false)
{
  *this = xmlNode;
}

OutputSerialization& OutputSerialization::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode cSVNode = resultNode.FirstChild("CSV");
    if(!cSVNode.IsNull())
    {
      m_cSV = cSVNode;
      m_cSVHasBeenSet = true;
    }
    XmlNode jSONNode = resultNode.FirstChild("JSON");
    if(!jSONNode.IsNull())
    {
      m_jSON = jSONNode;
      m_jSONHasBeenSet = true;
    }
  }

  return *this;
}

void OutputSerialization::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_cSVHasBeenSet)
  {
   XmlNode cSVNode = parentNode.CreateChildElement("CSV");
   m_cSV.AddToNode(cSVNode);
  }

  if(m_jSONHasBeenSet)
  {
   XmlNode jSONNode = parentNode.CreateChildElement("JSON");
   m_jSON.AddToNode(jSONNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
