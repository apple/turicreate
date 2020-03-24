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

#pragma once
#include <aws/s3/S3_EXPORTS.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/TopicConfiguration.h>
#include <aws/s3/model/QueueConfiguration.h>
#include <aws/s3/model/LambdaFunctionConfiguration.h>
#include <utility>

namespace Aws
{
template<typename RESULT_TYPE>
class AmazonWebServiceResult;

namespace Utils
{
namespace Xml
{
  class XmlDocument;
} // namespace Xml
} // namespace Utils
namespace S3
{
namespace Model
{
  /**
   * <p>A container for specifying the notification configuration of the bucket. If
   * this element is empty, notifications are turned off for the
   * bucket.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/NotificationConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API GetBucketNotificationConfigurationResult
  {
  public:
    GetBucketNotificationConfigurationResult();
    GetBucketNotificationConfigurationResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    GetBucketNotificationConfigurationResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>The topic to which notifications are sent and the events for which
     * notifications are generated.</p>
     */
    inline const Aws::Vector<TopicConfiguration>& GetTopicConfigurations() const{ return m_topicConfigurations; }

    /**
     * <p>The topic to which notifications are sent and the events for which
     * notifications are generated.</p>
     */
    inline void SetTopicConfigurations(const Aws::Vector<TopicConfiguration>& value) { m_topicConfigurations = value; }

    /**
     * <p>The topic to which notifications are sent and the events for which
     * notifications are generated.</p>
     */
    inline void SetTopicConfigurations(Aws::Vector<TopicConfiguration>&& value) { m_topicConfigurations = std::move(value); }

    /**
     * <p>The topic to which notifications are sent and the events for which
     * notifications are generated.</p>
     */
    inline GetBucketNotificationConfigurationResult& WithTopicConfigurations(const Aws::Vector<TopicConfiguration>& value) { SetTopicConfigurations(value); return *this;}

    /**
     * <p>The topic to which notifications are sent and the events for which
     * notifications are generated.</p>
     */
    inline GetBucketNotificationConfigurationResult& WithTopicConfigurations(Aws::Vector<TopicConfiguration>&& value) { SetTopicConfigurations(std::move(value)); return *this;}

    /**
     * <p>The topic to which notifications are sent and the events for which
     * notifications are generated.</p>
     */
    inline GetBucketNotificationConfigurationResult& AddTopicConfigurations(const TopicConfiguration& value) { m_topicConfigurations.push_back(value); return *this; }

    /**
     * <p>The topic to which notifications are sent and the events for which
     * notifications are generated.</p>
     */
    inline GetBucketNotificationConfigurationResult& AddTopicConfigurations(TopicConfiguration&& value) { m_topicConfigurations.push_back(std::move(value)); return *this; }


    /**
     * <p>The Amazon Simple Queue Service queues to publish messages to and the events
     * for which to publish messages.</p>
     */
    inline const Aws::Vector<QueueConfiguration>& GetQueueConfigurations() const{ return m_queueConfigurations; }

    /**
     * <p>The Amazon Simple Queue Service queues to publish messages to and the events
     * for which to publish messages.</p>
     */
    inline void SetQueueConfigurations(const Aws::Vector<QueueConfiguration>& value) { m_queueConfigurations = value; }

    /**
     * <p>The Amazon Simple Queue Service queues to publish messages to and the events
     * for which to publish messages.</p>
     */
    inline void SetQueueConfigurations(Aws::Vector<QueueConfiguration>&& value) { m_queueConfigurations = std::move(value); }

    /**
     * <p>The Amazon Simple Queue Service queues to publish messages to and the events
     * for which to publish messages.</p>
     */
    inline GetBucketNotificationConfigurationResult& WithQueueConfigurations(const Aws::Vector<QueueConfiguration>& value) { SetQueueConfigurations(value); return *this;}

    /**
     * <p>The Amazon Simple Queue Service queues to publish messages to and the events
     * for which to publish messages.</p>
     */
    inline GetBucketNotificationConfigurationResult& WithQueueConfigurations(Aws::Vector<QueueConfiguration>&& value) { SetQueueConfigurations(std::move(value)); return *this;}

    /**
     * <p>The Amazon Simple Queue Service queues to publish messages to and the events
     * for which to publish messages.</p>
     */
    inline GetBucketNotificationConfigurationResult& AddQueueConfigurations(const QueueConfiguration& value) { m_queueConfigurations.push_back(value); return *this; }

    /**
     * <p>The Amazon Simple Queue Service queues to publish messages to and the events
     * for which to publish messages.</p>
     */
    inline GetBucketNotificationConfigurationResult& AddQueueConfigurations(QueueConfiguration&& value) { m_queueConfigurations.push_back(std::move(value)); return *this; }


    /**
     * <p>Describes the AWS Lambda functions to invoke and the events for which to
     * invoke them.</p>
     */
    inline const Aws::Vector<LambdaFunctionConfiguration>& GetLambdaFunctionConfigurations() const{ return m_lambdaFunctionConfigurations; }

    /**
     * <p>Describes the AWS Lambda functions to invoke and the events for which to
     * invoke them.</p>
     */
    inline void SetLambdaFunctionConfigurations(const Aws::Vector<LambdaFunctionConfiguration>& value) { m_lambdaFunctionConfigurations = value; }

    /**
     * <p>Describes the AWS Lambda functions to invoke and the events for which to
     * invoke them.</p>
     */
    inline void SetLambdaFunctionConfigurations(Aws::Vector<LambdaFunctionConfiguration>&& value) { m_lambdaFunctionConfigurations = std::move(value); }

    /**
     * <p>Describes the AWS Lambda functions to invoke and the events for which to
     * invoke them.</p>
     */
    inline GetBucketNotificationConfigurationResult& WithLambdaFunctionConfigurations(const Aws::Vector<LambdaFunctionConfiguration>& value) { SetLambdaFunctionConfigurations(value); return *this;}

    /**
     * <p>Describes the AWS Lambda functions to invoke and the events for which to
     * invoke them.</p>
     */
    inline GetBucketNotificationConfigurationResult& WithLambdaFunctionConfigurations(Aws::Vector<LambdaFunctionConfiguration>&& value) { SetLambdaFunctionConfigurations(std::move(value)); return *this;}

    /**
     * <p>Describes the AWS Lambda functions to invoke and the events for which to
     * invoke them.</p>
     */
    inline GetBucketNotificationConfigurationResult& AddLambdaFunctionConfigurations(const LambdaFunctionConfiguration& value) { m_lambdaFunctionConfigurations.push_back(value); return *this; }

    /**
     * <p>Describes the AWS Lambda functions to invoke and the events for which to
     * invoke them.</p>
     */
    inline GetBucketNotificationConfigurationResult& AddLambdaFunctionConfigurations(LambdaFunctionConfiguration&& value) { m_lambdaFunctionConfigurations.push_back(std::move(value)); return *this; }

  private:

    Aws::Vector<TopicConfiguration> m_topicConfigurations;

    Aws::Vector<QueueConfiguration> m_queueConfigurations;

    Aws::Vector<LambdaFunctionConfiguration> m_lambdaFunctionConfigurations;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
