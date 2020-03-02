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
#include <aws/s3/model/Stats.h>
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
   * <p>Container for the Stats Event.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/StatsEvent">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API StatsEvent
  {
  public:
    StatsEvent();
    StatsEvent(const Aws::Utils::Xml::XmlNode& xmlNode);
    StatsEvent& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The Stats event details.</p>
     */
    inline const Stats& GetDetails() const{ return m_details; }

    /**
     * <p>The Stats event details.</p>
     */
    inline bool DetailsHasBeenSet() const { return m_detailsHasBeenSet; }

    /**
     * <p>The Stats event details.</p>
     */
    inline void SetDetails(const Stats& value) { m_detailsHasBeenSet = true; m_details = value; }

    /**
     * <p>The Stats event details.</p>
     */
    inline void SetDetails(Stats&& value) { m_detailsHasBeenSet = true; m_details = std::move(value); }

    /**
     * <p>The Stats event details.</p>
     */
    inline StatsEvent& WithDetails(const Stats& value) { SetDetails(value); return *this;}

    /**
     * <p>The Stats event details.</p>
     */
    inline StatsEvent& WithDetails(Stats&& value) { SetDetails(std::move(value)); return *this;}

  private:

    Stats m_details;
    bool m_detailsHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
