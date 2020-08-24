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
#include <aws/s3/model/StorageClass.h>
#include <aws/s3/model/AccessControlTranslation.h>
#include <aws/s3/model/EncryptionConfiguration.h>
#include <aws/s3/model/ReplicationTime.h>
#include <aws/s3/model/Metrics.h>
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
   * <p>Specifies information about where to publish analysis or configuration
   * results for an Amazon S3 bucket and S3 Replication Time Control (S3
   * RTC).</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/Destination">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API Destination
  {
  public:
    Destination();
    Destination(const Aws::Utils::Xml::XmlNode& xmlNode);
    Destination& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p> The Amazon Resource Name (ARN) of the bucket where you want Amazon S3 to
     * store the results.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p> The Amazon Resource Name (ARN) of the bucket where you want Amazon S3 to
     * store the results.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p> The Amazon Resource Name (ARN) of the bucket where you want Amazon S3 to
     * store the results.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p> The Amazon Resource Name (ARN) of the bucket where you want Amazon S3 to
     * store the results.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p> The Amazon Resource Name (ARN) of the bucket where you want Amazon S3 to
     * store the results.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p> The Amazon Resource Name (ARN) of the bucket where you want Amazon S3 to
     * store the results.</p>
     */
    inline Destination& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p> The Amazon Resource Name (ARN) of the bucket where you want Amazon S3 to
     * store the results.</p>
     */
    inline Destination& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p> The Amazon Resource Name (ARN) of the bucket where you want Amazon S3 to
     * store the results.</p>
     */
    inline Destination& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>Destination bucket owner account ID. In a cross-account scenario, if you
     * direct Amazon S3 to change replica ownership to the AWS account that owns the
     * destination bucket by specifying the <code>AccessControlTranslation</code>
     * property, this is the account ID of the destination bucket owner. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-change-owner.html">Replication
     * Additional Configuration: Changing the Replica Owner</a> in the <i>Amazon Simple
     * Storage Service Developer Guide</i>.</p>
     */
    inline const Aws::String& GetAccount() const{ return m_account; }

    /**
     * <p>Destination bucket owner account ID. In a cross-account scenario, if you
     * direct Amazon S3 to change replica ownership to the AWS account that owns the
     * destination bucket by specifying the <code>AccessControlTranslation</code>
     * property, this is the account ID of the destination bucket owner. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-change-owner.html">Replication
     * Additional Configuration: Changing the Replica Owner</a> in the <i>Amazon Simple
     * Storage Service Developer Guide</i>.</p>
     */
    inline bool AccountHasBeenSet() const { return m_accountHasBeenSet; }

    /**
     * <p>Destination bucket owner account ID. In a cross-account scenario, if you
     * direct Amazon S3 to change replica ownership to the AWS account that owns the
     * destination bucket by specifying the <code>AccessControlTranslation</code>
     * property, this is the account ID of the destination bucket owner. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-change-owner.html">Replication
     * Additional Configuration: Changing the Replica Owner</a> in the <i>Amazon Simple
     * Storage Service Developer Guide</i>.</p>
     */
    inline void SetAccount(const Aws::String& value) { m_accountHasBeenSet = true; m_account = value; }

    /**
     * <p>Destination bucket owner account ID. In a cross-account scenario, if you
     * direct Amazon S3 to change replica ownership to the AWS account that owns the
     * destination bucket by specifying the <code>AccessControlTranslation</code>
     * property, this is the account ID of the destination bucket owner. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-change-owner.html">Replication
     * Additional Configuration: Changing the Replica Owner</a> in the <i>Amazon Simple
     * Storage Service Developer Guide</i>.</p>
     */
    inline void SetAccount(Aws::String&& value) { m_accountHasBeenSet = true; m_account = std::move(value); }

    /**
     * <p>Destination bucket owner account ID. In a cross-account scenario, if you
     * direct Amazon S3 to change replica ownership to the AWS account that owns the
     * destination bucket by specifying the <code>AccessControlTranslation</code>
     * property, this is the account ID of the destination bucket owner. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-change-owner.html">Replication
     * Additional Configuration: Changing the Replica Owner</a> in the <i>Amazon Simple
     * Storage Service Developer Guide</i>.</p>
     */
    inline void SetAccount(const char* value) { m_accountHasBeenSet = true; m_account.assign(value); }

    /**
     * <p>Destination bucket owner account ID. In a cross-account scenario, if you
     * direct Amazon S3 to change replica ownership to the AWS account that owns the
     * destination bucket by specifying the <code>AccessControlTranslation</code>
     * property, this is the account ID of the destination bucket owner. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-change-owner.html">Replication
     * Additional Configuration: Changing the Replica Owner</a> in the <i>Amazon Simple
     * Storage Service Developer Guide</i>.</p>
     */
    inline Destination& WithAccount(const Aws::String& value) { SetAccount(value); return *this;}

    /**
     * <p>Destination bucket owner account ID. In a cross-account scenario, if you
     * direct Amazon S3 to change replica ownership to the AWS account that owns the
     * destination bucket by specifying the <code>AccessControlTranslation</code>
     * property, this is the account ID of the destination bucket owner. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-change-owner.html">Replication
     * Additional Configuration: Changing the Replica Owner</a> in the <i>Amazon Simple
     * Storage Service Developer Guide</i>.</p>
     */
    inline Destination& WithAccount(Aws::String&& value) { SetAccount(std::move(value)); return *this;}

    /**
     * <p>Destination bucket owner account ID. In a cross-account scenario, if you
     * direct Amazon S3 to change replica ownership to the AWS account that owns the
     * destination bucket by specifying the <code>AccessControlTranslation</code>
     * property, this is the account ID of the destination bucket owner. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-change-owner.html">Replication
     * Additional Configuration: Changing the Replica Owner</a> in the <i>Amazon Simple
     * Storage Service Developer Guide</i>.</p>
     */
    inline Destination& WithAccount(const char* value) { SetAccount(value); return *this;}


    /**
     * <p> The storage class to use when replicating objects, such as standard or
     * reduced redundancy. By default, Amazon S3 uses the storage class of the source
     * object to create the object replica. </p> <p>For valid values, see the
     * <code>StorageClass</code> element of the <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTreplication.html">PUT
     * Bucket replication</a> action in the <i>Amazon Simple Storage Service API
     * Reference</i>.</p>
     */
    inline const StorageClass& GetStorageClass() const{ return m_storageClass; }

    /**
     * <p> The storage class to use when replicating objects, such as standard or
     * reduced redundancy. By default, Amazon S3 uses the storage class of the source
     * object to create the object replica. </p> <p>For valid values, see the
     * <code>StorageClass</code> element of the <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTreplication.html">PUT
     * Bucket replication</a> action in the <i>Amazon Simple Storage Service API
     * Reference</i>.</p>
     */
    inline bool StorageClassHasBeenSet() const { return m_storageClassHasBeenSet; }

    /**
     * <p> The storage class to use when replicating objects, such as standard or
     * reduced redundancy. By default, Amazon S3 uses the storage class of the source
     * object to create the object replica. </p> <p>For valid values, see the
     * <code>StorageClass</code> element of the <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTreplication.html">PUT
     * Bucket replication</a> action in the <i>Amazon Simple Storage Service API
     * Reference</i>.</p>
     */
    inline void SetStorageClass(const StorageClass& value) { m_storageClassHasBeenSet = true; m_storageClass = value; }

    /**
     * <p> The storage class to use when replicating objects, such as standard or
     * reduced redundancy. By default, Amazon S3 uses the storage class of the source
     * object to create the object replica. </p> <p>For valid values, see the
     * <code>StorageClass</code> element of the <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTreplication.html">PUT
     * Bucket replication</a> action in the <i>Amazon Simple Storage Service API
     * Reference</i>.</p>
     */
    inline void SetStorageClass(StorageClass&& value) { m_storageClassHasBeenSet = true; m_storageClass = std::move(value); }

    /**
     * <p> The storage class to use when replicating objects, such as standard or
     * reduced redundancy. By default, Amazon S3 uses the storage class of the source
     * object to create the object replica. </p> <p>For valid values, see the
     * <code>StorageClass</code> element of the <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTreplication.html">PUT
     * Bucket replication</a> action in the <i>Amazon Simple Storage Service API
     * Reference</i>.</p>
     */
    inline Destination& WithStorageClass(const StorageClass& value) { SetStorageClass(value); return *this;}

    /**
     * <p> The storage class to use when replicating objects, such as standard or
     * reduced redundancy. By default, Amazon S3 uses the storage class of the source
     * object to create the object replica. </p> <p>For valid values, see the
     * <code>StorageClass</code> element of the <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTreplication.html">PUT
     * Bucket replication</a> action in the <i>Amazon Simple Storage Service API
     * Reference</i>.</p>
     */
    inline Destination& WithStorageClass(StorageClass&& value) { SetStorageClass(std::move(value)); return *this;}


    /**
     * <p>Specify this only in a cross-account scenario (where source and destination
     * bucket owners are not the same), and you want to change replica ownership to the
     * AWS account that owns the destination bucket. If this is not specified in the
     * replication configuration, the replicas are owned by same AWS account that owns
     * the source object.</p>
     */
    inline const AccessControlTranslation& GetAccessControlTranslation() const{ return m_accessControlTranslation; }

    /**
     * <p>Specify this only in a cross-account scenario (where source and destination
     * bucket owners are not the same), and you want to change replica ownership to the
     * AWS account that owns the destination bucket. If this is not specified in the
     * replication configuration, the replicas are owned by same AWS account that owns
     * the source object.</p>
     */
    inline bool AccessControlTranslationHasBeenSet() const { return m_accessControlTranslationHasBeenSet; }

    /**
     * <p>Specify this only in a cross-account scenario (where source and destination
     * bucket owners are not the same), and you want to change replica ownership to the
     * AWS account that owns the destination bucket. If this is not specified in the
     * replication configuration, the replicas are owned by same AWS account that owns
     * the source object.</p>
     */
    inline void SetAccessControlTranslation(const AccessControlTranslation& value) { m_accessControlTranslationHasBeenSet = true; m_accessControlTranslation = value; }

    /**
     * <p>Specify this only in a cross-account scenario (where source and destination
     * bucket owners are not the same), and you want to change replica ownership to the
     * AWS account that owns the destination bucket. If this is not specified in the
     * replication configuration, the replicas are owned by same AWS account that owns
     * the source object.</p>
     */
    inline void SetAccessControlTranslation(AccessControlTranslation&& value) { m_accessControlTranslationHasBeenSet = true; m_accessControlTranslation = std::move(value); }

    /**
     * <p>Specify this only in a cross-account scenario (where source and destination
     * bucket owners are not the same), and you want to change replica ownership to the
     * AWS account that owns the destination bucket. If this is not specified in the
     * replication configuration, the replicas are owned by same AWS account that owns
     * the source object.</p>
     */
    inline Destination& WithAccessControlTranslation(const AccessControlTranslation& value) { SetAccessControlTranslation(value); return *this;}

    /**
     * <p>Specify this only in a cross-account scenario (where source and destination
     * bucket owners are not the same), and you want to change replica ownership to the
     * AWS account that owns the destination bucket. If this is not specified in the
     * replication configuration, the replicas are owned by same AWS account that owns
     * the source object.</p>
     */
    inline Destination& WithAccessControlTranslation(AccessControlTranslation&& value) { SetAccessControlTranslation(std::move(value)); return *this;}


    /**
     * <p>A container that provides information about encryption. If
     * <code>SourceSelectionCriteria</code> is specified, you must specify this
     * element.</p>
     */
    inline const EncryptionConfiguration& GetEncryptionConfiguration() const{ return m_encryptionConfiguration; }

    /**
     * <p>A container that provides information about encryption. If
     * <code>SourceSelectionCriteria</code> is specified, you must specify this
     * element.</p>
     */
    inline bool EncryptionConfigurationHasBeenSet() const { return m_encryptionConfigurationHasBeenSet; }

    /**
     * <p>A container that provides information about encryption. If
     * <code>SourceSelectionCriteria</code> is specified, you must specify this
     * element.</p>
     */
    inline void SetEncryptionConfiguration(const EncryptionConfiguration& value) { m_encryptionConfigurationHasBeenSet = true; m_encryptionConfiguration = value; }

    /**
     * <p>A container that provides information about encryption. If
     * <code>SourceSelectionCriteria</code> is specified, you must specify this
     * element.</p>
     */
    inline void SetEncryptionConfiguration(EncryptionConfiguration&& value) { m_encryptionConfigurationHasBeenSet = true; m_encryptionConfiguration = std::move(value); }

    /**
     * <p>A container that provides information about encryption. If
     * <code>SourceSelectionCriteria</code> is specified, you must specify this
     * element.</p>
     */
    inline Destination& WithEncryptionConfiguration(const EncryptionConfiguration& value) { SetEncryptionConfiguration(value); return *this;}

    /**
     * <p>A container that provides information about encryption. If
     * <code>SourceSelectionCriteria</code> is specified, you must specify this
     * element.</p>
     */
    inline Destination& WithEncryptionConfiguration(EncryptionConfiguration&& value) { SetEncryptionConfiguration(std::move(value)); return *this;}


    /**
     * <p> A container specifying S3 Replication Time Control (S3 RTC), including
     * whether S3 RTC is enabled and the time when all objects and operations on
     * objects must be replicated. Must be specified together with a
     * <code>Metrics</code> block. </p>
     */
    inline const ReplicationTime& GetReplicationTime() const{ return m_replicationTime; }

    /**
     * <p> A container specifying S3 Replication Time Control (S3 RTC), including
     * whether S3 RTC is enabled and the time when all objects and operations on
     * objects must be replicated. Must be specified together with a
     * <code>Metrics</code> block. </p>
     */
    inline bool ReplicationTimeHasBeenSet() const { return m_replicationTimeHasBeenSet; }

    /**
     * <p> A container specifying S3 Replication Time Control (S3 RTC), including
     * whether S3 RTC is enabled and the time when all objects and operations on
     * objects must be replicated. Must be specified together with a
     * <code>Metrics</code> block. </p>
     */
    inline void SetReplicationTime(const ReplicationTime& value) { m_replicationTimeHasBeenSet = true; m_replicationTime = value; }

    /**
     * <p> A container specifying S3 Replication Time Control (S3 RTC), including
     * whether S3 RTC is enabled and the time when all objects and operations on
     * objects must be replicated. Must be specified together with a
     * <code>Metrics</code> block. </p>
     */
    inline void SetReplicationTime(ReplicationTime&& value) { m_replicationTimeHasBeenSet = true; m_replicationTime = std::move(value); }

    /**
     * <p> A container specifying S3 Replication Time Control (S3 RTC), including
     * whether S3 RTC is enabled and the time when all objects and operations on
     * objects must be replicated. Must be specified together with a
     * <code>Metrics</code> block. </p>
     */
    inline Destination& WithReplicationTime(const ReplicationTime& value) { SetReplicationTime(value); return *this;}

    /**
     * <p> A container specifying S3 Replication Time Control (S3 RTC), including
     * whether S3 RTC is enabled and the time when all objects and operations on
     * objects must be replicated. Must be specified together with a
     * <code>Metrics</code> block. </p>
     */
    inline Destination& WithReplicationTime(ReplicationTime&& value) { SetReplicationTime(std::move(value)); return *this;}


    /**
     * <p> A container specifying replication metrics-related settings enabling metrics
     * and Amazon S3 events for S3 Replication Time Control (S3 RTC). Must be specified
     * together with a <code>ReplicationTime</code> block. </p>
     */
    inline const Metrics& GetMetrics() const{ return m_metrics; }

    /**
     * <p> A container specifying replication metrics-related settings enabling metrics
     * and Amazon S3 events for S3 Replication Time Control (S3 RTC). Must be specified
     * together with a <code>ReplicationTime</code> block. </p>
     */
    inline bool MetricsHasBeenSet() const { return m_metricsHasBeenSet; }

    /**
     * <p> A container specifying replication metrics-related settings enabling metrics
     * and Amazon S3 events for S3 Replication Time Control (S3 RTC). Must be specified
     * together with a <code>ReplicationTime</code> block. </p>
     */
    inline void SetMetrics(const Metrics& value) { m_metricsHasBeenSet = true; m_metrics = value; }

    /**
     * <p> A container specifying replication metrics-related settings enabling metrics
     * and Amazon S3 events for S3 Replication Time Control (S3 RTC). Must be specified
     * together with a <code>ReplicationTime</code> block. </p>
     */
    inline void SetMetrics(Metrics&& value) { m_metricsHasBeenSet = true; m_metrics = std::move(value); }

    /**
     * <p> A container specifying replication metrics-related settings enabling metrics
     * and Amazon S3 events for S3 Replication Time Control (S3 RTC). Must be specified
     * together with a <code>ReplicationTime</code> block. </p>
     */
    inline Destination& WithMetrics(const Metrics& value) { SetMetrics(value); return *this;}

    /**
     * <p> A container specifying replication metrics-related settings enabling metrics
     * and Amazon S3 events for S3 Replication Time Control (S3 RTC). Must be specified
     * together with a <code>ReplicationTime</code> block. </p>
     */
    inline Destination& WithMetrics(Metrics&& value) { SetMetrics(std::move(value)); return *this;}

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_account;
    bool m_accountHasBeenSet;

    StorageClass m_storageClass;
    bool m_storageClassHasBeenSet;

    AccessControlTranslation m_accessControlTranslation;
    bool m_accessControlTranslationHasBeenSet;

    EncryptionConfiguration m_encryptionConfiguration;
    bool m_encryptionConfigurationHasBeenSet;

    ReplicationTime m_replicationTime;
    bool m_replicationTimeHasBeenSet;

    Metrics m_metrics;
    bool m_metricsHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
