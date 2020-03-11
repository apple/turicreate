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
  class AWS_S3_API ListObjectsRequest : public S3Request
  {
  public:
    ListObjectsRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "ListObjects"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>The name of the bucket containing the objects.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>The name of the bucket containing the objects.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The name of the bucket containing the objects.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The name of the bucket containing the objects.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The name of the bucket containing the objects.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The name of the bucket containing the objects.</p>
     */
    inline ListObjectsRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The name of the bucket containing the objects.</p>
     */
    inline ListObjectsRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The name of the bucket containing the objects.</p>
     */
    inline ListObjectsRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


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
    inline ListObjectsRequest& WithDelimiter(const Aws::String& value) { SetDelimiter(value); return *this;}

    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline ListObjectsRequest& WithDelimiter(Aws::String&& value) { SetDelimiter(std::move(value)); return *this;}

    /**
     * <p>A delimiter is a character you use to group keys.</p>
     */
    inline ListObjectsRequest& WithDelimiter(const char* value) { SetDelimiter(value); return *this;}


    
    inline const EncodingType& GetEncodingType() const{ return m_encodingType; }

    
    inline bool EncodingTypeHasBeenSet() const { return m_encodingTypeHasBeenSet; }

    
    inline void SetEncodingType(const EncodingType& value) { m_encodingTypeHasBeenSet = true; m_encodingType = value; }

    
    inline void SetEncodingType(EncodingType&& value) { m_encodingTypeHasBeenSet = true; m_encodingType = std::move(value); }

    
    inline ListObjectsRequest& WithEncodingType(const EncodingType& value) { SetEncodingType(value); return *this;}

    
    inline ListObjectsRequest& WithEncodingType(EncodingType&& value) { SetEncodingType(std::move(value)); return *this;}


    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline const Aws::String& GetMarker() const{ return m_marker; }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline bool MarkerHasBeenSet() const { return m_markerHasBeenSet; }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline void SetMarker(const Aws::String& value) { m_markerHasBeenSet = true; m_marker = value; }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline void SetMarker(Aws::String&& value) { m_markerHasBeenSet = true; m_marker = std::move(value); }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline void SetMarker(const char* value) { m_markerHasBeenSet = true; m_marker.assign(value); }

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline ListObjectsRequest& WithMarker(const Aws::String& value) { SetMarker(value); return *this;}

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline ListObjectsRequest& WithMarker(Aws::String&& value) { SetMarker(std::move(value)); return *this;}

    /**
     * <p>Specifies the key to start with when listing objects in a bucket.</p>
     */
    inline ListObjectsRequest& WithMarker(const char* value) { SetMarker(value); return *this;}


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
    inline ListObjectsRequest& WithMaxKeys(int value) { SetMaxKeys(value); return *this;}


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
    inline ListObjectsRequest& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline ListObjectsRequest& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>Limits the response to keys that begin with the specified prefix.</p>
     */
    inline ListObjectsRequest& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request. Bucket owners need not specify this parameter in their
     * requests.</p>
     */
    inline const RequestPayer& GetRequestPayer() const{ return m_requestPayer; }

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request. Bucket owners need not specify this parameter in their
     * requests.</p>
     */
    inline bool RequestPayerHasBeenSet() const { return m_requestPayerHasBeenSet; }

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request. Bucket owners need not specify this parameter in their
     * requests.</p>
     */
    inline void SetRequestPayer(const RequestPayer& value) { m_requestPayerHasBeenSet = true; m_requestPayer = value; }

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request. Bucket owners need not specify this parameter in their
     * requests.</p>
     */
    inline void SetRequestPayer(RequestPayer&& value) { m_requestPayerHasBeenSet = true; m_requestPayer = std::move(value); }

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request. Bucket owners need not specify this parameter in their
     * requests.</p>
     */
    inline ListObjectsRequest& WithRequestPayer(const RequestPayer& value) { SetRequestPayer(value); return *this;}

    /**
     * <p>Confirms that the requester knows that she or he will be charged for the list
     * objects request. Bucket owners need not specify this parameter in their
     * requests.</p>
     */
    inline ListObjectsRequest& WithRequestPayer(RequestPayer&& value) { SetRequestPayer(std::move(value)); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline ListObjectsRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline ListObjectsRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline ListObjectsRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline ListObjectsRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline ListObjectsRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline ListObjectsRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline ListObjectsRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline ListObjectsRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline ListObjectsRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_delimiter;
    bool m_delimiterHasBeenSet;

    EncodingType m_encodingType;
    bool m_encodingTypeHasBeenSet;

    Aws::String m_marker;
    bool m_markerHasBeenSet;

    int m_maxKeys;
    bool m_maxKeysHasBeenSet;

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    RequestPayer m_requestPayer;
    bool m_requestPayerHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
