/*
* Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <aws/s3/model/Grantee.h>
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

Grantee::Grantee() : 
    m_displayNameHasBeenSet(false),
    m_emailAddressHasBeenSet(false),
    m_iDHasBeenSet(false),
    m_typeHasBeenSet(false),
    m_uRIHasBeenSet(false)
{
}

Grantee::Grantee(const XmlNode& xmlNode) : 
    m_displayNameHasBeenSet(false),
    m_emailAddressHasBeenSet(false),
    m_iDHasBeenSet(false),
    m_typeHasBeenSet(false),
    m_uRIHasBeenSet(false)
{
  *this = xmlNode;
}

Grantee& Grantee::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode displayNameNode = resultNode.FirstChild("DisplayName");
    if(!displayNameNode.IsNull())
    {
      m_displayName = StringUtils::Trim(displayNameNode.GetText().c_str());
      m_displayNameHasBeenSet = true;
    }
    XmlNode emailAddressNode = resultNode.FirstChild("EmailAddress");
    if(!emailAddressNode.IsNull())
    {
      m_emailAddress = StringUtils::Trim(emailAddressNode.GetText().c_str());
      m_emailAddressHasBeenSet = true;
    }
    XmlNode iDNode = resultNode.FirstChild("ID");
    if(!iDNode.IsNull())
    {
      m_iD = StringUtils::Trim(iDNode.GetText().c_str());
      m_iDHasBeenSet = true;
    }
    XmlNode typeNode = resultNode.FirstChild("xsi:type");
    if(!typeNode.IsNull())
    {
      m_type = TypeMapper::GetTypeForName(StringUtils::Trim(typeNode.GetText().c_str()).c_str());
      m_typeHasBeenSet = true;
    }
    XmlNode uRINode = resultNode.FirstChild("URI");
    if(!uRINode.IsNull())
    {
      m_uRI = StringUtils::Trim(uRINode.GetText().c_str());
      m_uRIHasBeenSet = true;
    }
  }

  return *this;
}

void Grantee::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_displayNameHasBeenSet)
  {
   XmlNode displayNameNode = parentNode.CreateChildElement("DisplayName");
   displayNameNode.SetText(m_displayName);
  }

  if(m_emailAddressHasBeenSet)
  {
   XmlNode emailAddressNode = parentNode.CreateChildElement("EmailAddress");
   emailAddressNode.SetText(m_emailAddress);
  }

  if(m_iDHasBeenSet)
  {
   XmlNode iDNode = parentNode.CreateChildElement("ID");
   iDNode.SetText(m_iD);
  }

  if(m_typeHasBeenSet)
  {
   XmlNode typeNode = parentNode.CreateChildElement("Type");
   typeNode.SetText(TypeMapper::GetNameForType(m_type));
  }

  if(m_uRIHasBeenSet)
  {
   XmlNode uRINode = parentNode.CreateChildElement("xsi:type");
   uRINode.SetText(m_uRI);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
