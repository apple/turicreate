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
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/TargetGrant.h>
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
   * <p>Describes where logs are stored and the prefix that Amazon S3 assigns to all
   * log object keys for a bucket. For more information, see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTlogging.html">PUT
   * Bucket logging</a> in the <i>Amazon Simple Storage Service API
   * Reference</i>.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/LoggingEnabled">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API LoggingEnabled
  {
  public:
    LoggingEnabled();
    LoggingEnabled(const Aws::Utils::Xml::XmlNode& xmlNode);
    LoggingEnabled& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies the bucket where you want Amazon S3 to store server access logs.
     * You can have your logs delivered to any bucket that you own, including the same
     * bucket that is being logged. You can also configure multiple buckets to deliver
     * their logs to the same target bucket. In this case, you should choose a
     * different <code>TargetPrefix</code> for each source bucket so that the delivered
     * log files can be distinguished by key.</p>
     */
    inline const Aws::String& GetTargetBucket() const{ return m_targetBucket; }

    /**
     * <p>Specifies the bucket where you want Amazon S3 to store server access logs.
     * You can have your logs delivered to any bucket that you own, including the same
     * bucket that is being logged. You can also configure multiple buckets to deliver
     * their logs to the same target bucket. In this case, you should choose a
     * different <code>TargetPrefix</code> for each source bucket so that the delivered
     * log files can be distinguished by key.</p>
     */
    inline bool TargetBucketHasBeenSet() const { return m_targetBucketHasBeenSet; }

    /**
     * <p>Specifies the bucket where you want Amazon S3 to store server access logs.
     * You can have your logs delivered to any bucket that you own, including the same
     * bucket that is being logged. You can also configure multiple buckets to deliver
     * their logs to the same target bucket. In this case, you should choose a
     * different <code>TargetPrefix</code> for each source bucket so that the delivered
     * log files can be distinguished by key.</p>
     */
    inline void SetTargetBucket(const Aws::String& value) { m_targetBucketHasBeenSet = true; m_targetBucket = value; }

    /**
     * <p>Specifies the bucket where you want Amazon S3 to store server access logs.
     * You can have your logs delivered to any bucket that you own, including the same
     * bucket that is being logged. You can also configure multiple buckets to deliver
     * their logs to the same target bucket. In this case, you should choose a
     * different <code>TargetPrefix</code> for each source bucket so that the delivered
     * log files can be distinguished by key.</p>
     */
    inline void SetTargetBucket(Aws::String&& value) { m_targetBucketHasBeenSet = true; m_targetBucket = std::move(value); }

    /**
     * <p>Specifies the bucket where you want Amazon S3 to store server access logs.
     * You can have your logs delivered to any bucket that you own, including the same
     * bucket that is being logged. You can also configure multiple buckets to deliver
     * their logs to the same target bucket. In this case, you should choose a
     * different <code>TargetPrefix</code> for each source bucket so that the delivered
     * log files can be distinguished by key.</p>
     */
    inline void SetTargetBucket(const char* value) { m_targetBucketHasBeenSet = true; m_targetBucket.assign(value); }

    /**
     * <p>Specifies the bucket where you want Amazon S3 to store server access logs.
     * You can have your logs delivered to any bucket that you own, including the same
     * bucket that is being logged. You can also configure multiple buckets to deliver
     * their logs to the same target bucket. In this case, you should choose a
     * different <code>TargetPrefix</code> for each source bucket so that the delivered
     * log files can be distinguished by key.</p>
     */
    inline LoggingEnabled& WithTargetBucket(const Aws::String& value) { SetTargetBucket(value); return *this;}

    /**
     * <p>Specifies the bucket where you want Amazon S3 to store server access logs.
     * You can have your logs delivered to any bucket that you own, including the same
     * bucket that is being logged. You can also configure multiple buckets to deliver
     * their logs to the same target bucket. In this case, you should choose a
     * different <code>TargetPrefix</code> for each source bucket so that the delivered
     * log files can be distinguished by key.</p>
     */
    inline LoggingEnabled& WithTargetBucket(Aws::String&& value) { SetTargetBucket(std::move(value)); return *this;}

    /**
     * <p>Specifies the bucket where you want Amazon S3 to store server access logs.
     * You can have your logs delivered to any bucket that you own, including the same
     * bucket that is being logged. You can also configure multiple buckets to deliver
     * their logs to the same target bucket. In this case, you should choose a
     * different <code>TargetPrefix</code> for each source bucket so that the delivered
     * log files can be distinguished by key.</p>
     */
    inline LoggingEnabled& WithTargetBucket(const char* value) { SetTargetBucket(value); return *this;}


    /**
     * <p>Container for granting information.</p>
     */
    inline const Aws::Vector<TargetGrant>& GetTargetGrants() const{ return m_targetGrants; }

    /**
     * <p>Container for granting information.</p>
     */
    inline bool TargetGrantsHasBeenSet() const { return m_targetGrantsHasBeenSet; }

    /**
     * <p>Container for granting information.</p>
     */
    inline void SetTargetGrants(const Aws::Vector<TargetGrant>& value) { m_targetGrantsHasBeenSet = true; m_targetGrants = value; }

    /**
     * <p>Container for granting information.</p>
     */
    inline void SetTargetGrants(Aws::Vector<TargetGrant>&& value) { m_targetGrantsHasBeenSet = true; m_targetGrants = std::move(value); }

    /**
     * <p>Container for granting information.</p>
     */
    inline LoggingEnabled& WithTargetGrants(const Aws::Vector<TargetGrant>& value) { SetTargetGrants(value); return *this;}

    /**
     * <p>Container for granting information.</p>
     */
    inline LoggingEnabled& WithTargetGrants(Aws::Vector<TargetGrant>&& value) { SetTargetGrants(std::move(value)); return *this;}

    /**
     * <p>Container for granting information.</p>
     */
    inline LoggingEnabled& AddTargetGrants(const TargetGrant& value) { m_targetGrantsHasBeenSet = true; m_targetGrants.push_back(value); return *this; }

    /**
     * <p>Container for granting information.</p>
     */
    inline LoggingEnabled& AddTargetGrants(TargetGrant&& value) { m_targetGrantsHasBeenSet = true; m_targetGrants.push_back(std::move(value)); return *this; }


    /**
     * <p>A prefix for all log object keys. If you store log files from multiple Amazon
     * S3 buckets in a single bucket, you can use a prefix to distinguish which log
     * files came from which bucket.</p>
     */
    inline const Aws::String& GetTargetPrefix() const{ return m_targetPrefix; }

    /**
     * <p>A prefix for all log object keys. If you store log files from multiple Amazon
     * S3 buckets in a single bucket, you can use a prefix to distinguish which log
     * files came from which bucket.</p>
     */
    inline bool TargetPrefixHasBeenSet() const { return m_targetPrefixHasBeenSet; }

    /**
     * <p>A prefix for all log object keys. If you store log files from multiple Amazon
     * S3 buckets in a single bucket, you can use a prefix to distinguish which log
     * files came from which bucket.</p>
     */
    inline void SetTargetPrefix(const Aws::String& value) { m_targetPrefixHasBeenSet = true; m_targetPrefix = value; }

    /**
     * <p>A prefix for all log object keys. If you store log files from multiple Amazon
     * S3 buckets in a single bucket, you can use a prefix to distinguish which log
     * files came from which bucket.</p>
     */
    inline void SetTargetPrefix(Aws::String&& value) { m_targetPrefixHasBeenSet = true; m_targetPrefix = std::move(value); }

    /**
     * <p>A prefix for all log object keys. If you store log files from multiple Amazon
     * S3 buckets in a single bucket, you can use a prefix to distinguish which log
     * files came from which bucket.</p>
     */
    inline void SetTargetPrefix(const char* value) { m_targetPrefixHasBeenSet = true; m_targetPrefix.assign(value); }

    /**
     * <p>A prefix for all log object keys. If you store log files from multiple Amazon
     * S3 buckets in a single bucket, you can use a prefix to distinguish which log
     * files came from which bucket.</p>
     */
    inline LoggingEnabled& WithTargetPrefix(const Aws::String& value) { SetTargetPrefix(value); return *this;}

    /**
     * <p>A prefix for all log object keys. If you store log files from multiple Amazon
     * S3 buckets in a single bucket, you can use a prefix to distinguish which log
     * files came from which bucket.</p>
     */
    inline LoggingEnabled& WithTargetPrefix(Aws::String&& value) { SetTargetPrefix(std::move(value)); return *this;}

    /**
     * <p>A prefix for all log object keys. If you store log files from multiple Amazon
     * S3 buckets in a single bucket, you can use a prefix to distinguish which log
     * files came from which bucket.</p>
     */
    inline LoggingEnabled& WithTargetPrefix(const char* value) { SetTargetPrefix(value); return *this;}

  private:

    Aws::String m_targetBucket;
    bool m_targetBucketHasBeenSet;

    Aws::Vector<TargetGrant> m_targetGrants;
    bool m_targetGrantsHasBeenSet;

    Aws::String m_targetPrefix;
    bool m_targetPrefixHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
