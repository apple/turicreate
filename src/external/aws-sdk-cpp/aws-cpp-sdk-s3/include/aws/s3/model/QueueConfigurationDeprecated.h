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
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/Event.h>
#include <utility>

namespace Aws
{
namespace Utils
{
namespace Xml
{
  class XmlNode;
} // namespace Xml
} // namespace Utils
namespace S3
{
namespace Model
{

  /**
   * <p>This data type is deprecated. Use <a>QueueConfiguration</a> for the same
   * purposes. This data type specifies the configuration for publishing messages to
   * an Amazon Simple Queue Service (Amazon SQS) queue when Amazon S3 detects
   * specified events. </p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/QueueConfigurationDeprecated">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API QueueConfigurationDeprecated
  {
  public:
    QueueConfigurationDeprecated();
    QueueConfigurationDeprecated(const Aws::Utils::Xml::XmlNode& xmlNode);
    QueueConfigurationDeprecated& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    
    inline const Aws::String& GetId() const{ return m_id; }

    
    inline bool IdHasBeenSet() const { return m_idHasBeenSet; }

    
    inline void SetId(const Aws::String& value) { m_idHasBeenSet = true; m_id = value; }

    
    inline void SetId(Aws::String&& value) { m_idHasBeenSet = true; m_id = std::move(value); }

    
    inline void SetId(const char* value) { m_idHasBeenSet = true; m_id.assign(value); }

    
    inline QueueConfigurationDeprecated& WithId(const Aws::String& value) { SetId(value); return *this;}

    
    inline QueueConfigurationDeprecated& WithId(Aws::String&& value) { SetId(std::move(value)); return *this;}

    
    inline QueueConfigurationDeprecated& WithId(const char* value) { SetId(value); return *this;}


    /**
     * <p>A collection of bucket events for which to send notifications</p>
     */
    inline const Aws::Vector<Event>& GetEvents() const{ return m_events; }

    /**
     * <p>A collection of bucket events for which to send notifications</p>
     */
    inline bool EventsHasBeenSet() const { return m_eventsHasBeenSet; }

    /**
     * <p>A collection of bucket events for which to send notifications</p>
     */
    inline void SetEvents(const Aws::Vector<Event>& value) { m_eventsHasBeenSet = true; m_events = value; }

    /**
     * <p>A collection of bucket events for which to send notifications</p>
     */
    inline void SetEvents(Aws::Vector<Event>&& value) { m_eventsHasBeenSet = true; m_events = std::move(value); }

    /**
     * <p>A collection of bucket events for which to send notifications</p>
     */
    inline QueueConfigurationDeprecated& WithEvents(const Aws::Vector<Event>& value) { SetEvents(value); return *this;}

    /**
     * <p>A collection of bucket events for which to send notifications</p>
     */
    inline QueueConfigurationDeprecated& WithEvents(Aws::Vector<Event>&& value) { SetEvents(std::move(value)); return *this;}

    /**
     * <p>A collection of bucket events for which to send notifications</p>
     */
    inline QueueConfigurationDeprecated& AddEvents(const Event& value) { m_eventsHasBeenSet = true; m_events.push_back(value); return *this; }

    /**
     * <p>A collection of bucket events for which to send notifications</p>
     */
    inline QueueConfigurationDeprecated& AddEvents(Event&& value) { m_eventsHasBeenSet = true; m_events.push_back(std::move(value)); return *this; }


    /**
     * <p>The Amazon Resource Name (ARN) of the Amazon SQS queue to which Amazon S3
     * publishes a message when it detects events of the specified type. </p>
     */
    inline const Aws::String& GetQueue() const{ return m_queue; }

    /**
     * <p>The Amazon Resource Name (ARN) of the Amazon SQS queue to which Amazon S3
     * publishes a message when it detects events of the specified type. </p>
     */
    inline bool QueueHasBeenSet() const { return m_queueHasBeenSet; }

    /**
     * <p>The Amazon Resource Name (ARN) of the Amazon SQS queue to which Amazon S3
     * publishes a message when it detects events of the specified type. </p>
     */
    inline void SetQueue(const Aws::String& value) { m_queueHasBeenSet = true; m_queue = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the Amazon SQS queue to which Amazon S3
     * publishes a message when it detects events of the specified type. </p>
     */
    inline void SetQueue(Aws::String&& value) { m_queueHasBeenSet = true; m_queue = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the Amazon SQS queue to which Amazon S3
     * publishes a message when it detects events of the specified type. </p>
     */
    inline void SetQueue(const char* value) { m_queueHasBeenSet = true; m_queue.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the Amazon SQS queue to which Amazon S3
     * publishes a message when it detects events of the specified type. </p>
     */
    inline QueueConfigurationDeprecated& WithQueue(const Aws::String& value) { SetQueue(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the Amazon SQS queue to which Amazon S3
     * publishes a message when it detects events of the specified type. </p>
     */
    inline QueueConfigurationDeprecated& WithQueue(Aws::String&& value) { SetQueue(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the Amazon SQS queue to which Amazon S3
     * publishes a message when it detects events of the specified type. </p>
     */
    inline QueueConfigurationDeprecated& WithQueue(const char* value) { SetQueue(value); return *this;}

  private:

    Aws::String m_id;
    bool m_idHasBeenSet;

    Aws::Vector<Event> m_events;
    bool m_eventsHasBeenSet;

    Aws::String m_queue;
    bool m_queueHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
