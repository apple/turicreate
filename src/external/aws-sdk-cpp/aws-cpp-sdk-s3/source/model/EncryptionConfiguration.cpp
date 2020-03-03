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

#include <aws/s3/model/EncryptionConfiguration.h>
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

EncryptionConfiguration::EncryptionConfiguration() : 
    m_replicaKmsKeyIDHasBeenSet(false)
{
}

EncryptionConfiguration::EncryptionConfiguration(const XmlNode& xmlNode) : 
    m_replicaKmsKeyIDHasBeenSet(false)
{
  *this = xmlNode;
}

EncryptionConfiguration& EncryptionConfiguration::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode replicaKmsKeyIDNode = resultNode.FirstChild("ReplicaKmsKeyID");
    if(!replicaKmsKeyIDNode.IsNull())
    {
      m_replicaKmsKeyID = Aws::Utils::Xml::DecodeEscapedXmlText(replicaKmsKeyIDNode.GetText());
      m_replicaKmsKeyIDHasBeenSet = true;
    }
  }

  return *this;
}

void EncryptionConfiguration::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_replicaKmsKeyIDHasBeenSet)
  {
   XmlNode replicaKmsKeyIDNode = parentNode.CreateChildElement("ReplicaKmsKeyID");
   replicaKmsKeyIDNode.SetText(m_replicaKmsKeyID);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
