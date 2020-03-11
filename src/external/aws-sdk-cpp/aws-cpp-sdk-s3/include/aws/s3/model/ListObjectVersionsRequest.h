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
  class AWS_S3_API ListObjectVersionsRequest : public S3Request
  {
  public:
    ListObjectVersionsRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "ListObjectVersions"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;


    /**
     * <p>The bucket name that contains the objects. </p> <p>When using this API with
     * an access point, you must direct requests to the access point hostname. The
     * access point hostname takes the form
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
     * <p>The bucket name that contains the objects. </p> <p>When using this API with
     * an access point, you must direct requests to the access point hostname. The
     * access point hostname takes the form
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
     * <p>The bucket name that contains the objects. </p> <p>When using this API with
     * an access point, you must direct requests to the access point hostname. The
     * access point hostname takes the form
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
     * <p>The bucket name that contains the objects. </p> <p>When using this API with
     * an access point, you must direct requests to the access point hostname. The
     * access point hostname takes the form
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
     * <p>The bucket name that contains the objects. </p> <p>When using this API with
     * an access point, you must direct requests to the access point hostname. The
     * access point hostname takes the form
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
     * <p>The bucket name that contains the objects. </p> <p>When using this API with
     * an access point, you must direct requests to the access point hostname. The
     * access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ListObjectVersionsRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The bucket name that contains the objects. </p> <p>When using this API with
     * an access point, you must direct requests to the access point hostname. The
     * access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ListObjectVersionsRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The bucket name that contains the objects. </p> <p>When using this API with
     * an access point, you must direct requests to the access point hostname. The
     * access point hostname takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ListObjectVersionsRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>A delimiter is a character that you specify to group keys. All keys that
     * contain the same string between the <code>prefix</code> and the first occurrence
     * of the delimiter are grouped under a single result element in CommonPrefixes.
     * These groups are counted as one result against the max-keys limitation. These
     * keys are not returned elsewhere in the response.</p>
     */
    inline const Aws::String& GetDelimiter() const{ return m_delimiter; }

    /**
     * <p>A delimiter is a character that you specify to group keys. All keys that
     * contain the same string between the <code>prefix</code> and the first occurrence
     * of the delimiter are grouped under a single result element in CommonPrefixes.
     * These groups are counted as one result against the max-keys limitation. These
     * keys are not returned elsewhere in the response.</p>
     */
    inline bool DelimiterHasBeenSet() const { return m_delimiterHasBeenSet; }

    /**
     * <p>A delimiter is a character that you specify to group keys. All keys that
     * contain the same string between the <code>prefix</code> and the first occurrence
     * of the delimiter are grouped under a single result element in CommonPrefixes.
     * These groups are counted as one result against the max-keys limitation. These
     * keys are not returned elsewhere in the response.</p>
     */
    inline void SetDelimiter(const Aws::String& value) { m_delimiterHasBeenSet = true; m_delimiter = value; }

    /**
     * <p>A delimiter is a character that you specify to group keys. All keys that
     * contain the same string between the <code>prefix</code> and the first occurrence
     * of the delimiter are grouped under a single result element in CommonPrefixes.
     * These groups are counted as one result against the max-keys limitation. These
     * keys are not returned elsewhere in the response.</p>
     */
    inline void SetDelimiter(Aws::String&& value) { m_delimiterHasBeenSet = true; m_delimiter = std::move(value); }

    /**
     * <p>A delimiter is a character that you specify to group keys. All keys that
     * contain the same string between the <code>prefix</code> and the first occurrence
     * of the delimiter are grouped under a single result element in CommonPrefixes.
     * These groups are counted as one result against the max-keys limitation. These
     * keys are not returned elsewhere in the response.</p>
     */
    inline void SetDelimiter(const char* value) { m_delimiterHasBeenSet = true; m_delimiter.assign(value); }

    /**
     * <p>A delimiter is a character that you specify to group keys. All keys that
     * contain the same string between the <code>prefix</code> and the first occurrence
     * of the delimiter are grouped under a single result element in CommonPrefixes.
     * These groups are counted as one result against the max-keys limitation. These
     * keys are not returned elsewhere in the response.</p>
     */
    inline ListObjectVersionsRequest& WithDelimiter(const Aws::String& value) { SetDelimiter(value); return *this;}

    /**
     * <p>A delimiter is a character that you specify to group keys. All keys that
     * contain the same string between the <code>prefix</code> and the first occurrence
     * of the delimiter are grouped under a single result element in CommonPrefixes.
     * These groups are counted as one result against the max-keys limitation. These
     * keys are not returned elsewhere in the response.</p>
     */
    inline ListObjectVersionsRequest& WithDelimiter(Aws::String&& value) { SetDelimiter(std::move(value)); return *this;}

    /**
     * <p>A delimiter is a character that you specify to group keys. All keys that
     * contain the same string between the <code>prefix</code> and the first occurrence
     * of the delimiter are grouped under a single result element in CommonPrefixes.
     * These groups are counted as one result against the max-keys limitation. These
     * keys are not returned elsewhere in the response.</p>
     */
    inline ListObjectVersionsRequest& WithDelimiter(const char* value) { SetDelimiter(value); return *this;}


    
    inline const EncodingType& GetEncodingType() const{ return m_encodingType; }

    
    inline bool EncodingTypeHasBeenSet() const { return m_encodingTypeHasBeenSet; }

    
    inline void SetEncodingType(const EncodingType& value) { m_encodingTypeHasBeenSet = true; m_encodingType = value; }

    
    inline void SetEncodingType(EncodingType&& value) { m_encodingTypeHasBeenSet = true; m_encodingType = std::move(value); }

    
    inline ListObjectVersionsRequest& WithEncodingType(const EncodingType& value) { SetEncodingType(value); return *this;}

    
    inline ListObjectVersionsRequest& WithEncodingType(EncodingType&& value) { SetEncodingType(std::move(value)); return *this;}


    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline const Aws::String& GetKeyMarker() const{ return m_keyMarker; }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline bool KeyMarkerHasBeenSet() const { return m_keyMarkerHasBeenSet; }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline void SetKeyMarker(const Aws::String& value) { m_keyMarkerHasBeenSet = true; m_keyMarker = value; }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline void SetKeyMarker(Aws::String&& value) { m_keyMarkerHasBeenSet = true; m_keyMarker = std::move(value); }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline void SetKeyMarker(const char* value) { m_keyMarkerHasBeenSet = true; m_keyMarker.assign(value); }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline ListObjectVersionsRequest& WithKeyMarker(const Aws::String& value) { SetKeyMarker(value); return *this;}

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline ListObjectVersionsRequest& WithKeyMarker(Aws::String&& value) { SetKeyMarker(std::move(value)); return *this;}

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline ListObjectVersionsRequest& WithKeyMarker(const char* value) { SetKeyMarker(value); return *this;}


    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more. If additional keys satisfy the
     * search criteria, but were not returned because max-keys was exceeded, the
     * response contains &lt;isTruncated&gt;true&lt;/isTruncated&gt;. To return the
     * additional keys, see key-marker and version-id-marker.</p>
     */
    inline int GetMaxKeys() const{ return m_maxKeys; }

    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more. If additional keys satisfy the
     * search criteria, but were not returned because max-keys was exceeded, the
     * response contains &lt;isTruncated&gt;true&lt;/isTruncated&gt;. To return the
     * additional keys, see key-marker and version-id-marker.</p>
     */
    inline bool MaxKeysHasBeenSet() const { return m_maxKeysHasBeenSet; }

    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more. If additional keys satisfy the
     * search criteria, but were not returned because max-keys was exceeded, the
     * response contains &lt;isTruncated&gt;true&lt;/isTruncated&gt;. To return the
     * additional keys, see key-marker and version-id-marker.</p>
     */
    inline void SetMaxKeys(int value) { m_maxKeysHasBeenSet = true; m_maxKeys = value; }

    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more. If additional keys satisfy the
     * search criteria, but were not returned because max-keys was exceeded, the
     * response contains &lt;isTruncated&gt;true&lt;/isTruncated&gt;. To return the
     * additional keys, see key-marker and version-id-marker.</p>
     */
    inline ListObjectVersionsRequest& WithMaxKeys(int value) { SetMaxKeys(value); return *this;}


    /**
     * <p>Use this parameter to select only those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different groupings of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.) You can use prefix with delimiter to roll up numerous
     * objects into a single result under CommonPrefixes. </p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>Use this parameter to select only those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different groupings of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.) You can use prefix with delimiter to roll up numerous
     * objects into a single result under CommonPrefixes. </p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>Use this parameter to select only those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different groupings of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.) You can use prefix with delimiter to roll up numerous
     * objects into a single result under CommonPrefixes. </p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>Use this parameter to select only those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different groupings of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.) You can use prefix with delimiter to roll up numerous
     * objects into a single result under CommonPrefixes. </p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>Use this parameter to select only those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different groupings of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.) You can use prefix with delimiter to roll up numerous
     * objects into a single result under CommonPrefixes. </p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>Use this parameter to select only those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different groupings of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.) You can use prefix with delimiter to roll up numerous
     * objects into a single result under CommonPrefixes. </p>
     */
    inline ListObjectVersionsRequest& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>Use this parameter to select only those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different groupings of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.) You can use prefix with delimiter to roll up numerous
     * objects into a single result under CommonPrefixes. </p>
     */
    inline ListObjectVersionsRequest& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>Use this parameter to select only those keys that begin with the specified
     * prefix. You can use prefixes to separate a bucket into different groupings of
     * keys. (You can think of using prefix to make groups in the same way you'd use a
     * folder in a file system.) You can use prefix with delimiter to roll up numerous
     * objects into a single result under CommonPrefixes. </p>
     */
    inline ListObjectVersionsRequest& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>Specifies the object version you want to start listing from.</p>
     */
    inline const Aws::String& GetVersionIdMarker() const{ return m_versionIdMarker; }

    /**
     * <p>Specifies the object version you want to start listing from.</p>
     */
    inline bool VersionIdMarkerHasBeenSet() const { return m_versionIdMarkerHasBeenSet; }

    /**
     * <p>Specifies the object version you want to start listing from.</p>
     */
    inline void SetVersionIdMarker(const Aws::String& value) { m_versionIdMarkerHasBeenSet = true; m_versionIdMarker = value; }

    /**
     * <p>Specifies the object version you want to start listing from.</p>
     */
    inline void SetVersionIdMarker(Aws::String&& value) { m_versionIdMarkerHasBeenSet = true; m_versionIdMarker = std::move(value); }

    /**
     * <p>Specifies the object version you want to start listing from.</p>
     */
    inline void SetVersionIdMarker(const char* value) { m_versionIdMarkerHasBeenSet = true; m_versionIdMarker.assign(value); }

    /**
     * <p>Specifies the object version you want to start listing from.</p>
     */
    inline ListObjectVersionsRequest& WithVersionIdMarker(const Aws::String& value) { SetVersionIdMarker(value); return *this;}

    /**
     * <p>Specifies the object version you want to start listing from.</p>
     */
    inline ListObjectVersionsRequest& WithVersionIdMarker(Aws::String&& value) { SetVersionIdMarker(std::move(value)); return *this;}

    /**
     * <p>Specifies the object version you want to start listing from.</p>
     */
    inline ListObjectVersionsRequest& WithVersionIdMarker(const char* value) { SetVersionIdMarker(value); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline ListObjectVersionsRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline ListObjectVersionsRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline ListObjectVersionsRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline ListObjectVersionsRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline ListObjectVersionsRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline ListObjectVersionsRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline ListObjectVersionsRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline ListObjectVersionsRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline ListObjectVersionsRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_delimiter;
    bool m_delimiterHasBeenSet;

    EncodingType m_encodingType;
    bool m_encodingTypeHasBeenSet;

    Aws::String m_keyMarker;
    bool m_keyMarkerHasBeenSet;

    int m_maxKeys;
    bool m_maxKeysHasBeenSet;

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    Aws::String m_versionIdMarker;
    bool m_versionIdMarkerHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
