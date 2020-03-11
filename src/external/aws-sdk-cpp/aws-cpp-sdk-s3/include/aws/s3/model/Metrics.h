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
#include <aws/s3/model/MetricsStatus.h>
#include <aws/s3/model/ReplicationTimeValue.h>
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
   * <p> A container specifying replication metrics-related settings enabling metrics
   * and Amazon S3 events for S3 Replication Time Control (S3 RTC). Must be specified
   * together with a <code>ReplicationTime</code> block. </p><p><h3>See Also:</h3>  
   * <a href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/Metrics">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API Metrics
  {
  public:
    Metrics();
    Metrics(const Aws::Utils::Xml::XmlNode& xmlNode);
    Metrics& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p> Specifies whether the replication metrics are enabled. </p>
     */
    inline const MetricsStatus& GetStatus() const{ return m_status; }

    /**
     * <p> Specifies whether the replication metrics are enabled. </p>
     */
    inline bool StatusHasBeenSet() const { return m_statusHasBeenSet; }

    /**
     * <p> Specifies whether the replication metrics are enabled. </p>
     */
    inline void SetStatus(const MetricsStatus& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * <p> Specifies whether the replication metrics are enabled. </p>
     */
    inline void SetStatus(MetricsStatus&& value) { m_statusHasBeenSet = true; m_status = std::move(value); }

    /**
     * <p> Specifies whether the replication metrics are enabled. </p>
     */
    inline Metrics& WithStatus(const MetricsStatus& value) { SetStatus(value); return *this;}

    /**
     * <p> Specifies whether the replication metrics are enabled. </p>
     */
    inline Metrics& WithStatus(MetricsStatus&& value) { SetStatus(std::move(value)); return *this;}


    /**
     * <p> A container specifying the time threshold for emitting the
     * <code>s3:Replication:OperationMissedThreshold</code> event. </p>
     */
    inline const ReplicationTimeValue& GetEventThreshold() const{ return m_eventThreshold; }

    /**
     * <p> A container specifying the time threshold for emitting the
     * <code>s3:Replication:OperationMissedThreshold</code> event. </p>
     */
    inline bool EventThresholdHasBeenSet() const { return m_eventThresholdHasBeenSet; }

    /**
     * <p> A container specifying the time threshold for emitting the
     * <code>s3:Replication:OperationMissedThreshold</code> event. </p>
     */
    inline void SetEventThreshold(const ReplicationTimeValue& value) { m_eventThresholdHasBeenSet = true; m_eventThreshold = value; }

    /**
     * <p> A container specifying the time threshold for emitting the
     * <code>s3:Replication:OperationMissedThreshold</code> event. </p>
     */
    inline void SetEventThreshold(ReplicationTimeValue&& value) { m_eventThresholdHasBeenSet = true; m_eventThreshold = std::move(value); }

    /**
     * <p> A container specifying the time threshold for emitting the
     * <code>s3:Replication:OperationMissedThreshold</code> event. </p>
     */
    inline Metrics& WithEventThreshold(const ReplicationTimeValue& value) { SetEventThreshold(value); return *this;}

    /**
     * <p> A container specifying the time threshold for emitting the
     * <code>s3:Replication:OperationMissedThreshold</code> event. </p>
     */
    inline Metrics& WithEventThreshold(ReplicationTimeValue&& value) { SetEventThreshold(std::move(value)); return *this;}

  private:

    MetricsStatus m_status;
    bool m_statusHasBeenSet;

    ReplicationTimeValue m_eventThreshold;
    bool m_eventThresholdHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
