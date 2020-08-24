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
#include <aws/core/utils/stream/ResponseStream.h>
#include <aws/core/utils/Array.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/DateTime.h>
#include <aws/s3/model/ServerSideEncryption.h>
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <aws/s3/model/StorageClass.h>
#include <aws/s3/model/RequestCharged.h>
#include <aws/s3/model/ReplicationStatus.h>
#include <aws/s3/model/ObjectLockMode.h>
#include <aws/s3/model/ObjectLockLegalHoldStatus.h>
#include <utility>

namespace Aws
{
template<typename RESULT_TYPE>
class AmazonWebServiceResult;

namespace S3
{
namespace Model
{
  class AWS_S3_API GetObjectResult
  {
  public:
    GetObjectResult();
    //We have to define these because Microsoft doesn't auto generate them
    GetObjectResult(GetObjectResult&&);
    GetObjectResult& operator=(GetObjectResult&&);
    //we delete these because Microsoft doesn't handle move generation correctly
    //and we therefore don't trust them to get it right here either.
    GetObjectResult(const GetObjectResult&) = delete;
    GetObjectResult& operator=(const GetObjectResult&) = delete;


    GetObjectResult(Aws::AmazonWebServiceResult<Aws::Utils::Stream::ResponseStream>&& result);
    GetObjectResult& operator=(Aws::AmazonWebServiceResult<Aws::Utils::Stream::ResponseStream>&& result);



    /**
     * <p>Object data.</p>
     */
    inline Aws::IOStream& GetBody() { return m_body.GetUnderlyingStream(); }

    /**
     * <p>Object data.</p>
     */
    inline void ReplaceBody(Aws::IOStream* body) { m_body = Aws::Utils::Stream::ResponseStream(body); }


    /**
     * <p>Specifies whether the object retrieved was (true) or was not (false) a Delete
     * Marker. If false, this response header does not appear in the response.</p>
     */
    inline bool GetDeleteMarker() const{ return m_deleteMarker; }

    /**
     * <p>Specifies whether the object retrieved was (true) or was not (false) a Delete
     * Marker. If false, this response header does not appear in the response.</p>
     */
    inline void SetDeleteMarker(bool value) { m_deleteMarker = value; }

    /**
     * <p>Specifies whether the object retrieved was (true) or was not (false) a Delete
     * Marker. If false, this response header does not appear in the response.</p>
     */
    inline GetObjectResult& WithDeleteMarker(bool value) { SetDeleteMarker(value); return *this;}


    /**
     * <p>Indicates that a range of bytes was specified.</p>
     */
    inline const Aws::String& GetAcceptRanges() const{ return m_acceptRanges; }

    /**
     * <p>Indicates that a range of bytes was specified.</p>
     */
    inline void SetAcceptRanges(const Aws::String& value) { m_acceptRanges = value; }

    /**
     * <p>Indicates that a range of bytes was specified.</p>
     */
    inline void SetAcceptRanges(Aws::String&& value) { m_acceptRanges = std::move(value); }

    /**
     * <p>Indicates that a range of bytes was specified.</p>
     */
    inline void SetAcceptRanges(const char* value) { m_acceptRanges.assign(value); }

    /**
     * <p>Indicates that a range of bytes was specified.</p>
     */
    inline GetObjectResult& WithAcceptRanges(const Aws::String& value) { SetAcceptRanges(value); return *this;}

    /**
     * <p>Indicates that a range of bytes was specified.</p>
     */
    inline GetObjectResult& WithAcceptRanges(Aws::String&& value) { SetAcceptRanges(std::move(value)); return *this;}

    /**
     * <p>Indicates that a range of bytes was specified.</p>
     */
    inline GetObjectResult& WithAcceptRanges(const char* value) { SetAcceptRanges(value); return *this;}


    /**
     * <p>If the object expiration is configured (see PUT Bucket lifecycle), the
     * response includes this header. It includes the expiry-date and rule-id key-value
     * pairs providing object expiration information. The value of the rule-id is URL
     * encoded.</p>
     */
    inline const Aws::String& GetExpiration() const{ return m_expiration; }

    /**
     * <p>If the object expiration is configured (see PUT Bucket lifecycle), the
     * response includes this header. It includes the expiry-date and rule-id key-value
     * pairs providing object expiration information. The value of the rule-id is URL
     * encoded.</p>
     */
    inline void SetExpiration(const Aws::String& value) { m_expiration = value; }

    /**
     * <p>If the object expiration is configured (see PUT Bucket lifecycle), the
     * response includes this header. It includes the expiry-date and rule-id key-value
     * pairs providing object expiration information. The value of the rule-id is URL
     * encoded.</p>
     */
    inline void SetExpiration(Aws::String&& value) { m_expiration = std::move(value); }

    /**
     * <p>If the object expiration is configured (see PUT Bucket lifecycle), the
     * response includes this header. It includes the expiry-date and rule-id key-value
     * pairs providing object expiration information. The value of the rule-id is URL
     * encoded.</p>
     */
    inline void SetExpiration(const char* value) { m_expiration.assign(value); }

    /**
     * <p>If the object expiration is configured (see PUT Bucket lifecycle), the
     * response includes this header. It includes the expiry-date and rule-id key-value
     * pairs providing object expiration information. The value of the rule-id is URL
     * encoded.</p>
     */
    inline GetObjectResult& WithExpiration(const Aws::String& value) { SetExpiration(value); return *this;}

    /**
     * <p>If the object expiration is configured (see PUT Bucket lifecycle), the
     * response includes this header. It includes the expiry-date and rule-id key-value
     * pairs providing object expiration information. The value of the rule-id is URL
     * encoded.</p>
     */
    inline GetObjectResult& WithExpiration(Aws::String&& value) { SetExpiration(std::move(value)); return *this;}

    /**
     * <p>If the object expiration is configured (see PUT Bucket lifecycle), the
     * response includes this header. It includes the expiry-date and rule-id key-value
     * pairs providing object expiration information. The value of the rule-id is URL
     * encoded.</p>
     */
    inline GetObjectResult& WithExpiration(const char* value) { SetExpiration(value); return *this;}


    /**
     * <p>Provides information about object restoration operation and expiration time
     * of the restored object copy.</p>
     */
    inline const Aws::String& GetRestore() const{ return m_restore; }

    /**
     * <p>Provides information about object restoration operation and expiration time
     * of the restored object copy.</p>
     */
    inline void SetRestore(const Aws::String& value) { m_restore = value; }

    /**
     * <p>Provides information about object restoration operation and expiration time
     * of the restored object copy.</p>
     */
    inline void SetRestore(Aws::String&& value) { m_restore = std::move(value); }

    /**
     * <p>Provides information about object restoration operation and expiration time
     * of the restored object copy.</p>
     */
    inline void SetRestore(const char* value) { m_restore.assign(value); }

    /**
     * <p>Provides information about object restoration operation and expiration time
     * of the restored object copy.</p>
     */
    inline GetObjectResult& WithRestore(const Aws::String& value) { SetRestore(value); return *this;}

    /**
     * <p>Provides information about object restoration operation and expiration time
     * of the restored object copy.</p>
     */
    inline GetObjectResult& WithRestore(Aws::String&& value) { SetRestore(std::move(value)); return *this;}

    /**
     * <p>Provides information about object restoration operation and expiration time
     * of the restored object copy.</p>
     */
    inline GetObjectResult& WithRestore(const char* value) { SetRestore(value); return *this;}


    /**
     * <p>Last modified date of the object</p>
     */
    inline const Aws::Utils::DateTime& GetLastModified() const{ return m_lastModified; }

    /**
     * <p>Last modified date of the object</p>
     */
    inline void SetLastModified(const Aws::Utils::DateTime& value) { m_lastModified = value; }

    /**
     * <p>Last modified date of the object</p>
     */
    inline void SetLastModified(Aws::Utils::DateTime&& value) { m_lastModified = std::move(value); }

    /**
     * <p>Last modified date of the object</p>
     */
    inline GetObjectResult& WithLastModified(const Aws::Utils::DateTime& value) { SetLastModified(value); return *this;}

    /**
     * <p>Last modified date of the object</p>
     */
    inline GetObjectResult& WithLastModified(Aws::Utils::DateTime&& value) { SetLastModified(std::move(value)); return *this;}


    /**
     * <p>Size of the body in bytes.</p>
     */
    inline long long GetContentLength() const{ return m_contentLength; }

    /**
     * <p>Size of the body in bytes.</p>
     */
    inline void SetContentLength(long long value) { m_contentLength = value; }

    /**
     * <p>Size of the body in bytes.</p>
     */
    inline GetObjectResult& WithContentLength(long long value) { SetContentLength(value); return *this;}


    /**
     * <p>An ETag is an opaque identifier assigned by a web server to a specific
     * version of a resource found at a URL.</p>
     */
    inline const Aws::String& GetETag() const{ return m_eTag; }

    /**
     * <p>An ETag is an opaque identifier assigned by a web server to a specific
     * version of a resource found at a URL.</p>
     */
    inline void SetETag(const Aws::String& value) { m_eTag = value; }

    /**
     * <p>An ETag is an opaque identifier assigned by a web server to a specific
     * version of a resource found at a URL.</p>
     */
    inline void SetETag(Aws::String&& value) { m_eTag = std::move(value); }

    /**
     * <p>An ETag is an opaque identifier assigned by a web server to a specific
     * version of a resource found at a URL.</p>
     */
    inline void SetETag(const char* value) { m_eTag.assign(value); }

    /**
     * <p>An ETag is an opaque identifier assigned by a web server to a specific
     * version of a resource found at a URL.</p>
     */
    inline GetObjectResult& WithETag(const Aws::String& value) { SetETag(value); return *this;}

    /**
     * <p>An ETag is an opaque identifier assigned by a web server to a specific
     * version of a resource found at a URL.</p>
     */
    inline GetObjectResult& WithETag(Aws::String&& value) { SetETag(std::move(value)); return *this;}

    /**
     * <p>An ETag is an opaque identifier assigned by a web server to a specific
     * version of a resource found at a URL.</p>
     */
    inline GetObjectResult& WithETag(const char* value) { SetETag(value); return *this;}


    /**
     * <p>This is set to the number of metadata entries not returned in
     * <code>x-amz-meta</code> headers. This can happen if you create metadata using an
     * API like SOAP that supports more flexible metadata than the REST API. For
     * example, using SOAP, you can create metadata whose values are not legal HTTP
     * headers.</p>
     */
    inline int GetMissingMeta() const{ return m_missingMeta; }

    /**
     * <p>This is set to the number of metadata entries not returned in
     * <code>x-amz-meta</code> headers. This can happen if you create metadata using an
     * API like SOAP that supports more flexible metadata than the REST API. For
     * example, using SOAP, you can create metadata whose values are not legal HTTP
     * headers.</p>
     */
    inline void SetMissingMeta(int value) { m_missingMeta = value; }

    /**
     * <p>This is set to the number of metadata entries not returned in
     * <code>x-amz-meta</code> headers. This can happen if you create metadata using an
     * API like SOAP that supports more flexible metadata than the REST API. For
     * example, using SOAP, you can create metadata whose values are not legal HTTP
     * headers.</p>
     */
    inline GetObjectResult& WithMissingMeta(int value) { SetMissingMeta(value); return *this;}


    /**
     * <p>Version of the object.</p>
     */
    inline const Aws::String& GetVersionId() const{ return m_versionId; }

    /**
     * <p>Version of the object.</p>
     */
    inline void SetVersionId(const Aws::String& value) { m_versionId = value; }

    /**
     * <p>Version of the object.</p>
     */
    inline void SetVersionId(Aws::String&& value) { m_versionId = std::move(value); }

    /**
     * <p>Version of the object.</p>
     */
    inline void SetVersionId(const char* value) { m_versionId.assign(value); }

    /**
     * <p>Version of the object.</p>
     */
    inline GetObjectResult& WithVersionId(const Aws::String& value) { SetVersionId(value); return *this;}

    /**
     * <p>Version of the object.</p>
     */
    inline GetObjectResult& WithVersionId(Aws::String&& value) { SetVersionId(std::move(value)); return *this;}

    /**
     * <p>Version of the object.</p>
     */
    inline GetObjectResult& WithVersionId(const char* value) { SetVersionId(value); return *this;}


    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline const Aws::String& GetCacheControl() const{ return m_cacheControl; }

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline void SetCacheControl(const Aws::String& value) { m_cacheControl = value; }

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline void SetCacheControl(Aws::String&& value) { m_cacheControl = std::move(value); }

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline void SetCacheControl(const char* value) { m_cacheControl.assign(value); }

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline GetObjectResult& WithCacheControl(const Aws::String& value) { SetCacheControl(value); return *this;}

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline GetObjectResult& WithCacheControl(Aws::String&& value) { SetCacheControl(std::move(value)); return *this;}

    /**
     * <p>Specifies caching behavior along the request/reply chain.</p>
     */
    inline GetObjectResult& WithCacheControl(const char* value) { SetCacheControl(value); return *this;}


    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline const Aws::String& GetContentDisposition() const{ return m_contentDisposition; }

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline void SetContentDisposition(const Aws::String& value) { m_contentDisposition = value; }

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline void SetContentDisposition(Aws::String&& value) { m_contentDisposition = std::move(value); }

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline void SetContentDisposition(const char* value) { m_contentDisposition.assign(value); }

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline GetObjectResult& WithContentDisposition(const Aws::String& value) { SetContentDisposition(value); return *this;}

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline GetObjectResult& WithContentDisposition(Aws::String&& value) { SetContentDisposition(std::move(value)); return *this;}

    /**
     * <p>Specifies presentational information for the object.</p>
     */
    inline GetObjectResult& WithContentDisposition(const char* value) { SetContentDisposition(value); return *this;}


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
    inline void SetContentEncoding(const Aws::String& value) { m_contentEncoding = value; }

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline void SetContentEncoding(Aws::String&& value) { m_contentEncoding = std::move(value); }

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline void SetContentEncoding(const char* value) { m_contentEncoding.assign(value); }

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline GetObjectResult& WithContentEncoding(const Aws::String& value) { SetContentEncoding(value); return *this;}

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline GetObjectResult& WithContentEncoding(Aws::String&& value) { SetContentEncoding(std::move(value)); return *this;}

    /**
     * <p>Specifies what content encodings have been applied to the object and thus
     * what decoding mechanisms must be applied to obtain the media-type referenced by
     * the Content-Type header field.</p>
     */
    inline GetObjectResult& WithContentEncoding(const char* value) { SetContentEncoding(value); return *this;}


    /**
     * <p>The language the content is in.</p>
     */
    inline const Aws::String& GetContentLanguage() const{ return m_contentLanguage; }

    /**
     * <p>The language the content is in.</p>
     */
    inline void SetContentLanguage(const Aws::String& value) { m_contentLanguage = value; }

    /**
     * <p>The language the content is in.</p>
     */
    inline void SetContentLanguage(Aws::String&& value) { m_contentLanguage = std::move(value); }

    /**
     * <p>The language the content is in.</p>
     */
    inline void SetContentLanguage(const char* value) { m_contentLanguage.assign(value); }

    /**
     * <p>The language the content is in.</p>
     */
    inline GetObjectResult& WithContentLanguage(const Aws::String& value) { SetContentLanguage(value); return *this;}

    /**
     * <p>The language the content is in.</p>
     */
    inline GetObjectResult& WithContentLanguage(Aws::String&& value) { SetContentLanguage(std::move(value)); return *this;}

    /**
     * <p>The language the content is in.</p>
     */
    inline GetObjectResult& WithContentLanguage(const char* value) { SetContentLanguage(value); return *this;}


    /**
     * <p>The portion of the object returned in the response.</p>
     */
    inline const Aws::String& GetContentRange() const{ return m_contentRange; }

    /**
     * <p>The portion of the object returned in the response.</p>
     */
    inline void SetContentRange(const Aws::String& value) { m_contentRange = value; }

    /**
     * <p>The portion of the object returned in the response.</p>
     */
    inline void SetContentRange(Aws::String&& value) { m_contentRange = std::move(value); }

    /**
     * <p>The portion of the object returned in the response.</p>
     */
    inline void SetContentRange(const char* value) { m_contentRange.assign(value); }

    /**
     * <p>The portion of the object returned in the response.</p>
     */
    inline GetObjectResult& WithContentRange(const Aws::String& value) { SetContentRange(value); return *this;}

    /**
     * <p>The portion of the object returned in the response.</p>
     */
    inline GetObjectResult& WithContentRange(Aws::String&& value) { SetContentRange(std::move(value)); return *this;}

    /**
     * <p>The portion of the object returned in the response.</p>
     */
    inline GetObjectResult& WithContentRange(const char* value) { SetContentRange(value); return *this;}


    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline const Aws::String& GetContentType() const{ return m_contentType; }

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline void SetContentType(const Aws::String& value) { m_contentType = value; }

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline void SetContentType(Aws::String&& value) { m_contentType = std::move(value); }

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline void SetContentType(const char* value) { m_contentType.assign(value); }

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline GetObjectResult& WithContentType(const Aws::String& value) { SetContentType(value); return *this;}

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline GetObjectResult& WithContentType(Aws::String&& value) { SetContentType(std::move(value)); return *this;}

    /**
     * <p>A standard MIME type describing the format of the object data.</p>
     */
    inline GetObjectResult& WithContentType(const char* value) { SetContentType(value); return *this;}


    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline const Aws::Utils::DateTime& GetExpires() const{ return m_expires; }

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline void SetExpires(const Aws::Utils::DateTime& value) { m_expires = value; }

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline void SetExpires(Aws::Utils::DateTime&& value) { m_expires = std::move(value); }

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline GetObjectResult& WithExpires(const Aws::Utils::DateTime& value) { SetExpires(value); return *this;}

    /**
     * <p>The date and time at which the object is no longer cacheable.</p>
     */
    inline GetObjectResult& WithExpires(Aws::Utils::DateTime&& value) { SetExpires(std::move(value)); return *this;}


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
    inline void SetWebsiteRedirectLocation(const Aws::String& value) { m_websiteRedirectLocation = value; }

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline void SetWebsiteRedirectLocation(Aws::String&& value) { m_websiteRedirectLocation = std::move(value); }

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline void SetWebsiteRedirectLocation(const char* value) { m_websiteRedirectLocation.assign(value); }

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline GetObjectResult& WithWebsiteRedirectLocation(const Aws::String& value) { SetWebsiteRedirectLocation(value); return *this;}

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline GetObjectResult& WithWebsiteRedirectLocation(Aws::String&& value) { SetWebsiteRedirectLocation(std::move(value)); return *this;}

    /**
     * <p>If the bucket is configured as a website, redirects requests for this object
     * to another object in the same bucket or to an external URL. Amazon S3 stores the
     * value of this header in the object metadata.</p>
     */
    inline GetObjectResult& WithWebsiteRedirectLocation(const char* value) { SetWebsiteRedirectLocation(value); return *this;}


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
    inline GetObjectResult& WithServerSideEncryption(const ServerSideEncryption& value) { SetServerSideEncryption(value); return *this;}

    /**
     * <p>The server-side encryption algorithm used when storing this object in Amazon
     * S3 (for example, AES256, aws:kms).</p>
     */
    inline GetObjectResult& WithServerSideEncryption(ServerSideEncryption&& value) { SetServerSideEncryption(std::move(value)); return *this;}


    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline const Aws::Map<Aws::String, Aws::String>& GetMetadata() const{ return m_metadata; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline void SetMetadata(const Aws::Map<Aws::String, Aws::String>& value) { m_metadata = value; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline void SetMetadata(Aws::Map<Aws::String, Aws::String>&& value) { m_metadata = std::move(value); }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline GetObjectResult& WithMetadata(const Aws::Map<Aws::String, Aws::String>& value) { SetMetadata(value); return *this;}

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline GetObjectResult& WithMetadata(Aws::Map<Aws::String, Aws::String>&& value) { SetMetadata(std::move(value)); return *this;}

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline GetObjectResult& AddMetadata(const Aws::String& key, const Aws::String& value) { m_metadata.emplace(key, value); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline GetObjectResult& AddMetadata(Aws::String&& key, const Aws::String& value) { m_metadata.emplace(std::move(key), value); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline GetObjectResult& AddMetadata(const Aws::String& key, Aws::String&& value) { m_metadata.emplace(key, std::move(value)); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline GetObjectResult& AddMetadata(Aws::String&& key, Aws::String&& value) { m_metadata.emplace(std::move(key), std::move(value)); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline GetObjectResult& AddMetadata(const char* key, Aws::String&& value) { m_metadata.emplace(key, std::move(value)); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline GetObjectResult& AddMetadata(Aws::String&& key, const char* value) { m_metadata.emplace(std::move(key), value); return *this; }

    /**
     * <p>A map of metadata to store with the object in S3.</p>
     */
    inline GetObjectResult& AddMetadata(const char* key, const char* value) { m_metadata.emplace(key, value); return *this; }


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
    inline GetObjectResult& WithSSECustomerAlgorithm(const Aws::String& value) { SetSSECustomerAlgorithm(value); return *this;}

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header confirming the encryption
     * algorithm used.</p>
     */
    inline GetObjectResult& WithSSECustomerAlgorithm(Aws::String&& value) { SetSSECustomerAlgorithm(std::move(value)); return *this;}

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header confirming the encryption
     * algorithm used.</p>
     */
    inline GetObjectResult& WithSSECustomerAlgorithm(const char* value) { SetSSECustomerAlgorithm(value); return *this;}


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
    inline GetObjectResult& WithSSECustomerKeyMD5(const Aws::String& value) { SetSSECustomerKeyMD5(value); return *this;}

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header to provide round-trip message
     * integrity verification of the customer-provided encryption key.</p>
     */
    inline GetObjectResult& WithSSECustomerKeyMD5(Aws::String&& value) { SetSSECustomerKeyMD5(std::move(value)); return *this;}

    /**
     * <p>If server-side encryption with a customer-provided encryption key was
     * requested, the response will include this header to provide round-trip message
     * integrity verification of the customer-provided encryption key.</p>
     */
    inline GetObjectResult& WithSSECustomerKeyMD5(const char* value) { SetSSECustomerKeyMD5(value); return *this;}


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
    inline GetObjectResult& WithSSEKMSKeyId(const Aws::String& value) { SetSSEKMSKeyId(value); return *this;}

    /**
     * <p>If present, specifies the ID of the AWS Key Management Service (AWS KMS)
     * symmetric customer managed customer master key (CMK) that was used for the
     * object.</p>
     */
    inline GetObjectResult& WithSSEKMSKeyId(Aws::String&& value) { SetSSEKMSKeyId(std::move(value)); return *this;}

    /**
     * <p>If present, specifies the ID of the AWS Key Management Service (AWS KMS)
     * symmetric customer managed customer master key (CMK) that was used for the
     * object.</p>
     */
    inline GetObjectResult& WithSSEKMSKeyId(const char* value) { SetSSEKMSKeyId(value); return *this;}


    /**
     * <p>Provides storage class information of the object. Amazon S3 returns this
     * header for all objects except for Standard storage class objects.</p>
     */
    inline const StorageClass& GetStorageClass() const{ return m_storageClass; }

    /**
     * <p>Provides storage class information of the object. Amazon S3 returns this
     * header for all objects except for Standard storage class objects.</p>
     */
    inline void SetStorageClass(const StorageClass& value) { m_storageClass = value; }

    /**
     * <p>Provides storage class information of the object. Amazon S3 returns this
     * header for all objects except for Standard storage class objects.</p>
     */
    inline void SetStorageClass(StorageClass&& value) { m_storageClass = std::move(value); }

    /**
     * <p>Provides storage class information of the object. Amazon S3 returns this
     * header for all objects except for Standard storage class objects.</p>
     */
    inline GetObjectResult& WithStorageClass(const StorageClass& value) { SetStorageClass(value); return *this;}

    /**
     * <p>Provides storage class information of the object. Amazon S3 returns this
     * header for all objects except for Standard storage class objects.</p>
     */
    inline GetObjectResult& WithStorageClass(StorageClass&& value) { SetStorageClass(std::move(value)); return *this;}


    
    inline const RequestCharged& GetRequestCharged() const{ return m_requestCharged; }

    
    inline void SetRequestCharged(const RequestCharged& value) { m_requestCharged = value; }

    
    inline void SetRequestCharged(RequestCharged&& value) { m_requestCharged = std::move(value); }

    
    inline GetObjectResult& WithRequestCharged(const RequestCharged& value) { SetRequestCharged(value); return *this;}

    
    inline GetObjectResult& WithRequestCharged(RequestCharged&& value) { SetRequestCharged(std::move(value)); return *this;}


    /**
     * <p>Amazon S3 can return this if your request involves a bucket that is either a
     * source or destination in a replication rule.</p>
     */
    inline const ReplicationStatus& GetReplicationStatus() const{ return m_replicationStatus; }

    /**
     * <p>Amazon S3 can return this if your request involves a bucket that is either a
     * source or destination in a replication rule.</p>
     */
    inline void SetReplicationStatus(const ReplicationStatus& value) { m_replicationStatus = value; }

    /**
     * <p>Amazon S3 can return this if your request involves a bucket that is either a
     * source or destination in a replication rule.</p>
     */
    inline void SetReplicationStatus(ReplicationStatus&& value) { m_replicationStatus = std::move(value); }

    /**
     * <p>Amazon S3 can return this if your request involves a bucket that is either a
     * source or destination in a replication rule.</p>
     */
    inline GetObjectResult& WithReplicationStatus(const ReplicationStatus& value) { SetReplicationStatus(value); return *this;}

    /**
     * <p>Amazon S3 can return this if your request involves a bucket that is either a
     * source or destination in a replication rule.</p>
     */
    inline GetObjectResult& WithReplicationStatus(ReplicationStatus&& value) { SetReplicationStatus(std::move(value)); return *this;}


    /**
     * <p>The count of parts this object has.</p>
     */
    inline int GetPartsCount() const{ return m_partsCount; }

    /**
     * <p>The count of parts this object has.</p>
     */
    inline void SetPartsCount(int value) { m_partsCount = value; }

    /**
     * <p>The count of parts this object has.</p>
     */
    inline GetObjectResult& WithPartsCount(int value) { SetPartsCount(value); return *this;}


    /**
     * <p>The number of tags, if any, on the object.</p>
     */
    inline int GetTagCount() const{ return m_tagCount; }

    /**
     * <p>The number of tags, if any, on the object.</p>
     */
    inline void SetTagCount(int value) { m_tagCount = value; }

    /**
     * <p>The number of tags, if any, on the object.</p>
     */
    inline GetObjectResult& WithTagCount(int value) { SetTagCount(value); return *this;}


    /**
     * <p>The Object Lock mode currently in place for this object.</p>
     */
    inline const ObjectLockMode& GetObjectLockMode() const{ return m_objectLockMode; }

    /**
     * <p>The Object Lock mode currently in place for this object.</p>
     */
    inline void SetObjectLockMode(const ObjectLockMode& value) { m_objectLockMode = value; }

    /**
     * <p>The Object Lock mode currently in place for this object.</p>
     */
    inline void SetObjectLockMode(ObjectLockMode&& value) { m_objectLockMode = std::move(value); }

    /**
     * <p>The Object Lock mode currently in place for this object.</p>
     */
    inline GetObjectResult& WithObjectLockMode(const ObjectLockMode& value) { SetObjectLockMode(value); return *this;}

    /**
     * <p>The Object Lock mode currently in place for this object.</p>
     */
    inline GetObjectResult& WithObjectLockMode(ObjectLockMode&& value) { SetObjectLockMode(std::move(value)); return *this;}


    /**
     * <p>The date and time when this object's Object Lock will expire.</p>
     */
    inline const Aws::Utils::DateTime& GetObjectLockRetainUntilDate() const{ return m_objectLockRetainUntilDate; }

    /**
     * <p>The date and time when this object's Object Lock will expire.</p>
     */
    inline void SetObjectLockRetainUntilDate(const Aws::Utils::DateTime& value) { m_objectLockRetainUntilDate = value; }

    /**
     * <p>The date and time when this object's Object Lock will expire.</p>
     */
    inline void SetObjectLockRetainUntilDate(Aws::Utils::DateTime&& value) { m_objectLockRetainUntilDate = std::move(value); }

    /**
     * <p>The date and time when this object's Object Lock will expire.</p>
     */
    inline GetObjectResult& WithObjectLockRetainUntilDate(const Aws::Utils::DateTime& value) { SetObjectLockRetainUntilDate(value); return *this;}

    /**
     * <p>The date and time when this object's Object Lock will expire.</p>
     */
    inline GetObjectResult& WithObjectLockRetainUntilDate(Aws::Utils::DateTime&& value) { SetObjectLockRetainUntilDate(std::move(value)); return *this;}


    /**
     * <p>Indicates whether this object has an active legal hold. This field is only
     * returned if you have permission to view an object's legal hold status. </p>
     */
    inline const ObjectLockLegalHoldStatus& GetObjectLockLegalHoldStatus() const{ return m_objectLockLegalHoldStatus; }

    /**
     * <p>Indicates whether this object has an active legal hold. This field is only
     * returned if you have permission to view an object's legal hold status. </p>
     */
    inline void SetObjectLockLegalHoldStatus(const ObjectLockLegalHoldStatus& value) { m_objectLockLegalHoldStatus = value; }

    /**
     * <p>Indicates whether this object has an active legal hold. This field is only
     * returned if you have permission to view an object's legal hold status. </p>
     */
    inline void SetObjectLockLegalHoldStatus(ObjectLockLegalHoldStatus&& value) { m_objectLockLegalHoldStatus = std::move(value); }

    /**
     * <p>Indicates whether this object has an active legal hold. This field is only
     * returned if you have permission to view an object's legal hold status. </p>
     */
    inline GetObjectResult& WithObjectLockLegalHoldStatus(const ObjectLockLegalHoldStatus& value) { SetObjectLockLegalHoldStatus(value); return *this;}

    /**
     * <p>Indicates whether this object has an active legal hold. This field is only
     * returned if you have permission to view an object's legal hold status. </p>
     */
    inline GetObjectResult& WithObjectLockLegalHoldStatus(ObjectLockLegalHoldStatus&& value) { SetObjectLockLegalHoldStatus(std::move(value)); return *this;}


    
    inline const Aws::String& GetId2() const{ return m_id2; }

    
    inline void SetId2(const Aws::String& value) { m_id2 = value; }

    
    inline void SetId2(Aws::String&& value) { m_id2 = std::move(value); }

    
    inline void SetId2(const char* value) { m_id2.assign(value); }

    
    inline GetObjectResult& WithId2(const Aws::String& value) { SetId2(value); return *this;}

    
    inline GetObjectResult& WithId2(Aws::String&& value) { SetId2(std::move(value)); return *this;}

    
    inline GetObjectResult& WithId2(const char* value) { SetId2(value); return *this;}


    
    inline const Aws::String& GetRequestId() const{ return m_requestId; }

    
    inline void SetRequestId(const Aws::String& value) { m_requestId = value; }

    
    inline void SetRequestId(Aws::String&& value) { m_requestId = std::move(value); }

    
    inline void SetRequestId(const char* value) { m_requestId.assign(value); }

    
    inline GetObjectResult& WithRequestId(const Aws::String& value) { SetRequestId(value); return *this;}

    
    inline GetObjectResult& WithRequestId(Aws::String&& value) { SetRequestId(std::move(value)); return *this;}

    
    inline GetObjectResult& WithRequestId(const char* value) { SetRequestId(value); return *this;}

  private:

  Aws::Utils::Stream::ResponseStream m_body;

    bool m_deleteMarker;

    Aws::String m_acceptRanges;

    Aws::String m_expiration;

    Aws::String m_restore;

    Aws::Utils::DateTime m_lastModified;

    long long m_contentLength;

    Aws::String m_eTag;

    int m_missingMeta;

    Aws::String m_versionId;

    Aws::String m_cacheControl;

    Aws::String m_contentDisposition;

    Aws::String m_contentEncoding;

    Aws::String m_contentLanguage;

    Aws::String m_contentRange;

    Aws::String m_contentType;

    Aws::Utils::DateTime m_expires;

    Aws::String m_websiteRedirectLocation;

    ServerSideEncryption m_serverSideEncryption;

    Aws::Map<Aws::String, Aws::String> m_metadata;

    Aws::String m_sSECustomerAlgorithm;

    Aws::String m_sSECustomerKeyMD5;

    Aws::String m_sSEKMSKeyId;

    StorageClass m_storageClass;

    RequestCharged m_requestCharged;

    ReplicationStatus m_replicationStatus;

    int m_partsCount;

    int m_tagCount;

    ObjectLockMode m_objectLockMode;

    Aws::Utils::DateTime m_objectLockRetainUntilDate;

    ObjectLockLegalHoldStatus m_objectLockLegalHoldStatus;

    Aws::String m_id2;

    Aws::String m_requestId;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
