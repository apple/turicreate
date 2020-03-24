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
   * <p> A container specifying the time value for S3 Replication Time Control (S3
   * RTC) and replication metrics <code>EventThreshold</code>. </p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ReplicationTimeValue">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ReplicationTimeValue
  {
  public:
    ReplicationTimeValue();
    ReplicationTimeValue(const Aws::Utils::Xml::XmlNode& xmlNode);
    ReplicationTimeValue& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p> Contains an integer specifying time in minutes. </p> <p> Valid values: 15
     * minutes. </p>
     */
    inline int GetMinutes() const{ return m_minutes; }

    /**
     * <p> Contains an integer specifying time in minutes. </p> <p> Valid values: 15
     * minutes. </p>
     */
    inline bool MinutesHasBeenSet() const { return m_minutesHasBeenSet; }

    /**
     * <p> Contains an integer specifying time in minutes. </p> <p> Valid values: 15
     * minutes. </p>
     */
    inline void SetMinutes(int value) { m_minutesHasBeenSet = true; m_minutes = value; }

    /**
     * <p> Contains an integer specifying time in minutes. </p> <p> Valid values: 15
     * minutes. </p>
     */
    inline ReplicationTimeValue& WithMinutes(int value) { SetMinutes(value); return *this;}

  private:

    int m_minutes;
    bool m_minutesHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
