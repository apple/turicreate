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

#include <aws/s3/model/ServerSideEncryptionByDefault.h>
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

ServerSideEncryptionByDefault::ServerSideEncryptionByDefault() : 
    m_sSEAlgorithm(ServerSideEncryption::NOT_SET),
    m_sSEAlgorithmHasBeenSet(false),
    m_kMSMasterKeyIDHasBeenSet(false)
{
}

ServerSideEncryptionByDefault::ServerSideEncryptionByDefault(const XmlNode& xmlNode) : 
    m_sSEAlgorithm(ServerSideEncryption::NOT_SET),
    m_sSEAlgorithmHasBeenSet(false),
    m_kMSMasterKeyIDHasBeenSet(false)
{
  *this = xmlNode;
}

ServerSideEncryptionByDefault& ServerSideEncryptionByDefault::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode sSEAlgorithmNode = resultNode.FirstChild("SSEAlgorithm");
    if(!sSEAlgorithmNode.IsNull())
    {
      m_sSEAlgorithm = ServerSideEncryptionMapper::GetServerSideEncryptionForName(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(sSEAlgorithmNode.GetText()).c_str()).c_str());
      m_sSEAlgorithmHasBeenSet = true;
    }
    XmlNode kMSMasterKeyIDNode = resultNode.FirstChild("KMSMasterKeyID");
    if(!kMSMasterKeyIDNode.IsNull())
    {
      m_kMSMasterKeyID = Aws::Utils::Xml::DecodeEscapedXmlText(kMSMasterKeyIDNode.GetText());
      m_kMSMasterKeyIDHasBeenSet = true;
    }
  }

  return *this;
}

void ServerSideEncryptionByDefault::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_sSEAlgorithmHasBeenSet)
  {
   XmlNode sSEAlgorithmNode = parentNode.CreateChildElement("SSEAlgorithm");
   sSEAlgorithmNode.SetText(ServerSideEncryptionMapper::GetNameForServerSideEncryption(m_sSEAlgorithm));
  }

  if(m_kMSMasterKeyIDHasBeenSet)
  {
   XmlNode kMSMasterKeyIDNode = parentNode.CreateChildElement("KMSMasterKeyID");
   kMSMasterKeyIDNode.SetText(m_kMSMasterKeyID);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
