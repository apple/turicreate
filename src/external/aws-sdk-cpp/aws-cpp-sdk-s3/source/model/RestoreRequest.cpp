/*
* Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <aws/s3/model/RestoreRequest.h>
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

RestoreRequest::RestoreRequest() : 
    m_days(0),
    m_daysHasBeenSet(false)
{
}

RestoreRequest::RestoreRequest(const XmlNode& xmlNode) : 
    m_days(0),
    m_daysHasBeenSet(false)
{
  *this = xmlNode;
}

RestoreRequest& RestoreRequest::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode daysNode = resultNode.FirstChild("Days");
    if(!daysNode.IsNull())
    {
      m_days = StringUtils::ConvertToInt32(StringUtils::Trim(daysNode.GetText().c_str()).c_str());
      m_daysHasBeenSet = true;
    }
  }

  return *this;
}

void RestoreRequest::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_daysHasBeenSet)
  {
   XmlNode daysNode = parentNode.CreateChildElement("Days");
  ss << m_days;
   daysNode.SetText(ss.str());
  ss.str("");
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
