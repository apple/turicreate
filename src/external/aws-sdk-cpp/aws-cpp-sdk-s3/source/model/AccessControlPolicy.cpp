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

#include <aws/s3/model/AccessControlPolicy.h>
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

AccessControlPolicy::AccessControlPolicy() : 
    m_grantsHasBeenSet(false),
    m_ownerHasBeenSet(false)
{
}

AccessControlPolicy::AccessControlPolicy(const XmlNode& xmlNode) : 
    m_grantsHasBeenSet(false),
    m_ownerHasBeenSet(false)
{
  *this = xmlNode;
}

AccessControlPolicy& AccessControlPolicy::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode grantsNode = resultNode.FirstChild("AccessControlList");
    if(!grantsNode.IsNull())
    {
      XmlNode grantsMember = grantsNode.FirstChild("Grant");
      while(!grantsMember.IsNull())
      {
        m_grants.push_back(grantsMember);
        grantsMember = grantsMember.NextNode("Grant");
      }

      m_grantsHasBeenSet = true;
    }
    XmlNode ownerNode = resultNode.FirstChild("Owner");
    if(!ownerNode.IsNull())
    {
      m_owner = ownerNode;
      m_ownerHasBeenSet = true;
    }
  }

  return *this;
}

void AccessControlPolicy::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_grantsHasBeenSet)
  {
   XmlNode grantsParentNode = parentNode.CreateChildElement("AccessControlList");
   for(const auto& item : m_grants)
   {
     XmlNode grantsNode = grantsParentNode.CreateChildElement("Grant");
     item.AddToNode(grantsNode);
   }
  }

  if(m_ownerHasBeenSet)
  {
   XmlNode ownerNode = parentNode.CreateChildElement("Owner");
   m_owner.AddToNode(ownerNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
