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
#include <aws/core/utils/DateTime.h>
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
   * <p>A Retention configuration for an object.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ObjectLockRetention">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ObjectLockRetention
  {
  public:
    ObjectLockRetention();
    ObjectLockRetention(const Aws::Utils::Xml::XmlNode& xmlNode);
    ObjectLockRetention& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Indicates the Retention mode for the specified object.</p>
     */
    inline const ObjectLockRetentionMode& GetMode() const{ return m_mode; }

    /**
     * <p>Indicates the Retention mode for the specified object.</p>
     */
    inline bool ModeHasBeenSet() const { return m_modeHasBeenSet; }

    /**
     * <p>Indicates the Retention mode for the specified object.</p>
     */
    inline void SetMode(const ObjectLockRetentionMode& value) { m_modeHasBeenSet = true; m_mode = value; }

    /**
     * <p>Indicates the Retention mode for the specified object.</p>
     */
    inline void SetMode(ObjectLockRetentionMode&& value) { m_modeHasBeenSet = true; m_mode = std::move(value); }

    /**
     * <p>Indicates the Retention mode for the specified object.</p>
     */
    inline ObjectLockRetention& WithMode(const ObjectLockRetentionMode& value) { SetMode(value); return *this;}

    /**
     * <p>Indicates the Retention mode for the specified object.</p>
     */
    inline ObjectLockRetention& WithMode(ObjectLockRetentionMode&& value) { SetMode(std::move(value)); return *this;}


    /**
     * <p>The date on which this Object Lock Retention will expire.</p>
     */
    inline const Aws::Utils::DateTime& GetRetainUntilDate() const{ return m_retainUntilDate; }

    /**
     * <p>The date on which this Object Lock Retention will expire.</p>
     */
    inline bool RetainUntilDateHasBeenSet() const { return m_retainUntilDateHasBeenSet; }

    /**
     * <p>The date on which this Object Lock Retention will expire.</p>
     */
    inline void SetRetainUntilDate(const Aws::Utils::DateTime& value) { m_retainUntilDateHasBeenSet = true; m_retainUntilDate = value; }

    /**
     * <p>The date on which this Object Lock Retention will expire.</p>
     */
    inline void SetRetainUntilDate(Aws::Utils::DateTime&& value) { m_retainUntilDateHasBeenSet = true; m_retainUntilDate = std::move(value); }

    /**
     * <p>The date on which this Object Lock Retention will expire.</p>
     */
    inline ObjectLockRetention& WithRetainUntilDate(const Aws::Utils::DateTime& value) { SetRetainUntilDate(value); return *this;}

    /**
     * <p>The date on which this Object Lock Retention will expire.</p>
     */
    inline ObjectLockRetention& WithRetainUntilDate(Aws::Utils::DateTime&& value) { SetRetainUntilDate(std::move(value)); return *this;}

  private:

    ObjectLockRetentionMode m_mode;
    bool m_modeHasBeenSet;

    Aws::Utils::DateTime m_retainUntilDate;
    bool m_retainUntilDateHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
