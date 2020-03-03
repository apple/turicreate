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
   * <p>Container for the expiration for the lifecycle of the object.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/LifecycleExpiration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API LifecycleExpiration
  {
  public:
    LifecycleExpiration();
    LifecycleExpiration(const Aws::Utils::Xml::XmlNode& xmlNode);
    LifecycleExpiration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Indicates at what date the object is to be moved or deleted. Should be in GMT
     * ISO 8601 Format.</p>
     */
    inline const Aws::Utils::DateTime& GetDate() const{ return m_date; }

    /**
     * <p>Indicates at what date the object is to be moved or deleted. Should be in GMT
     * ISO 8601 Format.</p>
     */
    inline bool DateHasBeenSet() const { return m_dateHasBeenSet; }

    /**
     * <p>Indicates at what date the object is to be moved or deleted. Should be in GMT
     * ISO 8601 Format.</p>
     */
    inline void SetDate(const Aws::Utils::DateTime& value) { m_dateHasBeenSet = true; m_date = value; }

    /**
     * <p>Indicates at what date the object is to be moved or deleted. Should be in GMT
     * ISO 8601 Format.</p>
     */
    inline void SetDate(Aws::Utils::DateTime&& value) { m_dateHasBeenSet = true; m_date = std::move(value); }

    /**
     * <p>Indicates at what date the object is to be moved or deleted. Should be in GMT
     * ISO 8601 Format.</p>
     */
    inline LifecycleExpiration& WithDate(const Aws::Utils::DateTime& value) { SetDate(value); return *this;}

    /**
     * <p>Indicates at what date the object is to be moved or deleted. Should be in GMT
     * ISO 8601 Format.</p>
     */
    inline LifecycleExpiration& WithDate(Aws::Utils::DateTime&& value) { SetDate(std::move(value)); return *this;}


    /**
     * <p>Indicates the lifetime, in days, of the objects that are subject to the rule.
     * The value must be a non-zero positive integer.</p>
     */
    inline int GetDays() const{ return m_days; }

    /**
     * <p>Indicates the lifetime, in days, of the objects that are subject to the rule.
     * The value must be a non-zero positive integer.</p>
     */
    inline bool DaysHasBeenSet() const { return m_daysHasBeenSet; }

    /**
     * <p>Indicates the lifetime, in days, of the objects that are subject to the rule.
     * The value must be a non-zero positive integer.</p>
     */
    inline void SetDays(int value) { m_daysHasBeenSet = true; m_days = value; }

    /**
     * <p>Indicates the lifetime, in days, of the objects that are subject to the rule.
     * The value must be a non-zero positive integer.</p>
     */
    inline LifecycleExpiration& WithDays(int value) { SetDays(value); return *this;}


    /**
     * <p>Indicates whether Amazon S3 will remove a delete marker with no noncurrent
     * versions. If set to true, the delete marker will be expired; if set to false the
     * policy takes no action. This cannot be specified with Days or Date in a
     * Lifecycle Expiration Policy.</p>
     */
    inline bool GetExpiredObjectDeleteMarker() const{ return m_expiredObjectDeleteMarker; }

    /**
     * <p>Indicates whether Amazon S3 will remove a delete marker with no noncurrent
     * versions. If set to true, the delete marker will be expired; if set to false the
     * policy takes no action. This cannot be specified with Days or Date in a
     * Lifecycle Expiration Policy.</p>
     */
    inline bool ExpiredObjectDeleteMarkerHasBeenSet() const { return m_expiredObjectDeleteMarkerHasBeenSet; }

    /**
     * <p>Indicates whether Amazon S3 will remove a delete marker with no noncurrent
     * versions. If set to true, the delete marker will be expired; if set to false the
     * policy takes no action. This cannot be specified with Days or Date in a
     * Lifecycle Expiration Policy.</p>
     */
    inline void SetExpiredObjectDeleteMarker(bool value) { m_expiredObjectDeleteMarkerHasBeenSet = true; m_expiredObjectDeleteMarker = value; }

    /**
     * <p>Indicates whether Amazon S3 will remove a delete marker with no noncurrent
     * versions. If set to true, the delete marker will be expired; if set to false the
     * policy takes no action. This cannot be specified with Days or Date in a
     * Lifecycle Expiration Policy.</p>
     */
    inline LifecycleExpiration& WithExpiredObjectDeleteMarker(bool value) { SetExpiredObjectDeleteMarker(value); return *this;}

  private:

    Aws::Utils::DateTime m_date;
    bool m_dateHasBeenSet;

    int m_days;
    bool m_daysHasBeenSet;

    bool m_expiredObjectDeleteMarker;
    bool m_expiredObjectDeleteMarkerHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
