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

#include <aws/s3/model/SourceSelectionCriteria.h>
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

SourceSelectionCriteria::SourceSelectionCriteria() : 
    m_sseKmsEncryptedObjectsHasBeenSet(false)
{
}

SourceSelectionCriteria::SourceSelectionCriteria(const XmlNode& xmlNode) : 
    m_sseKmsEncryptedObjectsHasBeenSet(false)
{
  *this = xmlNode;
}

SourceSelectionCriteria& SourceSelectionCriteria::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode sseKmsEncryptedObjectsNode = resultNode.FirstChild("SseKmsEncryptedObjects");
    if(!sseKmsEncryptedObjectsNode.IsNull())
    {
      m_sseKmsEncryptedObjects = sseKmsEncryptedObjectsNode;
      m_sseKmsEncryptedObjectsHasBeenSet = true;
    }
  }

  return *this;
}

void SourceSelectionCriteria::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_sseKmsEncryptedObjectsHasBeenSet)
  {
   XmlNode sseKmsEncryptedObjectsNode = parentNode.CreateChildElement("SseKmsEncryptedObjects");
   m_sseKmsEncryptedObjects.AddToNode(sseKmsEncryptedObjectsNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
