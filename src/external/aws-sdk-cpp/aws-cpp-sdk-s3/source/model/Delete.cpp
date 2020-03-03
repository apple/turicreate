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

#include <aws/s3/model/Delete.h>
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

Delete::Delete() : 
    m_objectsHasBeenSet(false),
    m_quiet(false),
    m_quietHasBeenSet(false)
{
}

Delete::Delete(const XmlNode& xmlNode) : 
    m_objectsHasBeenSet(false),
    m_quiet(false),
    m_quietHasBeenSet(false)
{
  *this = xmlNode;
}

Delete& Delete::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode objectsNode = resultNode.FirstChild("Object");
    if(!objectsNode.IsNull())
    {
      XmlNode objectMember = objectsNode;
      while(!objectMember.IsNull())
      {
        m_objects.push_back(objectMember);
        objectMember = objectMember.NextNode("Object");
      }

      m_objectsHasBeenSet = true;
    }
    XmlNode quietNode = resultNode.FirstChild("Quiet");
    if(!quietNode.IsNull())
    {
      m_quiet = StringUtils::ConvertToBool(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(quietNode.GetText()).c_str()).c_str());
      m_quietHasBeenSet = true;
    }
  }

  return *this;
}

void Delete::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_objectsHasBeenSet)
  {
   for(const auto& item : m_objects)
   {
     XmlNode objectsNode = parentNode.CreateChildElement("Object");
     item.AddToNode(objectsNode);
   }
  }

  if(m_quietHasBeenSet)
  {
   XmlNode quietNode = parentNode.CreateChildElement("Quiet");
   ss << std::boolalpha << m_quiet;
   quietNode.SetText(ss.str());
   ss.str("");
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
