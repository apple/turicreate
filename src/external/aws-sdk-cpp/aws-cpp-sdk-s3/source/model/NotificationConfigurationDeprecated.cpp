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

#include <aws/s3/model/NotificationConfigurationDeprecated.h>
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

NotificationConfigurationDeprecated::NotificationConfigurationDeprecated() : 
    m_topicConfigurationHasBeenSet(false),
    m_queueConfigurationHasBeenSet(false),
    m_cloudFunctionConfigurationHasBeenSet(false)
{
}

NotificationConfigurationDeprecated::NotificationConfigurationDeprecated(const XmlNode& xmlNode) : 
    m_topicConfigurationHasBeenSet(false),
    m_queueConfigurationHasBeenSet(false),
    m_cloudFunctionConfigurationHasBeenSet(false)
{
  *this = xmlNode;
}

NotificationConfigurationDeprecated& NotificationConfigurationDeprecated::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode topicConfigurationNode = resultNode.FirstChild("TopicConfiguration");
    if(!topicConfigurationNode.IsNull())
    {
      m_topicConfiguration = topicConfigurationNode;
      m_topicConfigurationHasBeenSet = true;
    }
    XmlNode queueConfigurationNode = resultNode.FirstChild("QueueConfiguration");
    if(!queueConfigurationNode.IsNull())
    {
      m_queueConfiguration = queueConfigurationNode;
      m_queueConfigurationHasBeenSet = true;
    }
    XmlNode cloudFunctionConfigurationNode = resultNode.FirstChild("CloudFunctionConfiguration");
    if(!cloudFunctionConfigurationNode.IsNull())
    {
      m_cloudFunctionConfiguration = cloudFunctionConfigurationNode;
      m_cloudFunctionConfigurationHasBeenSet = true;
    }
  }

  return *this;
}

void NotificationConfigurationDeprecated::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_topicConfigurationHasBeenSet)
  {
   XmlNode topicConfigurationNode = parentNode.CreateChildElement("TopicConfiguration");
   m_topicConfiguration.AddToNode(topicConfigurationNode);
  }

  if(m_queueConfigurationHasBeenSet)
  {
   XmlNode queueConfigurationNode = parentNode.CreateChildElement("QueueConfiguration");
   m_queueConfiguration.AddToNode(queueConfigurationNode);
  }

  if(m_cloudFunctionConfigurationHasBeenSet)
  {
   XmlNode cloudFunctionConfigurationNode = parentNode.CreateChildElement("CloudFunctionConfiguration");
   m_cloudFunctionConfiguration.AddToNode(cloudFunctionConfigurationNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
