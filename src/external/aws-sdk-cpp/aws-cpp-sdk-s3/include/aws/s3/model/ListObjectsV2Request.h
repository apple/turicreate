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
  class AWS_S3_API ListObjectsV2Request : public S3Request
  {
  public:
    ListObjectsV2Request();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "ListObjectsV2"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>Bucket name to list. </p> <p>When using this API with an access point, you
     * must direct requests to the access point hostname. The access point hostname
     * takes the form
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
     * <p>Bucket name to list. </p> <p>When using this API with an access point, you
     * must direct requests to the access point hostname. The access point hostname
     * takes the form
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
     * <p>Bucket name to list. </p> <p>When using this API with an access point, you
     * must direct requests to the access point hostname. The access point hostname
     * takes the form
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
     * <p>Bucket name to list. </p> <p>When using this API with an access point, you
     * must direct requests to the access point hostname. The access point hostname
     * takes the form
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
     * <p>Bucket name to list. </p> <p>When using this API with an access point, you
     * must direct requests to the access point hostname. The access point hostname
     * takes the form
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
     * <p>Bucket name to list. </p> <p>When using this API with an access point, you
     * must direct requests to the access point hostname. The access point hostname
     * takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ListObjectsV2Request& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>Bucket name to list. </p> <p>When using this API with an access point, you
     * must direct requests to the access point hostname. The access point hostname
     * takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ListObjectsV2Request& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>Bucket name to list. </p> <p>When using this API with an access point, you
     * must direct requests to the access point hostname. The access point hostname
     * takes the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ListObjectsV2Request& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline const Aws::String& GetDelimiter() const{ return m_delimiter; }

    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline bool DelimiterHasBeenSet() const { return m_delimiterHasBeenSet; }

    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline void SetDelimiter(const Aws::String& value) { m_delimiterHasBeenSet = true; m_delimiter = value; }

    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline void SetDelimiter(Aws::String&& value) { m_delimiterHasBeenSet = true; m_delimiter = std::move(value); }

    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline void SetDelimiter(const char* value) { m_delimiterHasBeenSet = true; m_delimiter.assign(value); }

    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline ListObjectsV2Request& WithDelimiter(const Aws::String& value) { SetDelimiter(value); return *this;}

    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline ListObjectsV2Request& WithDelimiter(Aws::String&& value) { SetDelimiter(std::move(value)); return *this;}

    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline ListObjectsV2Request& WithDelimiter(const char* value) { SetDelimiter(value); return *this;}


    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline const EncodingType& GetEncodingType() const{ return m_encodingType; }

    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline bool EncodingTypeHasBeenSet() const { return m_encodingTypeHasBeenSet; }

    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline void SetEncodingType(const EncodingType& value) { m_encodingTypeHasBeenSet = true; m_encodingType = value; }

    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline void SetEncodingType(EncodingType&& value) { m_encodingTypeHasBeenSet = true; m_encodingType = std::move(value); }

    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline ListObjectsV2Request& WithEncodingType(const EncodingType& value) { SetEncodingType(value); return *this;}

    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline ListObjectsV2Request& WithEncodingType(EncodingType&& value) { SetEncodingType(std::move(value)); return *this;}


    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more.</p>
     */
    inline int GetMaxKeys() const{ return m_maxKeys; }

    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more.</p>
     */
    inline bool MaxKeysHasBeenSet() const { return m_maxKeysHasBeenSet; }

    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more.</p>
     */
    inline void SetMaxKeys(int value) { m_maxKeysHasBeenSet = true; m_maxKeys = value; }

    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more.</p>
     */
    inline ListObjectsV2Request& WithMaxKeys(int value) { SetMaxKeys(value); return *this;}


    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline ListObjectsV2Request& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline ListObjectsV2Request& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline ListObjectsV2Request& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>ContinuationToken indicates Amazon S3 that the list is being continued on
     * this bucket with a token. ContinuationToken is obfuscated and is not a real
     * key.</p>
     */
    inline const Aws::String& GetContinuationToken() const{ return m_continuationToken; }

    /**
     * <p>ContinuationToken indicates Amazon S3 that the list is being continued on
     * this bucket with a token. ContinuationToken is obfuscated and is not a real
     * key.</p>
     */
    inline bool ContinuationTokenHasBeenSet() const { return m_continuationTokenHasBeenSet; }

    /**
     * <p>ContinuationToken indicates Amazon S3 that the list is being continued on
     * this bucket with a token. ContinuationToken is obfuscated and is not a real
     * key.</p>
     */
    inline void SetContinuationToken(const Aws::String& value) { m_continuationTokenHasBeenSet = true; m_continuationToken = value; }

    /**
     * <p>ContinuationToken indicates Amazon S3 that the list is being continued on
     * this bucket with a token. ContinuationToken is obfuscated and is not a real
     * key.</p>
     */
    inline void SetContinuationToken(Aws::String&& value) { m_continuationTokenHasBeenSet = true; m_continuationToken = std::move(value); }

    /**
     * <p>ContinuationToken indicates Amazon S3 that the list is being continued on
     * this bucket with a token. ContinuationToken is obfuscated and is not a real
     * key.</p>
     */
    inline void SetContinuationToken(const char* value) { m_continuationTokenHasBeenSet = true; m_continuationToken.assign(value); }

    /**
     * <p>ContinuationToken indicates Amazon S3 that the list is being continued on
     * this bucket with a token. ContinuationToken is obfuscated and is not a real
     * key.</p>
     */
    inline ListObjectsV2Request& WithContinuationToken(const Aws::String& value) { SetContinuationToken(value); return *this;}

    /**
     * <p>ContinuationToken indicates Amazon S3 that the list is being continued on
     * this bucket with a token. ContinuationToken is obfuscated and is not a real
     * key.</p>
     */
    inline ListObjectsV2Request& WithContinuationToken(Aws::String&& value) { SetContinuationToken(std::move(value)); return *this;}

    /**
     * <p>ContinuationToken indicates Amazon S3 that the list is being continued on
     * this bucket with a token. ContinuationToken is obfuscated and is not a real
     * key.</p>
     */
    inline ListObjectsV2Request& WithContinuationToken(const char* value) { SetContinuationToken(value); return *this;}


    /**
     * <p>The owner field is not present in listV2 by default, if you want to return
     * owner field with each key in the result then set the fetch owner field to
     * true.</p>
     */
    inline bool GetFetchOwner() const{ return m_fetchOwner; }

    /**
     * <p>The owner field is not present in listV2 by default, if you want to return
     * owner field with each key in the result then set the fetch owner field to
     * true.</p>
     */
    inline bool FetchOwnerHasBeenSet() const { return m_fetchOwnerHasBeenSet; }

    /**
     * <p>The owner field is not present in listV2 by default, if you want to return
     * owner field with each key in the result then set the fetch owner field to
     * true.</p>
     */
    inline void SetFetchOwner(bool value) { m_fetchOwnerHasBeenSet = true; m_fetchOwner = value; }

    /**
     * <p>The owner field is not present in listV2 by default, if you want to return
     * owner field with each key in the result then set the fetch owner field to
     * true.</p>
     */
    inline ListObjectsV2Request& WithFetchOwner(bool value) { SetFetchOwner(value); return *this;}


    /**
     * <p>StartAfter is where you want Amazon S3 to start listing from. Amazon S3
     * starts listing after this specified key. StartAfter can be any key in the
     * bucket.</p>
     */
    inline const Aws::String& GetStartAfter() const{ return m_startAfter; }

    /**
     * <p>StartAfter is where you want Amazon S3 to start listing from. Amazon S3
     * starts listing after this specified key. StartAfter can be any key in the
     * bucket.</p>
     */
    inline bool StartAfterHasBeenSet() const { return m_startAfterHasBeenSet; }

    /**
     * <p>StartAfter is where you want Amazon S3 to start listing from. Amazon S3
     * starts listing after this specified key. StartAfter can be any key in the
     * bucket.</p>
     */
    inline void SetStartAfter(const Aws::String& value) { m_startAfterHasBeenSet = true; m_startAfter = value; }

    /**
     * <p>StartAfter is where you want Amazon S3 to start listing from. Amazon S3
     * starts listing after this specified key. StartAfter can be any key in the
     * bucket.</p>
     */
    inline void SetStartAfter(Aws::String&& value) { m_startAfterHasBeenSet = true; m_startAfter = std::move(value); }

    /**
     * <p>StartAfter is where you want Amazon S3 to start listing from. Amazon S3
     * starts listing after this specified key. StartAfter can be any key in the
     * bucket.</p>
     */
    inline void SetStartAfter(const char* value) { m_startAfterHasBeenSet = true; m_startAfter.assign(value); }

    /**
     * <p>StartAfter is where you want Amazon S3 to start listing from. Amazon S3
     * starts listing after this specified key. StartAfter can be any key in the
     * bucket.</p>
     */
    inline ListObjectsV2Request& WithStartAfter(const Aws::String& value) { SetStartAfter(value); return *this;}

    /**
     * <p>StartAfter is where you want Amazon S3 to start listing from. Amazon S3
     * starts listing after this specified key. StartAfter can be any key in the
     * bucket.</p>
     */
    inline ListObjectsV2Request& WithStartAfter(Aws::String&& value) { SetStartAfter(std::move(value)); return *this;}

    /**
     * <p>StartAfter is where you want Amazon S3 to start listing from. Amazon S3
     * starts listing after this specified key. StartAfter can be any key in the
     * bucket.</p>
     */
    inline ListObjectsV2Request& WithStartAfter(const char* value) { SetStartAfter(value); return *this;}


    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request in V2 style. Bucket owners need not specify this parameter in
     * their requests.</p>
     */
    inline const RequestPayer& GetRequestPayer() const{ return m_requestPayer; }

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request in V2 style. Bucket owners need not specify this parameter in
     * their requests.</p>
     */
    inline bool RequestPayerHasBeenSet() const { return m_requestPayerHasBeenSet; }

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request in V2 style. Bucket owners need not specify this parameter in
     * their requests.</p>
     */
    inline void SetRequestPayer(const RequestPayer& value) { m_requestPayerHasBeenSet = true; m_requestPayer = value; }

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request in V2 style. Bucket owners need not specify this parameter in
     * their requests.</p>
     */
    inline void SetRequestPayer(RequestPayer&& value) { m_requestPayerHasBeenSet = true; m_requestPayer = std::move(value); }

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request in V2 style. Bucket owners need not specify this parameter in
     * their requests.</p>
     */
    inline ListObjectsV2Request& WithRequestPayer(const RequestPayer& value) { SetRequestPayer(value); return *this;}

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request in V2 style. Bucket owners need not specify this parameter in
     * their requests.</p>
     */
    inline ListObjectsV2Request& WithRequestPayer(RequestPayer&& value) { SetRequestPayer(std::move(value)); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline ListObjectsV2Request& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline ListObjectsV2Request& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline ListObjectsV2Request& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline ListObjectsV2Request& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline ListObjectsV2Request& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline ListObjectsV2Request& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline ListObjectsV2Request& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline ListObjectsV2Request& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline ListObjectsV2Request& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_delimiter;
    bool m_delimiterHasBeenSet;

    EncodingType m_encodingType;
    bool m_encodingTypeHasBeenSet;

    int m_maxKeys;
    bool m_maxKeysHasBeenSet;

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    Aws::String m_continuationToken;
    bool m_continuationTokenHasBeenSet;

    bool m_fetchOwner;
    bool m_fetchOwnerHasBeenSet;

    Aws::String m_startAfter;
    bool m_startAfterHasBeenSet;

    RequestPayer m_requestPayer;
    bool m_requestPayerHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
