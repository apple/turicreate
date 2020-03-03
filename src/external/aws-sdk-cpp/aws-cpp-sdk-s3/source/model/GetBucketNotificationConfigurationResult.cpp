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

#include <aws/s3/model/GetBucketNotificationConfigurationResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

GetBucketNotificationConfigurationResult::GetBucketNotificationConfigurationResult()
{
}

GetBucketNotificationConfigurationResult::GetBucketNotificationConfigurationResult(const Aws::AmazonWebServiceResult<XmlDocument>& result)
{
  *this = result;
}

GetBucketNotificationConfigurationResult& GetBucketNotificationConfigurationResult::operator =(const Aws::AmazonWebServiceResult<XmlDocument>& result)
{
  const XmlDocument& xmlDocument = result.GetPayload();
  XmlNode resultNode = xmlDocument.GetRootElement();

  if(!resultNode.IsNull())
  {
    XmlNode topicConfigurationsNode = resultNode.FirstChild("TopicConfiguration");
    if(!topicConfigurationsNode.IsNull())
    {
      XmlNode topicConfigurationMember = topicConfigurationsNode;
      while(!topicConfigurationMember.IsNull())
      {
        m_topicConfigurations.push_back(topicConfigurationMember);
        topicConfigurationMember = topicConfigurationMember.NextNode("TopicConfiguration");
      }

    }
    XmlNode queueConfigurationsNode = resultNode.FirstChild("QueueConfiguration");
    if(!queueConfigurationsNode.IsNull())
    {
      XmlNode queueConfigurationMember = queueConfigurationsNode;
      while(!queueConfigurationMember.IsNull())
      {
        m_queueConfigurations.push_back(queueConfigurationMember);
        queueConfigurationMember = queueConfigurationMember.NextNode("QueueConfiguration");
      }

    }
    XmlNode lambdaFunctionConfigurationsNode = resultNode.FirstChild("CloudFunctionConfiguration");
    if(!lambdaFunctionConfigurationsNode.IsNull())
    {
      XmlNode cloudFunctionConfigurationMember = lambdaFunctionConfigurationsNode;
      while(!cloudFunctionConfigurationMember.IsNull())
      {
        m_lambdaFunctionConfigurations.push_back(cloudFunctionConfigurationMember);
        cloudFunctionConfigurationMember = cloudFunctionConfigurationMember.NextNode("CloudFunctionConfiguration");
      }

    }
  }

  return *this;
}
