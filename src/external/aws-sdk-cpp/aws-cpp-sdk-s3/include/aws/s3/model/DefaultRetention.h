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
#include <aws/s3/model/ObjectLockRetentionMode.h>
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
   * <p>The container element for specifying the default Object Lock retention
   * settings for new objects placed in the specified bucket.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/DefaultRetention">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API DefaultRetention
  {
  public:
    DefaultRetention();
    DefaultRetention(const Aws::Utils::Xml::XmlNode& xmlNode);
    DefaultRetention& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The default Object Lock retention mode you want to apply to new objects
     * placed in the specified bucket.</p>
     */
    inline const ObjectLockRetentionMode& GetMode() const{ return m_mode; }

    /**
     * <p>The default Object Lock retention mode you want to apply to new objects
     * placed in the specified bucket.</p>
     */
    inline bool ModeHasBeenSet() const { return m_modeHasBeenSet; }

    /**
     * <p>The default Object Lock retention mode you want to apply to new objects
     * placed in the specified bucket.</p>
     */
    inline void SetMode(const ObjectLockRetentionMode& value) { m_modeHasBeenSet = true; m_mode = value; }

    /**
     * <p>The default Object Lock retention mode you want to apply to new objects
     * placed in the specified bucket.</p>
     */
    inline void SetMode(ObjectLockRetentionMode&& value) { m_modeHasBeenSet = true; m_mode = std::move(value); }

    /**
     * <p>The default Object Lock retention mode you want to apply to new objects
     * placed in the specified bucket.</p>
     */
    inline DefaultRetention& WithMode(const ObjectLockRetentionMode& value) { SetMode(value); return *this;}

    /**
     * <p>The default Object Lock retention mode you want to apply to new objects
     * placed in the specified bucket.</p>
     */
    inline DefaultRetention& WithMode(ObjectLockRetentionMode&& value) { SetMode(std::move(value)); return *this;}


    /**
     * <p>The number of days that you want to specify for the default retention
     * period.</p>
     */
    inline int GetDays() const{ return m_days; }

    /**
     * <p>The number of days that you want to specify for the default retention
     * period.</p>
     */
    inline bool DaysHasBeenSet() const { return m_daysHasBeenSet; }

    /**
     * <p>The number of days that you want to specify for the default retention
     * period.</p>
     */
    inline void SetDays(int value) { m_daysHasBeenSet = true; m_days = value; }

    /**
     * <p>The number of days that you want to specify for the default retention
     * period.</p>
     */
    inline DefaultRetention& WithDays(int value) { SetDays(value); return *this;}


    /**
     * <p>The number of years that you want to specify for the default retention
     * period.</p>
     */
    inline int GetYears() const{ return m_years; }

    /**
     * <p>The number of years that you want to specify for the default retention
     * period.</p>
     */
    inline bool YearsHasBeenSet() const { return m_yearsHasBeenSet; }

    /**
     * <p>The number of years that you want to specify for the default retention
     * period.</p>
     */
    inline void SetYears(int value) { m_yearsHasBeenSet = true; m_years = value; }

    /**
     * <p>The number of years that you want to specify for the default retention
     * period.</p>
     */
    inline DefaultRetention& WithYears(int value) { SetYears(value); return *this;}

  private:

    ObjectLockRetentionMode m_mode;
    bool m_modeHasBeenSet;

    int m_days;
    bool m_daysHasBeenSet;

    int m_years;
    bool m_yearsHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
