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

#include <aws/s3/model/CreateBucketConfiguration.h>
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

CreateBucketConfiguration::CreateBucketConfiguration() : 
    m_locationConstraint(BucketLocationConstraint::NOT_SET),
    m_locationConstraintHasBeenSet(false)
{
}

CreateBucketConfiguration::CreateBucketConfiguration(const XmlNode& xmlNode) : 
    m_locationConstraint(BucketLocationConstraint::NOT_SET),
    m_locationConstraintHasBeenSet(false)
{
  *this = xmlNode;
}

CreateBucketConfiguration& CreateBucketConfiguration::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode locationConstraintNode = resultNode.FirstChild("LocationConstraint");
    if(!locationConstraintNode.IsNull())
    {
      m_locationConstraint = BucketLocationConstraintMapper::GetBucketLocationConstraintForName(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(locationConstraintNode.GetText()).c_str()).c_str());
      m_locationConstraintHasBeenSet = true;
    }
  }

  return *this;
}

void CreateBucketConfiguration::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_locationConstraintHasBeenSet)
  {
   XmlNode locationConstraintNode = parentNode.CreateChildElement("LocationConstraint");
   locationConstraintNode.SetText(BucketLocationConstraintMapper::GetNameForBucketLocationConstraint(m_locationConstraint));
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
