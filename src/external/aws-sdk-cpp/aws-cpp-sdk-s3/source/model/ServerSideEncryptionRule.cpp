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

#include <aws/s3/model/ServerSideEncryptionRule.h>
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

ServerSideEncryptionRule::ServerSideEncryptionRule() : 
    m_applyServerSideEncryptionByDefaultHasBeenSet(false)
{
}

ServerSideEncryptionRule::ServerSideEncryptionRule(const XmlNode& xmlNode) : 
    m_applyServerSideEncryptionByDefaultHasBeenSet(false)
{
  *this = xmlNode;
}

ServerSideEncryptionRule& ServerSideEncryptionRule::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode applyServerSideEncryptionByDefaultNode = resultNode.FirstChild("ApplyServerSideEncryptionByDefault");
    if(!applyServerSideEncryptionByDefaultNode.IsNull())
    {
      m_applyServerSideEncryptionByDefault = applyServerSideEncryptionByDefaultNode;
      m_applyServerSideEncryptionByDefaultHasBeenSet = true;
    }
  }

  return *this;
}

void ServerSideEncryptionRule::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_applyServerSideEncryptionByDefaultHasBeenSet)
  {
   XmlNode applyServerSideEncryptionByDefaultNode = parentNode.CreateChildElement("ApplyServerSideEncryptionByDefault");
   m_applyServerSideEncryptionByDefault.AddToNode(applyServerSideEncryptionByDefaultNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
