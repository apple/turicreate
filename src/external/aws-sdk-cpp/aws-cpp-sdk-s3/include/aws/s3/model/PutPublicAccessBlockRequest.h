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
#include <aws/s3/S3Request.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/PublicAccessBlockConfiguration.h>
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <utility>

namespace Aws
{
namespace Http
{
    class URI;
} //namespace Http
namespace S3
{
namespace Model
{

  /**
   */
  class AWS_S3_API PutPublicAccessBlockRequest : public S3Request
  {
  public:
    PutPublicAccessBlockRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "PutPublicAccessBlock"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>The name of the Amazon S3 bucket whose <code>PublicAccessBlock</code>
     * configuration you want to set.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>The name of the Amazon S3 bucket whose <code>PublicAccessBlock</code>
     * configuration you want to set.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The name of the Amazon S3 bucket whose <code>PublicAccessBlock</code>
     * configuration you want to set.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The name of the Amazon S3 bucket whose <code>PublicAccessBlock</code>
     * configuration you want to set.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The name of the Amazon S3 bucket whose <code>PublicAccessBlock</code>
     * configuration you want to set.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The name of the Amazon S3 bucket whose <code>PublicAccessBlock</code>
     * configuration you want to set.</p>
     */
    inline PutPublicAccessBlockRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The name of the Amazon S3 bucket whose <code>PublicAccessBlock</code>
     * configuration you want to set.</p>
     */
    inline PutPublicAccessBlockRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The name of the Amazon S3 bucket whose <code>PublicAccessBlock</code>
     * configuration you want to set.</p>
     */
    inline PutPublicAccessBlockRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>The MD5 hash of the <code>PutPublicAccessBlock</code> request body. </p>
     */
    inline const Aws::String& GetContentMD5() const{ return m_contentMD5; }

    /**
     * <p>The MD5 hash of the <code>PutPublicAccessBlock</code> request body. </p>
     */
    inline bool ContentMD5HasBeenSet() const { return m_contentMD5HasBeenSet; }

    /**
     * <p>The MD5 hash of the <code>PutPublicAccessBlock</code> request body. </p>
     */
    inline void SetContentMD5(const Aws::String& value) { m_contentMD5HasBeenSet = true; m_contentMD5 = value; }

    /**
     * <p>The MD5 hash of the <code>PutPublicAccessBlock</code> request body. </p>
     */
    inline void SetContentMD5(Aws::String&& value) { m_contentMD5HasBeenSet = true; m_contentMD5 = std::move(value); }

    /**
     * <p>The MD5 hash of the <code>PutPublicAccessBlock</code> request body. </p>
     */
    inline void SetContentMD5(const char* value) { m_contentMD5HasBeenSet = true; m_contentMD5.assign(value); }

    /**
     * <p>The MD5 hash of the <code>PutPublicAccessBlock</code> request body. </p>
     */
    inline PutPublicAccessBlockRequest& WithContentMD5(const Aws::String& value) { SetContentMD5(value); return *this;}

    /**
     * <p>The MD5 hash of the <code>PutPublicAccessBlock</code> request body. </p>
     */
    inline PutPublicAccessBlockRequest& WithContentMD5(Aws::String&& value) { SetContentMD5(std::move(value)); return *this;}

    /**
     * <p>The MD5 hash of the <code>PutPublicAccessBlock</code> request body. </p>
     */
    inline PutPublicAccessBlockRequest& WithContentMD5(const char* value) { SetContentMD5(value); return *this;}


    /**
     * <p>The <code>PublicAccessBlock</code> configuration that you want to apply to
     * this Amazon S3 bucket. You can enable the configuration options in any
     * combination. For more information about when Amazon S3 considers a bucket or
     * object public, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/access-control-block-public-access.html#access-control-block-public-access-policy-status">The
     * Meaning of "Public"</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline const PublicAccessBlockConfiguration& GetPublicAccessBlockConfiguration() const{ return m_publicAccessBlockConfiguration; }

    /**
     * <p>The <code>PublicAccessBlock</code> configuration that you want to apply to
     * this Amazon S3 bucket. You can enable the configuration options in any
     * combination. For more information about when Amazon S3 considers a bucket or
     * object public, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/access-control-block-public-access.html#access-control-block-public-access-policy-status">The
     * Meaning of "Public"</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline bool PublicAccessBlockConfigurationHasBeenSet() const { return m_publicAccessBlockConfigurationHasBeenSet; }

    /**
     * <p>The <code>PublicAccessBlock</code> configuration that you want to apply to
     * this Amazon S3 bucket. You can enable the configuration options in any
     * combination. For more information about when Amazon S3 considers a bucket or
     * object public, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/access-control-block-public-access.html#access-control-block-public-access-policy-status">The
     * Meaning of "Public"</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetPublicAccessBlockConfiguration(const PublicAccessBlockConfiguration& value) { m_publicAccessBlockConfigurationHasBeenSet = true; m_publicAccessBlockConfiguration = value; }

    /**
     * <p>The <code>PublicAccessBlock</code> configuration that you want to apply to
     * this Amazon S3 bucket. You can enable the configuration options in any
     * combination. For more information about when Amazon S3 considers a bucket or
     * object public, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/access-control-block-public-access.html#access-control-block-public-access-policy-status">The
     * Meaning of "Public"</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetPublicAccessBlockConfiguration(PublicAccessBlockConfiguration&& value) { m_publicAccessBlockConfigurationHasBeenSet = true; m_publicAccessBlockConfiguration = std::move(value); }

    /**
     * <p>The <code>PublicAccessBlock</code> configuration that you want to apply to
     * this Amazon S3 bucket. You can enable the configuration options in any
     * combination. For more information about when Amazon S3 considers a bucket or
     * object public, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/access-control-block-public-access.html#access-control-block-public-access-policy-status">The
     * Meaning of "Public"</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline PutPublicAccessBlockRequest& WithPublicAccessBlockConfiguration(const PublicAccessBlockConfiguration& value) { SetPublicAccessBlockConfiguration(value); return *this;}

    /**
     * <p>The <code>PublicAccessBlock</code> configuration that you want to apply to
     * this Amazon S3 bucket. You can enable the configuration options in any
     * combination. For more information about when Amazon S3 considers a bucket or
     * object public, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/access-control-block-public-access.html#access-control-block-public-access-policy-status">The
     * Meaning of "Public"</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline PutPublicAccessBlockRequest& WithPublicAccessBlockConfiguration(PublicAccessBlockConfiguration&& value) { SetPublicAccessBlockConfiguration(std::move(value)); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline PutPublicAccessBlockRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline PutPublicAccessBlockRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline PutPublicAccessBlockRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline PutPublicAccessBlockRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline PutPublicAccessBlockRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline PutPublicAccessBlockRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline PutPublicAccessBlockRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline PutPublicAccessBlockRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline PutPublicAccessBlockRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_contentMD5;
    bool m_contentMD5HasBeenSet;

    PublicAccessBlockConfiguration m_publicAccessBlockConfiguration;
    bool m_publicAccessBlockConfigurationHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
