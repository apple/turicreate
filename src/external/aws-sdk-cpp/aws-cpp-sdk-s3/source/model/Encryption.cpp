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

#include <aws/s3/model/Encryption.h>
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

Encryption::Encryption() : 
    m_encryptionType(ServerSideEncryption::NOT_SET),
    m_encryptionTypeHasBeenSet(false),
    m_kMSKeyIdHasBeenSet(false),
    m_kMSContextHasBeenSet(false)
{
}

Encryption::Encryption(const XmlNode& xmlNode) : 
    m_encryptionType(ServerSideEncryption::NOT_SET),
    m_encryptionTypeHasBeenSet(false),
    m_kMSKeyIdHasBeenSet(false),
    m_kMSContextHasBeenSet(false)
{
  *this = xmlNode;
}

Encryption& Encryption::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode encryptionTypeNode = resultNode.FirstChild("EncryptionType");
    if(!encryptionTypeNode.IsNull())
    {
      m_encryptionType = ServerSideEncryptionMapper::GetServerSideEncryptionForName(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(encryptionTypeNode.GetText()).c_str()).c_str());
      m_encryptionTypeHasBeenSet = true;
    }
    XmlNode kMSKeyIdNode = resultNode.FirstChild("KMSKeyId");
    if(!kMSKeyIdNode.IsNull())
    {
      m_kMSKeyId = Aws::Utils::Xml::DecodeEscapedXmlText(kMSKeyIdNode.GetText());
      m_kMSKeyIdHasBeenSet = true;
    }
    XmlNode kMSContextNode = resultNode.FirstChild("KMSContext");
    if(!kMSContextNode.IsNull())
    {
      m_kMSContext = Aws::Utils::Xml::DecodeEscapedXmlText(kMSContextNode.GetText());
      m_kMSContextHasBeenSet = true;
    }
  }

  return *this;
}

void Encryption::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_encryptionTypeHasBeenSet)
  {
   XmlNode encryptionTypeNode = parentNode.CreateChildElement("EncryptionType");
   encryptionTypeNode.SetText(ServerSideEncryptionMapper::GetNameForServerSideEncryption(m_encryptionType));
  }

  if(m_kMSKeyIdHasBeenSet)
  {
   XmlNode kMSKeyIdNode = parentNode.CreateChildElement("KMSKeyId");
   kMSKeyIdNode.SetText(m_kMSKeyId);
  }

  if(m_kMSContextHasBeenSet)
  {
   XmlNode kMSContextNode = parentNode.CreateChildElement("KMSContext");
   kMSContextNode.SetText(m_kMSContext);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
