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

#include <aws/s3/model/IndexDocument.h>
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

IndexDocument::IndexDocument() : 
    m_suffixHasBeenSet(false)
{
}

IndexDocument::IndexDocument(const XmlNode& xmlNode) : 
    m_suffixHasBeenSet(false)
{
  *this = xmlNode;
}

IndexDocument& IndexDocument::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode suffixNode = resultNode.FirstChild("Suffix");
    if(!suffixNode.IsNull())
    {
      m_suffix = Aws::Utils::Xml::DecodeEscapedXmlText(suffixNode.GetText());
      m_suffixHasBeenSet = true;
    }
  }

  return *this;
}

void IndexDocument::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_suffixHasBeenSet)
  {
   XmlNode suffixNode = parentNode.CreateChildElement("Suffix");
   suffixNode.SetText(m_suffix);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
