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
#include <aws/s3/model/MetadataDirective.h>
#include <aws/s3/model/TaggingDirective.h>
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
  class AWS_S3_API CopyObjectRequest : public S3Request
  {
  public:
    CopyObjectRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "CopyObject"; }

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
    inline CopyObjectRequest& WithACL(const ObjectCannedACL& value) { SetACL(value); return *this;}

    /**
     * <p>The canned ACL to apply to the object.</p>
     */
    inline CopyObjectRequest& WithACL(ObjectCannedACL&& value) { SetACL(std::move(value)); return *this;}


    /**
     * <p>The name of the destination bucket.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>The name of the destination bucket.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The name of the destination bucket.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The name of the destination bucket.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The name of the destination bucket.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The name of the destination bucket.</p>
     */
    inline CopyObjectRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The name of the destination bucket.</p>
     */
    inline CopyObjectRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The name of the destination bucket.</p>
     */
    inline CopyObjectRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


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
    inline CopyObjectRequest& WithCacheControl(const Aws::String& value) { SetCacheControl(value); return *this;}

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline CopyObjectRequest& WithCacheControl(Aws::String&& value) { SetCacheControl(std::move(value)); return *this;}

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline CopyObjectRequest& WithCacheControl(const char* value) { SetCacheControl(value); return *this;}


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
    inline CopyObjectRequest& WithContentDisposition(const Aws::String& value) { SetContentDisposition(value); return *this;}

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline CopyObjectRequest& WithContentDisposition(Aws::String&& value) { SetContentDisposition(std::move(value)); return *this;}

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline CopyObjectRequest& WithContentDisposition(const char* value) { SetContentDisposition(value); return *this;}


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
    inline CopyObjectRequest& WithContentEncoding(const Aws::String& value) { SetContentEncoding(value); return *this;}

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline CopyObjectRequest& WithContentEncoding(Aws::String&& value) { SetContentEncoding(std::move(value)); return *this;}

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline CopyObjectRequest& WithContentEncoding(const char* value) { SetContentEncoding(value); return *this;}


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
    inline CopyObjectRequest& WithContentLanguage(const Aws::String& value) { SetContentLanguage(value); return *this;}

    /**
     * <p>The language the content is in.</p>
     */
    inline CopyObjectRequest& WithContentLanguage(Aws::String&& value) { SetContentLanguage(std::move(value)); return *this;}

    /**
     * <p>The language the content is in.</p>
     */
    inline CopyObjectRequest& WithContentLanguage(const char* value) { SetContentLanguage(value); return *this;}


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
    inline CopyObjectRequest& WithContentType(const Aws::String& value) { SetContentType(value); return *this;}

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline CopyObjectRequest& WithContentType(Aws::String&& value) { SetContentType(std::move(value)); return *this;}

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline CopyObjectRequest& WithContentType(const char* value) { SetContentType(value); return *this;}


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
    inline CopyObjectRequest& WithCopySource(const Aws::String& value) { SetCopySource(value); return *this;}

    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline CopyObjectRequest& WithCopySource(Aws::String&& value) { SetCopySource(std::move(value)); return *this;}

    /**
     * <p>The name of the source bucket and key name of the source object, separated by
     * a slash (/). Must be URL-encoded.</p>
     */
    inline CopyObjectRequest& WithCopySource(const char* value) { SetCopySource(value); return *this;}


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
    inline CopyObjectRequest& WithCopySourceIfMatch(const Aws::String& value) { SetCopySourceIfMatch(value); return *this;}

    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline CopyObjectRequest& WithCopySourceIfMatch(Aws::String&& value) { SetCopySourceIfMatch(std::move(value)); return *this;}

    /**
     * <p>Copies the object if its entity tag (ETag) matches the specified tag.</p>
     */
    inline CopyObjectRequest& WithCopySourceIfMatch(const char* value) { SetCopySourceIfMatch(value); return *this;}


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
    inline CopyObjectRequest& WithCopySourceIfModifiedSince(const Aws::Utils::DateTime& value) { SetCopySourceIfModifiedSince(value); return *this;}

    /**
     * <p>Copies the object if it has been modified since the specified time.</p>
     */
    inline CopyObjectRequest& WithCopySourceIfModifiedSince(Aws::Utils::DateTime&& value) { SetCopySourceIfModifiedSince(std::move(value)); return *this;}


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
    inline CopyObjectRequest& WithCopySourceIfNoneMatch(const Aws::String& value) { SetCopySourceIfNoneMatch(value); return *this;}

    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline CopyObjectRequest& WithCopySourceIfNoneMatch(Aws::String&& value) { SetCopySourceIfNoneMatch(std::move(value)); return *this;}

    /**
     * <p>Copies the object if its entity tag (ETag) is different than the specified
     * ETag.</p>
     */
    inline CopyObjectRequest& WithCopySourceIfNoneMatch(const char* value) { SetCopySourceIfNoneMatch(value); return *this;}


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
    inline CopyObjectRequest& WithCopySourceIfUnmodifiedSince(const Aws::Utils::DateTime& value) { SetCopySourceIfUnmodifiedSince(value); return *this;}

    /**
     * <p>Copies the object if it hasn't been modified since the specified time.</p>
     */
    inline CopyObjectRequest& WithCopySourceIfUnmodifiedSince(Aws::Utils::DateTime&& value) { SetCopySourceIfUnmodifiedSince(std::move(value)); return *this;}


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
    inline CopyObjectRequest& WithExpires(const Aws::Utils::DateTime& value) { SetExpires(value); return *this;}

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline CopyObjectRequest& WithExpires(Aws::Utils::DateTime&& value) { SetExpires(std::move(value)); return *this;}


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
    inline CopyObjectRequest& WithGrantFullControl(const Aws::String& value) { SetGrantFullControl(value); return *this;}

    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline CopyObjectRequest& WithGrantFullControl(Aws::String&& value) { SetGrantFullControl(std::move(value)); return *this;}

    /**
     * <p>Gives the grantee READ, READ_ACP, and WRITE_ACP permissions on the
     * object.</p>
     */
    inline CopyObjectRequest& WithGrantFullControl(const char* value) { SetGrantFullControl(value); return *this;}


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
    inline CopyObjectRequest& WithGrantRead(const Aws::String& value) { SetGrantRead(value); return *this;}

    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline CopyObjectRequest& WithGrantRead(Aws::String&& value) { SetGrantRead(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to read the object data and its metadata.</p>
     */
    inline CopyObjectRequest& WithGrantRead(const char* value) { SetGrantRead(value); return *this;}


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
    inline CopyObjectRequest& WithGrantReadACP(const Aws::String& value) { SetGrantReadACP(value); return *this;}

    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline CopyObjectRequest& WithGrantReadACP(Aws::String&& value) { SetGrantReadACP(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to read the object ACL.</p>
     */
    inline CopyObjectRequest& WithGrantReadACP(const char* value) { SetGrantReadACP(value); return *this;}


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
    inline CopyObjectRequest& WithGrantWriteACP(const Aws::String& value) { SetGrantWriteACP(value); return *this;}

    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline CopyObjectRequest& WithGrantWriteACP(Aws::String&& value) { SetGrantWriteACP(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to write the ACL for the applicable object.</p>
     */
    inline CopyObjectRequest& WithGrantWriteACP(const char* value) { SetGrantWriteACP(value); return *this;}


    /**
     * <p>The key of the destination object.</p>
     */
    inline const Aws::String& GetKey() const{ return m_key; }

    /**
     * <p>The key of the destination object.</p>
     */
    inline bool KeyHasBeenSet() const { return m_keyHasBeenSet; }

    /**
     * <p>The key of the destination object.</p>
     */
    inline void SetKey(const Aws::String& value) { m_keyHasBeenSet = true; m_key = value; }

    /**
     * <p>The key of the destination object.</p>
     */
    inline void SetKey(Aws::String&& value) { m_keyHasBeenSet = true; m_key = std::move(value); }

    /**
     * <p>The key of the destination object.</p>
     */
    inline void SetKey(const char* value) { m_keyHasBeenSet = true; m_key.assign(value); }

    /**
     * <p>The key of the destination object.</p>
     */
    inline CopyObjectRequest& WithKey(const Aws::String& value) { SetKey(value); return *this;}

    /**
     * <p>The key of the destination object.</p>
     */
    inline CopyObjectRequest& WithKey(Aws::String&& value) { SetKey(std::move(value)); return *this;}

    /**
     * <p>The key of the destination object.</p>
     */
    inline CopyObjectRequest& WithKey(const char* value) { SetKey(value); return *this;}


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
    inline CopyObjectRequest& WithMetadata(const Aws::Map<Aws::String, Aws::String>& value) { SetMetadata(value); return *this;}

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CopyObjectRequest& WithMetadata(Aws::Map<Aws::String, Aws::String>&& value) { SetMetadata(std::move(value)); return *this;}

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CopyObjectRequest& AddMetadata(const Aws::String& key, const Aws::String& value) { m_metadataHasBeenSet = true; m_metadata.emplace(key, value); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CopyObjectRequest& AddMetadata(Aws::String&& key, const Aws::String& value) { m_metadataHasBeenSet = true; m_metadata.emplace(std::move(key), value); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CopyObjectRequest& AddMetadata(const Aws::String& key, Aws::String&& value) { m_metadataHasBeenSet = true; m_metadata.emplace(key, std::move(value)); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CopyObjectRequest& AddMetadata(Aws::String&& key, Aws::String&& value) { m_metadataHasBeenSet = true; m_metadata.emplace(std::move(key), std::move(value)); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CopyObjectRequest& AddMetadata(const char* key, Aws::String&& value) { m_metadataHasBeenSet = true; m_metadata.emplace(key, std::move(value)); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CopyObjectRequest& AddMetadata(Aws::String&& key, const char* value) { m_metadataHasBeenSet = true; m_metadata.emplace(std::move(key), value); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline CopyObjectRequest& AddMetadata(const char* key, const char* value) { m_metadataHasBeenSet = true; m_metadata.emplace(key, value); return *this; }


    /**
     * <p>Specifies whether the metadata is copied from the source object or replaced
     * with metadata provided in the request.</p>
     */
    inline const MetadataDirective& GetMetadataDirective() const{ return m_metadataDirective; }

    /**
     * <p>Specifies whether the metadata is copied from the source object or replaced
     * with metadata provided in the request.</p>
     */
    inline bool MetadataDirectiveHasBeenSet() const { return m_metadataDirectiveHasBeenSet; }

    /**
     * <p>Specifies whether the metadata is copied from the source object or replaced
     * with metadata provided in the request.</p>
     */
    inline void SetMetadataDirective(const MetadataDirective& value) { m_metadataDirectiveHasBeenSet = true; m_metadataDirective = value; }

    /**
     * <p>Specifies whether the metadata is copied from the source object or replaced
     * with metadata provided in the request.</p>
     */
    inline void SetMetadataDirective(MetadataDirective&& value) { m_metadataDirectiveHasBeenSet = true; m_metadataDirective = std::move(value); }

    /**
     * <p>Specifies whether the metadata is copied from the source object or replaced
     * with metadata provided in the request.</p>
     */
    inline CopyObjectRequest& WithMetadataDirective(const MetadataDirective& value) { SetMetadataDirective(value); return *this;}

    /**
     * <p>Specifies whether the metadata is copied from the source object or replaced
     * with metadata provided in the request.</p>
     */
    inline CopyObjectRequest& WithMetadataDirective(MetadataDirective&& value) { SetMetadataDirective(std::move(value)); return *this;}


    /**
     * <p>Specifies whether the object tag-set are copied from the source object or
     * replaced with tag-set provided in the request.</p>
     */
    inline const TaggingDirective& GetTaggingDirective() const{ return m_taggingDirective; }

    /**
     * <p>Specifies whether the object tag-set are copied from the source object or
     * replaced with tag-set provided in the request.</p>
     */
    inline bool TaggingDirectiveHasBeenSet() const { return m_taggingDirectiveHasBeenSet; }

    /**
     * <p>Specifies whether the object tag-set are copied from the source object or
     * replaced with tag-set provided in the request.</p>
     */
    inline void SetTaggingDirective(const TaggingDirective& value) { m_taggingDirectiveHasBeenSet = true; m_taggingDirective = value; }

    /**
     * <p>Specifies whether the object tag-set are copied from the source object or
     * replaced with tag-set provided in the request.</p>
     */
    inline void SetTaggingDirective(TaggingDirective&& value) { m_taggingDirectiveHasBeenSet = true; m_taggingDirective = std::move(value); }

    /**
     * <p>Specifies whether the object tag-set are copied from the source object or
     * replaced with tag-set provided in the request.</p>
     */
    inline CopyObjectRequest& WithTaggingDirective(const TaggingDirective& value) { SetTaggingDirective(value); return *this;}

    /**
     * <p>Specifies whether the object tag-set are copied from the source object or
     * replaced with tag-set provided in the request.</p>
     */
    inline CopyObjectRequest& WithTaggingDirective(TaggingDirective&& value) { SetTaggingDirective(std::move(value)); return *this;}


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
    inline CopyObjectRequest& WithServerSideEncryption(const ServerSideEncryption& value) { SetServerSideEncryption(value); return *this;}

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline CopyObjectRequest& WithServerSideEncryption(ServerSideEncryption&& value) { SetServerSideEncryption(std::move(value)); return *this;}


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
    inline CopyObjectRequest& WithStorageClass(const StorageClass& value) { SetStorageClass(value); return *this;}

    /**
     * <p>The type of storage to use for the object. Defaults to 'STANDARD'.</p>
     */
    inline CopyObjectRequest& WithStorageClass(StorageClass&& value) { SetStorageClass(std::move(value)); return *this;}


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
    inline CopyObjectRequest& WithWebsiteRedirectLocation(const Aws::String& value) { SetWebsiteRedirectLocation(value); return *this;}

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline CopyObjectRequest& WithWebsiteRedirectLocation(Aws::String&& value) { SetWebsiteRedirectLocation(std::move(value)); return *this;}

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline CopyObjectRequest& WithWebsiteRedirectLocation(const char* value) { SetWebsiteRedirectLocation(value); return *this;}


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
    inline CopyObjectRequest& WithSSECustomerAlgorithm(const Aws::String& value) { SetSSECustomerAlgorithm(value); return *this;}

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline CopyObjectRequest& WithSSECustomerAlgorithm(Aws::String&& value) { SetSSECustomerAlgorithm(std::move(value)); return *this;}

    /**
     * <p>Specifies the algorithm to use to when encrypting the object (for example,
     * AES256).</p>
     */
    inline CopyObjectRequest& WithSSECustomerAlgorithm(const char* value) { SetSSECustomerAlgorithm(value); return *this;}


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
    inline CopyObjectRequest& WithSSECustomerKey(const Aws::String& value) { SetSSECustomerKey(value); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline CopyObjectRequest& WithSSECustomerKey(Aws::String&& value) { SetSSECustomerKey(std::move(value)); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use in
     * encrypting data. This value is used to store the object and then it is
     * discarded; Amazon S3 does not store the encryption key. The key must be
     * appropriate for use with the algorithm specified in the
     * <code>x-amz-server-side​-encryption​-customer-algorithm</code> header.</p>
     */
    inline CopyObjectRequest& WithSSECustomerKey(const char* value) { SetSSECustomerKey(value); return *this;}


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
    inline CopyObjectRequest& WithSSECustomerKeyMD5(const Aws::String& value) { SetSSECustomerKeyMD5(value); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline CopyObjectRequest& WithSSECustomerKeyMD5(Aws::String&& value) { SetSSECustomerKeyMD5(std::move(value)); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline CopyObjectRequest& WithSSECustomerKeyMD5(const char* value) { SetSSECustomerKeyMD5(value); return *this;}


    /**
     * <p>Specifies the AWS KMS key ID to use for object encryption. All GET and PUT
     * requests for an object protected by AWS KMS will fail if not made via SSL or
     * using SigV4. For information about configuring using any of the officially
     * supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline const Aws::String& GetSSEKMSKeyId() const{ return m_sSEKMSKeyId; }

    /**
     * <p>Specifies the AWS KMS key ID to use for object encryption. All GET and PUT
     * requests for an object protected by AWS KMS will fail if not made via SSL or
     * using SigV4. For information about configuring using any of the officially
     * supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline bool SSEKMSKeyIdHasBeenSet() const { return m_sSEKMSKeyIdHasBeenSet; }

    /**
     * <p>Specifies the AWS KMS key ID to use for object encryption. All GET and PUT
     * requests for an object protected by AWS KMS will fail if not made via SSL or
     * using SigV4. For information about configuring using any of the officially
     * supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline void SetSSEKMSKeyId(const Aws::String& value) { m_sSEKMSKeyIdHasBeenSet = true; m_sSEKMSKeyId = value; }

    /**
     * <p>Specifies the AWS KMS key ID to use for object encryption. All GET and PUT
     * requests for an object protected by AWS KMS will fail if not made via SSL or
     * using SigV4. For information about configuring using any of the officially
     * supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline void SetSSEKMSKeyId(Aws::String&& value) { m_sSEKMSKeyIdHasBeenSet = true; m_sSEKMSKeyId = std::move(value); }

    /**
     * <p>Specifies the AWS KMS key ID to use for object encryption. All GET and PUT
     * requests for an object protected by AWS KMS will fail if not made via SSL or
     * using SigV4. For information about configuring using any of the officially
     * supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline void SetSSEKMSKeyId(const char* value) { m_sSEKMSKeyIdHasBeenSet = true; m_sSEKMSKeyId.assign(value); }

    /**
     * <p>Specifies the AWS KMS key ID to use for object encryption. All GET and PUT
     * requests for an object protected by AWS KMS will fail if not made via SSL or
     * using SigV4. For information about configuring using any of the officially
     * supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline CopyObjectRequest& WithSSEKMSKeyId(const Aws::String& value) { SetSSEKMSKeyId(value); return *this;}

    /**
     * <p>Specifies the AWS KMS key ID to use for object encryption. All GET and PUT
     * requests for an object protected by AWS KMS will fail if not made via SSL or
     * using SigV4. For information about configuring using any of the officially
     * supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline CopyObjectRequest& WithSSEKMSKeyId(Aws::String&& value) { SetSSEKMSKeyId(std::move(value)); return *this;}

    /**
     * <p>Specifies the AWS KMS key ID to use for object encryption. All GET and PUT
     * requests for an object protected by AWS KMS will fail if not made via SSL or
     * using SigV4. For information about configuring using any of the officially
     * supported AWS SDKs and AWS CLI, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/UsingAWSSDK.html#specify-signature-version">Specifying
     * the Signature Version in Request Authentication</a> in the <i>Amazon S3
     * Developer Guide</i>.</p>
     */
    inline CopyObjectRequest& WithSSEKMSKeyId(const char* value) { SetSSEKMSKeyId(value); return *this;}


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
    inline CopyObjectRequest& WithSSEKMSEncryptionContext(const Aws::String& value) { SetSSEKMSEncryptionContext(value); return *this;}

    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline CopyObjectRequest& WithSSEKMSEncryptionContext(Aws::String&& value) { SetSSEKMSEncryptionContext(std::move(value)); return *this;}

    /**
     * <p>Specifies the AWS KMS Encryption Context to use for object encryption. The
     * value of this header is a base64-encoded UTF-8 string holding JSON with the
     * encryption context key-value pairs.</p>
     */
    inline CopyObjectRequest& WithSSEKMSEncryptionContext(const char* value) { SetSSEKMSEncryptionContext(value); return *this;}


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
    inline CopyObjectRequest& WithCopySourceSSECustomerAlgorithm(const Aws::String& value) { SetCopySourceSSECustomerAlgorithm(value); return *this;}

    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline CopyObjectRequest& WithCopySourceSSECustomerAlgorithm(Aws::String&& value) { SetCopySourceSSECustomerAlgorithm(std::move(value)); return *this;}

    /**
     * <p>Specifies the algorithm to use when decrypting the source object (for
     * example, AES256).</p>
     */
    inline CopyObjectRequest& WithCopySourceSSECustomerAlgorithm(const char* value) { SetCopySourceSSECustomerAlgorithm(value); return *this;}


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
    inline CopyObjectRequest& WithCopySourceSSECustomerKey(const Aws::String& value) { SetCopySourceSSECustomerKey(value); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline CopyObjectRequest& WithCopySourceSSECustomerKey(Aws::String&& value) { SetCopySourceSSECustomerKey(std::move(value)); return *this;}

    /**
     * <p>Specifies the customer-provided encryption key for Amazon S3 to use to
     * decrypt the source object. The encryption key provided in this header must be
     * one that was used when the source object was created.</p>
     */
    inline CopyObjectRequest& WithCopySourceSSECustomerKey(const char* value) { SetCopySourceSSECustomerKey(value); return *this;}


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
    inline CopyObjectRequest& WithCopySourceSSECustomerKeyMD5(const Aws::String& value) { SetCopySourceSSECustomerKeyMD5(value); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline CopyObjectRequest& WithCopySourceSSECustomerKeyMD5(Aws::String&& value) { SetCopySourceSSECustomerKeyMD5(std::move(value)); return *this;}

    /**
     * <p>Specifies the 128-bit MD5 digest of the encryption key according to RFC 1321.
     * Amazon S3 uses this header for a message integrity check to ensure that the
     * encryption key was transmitted without error.</p>
     */
    inline CopyObjectRequest& WithCopySourceSSECustomerKeyMD5(const char* value) { SetCopySourceSSECustomerKeyMD5(value); return *this;}


    
    inline const RequestPayer& GetRequestPayer() const{ return m_requestPayer; }

    
    inline bool RequestPayerHasBeenSet() const { return m_requestPayerHasBeenSet; }

    
    inline void SetRequestPayer(const RequestPayer& value) { m_requestPayerHasBeenSet = true; m_requestPayer = value; }

    
    inline void SetRequestPayer(RequestPayer&& value) { m_requestPayerHasBeenSet = true; m_requestPayer = std::move(value); }

    
    inline CopyObjectRequest& WithRequestPayer(const RequestPayer& value) { SetRequestPayer(value); return *this;}

    
    inline CopyObjectRequest& WithRequestPayer(RequestPayer&& value) { SetRequestPayer(std::move(value)); return *this;}


    /**
     * <p>The tag-set for the object destination object this value must be used in
     * conjunction with the <code>TaggingDirective</code>. The tag-set must be encoded
     * as URL Query parameters.</p>
     */
    inline const Aws::String& GetTagging() const{ return m_tagging; }

    /**
     * <p>The tag-set for the object destination object this value must be used in
     * conjunction with the <code>TaggingDirective</code>. The tag-set must be encoded
     * as URL Query parameters.</p>
     */
    inline bool TaggingHasBeenSet() const { return m_taggingHasBeenSet; }

    /**
     * <p>The tag-set for the object destination object this value must be used in
     * conjunction with the <code>TaggingDirective</code>. The tag-set must be encoded
     * as URL Query parameters.</p>
     */
    inline void SetTagging(const Aws::String& value) { m_taggingHasBeenSet = true; m_tagging = value; }

    /**
     * <p>The tag-set for the object destination object this value must be used in
     * conjunction with the <code>TaggingDirective</code>. The tag-set must be encoded
     * as URL Query parameters.</p>
     */
    inline void SetTagging(Aws::String&& value) { m_taggingHasBeenSet = true; m_tagging = std::move(value); }

    /**
     * <p>The tag-set for the object destination object this value must be used in
     * conjunction with the <code>TaggingDirective</code>. The tag-set must be encoded
     * as URL Query parameters.</p>
     */
    inline void SetTagging(const char* value) { m_taggingHasBeenSet = true; m_tagging.assign(value); }

    /**
     * <p>The tag-set for the object destination object this value must be used in
     * conjunction with the <code>TaggingDirective</code>. The tag-set must be encoded
     * as URL Query parameters.</p>
     */
    inline CopyObjectRequest& WithTagging(const Aws::String& value) { SetTagging(value); return *this;}

    /**
     * <p>The tag-set for the object destination object this value must be used in
     * conjunction with the <code>TaggingDirective</code>. The tag-set must be encoded
     * as URL Query parameters.</p>
     */
    inline CopyObjectRequest& WithTagging(Aws::String&& value) { SetTagging(std::move(value)); return *this;}

    /**
     * <p>The tag-set for the object destination object this value must be used in
     * conjunction with the <code>TaggingDirective</code>. The tag-set must be encoded
     * as URL Query parameters.</p>
     */
    inline CopyObjectRequest& WithTagging(const char* value) { SetTagging(value); return *this;}


    /**
     * <p>The Object Lock mode that you want to apply to the copied object.</p>
     */
    inline const ObjectLockMode& GetObjectLockMode() const{ return m_objectLockMode; }

    /**
     * <p>The Object Lock mode that you want to apply to the copied object.</p>
     */
    inline bool ObjectLockModeHasBeenSet() const { return m_objectLockModeHasBeenSet; }

    /**
     * <p>The Object Lock mode that you want to apply to the copied object.</p>
     */
    inline void SetObjectLockMode(const ObjectLockMode& value) { m_objectLockModeHasBeenSet = true; m_objectLockMode = value; }

    /**
     * <p>The Object Lock mode that you want to apply to the copied object.</p>
     */
    inline void SetObjectLockMode(ObjectLockMode&& value) { m_objectLockModeHasBeenSet = true; m_objectLockMode = std::move(value); }

    /**
     * <p>The Object Lock mode that you want to apply to the copied object.</p>
     */
    inline CopyObjectRequest& WithObjectLockMode(const ObjectLockMode& value) { SetObjectLockMode(value); return *this;}

    /**
     * <p>The Object Lock mode that you want to apply to the copied object.</p>
     */
    inline CopyObjectRequest& WithObjectLockMode(ObjectLockMode&& value) { SetObjectLockMode(std::move(value)); return *this;}


    /**
     * <p>The date and time when you want the copied object's Object Lock to
     * expire.</p>
     */
    inline const Aws::Utils::DateTime& GetObjectLockRetainUntilDate() const{ return m_objectLockRetainUntilDate; }

    /**
     * <p>The date and time when you want the copied object's Object Lock to
     * expire.</p>
     */
    inline bool ObjectLockRetainUntilDateHasBeenSet() const { return m_objectLockRetainUntilDateHasBeenSet; }

    /**
     * <p>The date and time when you want the copied object's Object Lock to
     * expire.</p>
     */
    inline void SetObjectLockRetainUntilDate(const Aws::Utils::DateTime& value) { m_objectLockRetainUntilDateHasBeenSet = true; m_objectLockRetainUntilDate = value; }

    /**
     * <p>The date and time when you want the copied object's Object Lock to
     * expire.</p>
     */
    inline void SetObjectLockRetainUntilDate(Aws::Utils::DateTime&& value) { m_objectLockRetainUntilDateHasBeenSet = true; m_objectLockRetainUntilDate = std::move(value); }

    /**
     * <p>The date and time when you want the copied object's Object Lock to
     * expire.</p>
     */
    inline CopyObjectRequest& WithObjectLockRetainUntilDate(const Aws::Utils::DateTime& value) { SetObjectLockRetainUntilDate(value); return *this;}

    /**
     * <p>The date and time when you want the copied object's Object Lock to
     * expire.</p>
     */
    inline CopyObjectRequest& WithObjectLockRetainUntilDate(Aws::Utils::DateTime&& value) { SetObjectLockRetainUntilDate(std::move(value)); return *this;}


    /**
     * <p>Specifies whether you want to apply a Legal Hold to the copied object.</p>
     */
    inline const ObjectLockLegalHoldStatus& GetObjectLockLegalHoldStatus() const{ return m_objectLockLegalHoldStatus; }

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the copied object.</p>
     */
    inline bool ObjectLockLegalHoldStatusHasBeenSet() const { return m_objectLockLegalHoldStatusHasBeenSet; }

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the copied object.</p>
     */
    inline void SetObjectLockLegalHoldStatus(const ObjectLockLegalHoldStatus& value) { m_objectLockLegalHoldStatusHasBeenSet = true; m_objectLockLegalHoldStatus = value; }

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the copied object.</p>
     */
    inline void SetObjectLockLegalHoldStatus(ObjectLockLegalHoldStatus&& value) { m_objectLockLegalHoldStatusHasBeenSet = true; m_objectLockLegalHoldStatus = std::move(value); }

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the copied object.</p>
     */
    inline CopyObjectRequest& WithObjectLockLegalHoldStatus(const ObjectLockLegalHoldStatus& value) { SetObjectLockLegalHoldStatus(value); return *this;}

    /**
     * <p>Specifies whether you want to apply a Legal Hold to the copied object.</p>
     */
    inline CopyObjectRequest& WithObjectLockLegalHoldStatus(ObjectLockLegalHoldStatus&& value) { SetObjectLockLegalHoldStatus(std::move(value)); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline CopyObjectRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline CopyObjectRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline CopyObjectRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline CopyObjectRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline CopyObjectRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline CopyObjectRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline CopyObjectRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline CopyObjectRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline CopyObjectRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

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

    MetadataDirective m_metadataDirective;
    bool m_metadataDirectiveHasBeenSet;

    TaggingDirective m_taggingDirective;
    bool m_taggingDirectiveHasBeenSet;

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

    Aws::String m_copySourceSSECustomerAlgorithm;
    bool m_copySourceSSECustomerAlgorithmHasBeenSet;

    Aws::String m_copySourceSSECustomerKey;
    bool m_copySourceSSECustomerKeyHasBeenSet;

    Aws::String m_copySourceSSECustomerKeyMD5;
    bool m_copySourceSSECustomerKeyMD5HasBeenSet;

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
