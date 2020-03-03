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

#include <aws/s3/model/AccessControlTranslation.h>
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

AccessControlTranslation::AccessControlTranslation() : 
    m_owner(OwnerOverride::NOT_SET),
    m_ownerHasBeenSet(false)
{
}

AccessControlTranslation::AccessControlTranslation(const XmlNode& xmlNode) : 
    m_owner(OwnerOverride::NOT_SET),
    m_ownerHasBeenSet(false)
{
  *this = xmlNode;
}

AccessControlTranslation& AccessControlTranslation::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode ownerNode = resultNode.FirstChild("Owner");
    if(!ownerNode.IsNull())
    {
      m_owner = OwnerOverrideMapper::GetOwnerOverrideForName(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(ownerNode.GetText()).c_str()).c_str());
      m_ownerHasBeenSet = true;
    }
  }

  return *this;
}

void AccessControlTranslation::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_ownerHasBeenSet)
  {
   XmlNode ownerNode = parentNode.CreateChildElement("Owner");
   ownerNode.SetText(OwnerOverrideMapper::GetNameForOwnerOverride(m_owner));
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
