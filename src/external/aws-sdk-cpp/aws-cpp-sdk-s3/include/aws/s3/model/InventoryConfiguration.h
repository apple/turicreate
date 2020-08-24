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
#include <aws/s3/model/InventoryDestination.h>
#include <aws/s3/model/InventoryFilter.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/InventoryIncludedObjectVersions.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/InventorySchedule.h>
#include <aws/s3/model/InventoryOptionalField.h>
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
   * <p>Specifies the inventory configuration for an Amazon S3 bucket. For more
   * information, see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketGETInventoryConfig.html">GET
   * Bucket inventory</a> in the <i>Amazon Simple Storage Service API Reference</i>.
   * </p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/InventoryConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API InventoryConfiguration
  {
  public:
    InventoryConfiguration();
    InventoryConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    InventoryConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Contains information about where to publish the inventory results.</p>
     */
    inline const InventoryDestination& GetDestination() const{ return m_destination; }

    /**
     * <p>Contains information about where to publish the inventory results.</p>
     */
    inline bool DestinationHasBeenSet() const { return m_destinationHasBeenSet; }

    /**
     * <p>Contains information about where to publish the inventory results.</p>
     */
    inline void SetDestination(const InventoryDestination& value) { m_destinationHasBeenSet = true; m_destination = value; }

    /**
     * <p>Contains information about where to publish the inventory results.</p>
     */
    inline void SetDestination(InventoryDestination&& value) { m_destinationHasBeenSet = true; m_destination = std::move(value); }

    /**
     * <p>Contains information about where to publish the inventory results.</p>
     */
    inline InventoryConfiguration& WithDestination(const InventoryDestination& value) { SetDestination(value); return *this;}

    /**
     * <p>Contains information about where to publish the inventory results.</p>
     */
    inline InventoryConfiguration& WithDestination(InventoryDestination&& value) { SetDestination(std::move(value)); return *this;}


    /**
     * <p>Specifies whether the inventory is enabled or disabled. If set to
     * <code>True</code>, an inventory list is generated. If set to <code>False</code>,
     * no inventory list is generated.</p>
     */
    inline bool GetIsEnabled() const{ return m_isEnabled; }

    /**
     * <p>Specifies whether the inventory is enabled or disabled. If set to
     * <code>True</code>, an inventory list is generated. If set to <code>False</code>,
     * no inventory list is generated.</p>
     */
    inline bool IsEnabledHasBeenSet() const { return m_isEnabledHasBeenSet; }

    /**
     * <p>Specifies whether the inventory is enabled or disabled. If set to
     * <code>True</code>, an inventory list is generated. If set to <code>False</code>,
     * no inventory list is generated.</p>
     */
    inline void SetIsEnabled(bool value) { m_isEnabledHasBeenSet = true; m_isEnabled = value; }

    /**
     * <p>Specifies whether the inventory is enabled or disabled. If set to
     * <code>True</code>, an inventory list is generated. If set to <code>False</code>,
     * no inventory list is generated.</p>
     */
    inline InventoryConfiguration& WithIsEnabled(bool value) { SetIsEnabled(value); return *this;}


    /**
     * <p>Specifies an inventory filter. The inventory only includes objects that meet
     * the filter's criteria.</p>
     */
    inline const InventoryFilter& GetFilter() const{ return m_filter; }

    /**
     * <p>Specifies an inventory filter. The inventory only includes objects that meet
     * the filter's criteria.</p>
     */
    inline bool FilterHasBeenSet() const { return m_filterHasBeenSet; }

    /**
     * <p>Specifies an inventory filter. The inventory only includes objects that meet
     * the filter's criteria.</p>
     */
    inline void SetFilter(const InventoryFilter& value) { m_filterHasBeenSet = true; m_filter = value; }

    /**
     * <p>Specifies an inventory filter. The inventory only includes objects that meet
     * the filter's criteria.</p>
     */
    inline void SetFilter(InventoryFilter&& value) { m_filterHasBeenSet = true; m_filter = std::move(value); }

    /**
     * <p>Specifies an inventory filter. The inventory only includes objects that meet
     * the filter's criteria.</p>
     */
    inline InventoryConfiguration& WithFilter(const InventoryFilter& value) { SetFilter(value); return *this;}

    /**
     * <p>Specifies an inventory filter. The inventory only includes objects that meet
     * the filter's criteria.</p>
     */
    inline InventoryConfiguration& WithFilter(InventoryFilter&& value) { SetFilter(std::move(value)); return *this;}


    /**
     * <p>The ID used to identify the inventory configuration.</p>
     */
    inline const Aws::String& GetId() const{ return m_id; }

    /**
     * <p>The ID used to identify the inventory configuration.</p>
     */
    inline bool IdHasBeenSet() const { return m_idHasBeenSet; }

    /**
     * <p>The ID used to identify the inventory configuration.</p>
     */
    inline void SetId(const Aws::String& value) { m_idHasBeenSet = true; m_id = value; }

    /**
     * <p>The ID used to identify the inventory configuration.</p>
     */
    inline void SetId(Aws::String&& value) { m_idHasBeenSet = true; m_id = std::move(value); }

    /**
     * <p>The ID used to identify the inventory configuration.</p>
     */
    inline void SetId(const char* value) { m_idHasBeenSet = true; m_id.assign(value); }

    /**
     * <p>The ID used to identify the inventory configuration.</p>
     */
    inline InventoryConfiguration& WithId(const Aws::String& value) { SetId(value); return *this;}

    /**
     * <p>The ID used to identify the inventory configuration.</p>
     */
    inline InventoryConfiguration& WithId(Aws::String&& value) { SetId(std::move(value)); return *this;}

    /**
     * <p>The ID used to identify the inventory configuration.</p>
     */
    inline InventoryConfiguration& WithId(const char* value) { SetId(value); return *this;}


    /**
     * <p>Object versions to include in the inventory list. If set to <code>All</code>,
     * the list includes all the object versions, which adds the version-related fields
     * <code>VersionId</code>, <code>IsLatest</code>, and <code>DeleteMarker</code> to
     * the list. If set to <code>Current</code>, the list does not contain these
     * version-related fields.</p>
     */
    inline const InventoryIncludedObjectVersions& GetIncludedObjectVersions() const{ return m_includedObjectVersions; }

    /**
     * <p>Object versions to include in the inventory list. If set to <code>All</code>,
     * the list includes all the object versions, which adds the version-related fields
     * <code>VersionId</code>, <code>IsLatest</code>, and <code>DeleteMarker</code> to
     * the list. If set to <code>Current</code>, the list does not contain these
     * version-related fields.</p>
     */
    inline bool IncludedObjectVersionsHasBeenSet() const { return m_includedObjectVersionsHasBeenSet; }

    /**
     * <p>Object versions to include in the inventory list. If set to <code>All</code>,
     * the list includes all the object versions, which adds the version-related fields
     * <code>VersionId</code>, <code>IsLatest</code>, and <code>DeleteMarker</code> to
     * the list. If set to <code>Current</code>, the list does not contain these
     * version-related fields.</p>
     */
    inline void SetIncludedObjectVersions(const InventoryIncludedObjectVersions& value) { m_includedObjectVersionsHasBeenSet = true; m_includedObjectVersions = value; }

    /**
     * <p>Object versions to include in the inventory list. If set to <code>All</code>,
     * the list includes all the object versions, which adds the version-related fields
     * <code>VersionId</code>, <code>IsLatest</code>, and <code>DeleteMarker</code> to
     * the list. If set to <code>Current</code>, the list does not contain these
     * version-related fields.</p>
     */
    inline void SetIncludedObjectVersions(InventoryIncludedObjectVersions&& value) { m_includedObjectVersionsHasBeenSet = true; m_includedObjectVersions = std::move(value); }

    /**
     * <p>Object versions to include in the inventory list. If set to <code>All</code>,
     * the list includes all the object versions, which adds the version-related fields
     * <code>VersionId</code>, <code>IsLatest</code>, and <code>DeleteMarker</code> to
     * the list. If set to <code>Current</code>, the list does not contain these
     * version-related fields.</p>
     */
    inline InventoryConfiguration& WithIncludedObjectVersions(const InventoryIncludedObjectVersions& value) { SetIncludedObjectVersions(value); return *this;}

    /**
     * <p>Object versions to include in the inventory list. If set to <code>All</code>,
     * the list includes all the object versions, which adds the version-related fields
     * <code>VersionId</code>, <code>IsLatest</code>, and <code>DeleteMarker</code> to
     * the list. If set to <code>Current</code>, the list does not contain these
     * version-related fields.</p>
     */
    inline InventoryConfiguration& WithIncludedObjectVersions(InventoryIncludedObjectVersions&& value) { SetIncludedObjectVersions(std::move(value)); return *this;}


    /**
     * <p>Contains the optional fields that are included in the inventory results.</p>
     */
    inline const Aws::Vector<InventoryOptionalField>& GetOptionalFields() const{ return m_optionalFields; }

    /**
     * <p>Contains the optional fields that are included in the inventory results.</p>
     */
    inline bool OptionalFieldsHasBeenSet() const { return m_optionalFieldsHasBeenSet; }

    /**
     * <p>Contains the optional fields that are included in the inventory results.</p>
     */
    inline void SetOptionalFields(const Aws::Vector<InventoryOptionalField>& value) { m_optionalFieldsHasBeenSet = true; m_optionalFields = value; }

    /**
     * <p>Contains the optional fields that are included in the inventory results.</p>
     */
    inline void SetOptionalFields(Aws::Vector<InventoryOptionalField>&& value) { m_optionalFieldsHasBeenSet = true; m_optionalFields = std::move(value); }

    /**
     * <p>Contains the optional fields that are included in the inventory results.</p>
     */
    inline InventoryConfiguration& WithOptionalFields(const Aws::Vector<InventoryOptionalField>& value) { SetOptionalFields(value); return *this;}

    /**
     * <p>Contains the optional fields that are included in the inventory results.</p>
     */
    inline InventoryConfiguration& WithOptionalFields(Aws::Vector<InventoryOptionalField>&& value) { SetOptionalFields(std::move(value)); return *this;}

    /**
     * <p>Contains the optional fields that are included in the inventory results.</p>
     */
    inline InventoryConfiguration& AddOptionalFields(const InventoryOptionalField& value) { m_optionalFieldsHasBeenSet = true; m_optionalFields.push_back(value); return *this; }

    /**
     * <p>Contains the optional fields that are included in the inventory results.</p>
     */
    inline InventoryConfiguration& AddOptionalFields(InventoryOptionalField&& value) { m_optionalFieldsHasBeenSet = true; m_optionalFields.push_back(std::move(value)); return *this; }


    /**
     * <p>Specifies the schedule for generating inventory results.</p>
     */
    inline const InventorySchedule& GetSchedule() const{ return m_schedule; }

    /**
     * <p>Specifies the schedule for generating inventory results.</p>
     */
    inline bool ScheduleHasBeenSet() const { return m_scheduleHasBeenSet; }

    /**
     * <p>Specifies the schedule for generating inventory results.</p>
     */
    inline void SetSchedule(const InventorySchedule& value) { m_scheduleHasBeenSet = true; m_schedule = value; }

    /**
     * <p>Specifies the schedule for generating inventory results.</p>
     */
    inline void SetSchedule(InventorySchedule&& value) { m_scheduleHasBeenSet = true; m_schedule = std::move(value); }

    /**
     * <p>Specifies the schedule for generating inventory results.</p>
     */
    inline InventoryConfiguration& WithSchedule(const InventorySchedule& value) { SetSchedule(value); return *this;}

    /**
     * <p>Specifies the schedule for generating inventory results.</p>
     */
    inline InventoryConfiguration& WithSchedule(InventorySchedule&& value) { SetSchedule(std::move(value)); return *this;}

  private:

    InventoryDestination m_destination;
    bool m_destinationHasBeenSet;

    bool m_isEnabled;
    bool m_isEnabledHasBeenSet;

    InventoryFilter m_filter;
    bool m_filterHasBeenSet;

    Aws::String m_id;
    bool m_idHasBeenSet;

    InventoryIncludedObjectVersions m_includedObjectVersions;
    bool m_includedObjectVersionsHasBeenSet;

    Aws::Vector<InventoryOptionalField> m_optionalFields;
    bool m_optionalFieldsHasBeenSet;

    InventorySchedule m_schedule;
    bool m_scheduleHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
