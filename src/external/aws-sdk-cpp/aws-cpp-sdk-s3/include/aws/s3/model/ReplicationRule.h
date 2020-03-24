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
#include <aws/s3/model/ReplicationRuleFilter.h>
#include <aws/s3/model/ReplicationRuleStatus.h>
#include <aws/s3/model/SourceSelectionCriteria.h>
#include <aws/s3/model/ExistingObjectReplication.h>
#include <aws/s3/model/Destination.h>
#include <aws/s3/model/DeleteMarkerReplication.h>
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
   * <p>Specifies which Amazon S3 objects to replicate and where to store the
   * replicas.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ReplicationRule">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ReplicationRule
  {
  public:
    ReplicationRule();
    ReplicationRule(const Aws::Utils::Xml::XmlNode& xmlNode);
    ReplicationRule& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>A unique identifier for the rule. The maximum value is 255 characters.</p>
     */
    inline const Aws::String& GetID() const{ return m_iD; }

    /**
     * <p>A unique identifier for the rule. The maximum value is 255 characters.</p>
     */
    inline bool IDHasBeenSet() const { return m_iDHasBeenSet; }

    /**
     * <p>A unique identifier for the rule. The maximum value is 255 characters.</p>
     */
    inline void SetID(const Aws::String& value) { m_iDHasBeenSet = true; m_iD = value; }

    /**
     * <p>A unique identifier for the rule. The maximum value is 255 characters.</p>
     */
    inline void SetID(Aws::String&& value) { m_iDHasBeenSet = true; m_iD = std::move(value); }

    /**
     * <p>A unique identifier for the rule. The maximum value is 255 characters.</p>
     */
    inline void SetID(const char* value) { m_iDHasBeenSet = true; m_iD.assign(value); }

    /**
     * <p>A unique identifier for the rule. The maximum value is 255 characters.</p>
     */
    inline ReplicationRule& WithID(const Aws::String& value) { SetID(value); return *this;}

    /**
     * <p>A unique identifier for the rule. The maximum value is 255 characters.</p>
     */
    inline ReplicationRule& WithID(Aws::String&& value) { SetID(std::move(value)); return *this;}

    /**
     * <p>A unique identifier for the rule. The maximum value is 255 characters.</p>
     */
    inline ReplicationRule& WithID(const char* value) { SetID(value); return *this;}


    /**
     * <p>The priority associated with the rule. If you specify multiple rules in a
     * replication configuration, Amazon S3 prioritizes the rules to prevent conflicts
     * when filtering. If two or more rules identify the same object based on a
     * specified filter, the rule with higher priority takes precedence. For
     * example:</p> <ul> <li> <p>Same object quality prefix-based filter criteria if
     * prefixes you specified in multiple rules overlap </p> </li> <li> <p>Same object
     * qualify tag-based filter criteria specified in multiple rules</p> </li> </ul>
     * <p>For more information, see <a href="
     * https://docs.aws.amazon.com/AmazonS3/latest/dev/replication.html">Replication</a>
     * in the <i>Amazon Simple Storage Service Developer Guide</i>.</p>
     */
    inline int GetPriority() const{ return m_priority; }

    /**
     * <p>The priority associated with the rule. If you specify multiple rules in a
     * replication configuration, Amazon S3 prioritizes the rules to prevent conflicts
     * when filtering. If two or more rules identify the same object based on a
     * specified filter, the rule with higher priority takes precedence. For
     * example:</p> <ul> <li> <p>Same object quality prefix-based filter criteria if
     * prefixes you specified in multiple rules overlap </p> </li> <li> <p>Same object
     * qualify tag-based filter criteria specified in multiple rules</p> </li> </ul>
     * <p>For more information, see <a href="
     * https://docs.aws.amazon.com/AmazonS3/latest/dev/replication.html">Replication</a>
     * in the <i>Amazon Simple Storage Service Developer Guide</i>.</p>
     */
    inline bool PriorityHasBeenSet() const { return m_priorityHasBeenSet; }

    /**
     * <p>The priority associated with the rule. If you specify multiple rules in a
     * replication configuration, Amazon S3 prioritizes the rules to prevent conflicts
     * when filtering. If two or more rules identify the same object based on a
     * specified filter, the rule with higher priority takes precedence. For
     * example:</p> <ul> <li> <p>Same object quality prefix-based filter criteria if
     * prefixes you specified in multiple rules overlap </p> </li> <li> <p>Same object
     * qualify tag-based filter criteria specified in multiple rules</p> </li> </ul>
     * <p>For more information, see <a href="
     * https://docs.aws.amazon.com/AmazonS3/latest/dev/replication.html">Replication</a>
     * in the <i>Amazon Simple Storage Service Developer Guide</i>.</p>
     */
    inline void SetPriority(int value) { m_priorityHasBeenSet = true; m_priority = value; }

    /**
     * <p>The priority associated with the rule. If you specify multiple rules in a
     * replication configuration, Amazon S3 prioritizes the rules to prevent conflicts
     * when filtering. If two or more rules identify the same object based on a
     * specified filter, the rule with higher priority takes precedence. For
     * example:</p> <ul> <li> <p>Same object quality prefix-based filter criteria if
     * prefixes you specified in multiple rules overlap </p> </li> <li> <p>Same object
     * qualify tag-based filter criteria specified in multiple rules</p> </li> </ul>
     * <p>For more information, see <a href="
     * https://docs.aws.amazon.com/AmazonS3/latest/dev/replication.html">Replication</a>
     * in the <i>Amazon Simple Storage Service Developer Guide</i>.</p>
     */
    inline ReplicationRule& WithPriority(int value) { SetPriority(value); return *this;}


    
    inline const ReplicationRuleFilter& GetFilter() const{ return m_filter; }

    
    inline bool FilterHasBeenSet() const { return m_filterHasBeenSet; }

    
    inline void SetFilter(const ReplicationRuleFilter& value) { m_filterHasBeenSet = true; m_filter = value; }

    
    inline void SetFilter(ReplicationRuleFilter&& value) { m_filterHasBeenSet = true; m_filter = std::move(value); }

    
    inline ReplicationRule& WithFilter(const ReplicationRuleFilter& value) { SetFilter(value); return *this;}

    
    inline ReplicationRule& WithFilter(ReplicationRuleFilter&& value) { SetFilter(std::move(value)); return *this;}


    /**
     * <p>Specifies whether the rule is enabled.</p>
     */
    inline const ReplicationRuleStatus& GetStatus() const{ return m_status; }

    /**
     * <p>Specifies whether the rule is enabled.</p>
     */
    inline bool StatusHasBeenSet() const { return m_statusHasBeenSet; }

    /**
     * <p>Specifies whether the rule is enabled.</p>
     */
    inline void SetStatus(const ReplicationRuleStatus& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * <p>Specifies whether the rule is enabled.</p>
     */
    inline void SetStatus(ReplicationRuleStatus&& value) { m_statusHasBeenSet = true; m_status = std::move(value); }

    /**
     * <p>Specifies whether the rule is enabled.</p>
     */
    inline ReplicationRule& WithStatus(const ReplicationRuleStatus& value) { SetStatus(value); return *this;}

    /**
     * <p>Specifies whether the rule is enabled.</p>
     */
    inline ReplicationRule& WithStatus(ReplicationRuleStatus&& value) { SetStatus(std::move(value)); return *this;}


    /**
     * <p>A container that describes additional filters for identifying the source
     * objects that you want to replicate. You can choose to enable or disable the
     * replication of these objects. Currently, Amazon S3 supports only the filter that
     * you can specify for objects created with server-side encryption using a customer
     * master key (CMK) stored in AWS Key Management Service (SSE-KMS).</p>
     */
    inline const SourceSelectionCriteria& GetSourceSelectionCriteria() const{ return m_sourceSelectionCriteria; }

    /**
     * <p>A container that describes additional filters for identifying the source
     * objects that you want to replicate. You can choose to enable or disable the
     * replication of these objects. Currently, Amazon S3 supports only the filter that
     * you can specify for objects created with server-side encryption using a customer
     * master key (CMK) stored in AWS Key Management Service (SSE-KMS).</p>
     */
    inline bool SourceSelectionCriteriaHasBeenSet() const { return m_sourceSelectionCriteriaHasBeenSet; }

    /**
     * <p>A container that describes additional filters for identifying the source
     * objects that you want to replicate. You can choose to enable or disable the
     * replication of these objects. Currently, Amazon S3 supports only the filter that
     * you can specify for objects created with server-side encryption using a customer
     * master key (CMK) stored in AWS Key Management Service (SSE-KMS).</p>
     */
    inline void SetSourceSelectionCriteria(const SourceSelectionCriteria& value) { m_sourceSelectionCriteriaHasBeenSet = true; m_sourceSelectionCriteria = value; }

    /**
     * <p>A container that describes additional filters for identifying the source
     * objects that you want to replicate. You can choose to enable or disable the
     * replication of these objects. Currently, Amazon S3 supports only the filter that
     * you can specify for objects created with server-side encryption using a customer
     * master key (CMK) stored in AWS Key Management Service (SSE-KMS).</p>
     */
    inline void SetSourceSelectionCriteria(SourceSelectionCriteria&& value) { m_sourceSelectionCriteriaHasBeenSet = true; m_sourceSelectionCriteria = std::move(value); }

    /**
     * <p>A container that describes additional filters for identifying the source
     * objects that you want to replicate. You can choose to enable or disable the
     * replication of these objects. Currently, Amazon S3 supports only the filter that
     * you can specify for objects created with server-side encryption using a customer
     * master key (CMK) stored in AWS Key Management Service (SSE-KMS).</p>
     */
    inline ReplicationRule& WithSourceSelectionCriteria(const SourceSelectionCriteria& value) { SetSourceSelectionCriteria(value); return *this;}

    /**
     * <p>A container that describes additional filters for identifying the source
     * objects that you want to replicate. You can choose to enable or disable the
     * replication of these objects. Currently, Amazon S3 supports only the filter that
     * you can specify for objects created with server-side encryption using a customer
     * master key (CMK) stored in AWS Key Management Service (SSE-KMS).</p>
     */
    inline ReplicationRule& WithSourceSelectionCriteria(SourceSelectionCriteria&& value) { SetSourceSelectionCriteria(std::move(value)); return *this;}


    /**
     * <p/>
     */
    inline const ExistingObjectReplication& GetExistingObjectReplication() const{ return m_existingObjectReplication; }

    /**
     * <p/>
     */
    inline bool ExistingObjectReplicationHasBeenSet() const { return m_existingObjectReplicationHasBeenSet; }

    /**
     * <p/>
     */
    inline void SetExistingObjectReplication(const ExistingObjectReplication& value) { m_existingObjectReplicationHasBeenSet = true; m_existingObjectReplication = value; }

    /**
     * <p/>
     */
    inline void SetExistingObjectReplication(ExistingObjectReplication&& value) { m_existingObjectReplicationHasBeenSet = true; m_existingObjectReplication = std::move(value); }

    /**
     * <p/>
     */
    inline ReplicationRule& WithExistingObjectReplication(const ExistingObjectReplication& value) { SetExistingObjectReplication(value); return *this;}

    /**
     * <p/>
     */
    inline ReplicationRule& WithExistingObjectReplication(ExistingObjectReplication&& value) { SetExistingObjectReplication(std::move(value)); return *this;}


    /**
     * <p>A container for information about the replication destination and its
     * configurations including enabling the S3 Replication Time Control (S3 RTC).</p>
     */
    inline const Destination& GetDestination() const{ return m_destination; }

    /**
     * <p>A container for information about the replication destination and its
     * configurations including enabling the S3 Replication Time Control (S3 RTC).</p>
     */
    inline bool DestinationHasBeenSet() const { return m_destinationHasBeenSet; }

    /**
     * <p>A container for information about the replication destination and its
     * configurations including enabling the S3 Replication Time Control (S3 RTC).</p>
     */
    inline void SetDestination(const Destination& value) { m_destinationHasBeenSet = true; m_destination = value; }

    /**
     * <p>A container for information about the replication destination and its
     * configurations including enabling the S3 Replication Time Control (S3 RTC).</p>
     */
    inline void SetDestination(Destination&& value) { m_destinationHasBeenSet = true; m_destination = std::move(value); }

    /**
     * <p>A container for information about the replication destination and its
     * configurations including enabling the S3 Replication Time Control (S3 RTC).</p>
     */
    inline ReplicationRule& WithDestination(const Destination& value) { SetDestination(value); return *this;}

    /**
     * <p>A container for information about the replication destination and its
     * configurations including enabling the S3 Replication Time Control (S3 RTC).</p>
     */
    inline ReplicationRule& WithDestination(Destination&& value) { SetDestination(std::move(value)); return *this;}


    
    inline const DeleteMarkerReplication& GetDeleteMarkerReplication() const{ return m_deleteMarkerReplication; }

    
    inline bool DeleteMarkerReplicationHasBeenSet() const { return m_deleteMarkerReplicationHasBeenSet; }

    
    inline void SetDeleteMarkerReplication(const DeleteMarkerReplication& value) { m_deleteMarkerReplicationHasBeenSet = true; m_deleteMarkerReplication = value; }

    
    inline void SetDeleteMarkerReplication(DeleteMarkerReplication&& value) { m_deleteMarkerReplicationHasBeenSet = true; m_deleteMarkerReplication = std::move(value); }

    
    inline ReplicationRule& WithDeleteMarkerReplication(const DeleteMarkerReplication& value) { SetDeleteMarkerReplication(value); return *this;}

    
    inline ReplicationRule& WithDeleteMarkerReplication(DeleteMarkerReplication&& value) { SetDeleteMarkerReplication(std::move(value)); return *this;}

  private:

    Aws::String m_iD;
    bool m_iDHasBeenSet;

    int m_priority;
    bool m_priorityHasBeenSet;

    ReplicationRuleFilter m_filter;
    bool m_filterHasBeenSet;

    ReplicationRuleStatus m_status;
    bool m_statusHasBeenSet;

    SourceSelectionCriteria m_sourceSelectionCriteria;
    bool m_sourceSelectionCriteriaHasBeenSet;

    ExistingObjectReplication m_existingObjectReplication;
    bool m_existingObjectReplicationHasBeenSet;

    Destination m_destination;
    bool m_destinationHasBeenSet;

    DeleteMarkerReplication m_deleteMarkerReplication;
    bool m_deleteMarkerReplicationHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
