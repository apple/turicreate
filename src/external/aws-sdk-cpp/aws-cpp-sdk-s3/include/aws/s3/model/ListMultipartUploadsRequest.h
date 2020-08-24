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
#include <aws/s3/model/EncodingType.h>
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
  class AWS_S3_API ListMultipartUploadsRequest : public S3Request
  {
  public:
    ListMultipartUploadsRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "ListMultipartUploads"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;


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
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

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
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

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
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

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
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

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
    inline ListMultipartUploadsRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

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
    inline ListMultipartUploadsRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

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
    inline ListMultipartUploadsRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>Character you use to group keys.</p> <p>All keys that contain the same string
     * between the prefix, if specified, and the first occurrence of the delimiter
     * after the prefix are grouped under a single result element,
     * <code>CommonPrefixes</code>. If you don't specify the prefix parameter, then the
     * substring starts at the beginning of the key. The keys that are grouped under
     * <code>CommonPrefixes</code> result element are not returned elsewhere in the
     * response.</p>
     */
    inline const Aws::String& GetDelimiter() const{ return m_delimiter; }

    /**
     * <p>Character you use to group keys.</p> <p>All keys that contain the same string
     * between the prefix, if specified, and the first occurrence of the delimiter
     * after the prefix are grouped under a single result element,
     * <code>CommonPrefixes</code>. If you don't specify the prefix parameter, then the
     * substring starts at the beginning of the key. The keys that are grouped under
     * <code>CommonPrefixes</code> result element are not returned elsewhere in the
     * response.</p>
     */
    inline bool DelimiterHasBeenSet() const { return m_delimiterHasBeenSet; }

    /**
     * <p>Character you use to group keys.</p> <p>All keys that contain the same string
     * between the prefix, if specified, and the first occurrence of the delimiter
     * after the prefix are grouped under a single result element,
     * <code>CommonPrefixes</code>. If you don't specify the prefix parameter, then the
     * substring starts at the beginning of the key. The keys that are grouped under
     * <code>CommonPrefixes</code> result element are not returned elsewhere in the
     * response.</p>
     */
    inline void SetDelimiter(const Aws::String& value) { m_delimiterHasBeenSet = true; m_delimiter = value; }

    /**
     * <p>Character you use to group keys.</p> <p>All keys that contain the same string
     * between the prefix, if specified, and the first occurrence of the delimiter
     * after the prefix are grouped under a single result element,
     * <code>CommonPrefixes</code>. If you don't specify the prefix parameter, then the
     * substring starts at the beginning of the key. The keys that are grouped under
     * <code>CommonPrefixes</code> result element are not returned elsewhere in the
     * response.</p>
     */
    inline void SetDelimiter(Aws::String&& value) { m_delimiterHasBeenSet = true; m_delimiter = std::move(value); }

    /**
     * <p>Character you use to group keys.</p> <p>All keys that contain the same string
     * between the prefix, if specified, and the first occurrence of the delimiter
     * after the prefix are grouped under a single result element,
     * <code>CommonPrefixes</code>. If you don't specify the prefix parameter, then the
     * substring starts at the beginning of the key. The keys that are grouped under
     * <code>CommonPrefixes</code> result element are not returned elsewhere in the
     * response.</p>
     */
    inline void SetDelimiter(const char* value) { m_delimiterHasBeenSet = true; m_delimiter.assign(value); }

    /**
     * <p>Character you use to group keys.</p> <p>All keys that contain the same string
     * between the prefix, if specified, and the first occurrence of the delimiter
     * after the prefix are grouped under a single result element,
     * <code>CommonPrefixes</code>. If you don't specify the prefix parameter, then the
     * substring starts at the beginning of the key. The keys that are grouped under
     * <code>CommonPrefixes</code> result element are not returned elsewhere in the
     * response.</p>
     */
    inline ListMultipartUploadsRequest& WithDelimiter(const Aws::String& value) { SetDelimiter(value); return *this;}

    /**
     * <p>Character you use to group keys.</p> <p>All keys that contain the same string
     * between the prefix, if specified, and the first occurrence of the delimiter
     * after the prefix are grouped under a single result element,
     * <code>CommonPrefixes</code>. If you don't specify the prefix parameter, then the
     * substring starts at the beginning of the key. The keys that are grouped under
     * <code>CommonPrefixes</code> result element are not returned elsewhere in the
     * response.</p>
     */
    inline ListMultipartUploadsRequest& WithDelimiter(Aws::String&& value) { SetDelimiter(std::move(value)); return *this;}

    /**
     * <p>Character you use to group keys.</p> <p>All keys that contain the same string
     * between the prefix, if specified, and the first occurrence of the delimiter
     * after the prefix are grouped under a single result element,
     * <code>CommonPrefixes</code>. If you don't specify the prefix parameter, then the
     * substring starts at the beginning of the key. The keys that are grouped under
     * <code>CommonPrefixes</code> result element are not returned elsewhere in the
     * response.</p>
     */
    inline ListMultipartUploadsRequest& WithDelimiter(const char* value) { SetDelimiter(value); return *this;}


    
    inline const EncodingType& GetEncodingType() const{ return m_encodingType; }

    
    inline bool EncodingTypeHasBeenSet() const { return m_encodingTypeHasBeenSet; }

    
    inline void SetEncodingType(const EncodingType& value) { m_encodingTypeHasBeenSet = true; m_encodingType = value; }

    
    inline void SetEncodingType(EncodingType&& value) { m_encodingTypeHasBeenSet = true; m_encodingType = std::move(value); }

    
    inline ListMultipartUploadsRequest& WithEncodingType(const EncodingType& value) { SetEncodingType(value); return *this;}

    
    inline ListMultipartUploadsRequest& WithEncodingType(EncodingType&& value) { SetEncodingType(std::move(value)); return *this;}


    /**
     * <p>Together with upload-id-marker, this parameter specifies the multipart upload
     * after which listing should begin.</p> <p>If <code>upload-id-marker</code> is not
     * specified, only the keys lexicographically greater than the specified
     * <code>key-marker</code> will be included in the list.</p> <p>If
     * <code>upload-id-marker</code> is specified, any multipart uploads for a key
     * equal to the <code>key-marker</code> might also be included, provided those
     * multipart uploads have upload IDs lexicographically greater than the specified
     * <code>upload-id-marker</code>.</p>
     */
    inline const Aws::String& GetKeyMarker() const{ return m_keyMarker; }

    /**
     * <p>Together with upload-id-marker, this parameter specifies the multipart upload
     * after which listing should begin.</p> <p>If <code>upload-id-marker</code> is not
     * specified, only the keys lexicographically greater than the specified
     * <code>key-marker</code> will be included in the list.</p> <p>If
     * <code>upload-id-marker</code> is specified, any multipart uploads for a key
     * equal to the <code>key-marker</code> might also be included, provided those
     * multipart uploads have upload IDs lexicographically greater than the specified
     * <code>upload-id-marker</code>.</p>
     */
    inline bool KeyMarkerHasBeenSet() const { return m_keyMarkerHasBeenSet; }

    /**
     * <p>Together with upload-id-marker, this parameter specifies the multipart upload
     * after which listing should begin.</p> <p>If <code>upload-id-marker</code> is not
     * specified, only the keys lexicographically greater than the specified
     * <code>key-marker</code> will be included in the list.</p> <p>If
     * <code>upload-id-marker</code> is specified, any multipart uploads for a key
     * equal to the <code>key-marker</code> might also be included, provided those
     * multipart uploads have upload IDs lexicographically greater than the specified
     * <code>upload-id-marker</code>.</p>
     */
    inline void SetKeyMarker(const Aws::String& value) { m_keyMarkerHasBeenSet = true; m_keyMarker = value; }

    /**
     * <p>Together with upload-id-marker, this parameter specifies the multipart upload
     * after which listing should begin.</p> <p>If <code>upload-id-marker</code> is not
     * specified, only the keys lexicographically greater than the specified
     * <code>key-marker</code> will be included in the list.</p> <p>If
     * <code>upload-id-marker</code> is specified, any multipart uploads for a key
     * equal to the <code>key-marker</code> might also be included, provided those
     * multipart uploads have upload IDs lexicographically greater than the specified
     * <code>upload-id-marker</code>.</p>
     */
    inline void SetKeyMarker(Aws::String&& value) { m_keyMarkerHasBeenSet = true; m_keyMarker = std::move(value); }

    /**
     * <p>Together with upload-id-marker, this parameter specifies the multipart upload
     * after which listing should begin.</p> <p>If <code>upload-id-marker</code> is not
     * specified, only the keys lexicographically greater than the specified
     * <code>key-marker</code> will be included in the list.</p> <p>If
     * <code>upload-id-marker</code> is specified, any multipart uploads for a key
     * equal to the <code>key-marker</code> might also be included, provided those
     * multipart uploads have upload IDs lexicographically greater than the specified
     * <code>upload-id-marker</code>.</p>
     */
    inline void SetKeyMarker(const char* value) { m_keyMarkerHasBeenSet = true; m_keyMarker.assign(value); }

    /**
     * <p>Together with upload-id-marker, this parameter specifies the multipart upload
     * after which listing should begin.</p> <p>If <code>upload-id-marker</code> is not
     * specified, only the keys lexicographically greater than the specified
     * <code>key-marker</code> will be included in the list.</p> <p>If
     * <code>upload-id-marker</code> is specified, any multipart uploads for a key
     * equal to the <code>key-marker</code> might also be included, provided those
     * multipart uploads have upload IDs lexicographically greater than the specified
     * <code>upload-id-marker</code>.</p>
     */
    inline ListMultipartUploadsRequest& WithKeyMarker(const Aws::String& value) { SetKeyMarker(value); return *this;}

    /**
     * <p>Together with upload-id-marker, this parameter specifies the multipart upload
     * after which listing should begin.</p> <p>If <code>upload-id-marker</code> is not
     * specified, only the keys lexicographically greater than the specified
     * <code>key-marker</code> will be included in the list.</p> <p>If
     * <code>upload-id-marker</code> is specified, any multipart uploads for a key
     * equal to the <code>key-marker</code> might also be included, provided those
     * multipart uploads have upload IDs lexicographically greater than the specified
     * <code>upload-id-marker</code>.</p>
     */
    inline ListMultipartUploadsRequest& WithKeyMarker(Aws::String&& value) { SetKeyMarker(std::move(value)); return *this;}

    /**
     * <p>Together with upload-id-marker, this parameter specifies the multipart upload
     * after which listing should begin.</p> <p>If <code>upload-id-marker</code> is not
     * specified, only the keys lexicographically greater than the specified
     * <code>key-marker</code> will be included in the list.</p> <p>If
     * <code>upload-id-marker</code> is specified, any multipart uploads for a key
     * equal to the <code>key-marker</code> might also be included, provided those
     * multipart uploads have upload IDs lexicographically greater than the specified
     * <code>upload-id-marker</code>.</p>
     */
    inline ListMultipartUploadsRequest& WithKeyMarker(const char* value) { SetKeyMarker(value); return *this;}


    /**
     * <p>Sets the maximum number of multipart uploads, from 1 to 1,000, to return in
     * the response body. 1,000 is the maximum number of uploads that can be returned
     * in a response.</p>
     */
    inline int GetMaxUploads() const{ return m_maxUploads; }

    /**
     * <p>Sets the maximum number of multipart uploads, from 1 to 1,000, to return in
     * the response body. 1,000 is the maximum number of uploads that can be returned
     * in a response.</p>
     */
    inline bool MaxUploadsHasBeenSet() const { return m_maxUploadsHasBeenSet; }

    /**
     * <p>Sets the maximum number of multipart uploads, from 1 to 1,000, to return in
     * the response body. 1,000 is the maximum number of uploads that can be returned
     * in a response.</p>
     */
    inline void SetMaxUploads(int value) { m_maxUploadsHasBeenSet = true; m_maxUploads = value; }

    /**
     * <p>Sets the maximum number of multipart uploads, from 1 to 1,000, to return in
     * the response body. 1,000 is the maximum number of uploads that can be returned
     * in a response.</p>
     */
    inline ListMultipartUploadsRequest& WithMaxUploads(int value) { SetMaxUploads(value); return *this;}


    /**
     * <p>Lists in-progress uploads only for those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different grouping of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.)</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>Lists in-progress uploads only for those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different grouping of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.)</p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>Lists in-progress uploads only for those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different grouping of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.)</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>Lists in-progress uploads only for those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different grouping of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.)</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>Lists in-progress uploads only for those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different grouping of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.)</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>Lists in-progress uploads only for those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different grouping of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.)</p>
     */
    inline ListMultipartUploadsRequest& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>Lists in-progress uploads only for those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different grouping of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.)</p>
     */
    inline ListMultipartUploadsRequest& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>Lists in-progress uploads only for those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different grouping of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.)</p>
     */
    inline ListMultipartUploadsRequest& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>Together with key-marker, specifies the multipart upload after which listing
     * should begin. If key-marker is not specified, the upload-id-marker parameter is
     * ignored. Otherwise, any multipart uploads for a key equal to the key-marker
     * might be included in the list only if they have an upload ID lexicographically
     * greater than the specified <code>upload-id-marker</code>.</p>
     */
    inline const Aws::String& GetUploadIdMarker() const{ return m_uploadIdMarker; }

    /**
     * <p>Together with key-marker, specifies the multipart upload after which listing
     * should begin. If key-marker is not specified, the upload-id-marker parameter is
     * ignored. Otherwise, any multipart uploads for a key equal to the key-marker
     * might be included in the list only if they have an upload ID lexicographically
     * greater than the specified <code>upload-id-marker</code>.</p>
     */
    inline bool UploadIdMarkerHasBeenSet() const { return m_uploadIdMarkerHasBeenSet; }

    /**
     * <p>Together with key-marker, specifies the multipart upload after which listing
     * should begin. If key-marker is not specified, the upload-id-marker parameter is
     * ignored. Otherwise, any multipart uploads for a key equal to the key-marker
     * might be included in the list only if they have an upload ID lexicographically
     * greater than the specified <code>upload-id-marker</code>.</p>
     */
    inline void SetUploadIdMarker(const Aws::String& value) { m_uploadIdMarkerHasBeenSet = true; m_uploadIdMarker = value; }

    /**
     * <p>Together with key-marker, specifies the multipart upload after which listing
     * should begin. If key-marker is not specified, the upload-id-marker parameter is
     * ignored. Otherwise, any multipart uploads for a key equal to the key-marker
     * might be included in the list only if they have an upload ID lexicographically
     * greater than the specified <code>upload-id-marker</code>.</p>
     */
    inline void SetUploadIdMarker(Aws::String&& value) { m_uploadIdMarkerHasBeenSet = true; m_uploadIdMarker = std::move(value); }

    /**
     * <p>Together with key-marker, specifies the multipart upload after which listing
     * should begin. If key-marker is not specified, the upload-id-marker parameter is
     * ignored. Otherwise, any multipart uploads for a key equal to the key-marker
     * might be included in the list only if they have an upload ID lexicographically
     * greater than the specified <code>upload-id-marker</code>.</p>
     */
    inline void SetUploadIdMarker(const char* value) { m_uploadIdMarkerHasBeenSet = true; m_uploadIdMarker.assign(value); }

    /**
     * <p>Together with key-marker, specifies the multipart upload after which listing
     * should begin. If key-marker is not specified, the upload-id-marker parameter is
     * ignored. Otherwise, any multipart uploads for a key equal to the key-marker
     * might be included in the list only if they have an upload ID lexicographically
     * greater than the specified <code>upload-id-marker</code>.</p>
     */
    inline ListMultipartUploadsRequest& WithUploadIdMarker(const Aws::String& value) { SetUploadIdMarker(value); return *this;}

    /**
     * <p>Together with key-marker, specifies the multipart upload after which listing
     * should begin. If key-marker is not specified, the upload-id-marker parameter is
     * ignored. Otherwise, any multipart uploads for a key equal to the key-marker
     * might be included in the list only if they have an upload ID lexicographically
     * greater than the specified <code>upload-id-marker</code>.</p>
     */
    inline ListMultipartUploadsRequest& WithUploadIdMarker(Aws::String&& value) { SetUploadIdMarker(std::move(value)); return *this;}

    /**
     * <p>Together with key-marker, specifies the multipart upload after which listing
     * should begin. If key-marker is not specified, the upload-id-marker parameter is
     * ignored. Otherwise, any multipart uploads for a key equal to the key-marker
     * might be included in the list only if they have an upload ID lexicographically
     * greater than the specified <code>upload-id-marker</code>.</p>
     */
    inline ListMultipartUploadsRequest& WithUploadIdMarker(const char* value) { SetUploadIdMarker(value); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline ListMultipartUploadsRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline ListMultipartUploadsRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline ListMultipartUploadsRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline ListMultipartUploadsRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline ListMultipartUploadsRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline ListMultipartUploadsRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline ListMultipartUploadsRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline ListMultipartUploadsRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline ListMultipartUploadsRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_delimiter;
    bool m_delimiterHasBeenSet;

    EncodingType m_encodingType;
    bool m_encodingTypeHasBeenSet;

    Aws::String m_keyMarker;
    bool m_keyMarkerHasBeenSet;

    int m_maxUploads;
    bool m_maxUploadsHasBeenSet;

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    Aws::String m_uploadIdMarker;
    bool m_uploadIdMarkerHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
