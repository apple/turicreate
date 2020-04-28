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
#include <aws/s3/model/AnalyticsS3ExportFileFormat.h>
#include <aws/core/utils/memory/stl/AWSString.h>
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
   * <p>Contains information about where to publish the analytics
   * results.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/AnalyticsS3BucketDestination">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API AnalyticsS3BucketDestination
  {
  public:
    AnalyticsS3BucketDestination();
    AnalyticsS3BucketDestination(const Aws::Utils::Xml::XmlNode& xmlNode);
    AnalyticsS3BucketDestination& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies the file format used when exporting data to Amazon S3.</p>
     */
    inline const AnalyticsS3ExportFileFormat& GetFormat() const{ return m_format; }

    /**
     * <p>Specifies the file format used when exporting data to Amazon S3.</p>
     */
    inline bool FormatHasBeenSet() const { return m_formatHasBeenSet; }

    /**
     * <p>Specifies the file format used when exporting data to Amazon S3.</p>
     */
    inline void SetFormat(const AnalyticsS3ExportFileFormat& value) { m_formatHasBeenSet = true; m_format = value; }

    /**
     * <p>Specifies the file format used when exporting data to Amazon S3.</p>
     */
    inline void SetFormat(AnalyticsS3ExportFileFormat&& value) { m_formatHasBeenSet = true; m_format = std::move(value); }

    /**
     * <p>Specifies the file format used when exporting data to Amazon S3.</p>
     */
    inline AnalyticsS3BucketDestination& WithFormat(const AnalyticsS3ExportFileFormat& value) { SetFormat(value); return *this;}

    /**
     * <p>Specifies the file format used when exporting data to Amazon S3.</p>
     */
    inline AnalyticsS3BucketDestination& WithFormat(AnalyticsS3ExportFileFormat&& value) { SetFormat(std::move(value)); return *this;}


    /**
     * <p>The account ID that owns the destination bucket. If no account ID is
     * provided, the owner will not be validated prior to exporting data.</p>
     */
    inline const Aws::String& GetBucketAccountId() const{ return m_bucketAccountId; }

    /**
     * <p>The account ID that owns the destination bucket. If no account ID is
     * provided, the owner will not be validated prior to exporting data.</p>
     */
    inline bool BucketAccountIdHasBeenSet() const { return m_bucketAccountIdHasBeenSet; }

    /**
     * <p>The account ID that owns the destination bucket. If no account ID is
     * provided, the owner will not be validated prior to exporting data.</p>
     */
    inline void SetBucketAccountId(const Aws::String& value) { m_bucketAccountIdHasBeenSet = true; m_bucketAccountId = value; }

    /**
     * <p>The account ID that owns the destination bucket. If no account ID is
     * provided, the owner will not be validated prior to exporting data.</p>
     */
    inline void SetBucketAccountId(Aws::String&& value) { m_bucketAccountIdHasBeenSet = true; m_bucketAccountId = std::move(value); }

    /**
     * <p>The account ID that owns the destination bucket. If no account ID is
     * provided, the owner will not be validated prior to exporting data.</p>
     */
    inline void SetBucketAccountId(const char* value) { m_bucketAccountIdHasBeenSet = true; m_bucketAccountId.assign(value); }

    /**
     * <p>The account ID that owns the destination bucket. If no account ID is
     * provided, the owner will not be validated prior to exporting data.</p>
     */
    inline AnalyticsS3BucketDestination& WithBucketAccountId(const Aws::String& value) { SetBucketAccountId(value); return *this;}

    /**
     * <p>The account ID that owns the destination bucket. If no account ID is
     * provided, the owner will not be validated prior to exporting data.</p>
     */
    inline AnalyticsS3BucketDestination& WithBucketAccountId(Aws::String&& value) { SetBucketAccountId(std::move(value)); return *this;}

    /**
     * <p>The account ID that owns the destination bucket. If no account ID is
     * provided, the owner will not be validated prior to exporting data.</p>
     */
    inline AnalyticsS3BucketDestination& WithBucketAccountId(const char* value) { SetBucketAccountId(value); return *this;}


    /**
     * <p>The Amazon Resource Name (ARN) of the bucket to which data is exported.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket to which data is exported.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket to which data is exported.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket to which data is exported.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket to which data is exported.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket to which data is exported.</p>
     */
    inline AnalyticsS3BucketDestination& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket to which data is exported.</p>
     */
    inline AnalyticsS3BucketDestination& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket to which data is exported.</p>
     */
    inline AnalyticsS3BucketDestination& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>The prefix to use when exporting data. The prefix is prepended to all
     * results.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>The prefix to use when exporting data. The prefix is prepended to all
     * results.</p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>The prefix to use when exporting data. The prefix is prepended to all
     * results.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>The prefix to use when exporting data. The prefix is prepended to all
     * results.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>The prefix to use when exporting data. The prefix is prepended to all
     * results.</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>The prefix to use when exporting data. The prefix is prepended to all
     * results.</p>
     */
    inline AnalyticsS3BucketDestination& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>The prefix to use when exporting data. The prefix is prepended to all
     * results.</p>
     */
    inline AnalyticsS3BucketDestination& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>The prefix to use when exporting data. The prefix is prepended to all
     * results.</p>
     */
    inline AnalyticsS3BucketDestination& WithPrefix(const char* value) { SetPrefix(value); return *this;}

  private:

    AnalyticsS3ExportFileFormat m_format;
    bool m_formatHasBeenSet;

    Aws::String m_bucketAccountId;
    bool m_bucketAccountIdHasBeenSet;

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
