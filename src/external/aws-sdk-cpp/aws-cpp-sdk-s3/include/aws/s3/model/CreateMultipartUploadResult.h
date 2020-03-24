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
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/ServerSideEncryption.h>
#include <aws/s3/model/RequestCharged.h>
#include <utility>

namespace Aws
{
template<typename RESULT_TYPE>
class AmazonWebServiceResult;

namespace Utils
{
namespace Xml
{
  class XmlDocument;
} // namespace Xml
} // namespace Utils
namespace S3
{
namespace Model
{
  class AWS_S3_API CreateMultipartUploadResult
  {
  public:
    CreateMultipartUploadResult();
    CreateMultipartUploadResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    CreateMultipartUploadResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>If the bucket has a lifecycle rule configured with an action to abort
     * incomplete multipart uploads and the prefix in the lifecycle rule matches the
     * object name in the request, the response includes this header. The header
     * indicates when the initiated multipart upload becomes eligible for an abort
     * operation. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/mpuoverview.html#mpu-abort-incomplete-mpu-lifecycle-config">
     * Aborting Incomplete Multipart Uploads Using a Bucket Lifecycle Policy</a>.</p>
     * <p>The response also includes the <code>x-amz-abort-rule-id</code> header that
     * provides the ID of the lifecycle configuration rule that defines this
     * action.</p>
     */
    inline const Aws::Utils::DateTime& GetAbortDate() const{ return m_abortDate; }

    /**
     * <p>If the bucket has a lifecycle rule configured with an action to abort
     * incomplete multipart uploads and the prefix in the lifecycle rule matches the
     * object name in the request, the response includes this header. The header
     * indicates when the initiated multipart upload becomes eligible for an abort
     * operation. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/mpuoverview.html#mpu-abort-incomplete-mpu-lifecycle-config">
     * Aborting Incomplete Multipart Uploads Using a Bucket Lifecycle Policy</a>.</p>
     * <p>The response also includes the <code>x-amz-abort-rule-id</code> header that
     * provides the ID of the lifecycle configuration rule that defines this
     * action.</p>
     */
    inline void SetAbortDate(const Aws::Utils::DateTime& value) { m_abortDate = value; }

    /**
     * <p>If the bucket has a lifecycle rule configured with an action to abort
     * incomplete multipart uploads and the prefix in the lifecycle rule matches the
     * object name in the request, the response includes this header. The header
     * indicates when the initiated multipart upload becomes eligible for an abort
     * operation. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/mpuoverview.html#mpu-abort-incomplete-mpu-lifecycle-config">
     * Aborting Incomplete Multipart Uploads Using a Bucket Lifecycle Policy</a>.</p>
     * <p>The response also includes the <code>x-amz-abort-rule-id</code> header that
     * provides the ID of the lifecycle configuration rule that defines this
     * action.</p>
     */
    inline void SetAbortDate(Aws::Utils::DateTime&& value) { m_abortDate = std::move(value); }

    /**
     * <p>If the bucket has a lifecycle rule configured with an action to abort
     * incomplete multipart uploads and the prefix in the lifecycle rule matches the
     * object name in the request, the response includes this header. The header
     * indicates when the initiated multipart upload becomes eligible for an abort
     * operation. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/mpuoverview.html#mpu-abort-incomplete-mpu-lifecycle-config">
     * Aborting Incomplete Multipart Uploads Using a Bucket Lifecycle Policy</a>.</p>
     * <p>The response also includes the <code>x-amz-abort-rule-id</code> header that
     * provides the ID of the lifecycle configuration rule that defines this
     * action.</p>
     */
    inline CreateMultipartUploadResult& WithAbortDate(const Aws::Utils::DateTime& value) { SetAbortDate(value); return *this;}

    /**
     * <p>If the bucket has a lifecycle rule configured with an action to abort
     * incomplete multipart uploads and the prefix in the lifecycle rule matches the
     * object name in the request, the response includes this header. The header
     * indicates when the initiated multipart upload becomes eligible for an abort
     * operation. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/mpuoverview.html#mpu-abort-incomplete-mpu-lifecycle-config">
     * Aborting Incomplete Multipart Uploads Using a Bucket Lifecycle Policy</a>.</p>
     * <p>The response also includes the <code>x-amz-abort-rule-id</code> header that
     * provides the ID of the lifecycle configuration rule that defines this
     * action.</p>
     */
    inline CreateMultipartUploadResult& WithAbortDate(Aws::Utils::DateTime&& value) { SetAbortDate(std::move(value)); return *this;}


    /**
     * <p>This header is returned along with the <code>x-amz-abort-date</code> header.
     * It identifies the applicable lifecycle configuration rule that defines the
     * action to abort incomplete multipart uploads.</p>
     */
    inline const Aws::String& GetAbortRuleId() const{ return m_abortRuleId; }

    /**
     * <p>This header is returned along with the <code>x-amz-abort-date</code> header.
     * It identifies the applicable lifecycle configuration rule that defines the
     * action to abort incomplete multipart uploads.</p>
     */
    inline void SetAbortRuleId(const Aws::String& value) { m_abortRuleId = value; }

    /**
     * <p>This header is returned along with the <code>x-amz-abort-date</code> header.
     * It identifies the applicable lifecycle configuration rule that defines the
     * action to abort incomplete multipart uploads.</p>
     */
    inline void SetAbortRuleId(Aws::String&& value) { m_abortRuleId = std::move(value); }

    /**
     * <p>This header is returned along with the <code>x-amz-abort-date</code> header.
     * It identifies the applicable lifecycle configuration rule that defines the
     * action to abort incomplete multipart uploads.</p>
     */
    inline void SetAbortRuleId(const char* value) { m_abortRuleId.assign(value); }

    /**
     * <p>This header is returned along with the <code>x-amz-abort-date</code> header.
     * It identifies the applicable lifecycle configuration rule that defines the
     * action to abort incomplete multipart uploads.</p>
     */
    inline CreateMultipartUploadResult& WithAbortRuleId(const Aws::String& value) { SetAbortRuleId(value); return *this;}

    /**
     * <p>This header is returned along with the <code>x-amz-abort-date</code> header.
     * It identifies the applicable lifecycle configuration rule that defines the
     * action to abort incomplete multipart uploads.</p>
     */
    inline CreateMultipartUploadResult& WithAbortRuleId(Aws::String&& value) { SetAbortRuleId(std::move(value)); return *this;}

    /**
     * <p>This header is returned along with the <code>x-amz-abort-date</code> header.
     * It identifies the applicable lifecycle configuration rule that defines the
     * action to abort incomplete multipart uploads.</p>
     */
    inline CreateMultipartUploadResult& WithAbortRuleId(const char* value) { SetAbortRuleId(value); return *this;}


    /**
     * <p>Name of the bucket to which the multipart upload was initiated. </p> <p>When
     * using this API with an access point, you must direct requests to the access
     * point hostname. The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>Name of the bucket to which the multipart upload was initiated. </p> <p>When
     * using this API with an access point, you must direct requests to the access
     * point hostname. The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucket = value; }

    /**
     * <p>Name of the bucket to which the multipart upload was initiated. </p> <p>When
     * using this API with an access point, you must direct requests to the access
     * point hostname. The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucket = std::move(value); }

    /**
     * <p>Name of the bucket to which the multipart upload was initiated. </p> <p>When
     * using this API with an access point, you must direct requests to the access
     * point hostname. The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetBucket(const char* value) { m_bucket.assign(value); }

    /**
     * <p>Name of the bucket to which the multipart upload was initiated. </p> <p>When
     * using this API with an access point, you must direct requests to the access
     * point hostname. The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline CreateMultipartUploadResult& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>Name of the bucket to which the multipart upload was initiated. </p> <p>When
     * using this API with an access point, you must direct requests to the access
     * point hostname. The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline CreateMultipartUploadResult& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>Name of the bucket to which the multipart upload was initiated. </p> <p>When
     * using this API with an access point, you must direct requests to the access
     * point hostname. The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline CreateMultipartUploadResult& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline const Aws::String& GetKey() const{ return m_key; }

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline void SetKey(const Aws::String& value) { m_key = value; }

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline void SetKey(Aws::String&& value) { m_key = std::move(value); }

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline void SetKey(const char* value) { m_key.assign(value); }

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline CreateMultipartUploadResult& WithKey(const Aws::String& value) { SetKey(value); return *this;}

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline CreateMultipartUploadResult& WithKey(Aws::String&& value) { SetKey(std::move(value)); return *this;}

    /**
     * <p>Object key for which the multipart upload was initiated.</p>
     */
    inline CreateMultipartUploadResult& WithKey(const char* value) { SetKey(value); return *this;}


    /**
     * <p>ID for the initiated multipart upload.</p>
     */
    inline const Aws::String& GetUploadId() const{ return m_uploadId; }

    /**
     * <p>ID for the initiated multipart upload.</p>
     */
    inline void SetUploadId(const Aws::String& value) { m_uploadId = value; }

    /**
     * <p>ID for the initiated multipart upload.</p>
     */
    inline void SetUploadId(Aws::String&& value) { m_uploadId = std::move(value); }

    /**
     * <p>ID for the initiated multipart upload.</p>
     */
    inline void SetUploadId(const char* value) { m_uploadId.assign(value); }

    /**
     * <p>ID for the initiated multipart upload.</p>
     */
    inline CreateMultipartUploadResult& WithUploadId(const Aws::String& value) { SetUploadId(value); return *this;}

    /**
     * <p>ID for the initiated multipart upload.</p>
     */
    inline CreateMultipartUploadResult& WithUploadId(Aws::String&& value) { SetUploadId(std::move(value)); return *this;}

    /**
     * <p>ID for the initiated multipart upload.</p>
     */
    inline CreateMultipartUploadResult& WithUploadId(const char* value) { SetUploadId(value); return *this;}


    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline const ServerSideEncryption& GetServerSideEncryption() const{ return m_serverSideEncryption; }

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline void SetServerSideEncryption(const ServerSideEncryption& value) { m_serverSideEncryption = value; }

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline void SetServerSideEncryption(ServerSideEncryption&& value) { m_serverSideEncryption = std::move(value); }

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline CreateMultipartUploadResult& WithServerSideEncryption(const ServerSideEncryption& value) { SetServerSideEncryption(value); return *this;}

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline CreateMultipartUploadResult& WithServerSideEncryption(ServerSideEncryption&& value) { SetServerSideEncryption(std::move(value)); return *this;}


    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header confirming the encryption
     * algorithm used.</p>
     */
    inline const Aws::String& GetSSECustomerAlgorithm() const{ return m_sSECustomerAlgorithm; }

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header confirming the encryption
     * algorithm used.</p>
     */
    inline void SetSSECustomerAlgorithm(const Aws::String& value) { m_sSECustomerAlgorithm = value; }

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header confirming the encryption
     * algorithm used.</p>
     */
    inline void SetSSECustomerAlgorithm(Aws::String&& value) { m_sSECustomerAlgorithm = std::move(value); }

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header confirming the encryption
     * algorithm used.</p>
     */
    inline void SetSSECustomerAlgorithm(const char* value) { m_sSECustomerAlgorithm.assign(value); }

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header confirming the encryption
     * algorithm used.</p>
     */
    inline CreateMultipartUploadResult& WithSSECustomerAlgorithm(const Aws::String& value) { SetSSECustomerAlgorithm(value); return *this;}

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header confirming the encryption
     * algorithm used.</p>
     */
    inline CreateMultipartUploadResult& WithSSECustomerAlgorithm(Aws::String&& value) { SetSSECustomerAlgorithm(std::move(value)); return *this;}

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header confirming the encryption
     * algorithm used.</p>
     */
    inline CreateMultipartUploadResult& WithSSECustomerAlgorithm(const char* value) { SetSSECustomerAlgorithm(value); return *this;}


    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header to provide round-trip message
     * integrity verification of the customer-provided encryption key.</p>
     */
    inline const Aws::String& GetSSECustomerKeyMD5() const{ return m_sSECustomerKeyMD5; }

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header to provide round-trip message
     * integrity verification of the customer-provided encryption key.</p>
     */
    inline void SetSSECustomerKeyMD5(const Aws::String& value) { m_sSECustomerKeyMD5 = value; }

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header to provide round-trip message
     * integrity verification of the customer-provided encryption key.</p>
     */
    inline void SetSSECustomerKeyMD5(Aws::String&& value) { m_sSECustomerKeyMD5 = std::move(value); }

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header to provide round-trip message
     * integrity verification of the customer-provided encryption key.</p>
     */
    inline void SetSSECustomerKeyMD5(const char* value) { m_sSECustomerKeyMD5.assign(value); }

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header to provide round-trip message
     * integrity verification of the customer-provided encryption key.</p>
     */
    inline CreateMultipartUploadResult& WithSSECustomerKeyMD5(const Aws::String& value) { SetSSECustomerKeyMD5(value); return *this;}

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header to provide round-trip message
     * integrity verification of the customer-provided encryption key.</p>
     */
    inline CreateMultipartUploadResult& WithSSECustomerKeyMD5(Aws::String&& value) { SetSSECustomerKeyMD5(std::move(value)); return *this;}

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header to provide round-trip message
     * integrity verification of the customer-provided encryption key.</p>
     */
    inline CreateMultipartUploadResult& WithSSECustomerKeyMD5(const char* value) { SetSSECustomerKeyMD5(value); return *this;}


    /**
     * <p>If present, specifies the ID of the AWS Key Management Service (AWS KMS)
     * symmetric customer managed customer master key (CMK) that was used for the
     * object.</p>
     */
    inline const Aws::String& GetSSEKMSKeyId() const{ return m_sSEKMSKeyId; }

    /**
     * <p>If present, specifies the ID of the AWS Key Management Service (AWS KMS)
     * symmetric customer managed customer master key (CMK) that was used for the
     * object.</p>
     */
    inline void SetSSEKMSKeyId(const Aws::String& value) { m_sSEKMSKeyId = value; }

    /**
     * <p>If present, specifies the ID of the AWS Key Management Service (AWS KMS)
     * symmetric customer managed customer master key (CMK) that was used for the
     * object.</p>
     */
    inline void SetSSEKMSKeyId(Aws::String&& value) { m_sSEKMSKeyId = std::move(value); }

    /**
     * <p>If present, specifies the ID of the AWS Key Management Service (AWS KMS)
     * symmetric customer managed customer master key (CMK) that was used for the
     * object.</p>
     */
    inline void SetSSEKMSKeyId(const char* value) { m_sSEKMSKeyId.assign(value); }

    /**
     * <p>If present, specifies the ID of the AWS Key Management Service (AWS KMS)
     * symmetric customer managed customer master key (CMK) that was used for the
     * object.</p>
     */
    inline CreateMultipartUploadResult& WithSSEKMSKeyId(const Aws::String& value) { SetSSEKMSKeyId(value); return *this;}

    /**
     * <p>If present, specifies the ID of the AWS Key Management Service (AWS KMS)
     * symmetric customer managed customer master key (CMK) that was used for the
     * object.</p>
     */
    inline CreateMultipartUploadResult& WithSSEKMSKeyId(Aws::String&& value) { SetSSEKMSKeyId(std::move(value)); return *this;}

    /**
     * <p>If present, specifies the ID of the AWS Key Management Service (AWS KMS)
     * symmetric customer managed customer master key (CMK) that was used for the
     * object.</p>
     */
    inline CreateMultipartUploadResult& WithSSEKMSKeyId(const char* value) { SetSSEKMSKeyId(value); return *this;}


    /**
     * <p>If present, specifies the AWS KMS Encryption Context to use for object
     * encryption. The value of this header is a base64-encoded UTF-8 string holding
     * JSON with the encryption context key-value pairs.</p>
     */
    inline const Aws::String& GetSSEKMSEncryptionContext() const{ return m_sSEKMSEncryptionContext; }

    /**
     * <p>If present, specifies the AWS KMS Encryption Context to use for object
     * encryption. The value of this header is a base64-encoded UTF-8 string holding
     * JSON with the encryption context key-value pairs.</p>
     */
    inline void SetSSEKMSEncryptionContext(const Aws::String& value) { m_sSEKMSEncryptionContext = value; }

    /**
     * <p>If present, specifies the AWS KMS Encryption Context to use for object
     * encryption. The value of this header is a base64-encoded UTF-8 string holding
     * JSON with the encryption context key-value pairs.</p>
     */
    inline void SetSSEKMSEncryptionContext(Aws::String&& value) { m_sSEKMSEncryptionContext = std::move(value); }

    /**
     * <p>If present, specifies the AWS KMS Encryption Context to use for object
     * encryption. The value of this header is a base64-encoded UTF-8 string holding
     * JSON with the encryption context key-value pairs.</p>
     */
    inline void SetSSEKMSEncryptionContext(const char* value) { m_sSEKMSEncryptionContext.assign(value); }

    /**
     * <p>If present, specifies the AWS KMS Encryption Context to use for object
     * encryption. The value of this header is a base64-encoded UTF-8 string holding
     * JSON with the encryption context key-value pairs.</p>
     */
    inline CreateMultipartUploadResult& WithSSEKMSEncryptionContext(const Aws::String& value) { SetSSEKMSEncryptionContext(value); return *this;}

    /**
     * <p>If present, specifies the AWS KMS Encryption Context to use for object
     * encryption. The value of this header is a base64-encoded UTF-8 string holding
     * JSON with the encryption context key-value pairs.</p>
     */
    inline CreateMultipartUploadResult& WithSSEKMSEncryptionContext(Aws::String&& value) { SetSSEKMSEncryptionContext(std::move(value)); return *this;}

    /**
     * <p>If present, specifies the AWS KMS Encryption Context to use for object
     * encryption. The value of this header is a base64-encoded UTF-8 string holding
     * JSON with the encryption context key-value pairs.</p>
     */
    inline CreateMultipartUploadResult& WithSSEKMSEncryptionContext(const char* value) { SetSSEKMSEncryptionContext(value); return *this;}


    
    inline const RequestCharged& GetRequestCharged() const{ return m_requestCharged; }

    
    inline void SetRequestCharged(const RequestCharged& value) { m_requestCharged = value; }

    
    inline void SetRequestCharged(RequestCharged&& value) { m_requestCharged = std::move(value); }

    
    inline CreateMultipartUploadResult& WithRequestCharged(const RequestCharged& value) { SetRequestCharged(value); return *this;}

    
    inline CreateMultipartUploadResult& WithRequestCharged(RequestCharged&& value) { SetRequestCharged(std::move(value)); return *this;}

  private:

    Aws::Utils::DateTime m_abortDate;

    Aws::String m_abortRuleId;

    Aws::String m_bucket;

    Aws::String m_key;

    Aws::String m_uploadId;

    ServerSideEncryption m_serverSideEncryption;

    Aws::String m_sSECustomerAlgorithm;

    Aws::String m_sSECustomerKeyMD5;

    Aws::String m_sSEKMSKeyId;

    Aws::String m_sSEKMSEncryptionContext;

    RequestCharged m_requestCharged;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
