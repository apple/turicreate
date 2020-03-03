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
#include <aws/core/utils/DateTime.h>
#include <aws/s3/model/RequestPayer.h>
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
  class AWS_S3_API UploadPartCopyRequest : public S3Request
  {
  public:
    UploadPartCopyRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "UploadPartCopy"; }

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
    inline UploadPartCopyRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The bucket name.</p>
     */
    inline UploadPartCopyRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The bucket name.</p>
     */
    inline UploadPartCopyRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline const Aws::String& GetCopySource() const{ return m_copySource; }

    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline bool CopySourceHasBeenSet() const { return m_copySourceHasBeenSet; }

    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline void SetCopySource(const Aws::String& value) { m_copySourceHasBeenSet = true; m_copySource = value; }

    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline void SetCopySource(Aws::String&& value) { m_copySourceHasBeenSet = true; m_copySource = std::move(value); }

    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline void SetCopySource(const char* value) { m_copySourceHasBeenSet = true; m_copySource.assign(value); }

    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline UploadPartCopyRequest& WithCopySource(const Aws::String& value) { SetCopySource(value); return *this;}

    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline UploadPartCopyRequest& WithCopySource(Aws::String&& value) { SetCopySource(std::move(value)); return *this;}

    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline UploadPartCopyRequest& WithCopySource(const char* value) { SetCopySource(value); return *this;}


    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline const Aws::String& GetCopySourceIfMatch() const{ return m_copySourceIfMatch; }

    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline bool CopySourceIfMatchHasBeenSet() const { return m_copySourceIfMatchHasBeenSet; }

    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline void SetCopySourceIfMatch(const Aws::String& value) { m_copySourceIfMatchHasBeenSet = true; m_copySourceIfMatch = value; }

    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline void SetCopySourceIfMatch(Aws::String&& value) { m_copySourceIfMatchHasBeenSet = true; m_copySourceIfMatch = std::move(value); }

    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline void SetCopySourceIfMatch(const char* value) { m_copySourceIfMatchHasBeenSet = true; m_copySourceIfMatch.assign(value); }

    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfMatch(const Aws::String& value) { SetCopySourceIfMatch(value); return *this;}

    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfMatch(Aws::String&& value) { SetCopySourceIfMatch(std::move(value)); return *this;}

    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfMatch(const char* value) { SetCopySourceIfMatch(value); return *this;}


    /**
     * <p>Copies the object if it has been modified since the specified time.</p>
     */
    inline const Aws::Utils::DateTime& GetCopySourceIfModifiedSince() const{ return m_copySourceIfModifiedSince; }

    /**
     * <p>Copies the object if it has been modified since the specified time.</p>
     */
    inline bool CopySourceIfModifiedSinceHasBeenSet() const { return m_copySourceIfModifiedSinceHasBeenSet; }

    /**
     * <p>Copies the object if it has been modified since the specified time.</p>
     */
    inline void SetCopySourceIfModifiedSince(const Aws::Utils::DateTime& value) { m_copySourceIfModifiedSinceHasBeenSet = true; m_copySourceIfModifiedSince = value; }

    /**
     * <p>Copies the object if it has been modified since the specified time.</p>
     */
    inline void SetCopySourceIfModifiedSince(Aws::Utils::DateTime&& value) { m_copySourceIfModifiedSinceHasBeenSet = true; m_copySourceIfModifiedSince = std::move(value); }

    /**
     * <p>Copies the object if it has been modified since the specified time.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfModifiedSince(const Aws::Utils::DateTime& value) { SetCopySourceIfModifiedSince(value); return *this;}

    /**
     * <p>Copies the object if it has been modified since the specified time.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfModifiedSince(Aws::Utils::DateTime&& value) { SetCopySourceIfModifiedSince(std::move(value)); return *this;}


    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline const Aws::String& GetCopySourceIfNoneMatch() const{ return m_copySourceIfNoneMatch; }

    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline bool CopySourceIfNoneMatchHasBeenSet() const { return m_copySourceIfNoneMatchHasBeenSet; }

    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline void SetCopySourceIfNoneMatch(const Aws::String& value) { m_copySourceIfNoneMatchHasBeenSet = true; m_copySourceIfNoneMatch = value; }

    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline void SetCopySourceIfNoneMatch(Aws::String&& value) { m_copySourceIfNoneMatchHasBeenSet = true; m_copySourceIfNoneMatch = std::move(value); }

    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline void SetCopySourceIfNoneMatch(const char* value) { m_copySourceIfNoneMatchHasBeenSet = true; m_copySourceIfNoneMatch.assign(value); }

    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfNoneMatch(const Aws::String& value) { SetCopySourceIfNoneMatch(value); return *this;}

    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfNoneMatch(Aws::String&& value) { SetCopySourceIfNoneMatch(std::move(value)); return *this;}

    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfNoneMatch(const char* value) { SetCopySourceIfNoneMatch(value); return *this;}


    /**
     * <p>Copies the object if it hasn't been modified since the specified time.</p>
     */
    inline const Aws::Utils::DateTime& GetCopySourceIfUnmodifiedSince() const{ return m_copySourceIfUnmodifiedSince; }

    /**
     * <p>Copies the object if it hasn't been modified since the specified time.</p>
     */
    inline bool CopySourceIfUnmodifiedSinceHasBeenSet() const { return m_copySourceIfUnmodifiedSinceHasBeenSet; }

    /**
     * <p>Copies the object if it hasn't been modified since the specified time.</p>
     */
    inline void SetCopySourceIfUnmodifiedSince(const Aws::Utils::DateTime& value) { m_copySourceIfUnmodifiedSinceHasBeenSet = true; m_copySourceIfUnmodifiedSince = value; }

    /**
     * <p>Copies the object if it hasn't been modified since the specified time.</p>
     */
    inline void SetCopySourceIfUnmodifiedSince(Aws::Utils::DateTime&& value) { m_copySourceIfUnmodifiedSinceHasBeenSet = true; m_copySourceIfUnmodifiedSince = std::move(value); }

    /**
     * <p>Copies the object if it hasn't been modified since the specified time.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfUnmodifiedSince(const Aws::Utils::DateTime& value) { SetCopySourceIfUnmodifiedSince(value); return *this;}

    /**
     * <p>Copies the object if it hasn't been modified since the specified time.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceIfUnmodifiedSince(Aws::Utils::DateTime&& value) { SetCopySourceIfUnmodifiedSince(std::move(value)); return *this;}


    /**
     * <p>The range of bytes to copy from the source object. The range value must use
     * the form bytes=first-last, where the first and last are the zero-based byte
     * offsets to copy. For example, bytes=0-9 indicates that you want to copy the
     * first 10 bytes of the source. You can copy a range only if the source object is
     * greater than 5 MB.</p>
     */
    inline const Aws::String& GetCopySourceRange() const{ return m_copySourceRange; }

    /**
     * <p>The range of bytes to copy from the source object. The range value must use
     * the form bytes=first-last, where the first and last are the zero-based byte
     * offsets to copy. For example, bytes=0-9 indicates that you want to copy the
     * first 10 bytes of the source. You can copy a range only if the source object is
     * greater than 5 MB.</p>
     */
    inline bool CopySourceRangeHasBeenSet() const { return m_copySourceRangeHasBeenSet; }

    /**
     * <p>The range of bytes to copy from the source object. The range value must use
     * the form bytes=first-last, where the first and last are the zero-based byte
     * offsets to copy. For example, bytes=0-9 indicates that you want to copy the
     * first 10 bytes of the source. You can copy a range only if the source object is
     * greater than 5 MB.</p>
     */
    inline void SetCopySourceRange(const Aws::String& value) { m_copySourceRangeHasBeenSet = true; m_copySourceRange = value; }

    /**
     * <p>The range of bytes to copy from the source object. The range value must use
     * the form bytes=first-last, where the first and last are the zero-based byte
     * offsets to copy. For example, bytes=0-9 indicates that you want to copy the
     * first 10 bytes of the source. You can copy a range only if the source object is
     * greater than 5 MB.</p>
     */
    inline void SetCopySourceRange(Aws::String&& value) { m_copySourceRangeHasBeenSet = true; m_copySourceRange = std::move(value); }

    /**
     * <p>The range of bytes to copy from the source object. The range value must use
     * the form bytes=first-last, where the first and last are the zero-based byte
     * offsets to copy. For example, bytes=0-9 indicates that you want to copy the
     * first 10 bytes of the source. You can copy a range only if the source object is
     * greater than 5 MB.</p>
     */
    inline void SetCopySourceRange(const char* value) { m_copySourceRangeHasBeenSet = true; m_copySourceRange.assign(value); }

    /**
     * <p>The range of bytes to copy from the source object. The range value must use
     * the form bytes=first-last, where the first and last are the zero-based byte
     * offsets to copy. For example, bytes=0-9 indicates that you want to copy the
     * first 10 bytes of the source. You can copy a range only if the source object is
     * greater than 5 MB.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceRange(const Aws::String& value) { SetCopySourceRange(value); return *this;}

    /**
     * <p>The range of bytes to copy from the source object. The range value must use
     * the form bytes=first-last, where the first and last are the zero-based byte
     * offsets to copy. For example, bytes=0-9 indicates that you want to copy the
     * first 10 bytes of the source. You can copy a range only if the source object is
     * greater than 5 MB.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceRange(Aws::String&& value) { SetCopySourceRange(std::move(value)); return *this;}

    /**
     * <p>The range of bytes to copy from the source object. The range value must use
     * the form bytes=first-last, where the first and last are the zero-based byte
     * offsets to copy. For example, bytes=0-9 indicates that you want to copy the
     * first 10 bytes of the source. You can copy a range only if the source object is
     * greater than 5 MB.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceRange(const char* value) { SetCopySourceRange(value); return *this;}


    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline const Aws::String& GetKey() const{ return m_key; }

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline bool KeyHasBeenSet() const { return m_keyHasBeenSet; }

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline void SetKey(const Aws::String& value) { m_keyHasBeenSet = true; m_key = value; }

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline void SetKey(Aws::String&& value) { m_keyHasBeenSet = true; m_key = std::move(value); }

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline void SetKey(const char* value) { m_keyHasBeenSet = true; m_key.assign(value); }

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline UploadPartCopyRequest& WithKey(const Aws::String& value) { SetKey(value); return *this;}

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline UploadPartCopyRequest& WithKey(Aws::String&& value) { SetKey(std::move(value)); return *this;}

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline UploadPartCopyRequest& WithKey(const char* value) { SetKey(value); return *this;}


    /**
     * <p>Part number of part being copied. This is a positive integer between 1 and
     * 10,000.</p>
     */
    inline int GetPartNumber() const{ return m_partNumber; }

    /**
     * <p>Part number of part being copied. This is a positive integer between 1 and
     * 10,000.</p>
     */
    inline bool PartNumberHasBeenSet() const { return m_partNumberHasBeenSet; }

    /**
     * <p>Part number of part being copied. This is a positive integer between 1 and
     * 10,000.</p>
     */
    inline void SetPartNumber(int value) { m_partNumberHasBeenSet = true; m_partNumber = value; }

    /**
     * <p>Part number of part being copied. This is a positive integer between 1 and
     * 10,000.</p>
     */
    inline UploadPartCopyRequest& WithPartNumber(int value) { SetPartNumber(value); return *this;}


    /**
     * <p>Upload ID identifying the multipart upload whose part is being copied.</p>
     */
    inline const Aws::String& GetUploadId() const{ return m_uploadId; }

    /**
     * <p>Upload ID identifying the multipart upload whose part is being copied.</p>
     */
    inline bool UploadIdHasBeenSet() const { return m_uploadIdHasBeenSet; }

    /**
     * <p>Upload ID identifying the multipart upload whose part is being copied.</p>
     */
    inline void SetUploadId(const Aws::String& value) { m_uploadIdHasBeenSet = true; m_uploadId = value; }

    /**
     * <p>Upload ID identifying the multipart upload whose part is being copied.</p>
     */
    inline void SetUploadId(Aws::String&& value) { m_uploadIdHasBeenSet = true; m_uploadId = std::move(value); }

    /**
     * <p>Upload ID identifying the multipart upload whose part is being copied.</p>
     */
    inline void SetUploadId(const char* value) { m_uploadIdHasBeenSet = true; m_uploadId.assign(value); }

    /**
     * <p>Upload ID identifying the multipart upload whose part is being copied.</p>
     */
    inline UploadPartCopyRequest& WithUploadId(const Aws::String& value) { SetUploadId(value); return *this;}

    /**
     * <p>Upload ID identifying the multipart upload whose part is being copied.</p>
     */
    inline UploadPartCopyRequest& WithUploadId(Aws::String&& value) { SetUploadId(std::move(value)); return *this;}

    /**
     * <p>Upload ID identifying the multipart upload whose part is being copied.</p>
     */
    inline UploadPartCopyRequest& WithUploadId(const char* value) { SetUploadId(value); return *this;}


    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline const Aws::String& GetSSECustomerAlgorithm() const{ return m_sSECustomerAlgorithm; }

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline bool SSECustomerAlgorithmHasBeenSet() const { return m_sSECustomerAlgorithmHasBeenSet; }

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline void SetSSECustomerAlgorithm(const Aws::String& value) { m_sSECustomerAlgorithmHasBeenSet = true; m_sSECustomerAlgorithm = value; }

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline void SetSSECustomerAlgorithm(Aws::String&& value) { m_sSECustomerAlgorithmHasBeenSet = true; m_sSECustomerAlgorithm = std::move(value); }

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline void SetSSECustomerAlgorithm(const char* value) { m_sSECustomerAlgorithmHasBeenSet = true; m_sSECustomerAlgorithm.assign(value); }

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline UploadPartCopyRequest& WithSSECustomerAlgorithm(const Aws::String& value) { SetSSECustomerAlgorithm(value); return *this;}

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline UploadPartCopyRequest& WithSSECustomerAlgorithm(Aws::String&& value) { SetSSECustomerAlgorithm(std::move(value)); return *this;}

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline UploadPartCopyRequest& WithSSECustomerAlgorithm(const char* value) { SetSSECustomerAlgorithm(value); return *this;}


    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header. This must
     * be the same encryption key specified in the initiate multipart upload
     * request.</p>
     */
    inline const Aws::String& GetSSECustomerKey() const{ return m_sSECustomerKey; }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header. This must
     * be the same encryption key specified in the initiate multipart upload
     * request.</p>
     */
    inline bool SSECustomerKeyHasBeenSet() const { return m_sSECustomerKeyHasBeenSet; }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header. This must
     * be the same encryption key specified in the initiate multipart upload
     * request.</p>
     */
    inline void SetSSECustomerKey(const Aws::String& value) { m_sSECustomerKeyHasBeenSet = true; m_sSECustomerKey = value; }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header. This must
     * be the same encryption key specified in the initiate multipart upload
     * request.</p>
     */
    inline void SetSSECustomerKey(Aws::String&& value) { m_sSECustomerKeyHasBeenSet = true; m_sSECustomerKey = std::move(value); }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header. This must
     * be the same encryption key specified in the initiate multipart upload
     * request.</p>
     */
    inline void SetSSECustomerKey(const char* value) { m_sSECustomerKeyHasBeenSet = true; m_sSECustomerKey.assign(value); }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header. This must
     * be the same encryption key specified in the initiate multipart upload
     * request.</p>
     */
    inline UploadPartCopyRequest& WithSSECustomerKey(const Aws::String& value) { SetSSECustomerKey(value); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header. This must
     * be the same encryption key specified in the initiate multipart upload
     * request.</p>
     */
    inline UploadPartCopyRequest& WithSSECustomerKey(Aws::String&& value) { SetSSECustomerKey(std::move(value)); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header. This must
     * be the same encryption key specified in the initiate multipart upload
     * request.</p>
     */
    inline UploadPartCopyRequest& WithSSECustomerKey(const char* value) { SetSSECustomerKey(value); return *this;}


    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline const Aws::String& GetSSECustomerKeyMD5() const{ return m_sSECustomerKeyMD5; }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline bool SSECustomerKeyMD5HasBeenSet() const { return m_sSECustomerKeyMD5HasBeenSet; }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline void SetSSECustomerKeyMD5(const Aws::String& value) { m_sSECustomerKeyMD5HasBeenSet = true; m_sSECustomerKeyMD5 = value; }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline void SetSSECustomerKeyMD5(Aws::String&& value) { m_sSECustomerKeyMD5HasBeenSet = true; m_sSECustomerKeyMD5 = std::move(value); }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline void SetSSECustomerKeyMD5(const char* value) { m_sSECustomerKeyMD5HasBeenSet = true; m_sSECustomerKeyMD5.assign(value); }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline UploadPartCopyRequest& WithSSECustomerKeyMD5(const Aws::String& value) { SetSSECustomerKeyMD5(value); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline UploadPartCopyRequest& WithSSECustomerKeyMD5(Aws::String&& value) { SetSSECustomerKeyMD5(std::move(value)); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline UploadPartCopyRequest& WithSSECustomerKeyMD5(const char* value) { SetSSECustomerKeyMD5(value); return *this;}


    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline const Aws::String& GetCopySourceSSECustomerAlgorithm() const{ return m_copySourceSSECustomerAlgorithm; }

    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline bool CopySourceSSECustomerAlgorithmHasBeenSet() const { return m_copySourceSSECustomerAlgorithmHasBeenSet; }

    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline void SetCopySourceSSECustomerAlgorithm(const Aws::String& value) { m_copySourceSSECustomerAlgorithmHasBeenSet = true; m_copySourceSSECustomerAlgorithm = value; }

    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline void SetCopySourceSSECustomerAlgorithm(Aws::String&& value) { m_copySourceSSECustomerAlgorithmHasBeenSet = true; m_copySourceSSECustomerAlgorithm = std::move(value); }

    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline void SetCopySourceSSECustomerAlgorithm(const char* value) { m_copySourceSSECustomerAlgorithmHasBeenSet = true; m_copySourceSSECustomerAlgorithm.assign(value); }

    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline UploadPartCopyRequest& WithCopySourceSSECustomerAlgorithm(const Aws::String& value) { SetCopySourceSSECustomerAlgorithm(value); return *this;}

    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline UploadPartCopyRequest& WithCopySourceSSECustomerAlgorithm(Aws::String&& value) { SetCopySourceSSECustomerAlgorithm(std::move(value)); return *this;}

    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline UploadPartCopyRequest& WithCopySourceSSECustomerAlgorithm(const char* value) { SetCopySourceSSECustomerAlgorithm(value); return *this;}


    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline const Aws::String& GetCopySourceSSECustomerKey() const{ return m_copySourceSSECustomerKey; }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline bool CopySourceSSECustomerKeyHasBeenSet() const { return m_copySourceSSECustomerKeyHasBeenSet; }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline void SetCopySourceSSECustomerKey(const Aws::String& value) { m_copySourceSSECustomerKeyHasBeenSet = true; m_copySourceSSECustomerKey = value; }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline void SetCopySourceSSECustomerKey(Aws::String&& value) { m_copySourceSSECustomerKeyHasBeenSet = true; m_copySourceSSECustomerKey = std::move(value); }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline void SetCopySourceSSECustomerKey(const char* value) { m_copySourceSSECustomerKeyHasBeenSet = true; m_copySourceSSECustomerKey.assign(value); }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceSSECustomerKey(const Aws::String& value) { SetCopySourceSSECustomerKey(value); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceSSECustomerKey(Aws::String&& value) { SetCopySourceSSECustomerKey(std::move(value)); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceSSECustomerKey(const char* value) { SetCopySourceSSECustomerKey(value); return *this;}


    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline const Aws::String& GetCopySourceSSECustomerKeyMD5() const{ return m_copySourceSSECustomerKeyMD5; }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline bool CopySourceSSECustomerKeyMD5HasBeenSet() const { return m_copySourceSSECustomerKeyMD5HasBeenSet; }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline void SetCopySourceSSECustomerKeyMD5(const Aws::String& value) { m_copySourceSSECustomerKeyMD5HasBeenSet = true; m_copySourceSSECustomerKeyMD5 = value; }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline void SetCopySourceSSECustomerKeyMD5(Aws::String&& value) { m_copySourceSSECustomerKeyMD5HasBeenSet = true; m_copySourceSSECustomerKeyMD5 = std::move(value); }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline void SetCopySourceSSECustomerKeyMD5(const char* value) { m_copySourceSSECustomerKeyMD5HasBeenSet = true; m_copySourceSSECustomerKeyMD5.assign(value); }

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceSSECustomerKeyMD5(const Aws::String& value) { SetCopySourceSSECustomerKeyMD5(value); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceSSECustomerKeyMD5(Aws::String&& value) { SetCopySourceSSECustomerKeyMD5(std::move(value)); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline UploadPartCopyRequest& WithCopySourceSSECustomerKeyMD5(const char* value) { SetCopySourceSSECustomerKeyMD5(value); return *this;}


    
    inline const RequestPayer& GetRequestPayer() const{ return m_requestPayer; }

    
    inline bool RequestPayerHasBeenSet() const { return m_requestPayerHasBeenSet; }

    
    inline void SetRequestPayer(const RequestPayer& value) { m_requestPayerHasBeenSet = true; m_requestPayer = value; }

    
    inline void SetRequestPayer(RequestPayer&& value) { m_requestPayerHasBeenSet = true; m_requestPayer = std::move(value); }

    
    inline UploadPartCopyRequest& WithRequestPayer(const RequestPayer& value) { SetRequestPayer(value); return *this;}

    
    inline UploadPartCopyRequest& WithRequestPayer(RequestPayer&& value) { SetRequestPayer(std::move(value)); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline UploadPartCopyRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline UploadPartCopyRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline UploadPartCopyRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline UploadPartCopyRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline UploadPartCopyRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline UploadPartCopyRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline UploadPartCopyRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline UploadPartCopyRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline UploadPartCopyRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_copySource;
    bool m_copySourceHasBeenSet;

    Aws::String m_copySourceIfMatch;
    bool m_copySourceIfMatchHasBeenSet;

    Aws::Utils::DateTime m_copySourceIfModifiedSince;
    bool m_copySourceIfModifiedSinceHasBeenSet;

    Aws::String m_copySourceIfNoneMatch;
    bool m_copySourceIfNoneMatchHasBeenSet;

    Aws::Utils::DateTime m_copySourceIfUnmodifiedSince;
    bool m_copySourceIfUnmodifiedSinceHasBeenSet;

    Aws::String m_copySourceRange;
    bool m_copySourceRangeHasBeenSet;

    Aws::String m_key;
    bool m_keyHasBeenSet;

    int m_partNumber;
    bool m_partNumberHasBeenSet;

    Aws::String m_uploadId;
    bool m_uploadIdHasBeenSet;

    Aws::String m_sSECustomerAlgorithm;
    bool m_sSECustomerAlgorithmHasBeenSet;

    Aws::String m_sSECustomerKey;
    bool m_sSECustomerKeyHasBeenSet;

    Aws::String m_sSECustomerKeyMD5;
    bool m_sSECustomerKeyMD5HasBeenSet;

    Aws::String m_copySourceSSECustomerAlgorithm;
    bool m_copySourceSSECustomerAlgorithmHasBeenSet;

    Aws::String m_copySourceSSECustomerKey;
    bool m_copySourceSSECustomerKeyHasBeenSet;

    Aws::String m_copySourceSSECustomerKeyMD5;
    bool m_copySourceSSECustomerKeyMD5HasBeenSet;

    RequestPayer m_requestPayer;
    bool m_requestPayerHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
