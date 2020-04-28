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

#include <aws/s3/model/ReplicationConfiguration.h>
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

ReplicationConfiguration::ReplicationConfiguration() : 
    m_roleHasBeenSet(false),
    m_rulesHasBeenSet(false)
{
}

ReplicationConfiguration::ReplicationConfiguration(const XmlNode& xmlNode) : 
    m_roleHasBeenSet(false),
    m_rulesHasBeenSet(false)
{
  *this = xmlNode;
}

ReplicationConfiguration& ReplicationConfiguration::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode roleNode = resultNode.FirstChild("Role");
    if(!roleNode.IsNull())
    {
      m_role = Aws::Utils::Xml::DecodeEscapedXmlText(roleNode.GetText());
      m_roleHasBeenSet = true;
    }
    XmlNode rulesNode = resultNode.FirstChild("Rule");
    if(!rulesNode.IsNull())
    {
      XmlNode ruleMember = rulesNode;
      while(!ruleMember.IsNull())
      {
        m_rules.push_back(ruleMember);
        ruleMember = ruleMember.NextNode("Rule");
      }

      m_rulesHasBeenSet = true;
    }
  }

  return *this;
}

void ReplicationConfiguration::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_roleHasBeenSet)
  {
   XmlNode roleNode = parentNode.CreateChildElement("Role");
   roleNode.SetText(m_role);
  }

  if(m_rulesHasBeenSet)
  {
   for(const auto& item : m_rules)
   {
     XmlNode rulesNode = parentNode.CreateChildElement("Rule");
     item.AddToNode(rulesNode);
   }
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
