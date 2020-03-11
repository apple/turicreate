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
  class AWS_S3_API DeleteObjectRequest : public S3Request
  {
  public:
    DeleteObjectRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "DeleteObject"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>The bucket name of the bucket containing the object. </p> <p>When using this
     * API with an access point, you must direct requests to the access point hostname.
     * The access point hostname takes the form
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
     * <p>The bucket name of the bucket containing the object. </p> <p>When using this
     * API with an access point, you must direct requests to the access point hostname.
     * The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The bucket name of the bucket containing the object. </p> <p>When using this
     * API with an access point, you must direct requests to the access point hostname.
     * The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The bucket name of the bucket containing the object. </p> <p>When using this
     * API with an access point, you must direct requests to the access point hostname.
     * The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The bucket name of the bucket containing the object. </p> <p>When using this
     * API with an access point, you must direct requests to the access point hostname.
     * The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The bucket name of the bucket containing the object. </p> <p>When using this
     * API with an access point, you must direct requests to the access point hostname.
     * The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline DeleteObjectRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The bucket name of the bucket containing the object. </p> <p>When using this
     * API with an access point, you must direct requests to the access point hostname.
     * The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline DeleteObjectRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The bucket name of the bucket containing the object. </p> <p>When using this
     * API with an access point, you must direct requests to the access point hostname.
     * The access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline DeleteObjectRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>Key name of the object to delete.</p>
     */
    inline const Aws::String& GetKey() const{ return m_key; }

    /**
     * <p>Key name of the object to delete.</p>
     */
    inline bool KeyHasBeenSet() const { return m_keyHasBeenSet; }

    /**
     * <p>Key name of the object to delete.</p>
     */
    inline void SetKey(const Aws::String& value) { m_keyHasBeenSet = true; m_key = value; }

    /**
     * <p>Key name of the object to delete.</p>
     */
    inline void SetKey(Aws::String&& value) { m_keyHasBeenSet = true; m_key = std::move(value); }

    /**
     * <p>Key name of the object to delete.</p>
     */
    inline void SetKey(const char* value) { m_keyHasBeenSet = true; m_key.assign(value); }

    /**
     * <p>Key name of the object to delete.</p>
     */
    inline DeleteObjectRequest& WithKey(const Aws::String& value) { SetKey(value); return *this;}

    /**
     * <p>Key name of the object to delete.</p>
     */
    inline DeleteObjectRequest& WithKey(Aws::String&& value) { SetKey(std::move(value)); return *this;}

    /**
     * <p>Key name of the object to delete.</p>
     */
    inline DeleteObjectRequest& WithKey(const char* value) { SetKey(value); return *this;}


    /**
     * <p>The concatenation of the authentication device's serial number, a space, and
     * the value that is displayed on your authentication device. Required to
     * permanently delete a versioned object if versioning is configured with MFA
     * delete enabled.</p>
     */
    inline const Aws::String& GetMFA() const{ return m_mFA; }

    /**
     * <p>The concatenation of the authentication device's serial number, a space, and
     * the value that is displayed on your authentication device. Required to
     * permanently delete a versioned object if versioning is configured with MFA
     * delete enabled.</p>
     */
    inline bool MFAHasBeenSet() const { return m_mFAHasBeenSet; }

    /**
     * <p>The concatenation of the authentication device's serial number, a space, and
     * the value that is displayed on your authentication device. Required to
     * permanently delete a versioned object if versioning is configured with MFA
     * delete enabled.</p>
     */
    inline void SetMFA(const Aws::String& value) { m_mFAHasBeenSet = true; m_mFA = value; }

    /**
     * <p>The concatenation of the authentication device's serial number, a space, and
     * the value that is displayed on your authentication device. Required to
     * permanently delete a versioned object if versioning is configured with MFA
     * delete enabled.</p>
     */
    inline void SetMFA(Aws::String&& value) { m_mFAHasBeenSet = true; m_mFA = std::move(value); }

    /**
     * <p>The concatenation of the authentication device's serial number, a space, and
     * the value that is displayed on your authentication device. Required to
     * permanently delete a versioned object if versioning is configured with MFA
     * delete enabled.</p>
     */
    inline void SetMFA(const char* value) { m_mFAHasBeenSet = true; m_mFA.assign(value); }

    /**
     * <p>The concatenation of the authentication device's serial number, a space, and
     * the value that is displayed on your authentication device. Required to
     * permanently delete a versioned object if versioning is configured with MFA
     * delete enabled.</p>
     */
    inline DeleteObjectRequest& WithMFA(const Aws::String& value) { SetMFA(value); return *this;}

    /**
     * <p>The concatenation of the authentication device's serial number, a space, and
     * the value that is displayed on your authentication device. Required to
     * permanently delete a versioned object if versioning is configured with MFA
     * delete enabled.</p>
     */
    inline DeleteObjectRequest& WithMFA(Aws::String&& value) { SetMFA(std::move(value)); return *this;}

    /**
     * <p>The concatenation of the authentication device's serial number, a space, and
     * the value that is displayed on your authentication device. Required to
     * permanently delete a versioned object if versioning is configured with MFA
     * delete enabled.</p>
     */
    inline DeleteObjectRequest& WithMFA(const char* value) { SetMFA(value); return *this;}


    /**
     * <p>VersionId used to reference a specific version of the object.</p>
     */
    inline const Aws::String& GetVersionId() const{ return m_versionId; }

    /**
     * <p>VersionId used to reference a specific version of the object.</p>
     */
    inline bool VersionIdHasBeenSet() const { return m_versionIdHasBeenSet; }

    /**
     * <p>VersionId used to reference a specific version of the object.</p>
     */
    inline void SetVersionId(const Aws::String& value) { m_versionIdHasBeenSet = true; m_versionId = value; }

    /**
     * <p>VersionId used to reference a specific version of the object.</p>
     */
    inline void SetVersionId(Aws::String&& value) { m_versionIdHasBeenSet = true; m_versionId = std::move(value); }

    /**
     * <p>VersionId used to reference a specific version of the object.</p>
     */
    inline void SetVersionId(const char* value) { m_versionIdHasBeenSet = true; m_versionId.assign(value); }

    /**
     * <p>VersionId used to reference a specific version of the object.</p>
     */
    inline DeleteObjectRequest& WithVersionId(const Aws::String& value) { SetVersionId(value); return *this;}

    /**
     * <p>VersionId used to reference a specific version of the object.</p>
     */
    inline DeleteObjectRequest& WithVersionId(Aws::String&& value) { SetVersionId(std::move(value)); return *this;}

    /**
     * <p>VersionId used to reference a specific version of the object.</p>
     */
    inline DeleteObjectRequest& WithVersionId(const char* value) { SetVersionId(value); return *this;}


    
    inline const RequestPayer& GetRequestPayer() const{ return m_requestPayer; }

    
    inline bool RequestPayerHasBeenSet() const { return m_requestPayerHasBeenSet; }

    
    inline void SetRequestPayer(const RequestPayer& value) { m_requestPayerHasBeenSet = true; m_requestPayer = value; }

    
    inline void SetRequestPayer(RequestPayer&& value) { m_requestPayerHasBeenSet = true; m_requestPayer = std::move(value); }

    
    inline DeleteObjectRequest& WithRequestPayer(const RequestPayer& value) { SetRequestPayer(value); return *this;}

    
    inline DeleteObjectRequest& WithRequestPayer(RequestPayer&& value) { SetRequestPayer(std::move(value)); return *this;}


    /**
     * <p>Indicates whether S3 Object Lock should bypass Governance-mode restrictions
     * to process this operation.</p>
     */
    inline bool GetBypassGovernanceRetention() const{ return m_bypassGovernanceRetention; }

    /**
     * <p>Indicates whether S3 Object Lock should bypass Governance-mode restrictions
     * to process this operation.</p>
     */
    inline bool BypassGovernanceRetentionHasBeenSet() const { return m_bypassGovernanceRetentionHasBeenSet; }

    /**
     * <p>Indicates whether S3 Object Lock should bypass Governance-mode restrictions
     * to process this operation.</p>
     */
    inline void SetBypassGovernanceRetention(bool value) { m_bypassGovernanceRetentionHasBeenSet = true; m_bypassGovernanceRetention = value; }

    /**
     * <p>Indicates whether S3 Object Lock should bypass Governance-mode restrictions
     * to process this operation.</p>
     */
    inline DeleteObjectRequest& WithBypassGovernanceRetention(bool value) { SetBypassGovernanceRetention(value); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline DeleteObjectRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline DeleteObjectRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline DeleteObjectRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline DeleteObjectRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline DeleteObjectRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline DeleteObjectRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline DeleteObjectRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline DeleteObjectRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline DeleteObjectRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_key;
    bool m_keyHasBeenSet;

    Aws::String m_mFA;
    bool m_mFAHasBeenSet;

    Aws::String m_versionId;
    bool m_versionIdHasBeenSet;

    RequestPayer m_requestPayer;
    bool m_requestPayerHasBeenSet;

    bool m_bypassGovernanceRetention;
    bool m_bypassGovernanceRetentionHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
