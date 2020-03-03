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

#include <aws/s3/model/ObjectLockConfiguration.h>
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

ObjectLockConfiguration::ObjectLockConfiguration() : 
    m_objectLockEnabled(ObjectLockEnabled::NOT_SET),
    m_objectLockEnabledHasBeenSet(false),
    m_ruleHasBeenSet(false)
{
}

ObjectLockConfiguration::ObjectLockConfiguration(const XmlNode& xmlNode) : 
    m_objectLockEnabled(ObjectLockEnabled::NOT_SET),
    m_objectLockEnabledHasBeenSet(false),
    m_ruleHasBeenSet(false)
{
  *this = xmlNode;
}

ObjectLockConfiguration& ObjectLockConfiguration::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode objectLockEnabledNode = resultNode.FirstChild("ObjectLockEnabled");
    if(!objectLockEnabledNode.IsNull())
    {
      m_objectLockEnabled = ObjectLockEnabledMapper::GetObjectLockEnabledForName(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(objectLockEnabledNode.GetText()).c_str()).c_str());
      m_objectLockEnabledHasBeenSet = true;
    }
    XmlNode ruleNode = resultNode.FirstChild("Rule");
    if(!ruleNode.IsNull())
    {
      m_rule = ruleNode;
      m_ruleHasBeenSet = true;
    }
  }

  return *this;
}

void ObjectLockConfiguration::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_objectLockEnabledHasBeenSet)
  {
   XmlNode objectLockEnabledNode = parentNode.CreateChildElement("ObjectLockEnabled");
   objectLockEnabledNode.SetText(ObjectLockEnabledMapper::GetNameForObjectLockEnabled(m_objectLockEnabled));
  }

  if(m_ruleHasBeenSet)
  {
   XmlNode ruleNode = parentNode.CreateChildElement("Rule");
   m_rule.AddToNode(ruleNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
