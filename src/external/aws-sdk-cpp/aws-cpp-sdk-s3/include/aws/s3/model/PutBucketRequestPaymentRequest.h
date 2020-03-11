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
#include <aws/s3/model/RequestPaymentConfiguration.h>
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
  class AWS_S3_API PutBucketRequestPaymentRequest : public S3Request
  {
  public:
    PutBucketRequestPaymentRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "PutBucketRequestPayment"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>The bucket name.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>The bucket name.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The bucket name.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The bucket name.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The bucket name.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The bucket name.</p>
     */
    inline PutBucketRequestPaymentRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The bucket name.</p>
     */
    inline PutBucketRequestPaymentRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The bucket name.</p>
     */
    inline PutBucketRequestPaymentRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>&gt;The base64-encoded 128-bit MD5 digest of the data. You must use this
     * header as a message integrity check to verify that the request body was not
     * corrupted in transit. For more information, see <a
     * href="http://www.ietf.org/rfc/rfc1864.txt">RFC 1864</a>.</p>
     */
    inline const Aws::String& GetContentMD5() const{ return m_contentMD5; }

    /**
     * <p>&gt;The base64-encoded 128-bit MD5 digest of the data. You must use this
     * header as a message integrity check to verify that the request body was not
     * corrupted in transit. For more information, see <a
     * href="http://www.ietf.org/rfc/rfc1864.txt">RFC 1864</a>.</p>
     */
    inline bool ContentMD5HasBeenSet() const { return m_contentMD5HasBeenSet; }

    /**
     * <p>&gt;The base64-encoded 128-bit MD5 digest of the data. You must use this
     * header as a message integrity check to verify that the request body was not
     * corrupted in transit. For more information, see <a
     * href="http://www.ietf.org/rfc/rfc1864.txt">RFC 1864</a>.</p>
     */
    inline void SetContentMD5(const Aws::String& value) { m_contentMD5HasBeenSet = true; m_contentMD5 = value; }

    /**
     * <p>&gt;The base64-encoded 128-bit MD5 digest of the data. You must use this
     * header as a message integrity check to verify that the request body was not
     * corrupted in transit. For more information, see <a
     * href="http://www.ietf.org/rfc/rfc1864.txt">RFC 1864</a>.</p>
     */
    inline void SetContentMD5(Aws::String&& value) { m_contentMD5HasBeenSet = true; m_contentMD5 = std::move(value); }

    /**
     * <p>&gt;The base64-encoded 128-bit MD5 digest of the data. You must use this
     * header as a message integrity check to verify that the request body was not
     * corrupted in transit. For more information, see <a
     * href="http://www.ietf.org/rfc/rfc1864.txt">RFC 1864</a>.</p>
     */
    inline void SetContentMD5(const char* value) { m_contentMD5HasBeenSet = true; m_contentMD5.assign(value); }

    /**
     * <p>&gt;The base64-encoded 128-bit MD5 digest of the data. You must use this
     * header as a message integrity check to verify that the request body was not
     * corrupted in transit. For more information, see <a
     * href="http://www.ietf.org/rfc/rfc1864.txt">RFC 1864</a>.</p>
     */
    inline PutBucketRequestPaymentRequest& WithContentMD5(const Aws::String& value) { SetContentMD5(value); return *this;}

    /**
     * <p>&gt;The base64-encoded 128-bit MD5 digest of the data. You must use this
     * header as a message integrity check to verify that the request body was not
     * corrupted in transit. For more information, see <a
     * href="http://www.ietf.org/rfc/rfc1864.txt">RFC 1864</a>.</p>
     */
    inline PutBucketRequestPaymentRequest& WithContentMD5(Aws::String&& value) { SetContentMD5(std::move(value)); return *this;}

    /**
     * <p>&gt;The base64-encoded 128-bit MD5 digest of the data. You must use this
     * header as a message integrity check to verify that the request body was not
     * corrupted in transit. For more information, see <a
     * href="http://www.ietf.org/rfc/rfc1864.txt">RFC 1864</a>.</p>
     */
    inline PutBucketRequestPaymentRequest& WithContentMD5(const char* value) { SetContentMD5(value); return *this;}


    /**
     * <p>Container for Payer.</p>
     */
    inline const RequestPaymentConfiguration& GetRequestPaymentConfiguration() const{ return m_requestPaymentConfiguration; }

    /**
     * <p>Container for Payer.</p>
     */
    inline bool RequestPaymentConfigurationHasBeenSet() const { return m_requestPaymentConfigurationHasBeenSet; }

    /**
     * <p>Container for Payer.</p>
     */
    inline void SetRequestPaymentConfiguration(const RequestPaymentConfiguration& value) { m_requestPaymentConfigurationHasBeenSet = true; m_requestPaymentConfiguration = value; }

    /**
     * <p>Container for Payer.</p>
     */
    inline void SetRequestPaymentConfiguration(RequestPaymentConfiguration&& value) { m_requestPaymentConfigurationHasBeenSet = true; m_requestPaymentConfiguration = std::move(value); }

    /**
     * <p>Container for Payer.</p>
     */
    inline PutBucketRequestPaymentRequest& WithRequestPaymentConfiguration(const RequestPaymentConfiguration& value) { SetRequestPaymentConfiguration(value); return *this;}

    /**
     * <p>Container for Payer.</p>
     */
    inline PutBucketRequestPaymentRequest& WithRequestPaymentConfiguration(RequestPaymentConfiguration&& value) { SetRequestPaymentConfiguration(std::move(value)); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline PutBucketRequestPaymentRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline PutBucketRequestPaymentRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline PutBucketRequestPaymentRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline PutBucketRequestPaymentRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline PutBucketRequestPaymentRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline PutBucketRequestPaymentRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline PutBucketRequestPaymentRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline PutBucketRequestPaymentRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline PutBucketRequestPaymentRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_contentMD5;
    bool m_contentMD5HasBeenSet;

    RequestPaymentConfiguration m_requestPaymentConfiguration;
    bool m_requestPaymentConfigurationHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
