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
#include <aws/s3/model/ObjectCannedACL.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <aws/s3/model/ServerSideEncryption.h>
#include <aws/s3/model/StorageClass.h>
#include <aws/s3/model/RequestPayer.h>
#include <aws/s3/model/ObjectLockMode.h>
#include <aws/s3/model/ObjectLockLegalHoldStatus.h>
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
  class AWS_S3_API CreateMultipartUploadRequest : public S3Request
  {
  public:
    CreateMultipartUploadRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "CreateMultipartUpload"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>The canned ACL to apply to the object.</p>
     */
    inline const ObjectCannedACL& GetACL() const{ return m_aCL; }

    /**
     * <p>The canned ACL to apply to the object.</p>
     */
    inline bool ACLHasBeenSet() const { return m_aCLHasBeenSet; }

    /**
     * <p>The canned ACL to apply to the object.</p>
     */
    inline void SetACL(const ObjectCannedACL& value) { m_aCLHasBeenSet = true; m_aCL = value; }

    /**
     * <p>The canned ACL to apply to the object.</p>
     */
    inline void SetACL(ObjectCannedACL&& value) { m_aCLHasBeenSet = true; m_aCL = std::move(value); }

    /**
     * <p>The canned ACL to apply to the object.</p>
     */
    inline CreateMultipartUploadRequest& WithACL(const ObjectCannedACL& value) { SetACL(value); return *this;}

    /**
     * <p>The canned ACL to apply to the object.</p>
     */
    inline CreateMultipartUploadRequest& WithACL(ObjectCannedACL&& value) { SetACL(std::move(value)); return *this;}


    /**
     * <p>The name of the bucket to which to initiate the upload</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>The name of the bucket to which to initiate the upload</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The name of the bucket to which to initiate the upload</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The name of the bucket to which to initiate the upload</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The name of the bucket to which to initiate the upload</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The name of the bucket to which to initiate the upload</p>
     */
    inline CreateMultipartUploadRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The name of the bucket to which to initiate the upload</p>
     */
    inline CreateMultipartUploadRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The name of the bucket to which to initiate the upload</p>
     */
    inline CreateMultipartUploadRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline const Aws::String& GetCacheControl() const{ return m_cacheControl; }

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline bool CacheControlHasBeenSet() const { return m_cacheControlHasBeenSet; }

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline void SetCacheControl(const Aws::String& value) { m_cacheControlHasBeenSet = true; m_cacheControl = value; }

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline void SetCacheControl(Aws::String&& value) { m_cacheControlHasBeenSet = true; m_cacheControl = std::move(value); }

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline void SetCacheControl(const char* value) { m_cacheControlHasBeenSet = true; m_cacheControl.assign(value); }

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline CreateMultipartUploadRequest& WithCacheControl(const Aws::String& value) { SetCacheControl(value); return *this;}

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline CreateMultipartUploadRequest& WithCacheControl(Aws::String&& value) { SetCacheControl(std::move(value)); return *this;}

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline CreateMultipartUploadRequest& WithCacheControl(const char* value) { SetCacheControl(value); return *this;}


    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline const Aws::String& GetContentDisposition() const{ return m_contentDisposition; }

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline bool ContentDispositionHasBeenSet() const { return m_contentDispositionHasBeenSet; }

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline void SetContentDisposition(const Aws::String& value) { m_contentDispositionHasBeenSet = true; m_contentDisposition = value; }

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline void SetContentDisposition(Aws::String&& value) { m_contentDispositionHasBeenSet = true; m_contentDisposition = std::move(value); }

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline void SetContentDisposition(const char* value) { m_contentDispositionHasBeenSet = true; m_contentDisposition.assign(value); }

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline CreateMultipartUploadRequest& WithContentDisposition(const Aws::String& value) { SetContentDisposition(value); return *this;}

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline CreateMultipartUploadRequest& WithContentDisposition(Aws::String&& value) { SetContentDisposition(std::move(value)); return *this;}

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline CreateMultipartUploadRequest& WithContentDisposition(const char* value) { SetContentDisposition(value); return *this;}


    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline const Aws::String& GetContentEncoding() const{ return m_contentEncoding; }

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline bool ContentEncodingHasBeenSet() const { return m_contentEncodingHasBeenSet; }

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline void SetContentEncoding(const Aws::String& value) { m_contentEncodingHasBeenSet = true; m_contentEncoding = value; }

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline void SetContentEncoding(Aws::String&& value) { m_contentEncodingHasBeenSet = true; m_contentEncoding = std::move(value); }

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline void SetContentEncoding(const char* value) { m_contentEncodingHasBeenSet = true; m_contentEncoding.assign(value); }

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline CreateMultipartUploadRequest& WithContentEncoding(const Aws::String& value) { SetContentEncoding(value); return *this;}

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline CreateMultipartUploadRequest& WithContentEncoding(Aws::String&& value) { SetContentEncoding(std::move(value)); return *this;}

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline CreateMultipartUploadRequest& WithContentEncoding(const char* value) { SetContentEncoding(value); return *this;}


    /**
     * <p>The language the content is in.</p>
     */
    inline const Aws::String& GetContentLanguage() const{ return m_contentLanguage; }

    /**
     * <p>The language the content is in.</p>
     */
    inline bool ContentLanguageHasBeenSet() const { return m_contentLanguageHasBeenSet; }

    /**
     * <p>The language the content is in.</p>
     */
    inline void SetContentLanguage(const Aws::String& value) { m_contentLanguageHasBeenSet = true; m_contentLanguage = value; }

    /**
     * <p>The language the content is in.</p>
     */
    inline void SetContentLanguage(Aws::String&& value) { m_contentLanguageHasBeenSet = true; m_contentLanguage = std::move(value); }

    /**
     * <p>The language the content is in.</p>
     */
    inline void SetContentLanguage(const char* value) { m_contentLanguageHasBeenSet = true; m_contentLanguage.assign(value); }

    /**
     * <p>The language the content is in.</p>
     */
    inline CreateMultipartUploadRequest& WithContentLanguage(const Aws::String& value) { SetContentLanguage(value); return *this;}

    /**
     * <p>The language the content is in.</p>
     */
    inline CreateMultipartUploadRequest& WithContentLanguage(Aws::String&& value) { SetContentLanguage(std::move(value)); return *this;}

    /**
     * <p>The language the content is in.</p>
     */
    inline CreateMultipartUploadRequest& WithContentLanguage(const char* value) { SetContentLanguage(value); return *this;}


    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline const Aws::String& GetContentType() const{ return m_contentType; }

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline bool ContentTypeHasBeenSet() const { return m_contentTypeHasBeenSet; }

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline void SetContentType(const Aws::String& value) { m_contentTypeHasBeenSet = true; m_contentType = value; }

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline void SetContentType(Aws::String&& value) { m_contentTypeHasBeenSet = true; m_contentType = std::move(value); }

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline void SetContentType(const char* value) { m_contentTypeHasBeenSet = true; m_contentType.assign(value); }

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline CreateMultipartUploadRequest& WithContentType(const Aws::String& value) { SetContentType(value); return *this;}

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline CreateMultipartUploadRequest& WithContentType(Aws::String&& value) { SetContentType(std::move(value)); return *this;}

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline CreateMultipartUploadRequest& WithContentType(const char* value) { SetContentType(value); return *this;}


    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline const Aws::Utils::DateTime& GetExpires() const{ return m_expires; }

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline bool ExpiresHasBeenSet() const { return m_expiresHasBeenSet; }

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline void SetExpires(const Aws::Utils::DateTime& value) { m_expiresHasBeenSet = true; m_expires = value; }

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline void SetExpires(Aws::Utils::DateTime&& value) { m_expiresHasBeenSet = true; m_expires = std::move(value); }

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline CreateMultipartUploadRequest& WithExpires(const Aws::Utils::DateTime& value) { SetExpires(value); return *this;}

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline CreateMultipartUploadRequest& WithExpires(Aws::Utils::DateTime&& value) { SetExpires(std::move(value)); return *this;}


    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline const Aws::String& GetGrantFullControl() const{ return m_grantFullControl; }

    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline bool GrantFullControlHasBeenSet() const { return m_grantFullControlHasBeenSet; }

    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline void SetGrantFullControl(const Aws::String& value) { m_grantFullControlHasBeenSet = true; m_grantFullControl = value; }

    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline void SetGrantFullControl(Aws::String&& value) { m_grantFullControlHasBeenSet = true; m_grantFullControl = std::move(value); }

    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline void SetGrantFullControl(const char* value) { m_grantFullControlHasBeenSet = true; m_grantFullControl.assign(value); }

    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantFullControl(const Aws::String& value) { SetGrantFullControl(value); return *this;}

    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantFullControl(Aws::String&& value) { SetGrantFullControl(std::move(value)); return *this;}

    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantFullControl(const char* value) { SetGrantFullControl(value); return *this;}


    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline const Aws::String& GetGrantRead() const{ return m_grantRead; }

    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline bool GrantReadHasBeenSet() const { return m_grantReadHasBeenSet; }

    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline void SetGrantRead(const Aws::String& value) { m_grantReadHasBeenSet = true; m_grantRead = value; }

    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline void SetGrantRead(Aws::String&& value) { m_grantReadHasBeenSet = true; m_grantRead = std::move(value); }

    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline void SetGrantRead(const char* value) { m_grantReadHasBeenSet = true; m_grantRead.assign(value); }

    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantRead(const Aws::String& value) { SetGrantRead(value); return *this;}

    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantRead(Aws::String&& value) { SetGrantRead(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantRead(const char* value) { SetGrantRead(value); return *this;}


    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline const Aws::String& GetGrantReadACP() const{ return m_grantReadACP; }

    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline bool GrantReadACPHasBeenSet() const { return m_grantReadACPHasBeenSet; }

    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline void SetGrantReadACP(const Aws::String& value) { m_grantReadACPHasBeenSet = true; m_grantReadACP = value; }

    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline void SetGrantReadACP(Aws::String&& value) { m_grantReadACPHasBeenSet = true; m_grantReadACP = std::move(value); }

    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline void SetGrantReadACP(const char* value) { m_grantReadACPHasBeenSet = true; m_grantReadACP.assign(value); }

    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantReadACP(const Aws::String& value) { SetGrantReadACP(value); return *this;}

    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantReadACP(Aws::String&& value) { SetGrantReadACP(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantReadACP(const char* value) { SetGrantReadACP(value); return *this;}


    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline const Aws::String& GetGrantWriteACP() const{ return m_grantWriteACP; }

    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline bool GrantWriteACPHasBeenSet() const { return m_grantWriteACPHasBeenSet; }

    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline void SetGrantWriteACP(const Aws::String& value) { m_grantWriteACPHasBeenSet = true; m_grantWriteACP = value; }

    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline void SetGrantWriteACP(Aws::String&& value) { m_grantWriteACPHasBeenSet = true; m_grantWriteACP = std::move(value); }

    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline void SetGrantWriteACP(const char* value) { m_grantWriteACPHasBeenSet = true; m_grantWriteACP.assign(value); }

    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantWriteACP(const Aws::String& value) { SetGrantWriteACP(value); return *this;}

    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantWriteACP(Aws::String&& value) { SetGrantWriteACP(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline CreateMultipartUploadRequest& WithGrantWriteACP(const char* value) { SetGrantWriteACP(value); return *this;}


    /**
     * <p>Object key for which the multipart upload is to be initiated.</p>
     */
    inline const Aws::String& GetKey() const{ return m_key; }

    /**
     * <p>Object key for which the multipart upload is to be initiated.</p>
     */
    inline bool KeyHasBeenSet() const { return m_keyHasBeenSet; }

    /**
     * <p>Object key for which the multipart upload is to be initiated.</p>
     */
    inline void SetKey(const Aws::String& value) { m_keyHasBeenSet = true; m_key = value; }

    /**
     * <p>Object key for which the multipart upload is to be initiated.</p>
     */
    inline void SetKey(Aws::String&& value) { m_keyHasBeenSet = true; m_key = std::move(value); }

    /**
     * <p>Object key for which the multipart upload is to be initiated.</p>
     */
    inline void SetKey(const char* value) { m_keyHasBeenSet = true; m_key.assign(value); }

    /**
     * <p>Object key for which the multipart upload is to be initiated.</p>
     */
    inline CreateMultipartUploadRequest& WithKey(const Aws::String& value) { SetKey(value); return *this;}

    /**
     * <p>Object key for which the multipart upload is to be initiated.</p>
     */
    inline CreateMultipartUploadRequest& WithKey(Aws::String&& value) { SetKey(std::move(value)); return *this;}

    /**
     * <p>Object key for which the multipart upload is to be initiated.</p>
     */
    inline CreateMultipartUploadRequest& WithKey(const char* value) { SetKey(value); return *this;}


    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline const Aws::Map<Aws::String, Aws::String>& GetMetadata() const{ return m_metadata; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline bool MetadataHasBeenSet() const { return m_metadataHasBeenSet; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline void SetMetadata(const Aws::Map<Aws::String, Aws::String>& value) { m_metadataHasBeenSet = true; m_metadata = value; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline void SetMetadata(Aws::Map<Aws::String, Aws::String>&& value) { m_metadataHasBeenSet = true; m_metadata = std::move(value); }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CreateMultipartUploadRequest& WithMetadata(const Aws::Map<Aws::String, Aws::String>& value) { SetMetadata(value); return *this;}

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CreateMultipartUploadRequest& WithMetadata(Aws::Map<Aws::String, Aws::String>&& value) { SetMetadata(std::move(value)); return *this;}

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CreateMultipartUploadRequest& AddMetadata(const Aws::String& key, const Aws::String& value) { m_metadataHasBeenSet = true; m_metadata.emplace(key, value); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CreateMultipartUploadRequest& AddMetadata(Aws::String&& key, const Aws::String& value) { m_metadataHasBeenSet = true; m_metadata.emplace(std::move(key), value); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CreateMultipartUploadRequest& AddMetadata(const Aws::String& key, Aws::String&& value) { m_metadataHasBeenSet = true; m_metadata.emplace(key, std::move(value)); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CreateMultipartUploadRequest& AddMetadata(Aws::String&& key, Aws::String&& value) { m_metadataHasBeenSet = true; m_metadata.emplace(std::move(key), std::move(value)); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CreateMultipartUploadRequest& AddMetadata(const char* key, Aws::String&& value) { m_metadataHasBeenSet = true; m_metadata.emplace(key, std::move(value)); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CreateMultipartUploadRequest& AddMetadata(Aws::String&& key, const char* value) { m_metadataHasBeenSet = true; m_metadata.emplace(std::move(key), value); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CreateMultipartUploadRequest& AddMetadata(const char* key, const char* value) { m_metadataHasBeenSet = true; m_metadata.emplace(key, value); return *this; }


    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline const ServerSideEncryption& GetServerSideEncryption() const{ return m_serverSideEncryption; }

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline bool ServerSideEncryptionHasBeenSet() const { return m_serverSideEncryptionHasBeenSet; }

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline void SetServerSideEncryption(const ServerSideEncryption& value) { m_serverSideEncryptionHasBeenSet = true; m_serverSideEncryption = value; }

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline void SetServerSideEncryption(ServerSideEncryption&& value) { m_serverSideEncryptionHasBeenSet = true; m_serverSideEncryption = std::move(value); }

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline CreateMultipartUploadRequest& WithServerSideEncryption(const ServerSideEncryption& value) { SetServerSideEncryption(value); return *this;}

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline CreateMultipartUploadRequest& WithServerSideEncryption(ServerSideEncryption&& value) { SetServerSideEncryption(std::move(value)); return *this;}


    /**
     * <p>The type of storage to use for the object. Defaults to 'STANDARD'.</p>
     */
    inline const StorageClass& GetStorageClass() const{ return m_storageClass; }

    /**
     * <p>The type of storage to use for the object. Defaults to 'STANDARD'.</p>
     */
    inline bool StorageClassHasBeenSet() const { return m_storageClassHasBeenSet; }

    /**
     * <p>The type of storage to use for the object. Defaults to 'STANDARD'.</p>
     */
    inline void SetStorageClass(const StorageClass& value) { m_storageClassHasBeenSet = true; m_storageClass = value; }

    /**
     * <p>The type of storage to use for the object. Defaults to 'STANDARD'.</p>
     */
    inline void SetStorageClass(StorageClass&& value) { m_storageClassHasBeenSet = true; m_storageClass = std::move(value); }

    /**
     * <p>The type of storage to use for the object. Defaults to 'STANDARD'.</p>
     */
    inline CreateMultipartUploadRequest& WithStorageClass(const StorageClass& value) { SetStorageClass(value); return *this;}

    /**
     * <p>The type of storage to use for the object. Defaults to 'STANDARD'.</p>
     */
    inline CreateMultipartUploadRequest& WithStorageClass(StorageClass&& value) { SetStorageClass(std::move(value)); return *this;}


    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline const Aws::String& GetWebsiteRedirectLocation() const{ return m_websiteRedirectLocation; }

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline bool WebsiteRedirectLocationHasBeenSet() const { return m_websiteRedirectLocationHasBeenSet; }

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline void SetWebsiteRedirectLocation(const Aws::String& value) { m_websiteRedirectLocationHasBeenSet = true; m_websiteRedirectLocation = value; }

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline void SetWebsiteRedirectLocation(Aws::String&& value) { m_websiteRedirectLocationHasBeenSet = true; m_websiteRedirectLocation = std::move(value); }

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline void SetWebsiteRedirectLocation(const char* value) { m_websiteRedirectLocationHasBeenSet = true; m_websiteRedirectLocation.assign(value); }

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline CreateMultipartUploadRequest& WithWebsiteRedirectLocation(const Aws::String& value) { SetWebsiteRedirectLocation(value); return *this;}

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline CreateMultipartUploadRequest& WithWebsiteRedirectLocation(Aws::String&& value) { SetWebsiteRedirectLocation(std::move(value)); return *this;}

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline CreateMultipartUploadRequest& WithWebsiteRedirectLocation(const char* value) { SetWebsiteRedirectLocation(value); return *this;}


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
    inline CreateMultipartUploadRequest& WithSSECustomerAlgorithm(const Aws::String& value) { SetSSECustomerAlgorithm(value); return *this;}

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline CreateMultipartUploadRequest& WithSSECustomerAlgorithm(Aws::String&& value) { SetSSECustomerAlgorithm(std::move(value)); return *this;}

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline CreateMultipartUploadRequest& WithSSECustomerAlgorithm(const char* value) { SetSSECustomerAlgorithm(value); return *this;}


    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline const Aws::String& GetSSECustomerKey() const{ return m_sSECustomerKey; }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline bool SSECustomerKeyHasBeenSet() const { return m_sSECustomerKeyHasBeenSet; }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline void SetSSECustomerKey(const Aws::String& value) { m_sSECustomerKeyHasBeenSet = true; m_sSECustomerKey = value; }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline void SetSSECustomerKey(Aws::String&& value) { m_sSECustomerKeyHasBeenSet = true; m_sSECustomerKey = std::move(value); }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline void SetSSECustomerKey(const char* value) { m_sSECustomerKeyHasBeenSet = true; m_sSECustomerKey.assign(value); }

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline CreateMultipartUploadRequest& WithSSECustomerKey(const Aws::String& value) { SetSSECustomerKey(value); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline CreateMultipartUploadRequest& WithSSECustomerKey(Aws::String&& value) { SetSSECustomerKey(std::move(value)); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline CreateMultipartUploadRequest& WithSSECustomerKey(const char* value) { SetSSECustomerKey(value); return *this;}


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
    inline CreateMultipartUploadRequest& WithSSECustomerKeyMD5(const Aws::String& value) { SetSSECustomerKeyMD5(value); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline CreateMultipartUploadRequest& WithSSECustomerKeyMD5(Aws::String&& value) { SetSSECustomerKeyMD5(std::move(value)); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline CreateMultipartUploadRequest& WithSSECustomerKeyMD5(const char* value) { SetSSECustomerKeyMD5(value); return *this;}


    /**
     * <p>Specifies the ID of the symmetric customer managed AWS KMS CMK to use for
     * object encryption. All GET and PUT requests for an object protected by AWS KMS
     * will fail if not made via SSL or using SigV4. For information about configuring
     * using any of the officially supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/http:/docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline const Aws::String& GetSSEKMSKeyId() const{ return m_sSEKMSKeyId; }

    /**
     * <p>Specifies the ID of the symmetric customer managed AWS KMS CMK to use for
     * object encryption. All GET and PUT requests for an object protected by AWS KMS
     * will fail if not made via SSL or using SigV4. For information about configuring
     * using any of the officially supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/http:/docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline bool SSEKMSKeyIdHasBeenSet() const { return m_sSEKMSKeyIdHasBeenSet; }

    /**
     * <p>Specifies the ID of the symmetric customer managed AWS KMS CMK to use for
     * object encryption. All GET and PUT requests for an object protected by AWS KMS
     * will fail if not made via SSL or using SigV4. For information about configuring
     * using any of the officially supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/http:/docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline void SetSSEKMSKeyId(const Aws::String& value) { m_sSEKMSKeyIdHasBeenSet = true; m_sSEKMSKeyId = value; }

    /**
     * <p>Specifies the ID of the symmetric customer managed AWS KMS CMK to use for
     * object encryption. All GET and PUT requests for an object protected by AWS KMS
     * will fail if not made via SSL or using SigV4. For information about configuring
     * using any of the officially supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/http:/docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline void SetSSEKMSKeyId(Aws::String&& value) { m_sSEKMSKeyIdHasBeenSet = true; m_sSEKMSKeyId = std::move(value); }

    /**
     * <p>Specifies the ID of the symmetric customer managed AWS KMS CMK to use for
     * object encryption. All GET and PUT requests for an object protected by AWS KMS
     * will fail if not made via SSL or using SigV4. For information about configuring
     * using any of the officially supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/http:/docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline void SetSSEKMSKeyId(const char* value) { m_sSEKMSKeyIdHasBeenSet = true; m_sSEKMSKeyId.assign(value); }

    /**
     * <p>Specifies the ID of the symmetric customer managed AWS KMS CMK to use for
     * object encryption. All GET and PUT requests for an object protected by AWS KMS
     * will fail if not made via SSL or using SigV4. For information about configuring
     * using any of the officially supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/http:/docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline CreateMultipartUploadRequest& WithSSEKMSKeyId(const Aws::String& value) { SetSSEKMSKeyId(value); return *this;}

    /**
     * <p>Specifies the ID of the symmetric customer managed AWS KMS CMK to use for
     * object encryption. All GET and PUT requests for an object protected by AWS KMS
     * will fail if not made via SSL or using SigV4. For information about configuring
     * using any of the officially supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/http:/docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline CreateMultipartUploadRequest& WithSSEKMSKeyId(Aws::String&& value) { SetSSEKMSKeyId(std::move(value)); return *this;}

    /**
     * <p>Specifies the ID of the symmetric customer managed AWS KMS CMK to use for
     * object encryption. All GET and PUT requests for an object protected by AWS KMS
     * will fail if not made via SSL or using SigV4. For information about configuring
     * using any of the officially supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/http:/docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline CreateMultipartUploadRequest& WithSSEKMSKeyId(const char* value) { SetSSEKMSKeyId(value); return *this;}


    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline const Aws::String& GetSSEKMSEncryptionContext() const{ return m_sSEKMSEncryptionContext; }

    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline bool SSEKMSEncryptionContextHasBeenSet() const { return m_sSEKMSEncryptionContextHasBeenSet; }

    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline void SetSSEKMSEncryptionContext(const Aws::String& value) { m_sSEKMSEncryptionContextHasBeenSet = true; m_sSEKMSEncryptionContext = value; }

    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline void SetSSEKMSEncryptionContext(Aws::String&& value) { m_sSEKMSEncryptionContextHasBeenSet = true; m_sSEKMSEncryptionContext = std::move(value); }

    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline void SetSSEKMSEncryptionContext(const char* value) { m_sSEKMSEncryptionContextHasBeenSet = true; m_sSEKMSEncryptionContext.assign(value); }

    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline CreateMultipartUploadRequest& WithSSEKMSEncryptionContext(const Aws::String& value) { SetSSEKMSEncryptionContext(value); return *this;}

    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline CreateMultipartUploadRequest& WithSSEKMSEncryptionContext(Aws::String&& value) { SetSSEKMSEncryptionContext(std::move(value)); return *this;}

    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline CreateMultipartUploadRequest& WithSSEKMSEncryptionContext(const char* value) { SetSSEKMSEncryptionContext(value); return *this;}


    
    inline const RequestPayer& GetRequestPayer() const{ return m_requestPayer; }

    
    inline bool RequestPayerHasBeenSet() const { return m_requestPayerHasBeenSet; }

    
    inline void SetRequestPayer(const RequestPayer& value) { m_requestPayerHasBeenSet = true; m_requestPayer = value; }

    
    inline void SetRequestPayer(RequestPayer&& value) { m_requestPayerHasBeenSet = true; m_requestPayer = std::move(value); }

    
    inline CreateMultipartUploadRequest& WithRequestPayer(const RequestPayer& value) { SetRequestPayer(value); return *this;}

    
    inline CreateMultipartUploadRequest& WithRequestPayer(RequestPayer&& value) { SetRequestPayer(std::move(value)); return *this;}


    /**
     * <p>The tag-set for the object. The tag-set must be encoded as URL Query
     * parameters.</p>
     */
    inline const Aws::String& GetTagging() const{ return m_tagging; }

    /**
     * <p>The tag-set for the object. The tag-set must be encoded as URL Query
     * parameters.</p>
     */
    inline bool TaggingHasBeenSet() const { return m_taggingHasBeenSet; }

    /**
     * <p>The tag-set for the object. The tag-set must be encoded as URL Query
     * parameters.</p>
     */
    inline void SetTagging(const Aws::String& value) { m_taggingHasBeenSet = true; m_tagging = value; }

    /**
     * <p>The tag-set for the object. The tag-set must be encoded as URL Query
     * parameters.</p>
     */
    inline void SetTagging(Aws::String&& value) { m_taggingHasBeenSet = true; m_tagging = std::move(value); }

    /**
     * <p>The tag-set for the object. The tag-set must be encoded as URL Query
     * parameters.</p>
     */
    inline void SetTagging(const char* value) { m_taggingHasBeenSet = true; m_tagging.assign(value); }

    /**
     * <p>The tag-set for the object. The tag-set must be encoded as URL Query
     * parameters.</p>
     */
    inline CreateMultipartUploadRequest& WithTagging(const Aws::String& value) { SetTagging(value); return *this;}

    /**
     * <p>The tag-set for the object. The tag-set must be encoded as URL Query
     * parameters.</p>
     */
    inline CreateMultipartUploadRequest& WithTagging(Aws::String&& value) { SetTagging(std::move(value)); return *this;}

    /**
     * <p>The tag-set for the object. The tag-set must be encoded as URL Query
     * parameters.</p>
     */
    inline CreateMultipartUploadRequest& WithTagging(const char* value) { SetTagging(value); return *this;}


    /**
     * <p>Specifies the Object Lock mode that you want to apply to the uploaded
     * object.</p>
     */
    inline const ObjectLockMode& GetObjectLockMode() const{ return m_objectLockMode; }

    /**
     * <p>Specifies the Object Lock mode that you want to apply to the uploaded
     * object.</p>
     */
    inline bool ObjectLockModeHasBeenSet() const { return m_objectLockModeHasBeenSet; }

    /**
     * <p>Specifies the Object Lock mode that you want to apply to the uploaded
     * object.</p>
     */
    inline void SetObjectLockMode(const ObjectLockMode& value) { m_objectLockModeHasBeenSet = true; m_objectLockMode = value; }

    /**
     * <p>Specifies the Object Lock mode that you want to apply to the uploaded
     * object.</p>
     */
    inline void SetObjectLockMode(ObjectLockMode&& value) { m_objectLockModeHasBeenSet = true; m_objectLockMode = std::move(value); }

    /**
     * <p>Specifies the Object Lock mode that you want to apply to the uploaded
     * object.</p>
     */
    inline CreateMultipartUploadRequest& WithObjectLockMode(const ObjectLockMode& value) { SetObjectLockMode(value); return *this;}

    /**
     * <p>Specifies the Object Lock mode that you want to apply to the uploaded
     * object.</p>
     */
    inline CreateMultipartUploadRequest& WithObjectLockMode(ObjectLockMode&& value) { SetObjectLockMode(std::move(value)); return *this;}


    /**
     * <p>Specifies the date and time when you want the Object Lock to expire.</p>
     */
    inline const Aws::Utils::DateTime& GetObjectLockRetainUntilDate() const{ return m_objectLockRetainUntilDate; }

    /**
     * <p>Specifies the date and time when you want the Object Lock to expire.</p>
     */
    inline bool ObjectLockRetainUntilDateHasBeenSet() const { return m_objectLockRetainUntilDateHasBeenSet; }

    /**
     * <p>Specifies the date and time when you want the Object Lock to expire.</p>
     */
    inline void SetObjectLockRetainUntilDate(const Aws::Utils::DateTime& value) { m_objectLockRetainUntilDateHasBeenSet = true; m_objectLockRetainUntilDate = value; }

    /**
     * <p>Specifies the date and time when you want the Object Lock to expire.</p>
     */
    inline void SetObjectLockRetainUntilDate(Aws::Utils::DateTime&& value) { m_objectLockRetainUntilDateHasBeenSet = true; m_objectLockRetainUntilDate = std::move(value); }

    /**
     * <p>Specifies the date and time when you want the Object Lock to expire.</p>
     */
    inline CreateMultipartUploadRequest& WithObjectLockRetainUntilDate(const Aws::Utils::DateTime& value) { SetObjectLockRetainUntilDate(value); return *this;}

    /**
     * <p>Specifies the date and time when you want the Object Lock to expire.</p>
     */
    inline CreateMultipartUploadRequest& WithObjectLockRetainUntilDate(Aws::Utils::DateTime&& value) { SetObjectLockRetainUntilDate(std::move(value)); return *this;}


    /**
     * <p>Specifies whether you want to apply a Legal Hold to the uploaded object.</p>
     */
    inline const ObjectLockLegalHoldStatus& GetObjectLockLegalHoldStatus() const{ return m_objectLockLegalHoldStatus; }

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the uploaded object.</p>
     */
    inline bool ObjectLockLegalHoldStatusHasBeenSet() const { return m_objectLockLegalHoldStatusHasBeenSet; }

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the uploaded object.</p>
     */
    inline void SetObjectLockLegalHoldStatus(const ObjectLockLegalHoldStatus& value) { m_objectLockLegalHoldStatusHasBeenSet = true; m_objectLockLegalHoldStatus = value; }

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the uploaded object.</p>
     */
    inline void SetObjectLockLegalHoldStatus(ObjectLockLegalHoldStatus&& value) { m_objectLockLegalHoldStatusHasBeenSet = true; m_objectLockLegalHoldStatus = std::move(value); }

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the uploaded object.</p>
     */
    inline CreateMultipartUploadRequest& WithObjectLockLegalHoldStatus(const ObjectLockLegalHoldStatus& value) { SetObjectLockLegalHoldStatus(value); return *this;}

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the uploaded object.</p>
     */
    inline CreateMultipartUploadRequest& WithObjectLockLegalHoldStatus(ObjectLockLegalHoldStatus&& value) { SetObjectLockLegalHoldStatus(std::move(value)); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline CreateMultipartUploadRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline CreateMultipartUploadRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline CreateMultipartUploadRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline CreateMultipartUploadRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline CreateMultipartUploadRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline CreateMultipartUploadRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline CreateMultipartUploadRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline CreateMultipartUploadRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline CreateMultipartUploadRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    ObjectCannedACL m_aCL;
    bool m_aCLHasBeenSet;

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_cacheControl;
    bool m_cacheControlHasBeenSet;

    Aws::String m_contentDisposition;
    bool m_contentDispositionHasBeenSet;

    Aws::String m_contentEncoding;
    bool m_contentEncodingHasBeenSet;

    Aws::String m_contentLanguage;
    bool m_contentLanguageHasBeenSet;

    Aws::String m_contentType;
    bool m_contentTypeHasBeenSet;

    Aws::Utils::DateTime m_expires;
    bool m_expiresHasBeenSet;

    Aws::String m_grantFullControl;
    bool m_grantFullControlHasBeenSet;

    Aws::String m_grantRead;
    bool m_grantReadHasBeenSet;

    Aws::String m_grantReadACP;
    bool m_grantReadACPHasBeenSet;

    Aws::String m_grantWriteACP;
    bool m_grantWriteACPHasBeenSet;

    Aws::String m_key;
    bool m_keyHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_metadata;
    bool m_metadataHasBeenSet;

    ServerSideEncryption m_serverSideEncryption;
    bool m_serverSideEncryptionHasBeenSet;

    StorageClass m_storageClass;
    bool m_storageClassHasBeenSet;

    Aws::String m_websiteRedirectLocation;
    bool m_websiteRedirectLocationHasBeenSet;

    Aws::String m_sSECustomerAlgorithm;
    bool m_sSECustomerAlgorithmHasBeenSet;

    Aws::String m_sSECustomerKey;
    bool m_sSECustomerKeyHasBeenSet;

    Aws::String m_sSECustomerKeyMD5;
    bool m_sSECustomerKeyMD5HasBeenSet;

    Aws::String m_sSEKMSKeyId;
    bool m_sSEKMSKeyIdHasBeenSet;

    Aws::String m_sSEKMSEncryptionContext;
    bool m_sSEKMSEncryptionContextHasBeenSet;

    RequestPayer m_requestPayer;
    bool m_requestPayerHasBeenSet;

    Aws::String m_tagging;
    bool m_taggingHasBeenSet;

    ObjectLockMode m_objectLockMode;
    bool m_objectLockModeHasBeenSet;

    Aws::Utils::DateTime m_objectLockRetainUntilDate;
    bool m_objectLockRetainUntilDateHasBeenSet;

    ObjectLockLegalHoldStatus m_objectLockLegalHoldStatus;
    bool m_objectLockLegalHoldStatusHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
