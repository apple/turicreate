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

#include <aws/s3/model/InventoryS3BucketDestination.h>
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

InventoryS3BucketDestination::InventoryS3BucketDestination() : 
    m_accountIdHasBeenSet(false),
    m_bucketHasBeenSet(false),
    m_format(InventoryFormat::NOT_SET),
    m_formatHasBeenSet(false),
    m_prefixHasBeenSet(false),
    m_encryptionHasBeenSet(false)
{
}

InventoryS3BucketDestination::InventoryS3BucketDestination(const XmlNode& xmlNode) : 
    m_accountIdHasBeenSet(false),
    m_bucketHasBeenSet(false),
    m_format(InventoryFormat::NOT_SET),
    m_formatHasBeenSet(false),
    m_prefixHasBeenSet(false),
    m_encryptionHasBeenSet(false)
{
  *this = xmlNode;
}

InventoryS3BucketDestination& InventoryS3BucketDestination::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode accountIdNode = resultNode.FirstChild("AccountId");
    if(!accountIdNode.IsNull())
    {
      m_accountId = Aws::Utils::Xml::DecodeEscapedXmlText(accountIdNode.GetText());
      m_accountIdHasBeenSet = true;
    }
    XmlNode bucketNode = resultNode.FirstChild("Bucket");
    if(!bucketNode.IsNull())
    {
      m_bucket = Aws::Utils::Xml::DecodeEscapedXmlText(bucketNode.GetText());
      m_bucketHasBeenSet = true;
    }
    XmlNode formatNode = resultNode.FirstChild("Format");
    if(!formatNode.IsNull())
    {
      m_format = InventoryFormatMapper::GetInventoryFormatForName(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(formatNode.GetText()).c_str()).c_str());
      m_formatHasBeenSet = true;
    }
    XmlNode prefixNode = resultNode.FirstChild("Prefix");
    if(!prefixNode.IsNull())
    {
      m_prefix = Aws::Utils::Xml::DecodeEscapedXmlText(prefixNode.GetText());
      m_prefixHasBeenSet = true;
    }
    XmlNode encryptionNode = resultNode.FirstChild("Encryption");
    if(!encryptionNode.IsNull())
    {
      m_encryption = encryptionNode;
      m_encryptionHasBeenSet = true;
    }
  }

  return *this;
}

void InventoryS3BucketDestination::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_accountIdHasBeenSet)
  {
   XmlNode accountIdNode = parentNode.CreateChildElement("AccountId");
   accountIdNode.SetText(m_accountId);
  }

  if(m_bucketHasBeenSet)
  {
   XmlNode bucketNode = parentNode.CreateChildElement("Bucket");
   bucketNode.SetText(m_bucket);
  }

  if(m_formatHasBeenSet)
  {
   XmlNode formatNode = parentNode.CreateChildElement("Format");
   formatNode.SetText(InventoryFormatMapper::GetNameForInventoryFormat(m_format));
  }

  if(m_prefixHasBeenSet)
  {
   XmlNode prefixNode = parentNode.CreateChildElement("Prefix");
   prefixNode.SetText(m_prefix);
  }

  if(m_encryptionHasBeenSet)
  {
   XmlNode encryptionNode = parentNode.CreateChildElement("Encryption");
   m_encryption.AddToNode(encryptionNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
