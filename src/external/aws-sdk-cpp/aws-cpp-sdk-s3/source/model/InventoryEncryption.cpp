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

#include <aws/s3/model/InventoryEncryption.h>
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

InventoryEncryption::InventoryEncryption() : 
    m_sSES3HasBeenSet(false),
    m_sSEKMSHasBeenSet(false)
{
}

InventoryEncryption::InventoryEncryption(const XmlNode& xmlNode) : 
    m_sSES3HasBeenSet(false),
    m_sSEKMSHasBeenSet(false)
{
  *this = xmlNode;
}

InventoryEncryption& InventoryEncryption::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode sSES3Node = resultNode.FirstChild("SSE-S3");
    if(!sSES3Node.IsNull())
    {
      m_sSES3 = sSES3Node;
      m_sSES3HasBeenSet = true;
    }
    XmlNode sSEKMSNode = resultNode.FirstChild("SSE-KMS");
    if(!sSEKMSNode.IsNull())
    {
      m_sSEKMS = sSEKMSNode;
      m_sSEKMSHasBeenSet = true;
    }
  }

  return *this;
}

void InventoryEncryption::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_sSES3HasBeenSet)
  {
   XmlNode sSES3Node = parentNode.CreateChildElement("SSE-S3");
   m_sSES3.AddToNode(sSES3Node);
  }

  if(m_sSEKMSHasBeenSet)
  {
   XmlNode sSEKMSNode = parentNode.CreateChildElement("SSE-KMS");
   m_sSEKMS.AddToNode(sSEKMSNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
