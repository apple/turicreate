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

#include <aws/s3/model/OutputLocation.h>
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

OutputLocation::OutputLocation() : 
    m_s3HasBeenSet(false)
{
}

OutputLocation::OutputLocation(const XmlNode& xmlNode) : 
    m_s3HasBeenSet(false)
{
  *this = xmlNode;
}

OutputLocation& OutputLocation::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode s3Node = resultNode.FirstChild("S3");
    if(!s3Node.IsNull())
    {
      m_s3 = s3Node;
      m_s3HasBeenSet = true;
    }
  }

  return *this;
}

void OutputLocation::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_s3HasBeenSet)
  {
   XmlNode s3Node = parentNode.CreateChildElement("S3");
   m_s3.AddToNode(s3Node);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
