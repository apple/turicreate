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

#include <aws/s3/model/RoutingRule.h>
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

RoutingRule::RoutingRule() : 
    m_conditionHasBeenSet(false),
    m_redirectHasBeenSet(false)
{
}

RoutingRule::RoutingRule(const XmlNode& xmlNode) : 
    m_conditionHasBeenSet(false),
    m_redirectHasBeenSet(false)
{
  *this = xmlNode;
}

RoutingRule& RoutingRule::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode conditionNode = resultNode.FirstChild("Condition");
    if(!conditionNode.IsNull())
    {
      m_condition = conditionNode;
      m_conditionHasBeenSet = true;
    }
    XmlNode redirectNode = resultNode.FirstChild("Redirect");
    if(!redirectNode.IsNull())
    {
      m_redirect = redirectNode;
      m_redirectHasBeenSet = true;
    }
  }

  return *this;
}

void RoutingRule::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_conditionHasBeenSet)
  {
   XmlNode conditionNode = parentNode.CreateChildElement("Condition");
   m_condition.AddToNode(conditionNode);
  }

  if(m_redirectHasBeenSet)
  {
   XmlNode redirectNode = parentNode.CreateChildElement("Redirect");
   m_redirect.AddToNode(redirectNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
