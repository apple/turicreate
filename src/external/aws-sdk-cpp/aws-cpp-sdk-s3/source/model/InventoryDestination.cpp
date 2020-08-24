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

#include <aws/s3/model/InventoryDestination.h>
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

InventoryDestination::InventoryDestination() : 
    m_s3BucketDestinationHasBeenSet(false)
{
}

InventoryDestination::InventoryDestination(const XmlNode& xmlNode) : 
    m_s3BucketDestinationHasBeenSet(false)
{
  *this = xmlNode;
}

InventoryDestination& InventoryDestination::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode s3BucketDestinationNode = resultNode.FirstChild("S3BucketDestination");
    if(!s3BucketDestinationNode.IsNull())
    {
      m_s3BucketDestination = s3BucketDestinationNode;
      m_s3BucketDestinationHasBeenSet = true;
    }
  }

  return *this;
}

void InventoryDestination::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_s3BucketDestinationHasBeenSet)
  {
   XmlNode s3BucketDestinationNode = parentNode.CreateChildElement("S3BucketDestination");
   m_s3BucketDestination.AddToNode(s3BucketDestinationNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
