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

#include <aws/s3/model/SSEKMS.h>
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

SSEKMS::SSEKMS() : 
    m_keyIdHasBeenSet(false)
{
}

SSEKMS::SSEKMS(const XmlNode& xmlNode) : 
    m_keyIdHasBeenSet(false)
{
  *this = xmlNode;
}

SSEKMS& SSEKMS::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode keyIdNode = resultNode.FirstChild("KeyId");
    if(!keyIdNode.IsNull())
    {
      m_keyId = Aws::Utils::Xml::DecodeEscapedXmlText(keyIdNode.GetText());
      m_keyIdHasBeenSet = true;
    }
  }

  return *this;
}

void SSEKMS::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_keyIdHasBeenSet)
  {
   XmlNode keyIdNode = parentNode.CreateChildElement("KeyId");
   keyIdNode.SetText(m_keyId);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
