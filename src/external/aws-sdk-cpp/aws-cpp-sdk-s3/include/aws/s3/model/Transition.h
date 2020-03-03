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
#include <aws/s3/model/TransitionStorageClass.h>
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
   * <p>Specifies when an object transitions to a specified storage
   * class.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/Transition">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API Transition
  {
  public:
    Transition();
    Transition(const Aws::Utils::Xml::XmlNode& xmlNode);
    Transition& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Indicates when objects are transitioned to the specified storage class. The
     * date value must be in ISO 8601 format. The time is always midnight UTC.</p>
     */
    inline const Aws::Utils::DateTime& GetDate() const{ return m_date; }

    /**
     * <p>Indicates when objects are transitioned to the specified storage class. The
     * date value must be in ISO 8601 format. The time is always midnight UTC.</p>
     */
    inline bool DateHasBeenSet() const { return m_dateHasBeenSet; }

    /**
     * <p>Indicates when objects are transitioned to the specified storage class. The
     * date value must be in ISO 8601 format. The time is always midnight UTC.</p>
     */
    inline void SetDate(const Aws::Utils::DateTime& value) { m_dateHasBeenSet = true; m_date = value; }

    /**
     * <p>Indicates when objects are transitioned to the specified storage class. The
     * date value must be in ISO 8601 format. The time is always midnight UTC.</p>
     */
    inline void SetDate(Aws::Utils::DateTime&& value) { m_dateHasBeenSet = true; m_date = std::move(value); }

    /**
     * <p>Indicates when objects are transitioned to the specified storage class. The
     * date value must be in ISO 8601 format. The time is always midnight UTC.</p>
     */
    inline Transition& WithDate(const Aws::Utils::DateTime& value) { SetDate(value); return *this;}

    /**
     * <p>Indicates when objects are transitioned to the specified storage class. The
     * date value must be in ISO 8601 format. The time is always midnight UTC.</p>
     */
    inline Transition& WithDate(Aws::Utils::DateTime&& value) { SetDate(std::move(value)); return *this;}


    /**
     * <p>Indicates the number of days after creation when objects are transitioned to
     * the specified storage class. The value must be a positive integer.</p>
     */
    inline int GetDays() const{ return m_days; }

    /**
     * <p>Indicates the number of days after creation when objects are transitioned to
     * the specified storage class. The value must be a positive integer.</p>
     */
    inline bool DaysHasBeenSet() const { return m_daysHasBeenSet; }

    /**
     * <p>Indicates the number of days after creation when objects are transitioned to
     * the specified storage class. The value must be a positive integer.</p>
     */
    inline void SetDays(int value) { m_daysHasBeenSet = true; m_days = value; }

    /**
     * <p>Indicates the number of days after creation when objects are transitioned to
     * the specified storage class. The value must be a positive integer.</p>
     */
    inline Transition& WithDays(int value) { SetDays(value); return *this;}


    /**
     * <p>The storage class to which you want the object to transition.</p>
     */
    inline const TransitionStorageClass& GetStorageClass() const{ return m_storageClass; }

    /**
     * <p>The storage class to which you want the object to transition.</p>
     */
    inline bool StorageClassHasBeenSet() const { return m_storageClassHasBeenSet; }

    /**
     * <p>The storage class to which you want the object to transition.</p>
     */
    inline void SetStorageClass(const TransitionStorageClass& value) { m_storageClassHasBeenSet = true; m_storageClass = value; }

    /**
     * <p>The storage class to which you want the object to transition.</p>
     */
    inline void SetStorageClass(TransitionStorageClass&& value) { m_storageClassHasBeenSet = true; m_storageClass = std::move(value); }

    /**
     * <p>The storage class to which you want the object to transition.</p>
     */
    inline Transition& WithStorageClass(const TransitionStorageClass& value) { SetStorageClass(value); return *this;}

    /**
     * <p>The storage class to which you want the object to transition.</p>
     */
    inline Transition& WithStorageClass(TransitionStorageClass&& value) { SetStorageClass(std::move(value)); return *this;}

  private:

    Aws::Utils::DateTime m_date;
    bool m_dateHasBeenSet;

    int m_days;
    bool m_daysHasBeenSet;

    TransitionStorageClass m_storageClass;
    bool m_storageClassHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
