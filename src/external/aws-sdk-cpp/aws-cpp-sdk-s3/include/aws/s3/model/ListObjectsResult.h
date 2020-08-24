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
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/EncodingType.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/CommonPrefix.h>
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
  class AWS_S3_API ListObjectsResult
  {
  public:
    ListObjectsResult();
    ListObjectsResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    ListObjectsResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>A flag that indicates whether Amazon S3 returned all of the results that
     * satisfied the search criteria.</p>
     */
    inline bool GetIsTruncated() const{ return m_isTruncated; }

    /**
     * <p>A flag that indicates whether Amazon S3 returned all of the results that
     * satisfied the search criteria.</p>
     */
    inline void SetIsTruncated(bool value) { m_isTruncated = value; }

    /**
     * <p>A flag that indicates whether Amazon S3 returned all of the results that
     * satisfied the search criteria.</p>
     */
    inline ListObjectsResult& WithIsTruncated(bool value) { SetIsTruncated(value); return *this;}


    /**
     * <p>Indicates where in the bucket listing begins. Marker is included in the
     * response if it was sent with the request.</p>
     */
    inline const Aws::String& GetMarker() const{ return m_marker; }

    /**
     * <p>Indicates where in the bucket listing begins. Marker is included in the
     * response if it was sent with the request.</p>
     */
    inline void SetMarker(const Aws::String& value) { m_marker = value; }

    /**
     * <p>Indicates where in the bucket listing begins. Marker is included in the
     * response if it was sent with the request.</p>
     */
    inline void SetMarker(Aws::String&& value) { m_marker = std::move(value); }

    /**
     * <p>Indicates where in the bucket listing begins. Marker is included in the
     * response if it was sent with the request.</p>
     */
    inline void SetMarker(const char* value) { m_marker.assign(value); }

    /**
     * <p>Indicates where in the bucket listing begins. Marker is included in the
     * response if it was sent with the request.</p>
     */
    inline ListObjectsResult& WithMarker(const Aws::String& value) { SetMarker(value); return *this;}

    /**
     * <p>Indicates where in the bucket listing begins. Marker is included in the
     * response if it was sent with the request.</p>
     */
    inline ListObjectsResult& WithMarker(Aws::String&& value) { SetMarker(std::move(value)); return *this;}

    /**
     * <p>Indicates where in the bucket listing begins. Marker is included in the
     * response if it was sent with the request.</p>
     */
    inline ListObjectsResult& WithMarker(const char* value) { SetMarker(value); return *this;}


    /**
     * <p>When response is truncated (the IsTruncated element value in the response is
     * true), you can use the key name in this field as marker in the subsequent
     * request to get next set of objects. Amazon S3 lists objects in alphabetical
     * order Note: This element is returned only if you have delimiter request
     * parameter specified. If response does not include the NextMaker and it is
     * truncated, you can use the value of the last Key in the response as the marker
     * in the subsequent request to get the next set of object keys.</p>
     */
    inline const Aws::String& GetNextMarker() const{ return m_nextMarker; }

    /**
     * <p>When response is truncated (the IsTruncated element value in the response is
     * true), you can use the key name in this field as marker in the subsequent
     * request to get next set of objects. Amazon S3 lists objects in alphabetical
     * order Note: This element is returned only if you have delimiter request
     * parameter specified. If response does not include the NextMaker and it is
     * truncated, you can use the value of the last Key in the response as the marker
     * in the subsequent request to get the next set of object keys.</p>
     */
    inline void SetNextMarker(const Aws::String& value) { m_nextMarker = value; }

    /**
     * <p>When response is truncated (the IsTruncated element value in the response is
     * true), you can use the key name in this field as marker in the subsequent
     * request to get next set of objects. Amazon S3 lists objects in alphabetical
     * order Note: This element is returned only if you have delimiter request
     * parameter specified. If response does not include the NextMaker and it is
     * truncated, you can use the value of the last Key in the response as the marker
     * in the subsequent request to get the next set of object keys.</p>
     */
    inline void SetNextMarker(Aws::String&& value) { m_nextMarker = std::move(value); }

    /**
     * <p>When response is truncated (the IsTruncated element value in the response is
     * true), you can use the key name in this field as marker in the subsequent
     * request to get next set of objects. Amazon S3 lists objects in alphabetical
     * order Note: This element is returned only if you have delimiter request
     * parameter specified. If response does not include the NextMaker and it is
     * truncated, you can use the value of the last Key in the response as the marker
     * in the subsequent request to get the next set of object keys.</p>
     */
    inline void SetNextMarker(const char* value) { m_nextMarker.assign(value); }

    /**
     * <p>When response is truncated (the IsTruncated element value in the response is
     * true), you can use the key name in this field as marker in the subsequent
     * request to get next set of objects. Amazon S3 lists objects in alphabetical
     * order Note: This element is returned only if you have delimiter request
     * parameter specified. If response does not include the NextMaker and it is
     * truncated, you can use the value of the last Key in the response as the marker
     * in the subsequent request to get the next set of object keys.</p>
     */
    inline ListObjectsResult& WithNextMarker(const Aws::String& value) { SetNextMarker(value); return *this;}

    /**
     * <p>When response is truncated (the IsTruncated element value in the response is
     * true), you can use the key name in this field as marker in the subsequent
     * request to get next set of objects. Amazon S3 lists objects in alphabetical
     * order Note: This element is returned only if you have delimiter request
     * parameter specified. If response does not include the NextMaker and it is
     * truncated, you can use the value of the last Key in the response as the marker
     * in the subsequent request to get the next set of object keys.</p>
     */
    inline ListObjectsResult& WithNextMarker(Aws::String&& value) { SetNextMarker(std::move(value)); return *this;}

    /**
     * <p>When response is truncated (the IsTruncated element value in the response is
     * true), you can use the key name in this field as marker in the subsequent
     * request to get next set of objects. Amazon S3 lists objects in alphabetical
     * order Note: This element is returned only if you have delimiter request
     * parameter specified. If response does not include the NextMaker and it is
     * truncated, you can use the value of the last Key in the response as the marker
     * in the subsequent request to get the next set of object keys.</p>
     */
    inline ListObjectsResult& WithNextMarker(const char* value) { SetNextMarker(value); return *this;}


    /**
     * <p>Metadata about each object returned.</p>
     */
    inline const Aws::Vector<Object>& GetContents() const{ return m_contents; }

    /**
     * <p>Metadata about each object returned.</p>
     */
    inline void SetContents(const Aws::Vector<Object>& value) { m_contents = value; }

    /**
     * <p>Metadata about each object returned.</p>
     */
    inline void SetContents(Aws::Vector<Object>&& value) { m_contents = std::move(value); }

    /**
     * <p>Metadata about each object returned.</p>
     */
    inline ListObjectsResult& WithContents(const Aws::Vector<Object>& value) { SetContents(value); return *this;}

    /**
     * <p>Metadata about each object returned.</p>
     */
    inline ListObjectsResult& WithContents(Aws::Vector<Object>&& value) { SetContents(std::move(value)); return *this;}

    /**
     * <p>Metadata about each object returned.</p>
     */
    inline ListObjectsResult& AddContents(const Object& value) { m_contents.push_back(value); return *this; }

    /**
     * <p>Metadata about each object returned.</p>
     */
    inline ListObjectsResult& AddContents(Object&& value) { m_contents.push_back(std::move(value)); return *this; }


    /**
     * <p>Bucket name.</p>
     */
    inline const Aws::String& GetName() const{ return m_name; }

    /**
     * <p>Bucket name.</p>
     */
    inline void SetName(const Aws::String& value) { m_name = value; }

    /**
     * <p>Bucket name.</p>
     */
    inline void SetName(Aws::String&& value) { m_name = std::move(value); }

    /**
     * <p>Bucket name.</p>
     */
    inline void SetName(const char* value) { m_name.assign(value); }

    /**
     * <p>Bucket name.</p>
     */
    inline ListObjectsResult& WithName(const Aws::String& value) { SetName(value); return *this;}

    /**
     * <p>Bucket name.</p>
     */
    inline ListObjectsResult& WithName(Aws::String&& value) { SetName(std::move(value)); return *this;}

    /**
     * <p>Bucket name.</p>
     */
    inline ListObjectsResult& WithName(const char* value) { SetName(value); return *this;}


    /**
     * <p>Keys that begin with the indicated prefix.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>Keys that begin with the indicated prefix.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefix = value; }

    /**
     * <p>Keys that begin with the indicated prefix.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefix = std::move(value); }

    /**
     * <p>Keys that begin with the indicated prefix.</p>
     */
    inline void SetPrefix(const char* value) { m_prefix.assign(value); }

    /**
     * <p>Keys that begin with the indicated prefix.</p>
     */
    inline ListObjectsResult& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>Keys that begin with the indicated prefix.</p>
     */
    inline ListObjectsResult& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>Keys that begin with the indicated prefix.</p>
     */
    inline ListObjectsResult& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * <code>CommonPrefixes</code> collection. These rolled-up keys are not returned
     * elsewhere in the response. Each rolled-up result counts as only one return
     * against the <code>MaxKeys</code> value.</p>
     */
    inline const Aws::String& GetDelimiter() const{ return m_delimiter; }

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * <code>CommonPrefixes</code> collection. These rolled-up keys are not returned
     * elsewhere in the response. Each rolled-up result counts as only one return
     * against the <code>MaxKeys</code> value.</p>
     */
    inline void SetDelimiter(const Aws::String& value) { m_delimiter = value; }

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * <code>CommonPrefixes</code> collection. These rolled-up keys are not returned
     * elsewhere in the response. Each rolled-up result counts as only one return
     * against the <code>MaxKeys</code> value.</p>
     */
    inline void SetDelimiter(Aws::String&& value) { m_delimiter = std::move(value); }

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * <code>CommonPrefixes</code> collection. These rolled-up keys are not returned
     * elsewhere in the response. Each rolled-up result counts as only one return
     * against the <code>MaxKeys</code> value.</p>
     */
    inline void SetDelimiter(const char* value) { m_delimiter.assign(value); }

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * <code>CommonPrefixes</code> collection. These rolled-up keys are not returned
     * elsewhere in the response. Each rolled-up result counts as only one return
     * against the <code>MaxKeys</code> value.</p>
     */
    inline ListObjectsResult& WithDelimiter(const Aws::String& value) { SetDelimiter(value); return *this;}

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * <code>CommonPrefixes</code> collection. These rolled-up keys are not returned
     * elsewhere in the response. Each rolled-up result counts as only one return
     * against the <code>MaxKeys</code> value.</p>
     */
    inline ListObjectsResult& WithDelimiter(Aws::String&& value) { SetDelimiter(std::move(value)); return *this;}

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * <code>CommonPrefixes</code> collection. These rolled-up keys are not returned
     * elsewhere in the response. Each rolled-up result counts as only one return
     * against the <code>MaxKeys</code> value.</p>
     */
    inline ListObjectsResult& WithDelimiter(const char* value) { SetDelimiter(value); return *this;}


    /**
     * <p>The maximum number of keys returned in the response body.</p>
     */
    inline int GetMaxKeys() const{ return m_maxKeys; }

    /**
     * <p>The maximum number of keys returned in the response body.</p>
     */
    inline void SetMaxKeys(int value) { m_maxKeys = value; }

    /**
     * <p>The maximum number of keys returned in the response body.</p>
     */
    inline ListObjectsResult& WithMaxKeys(int value) { SetMaxKeys(value); return *this;}


    /**
     * <p>All of the keys rolled up in a common prefix count as a single return when
     * calculating the number of returns. </p> <p>A response can contain CommonPrefixes
     * only if you specify a delimiter.</p> <p>CommonPrefixes contains all (if there
     * are any) keys between Prefix and the next occurrence of the string specified by
     * the delimiter.</p> <p> CommonPrefixes lists keys that act like subdirectories in
     * the directory specified by Prefix.</p> <p>For example, if the prefix is notes/
     * and the delimiter is a slash (/) as in notes/summer/july, the common prefix is
     * notes/summer/. All of the keys that roll up into a common prefix count as a
     * single return when calculating the number of returns.</p>
     */
    inline const Aws::Vector<CommonPrefix>& GetCommonPrefixes() const{ return m_commonPrefixes; }

    /**
     * <p>All of the keys rolled up in a common prefix count as a single return when
     * calculating the number of returns. </p> <p>A response can contain CommonPrefixes
     * only if you specify a delimiter.</p> <p>CommonPrefixes contains all (if there
     * are any) keys between Prefix and the next occurrence of the string specified by
     * the delimiter.</p> <p> CommonPrefixes lists keys that act like subdirectories in
     * the directory specified by Prefix.</p> <p>For example, if the prefix is notes/
     * and the delimiter is a slash (/) as in notes/summer/july, the common prefix is
     * notes/summer/. All of the keys that roll up into a common prefix count as a
     * single return when calculating the number of returns.</p>
     */
    inline void SetCommonPrefixes(const Aws::Vector<CommonPrefix>& value) { m_commonPrefixes = value; }

    /**
     * <p>All of the keys rolled up in a common prefix count as a single return when
     * calculating the number of returns. </p> <p>A response can contain CommonPrefixes
     * only if you specify a delimiter.</p> <p>CommonPrefixes contains all (if there
     * are any) keys between Prefix and the next occurrence of the string specified by
     * the delimiter.</p> <p> CommonPrefixes lists keys that act like subdirectories in
     * the directory specified by Prefix.</p> <p>For example, if the prefix is notes/
     * and the delimiter is a slash (/) as in notes/summer/july, the common prefix is
     * notes/summer/. All of the keys that roll up into a common prefix count as a
     * single return when calculating the number of returns.</p>
     */
    inline void SetCommonPrefixes(Aws::Vector<CommonPrefix>&& value) { m_commonPrefixes = std::move(value); }

    /**
     * <p>All of the keys rolled up in a common prefix count as a single return when
     * calculating the number of returns. </p> <p>A response can contain CommonPrefixes
     * only if you specify a delimiter.</p> <p>CommonPrefixes contains all (if there
     * are any) keys between Prefix and the next occurrence of the string specified by
     * the delimiter.</p> <p> CommonPrefixes lists keys that act like subdirectories in
     * the directory specified by Prefix.</p> <p>For example, if the prefix is notes/
     * and the delimiter is a slash (/) as in notes/summer/july, the common prefix is
     * notes/summer/. All of the keys that roll up into a common prefix count as a
     * single return when calculating the number of returns.</p>
     */
    inline ListObjectsResult& WithCommonPrefixes(const Aws::Vector<CommonPrefix>& value) { SetCommonPrefixes(value); return *this;}

    /**
     * <p>All of the keys rolled up in a common prefix count as a single return when
     * calculating the number of returns. </p> <p>A response can contain CommonPrefixes
     * only if you specify a delimiter.</p> <p>CommonPrefixes contains all (if there
     * are any) keys between Prefix and the next occurrence of the string specified by
     * the delimiter.</p> <p> CommonPrefixes lists keys that act like subdirectories in
     * the directory specified by Prefix.</p> <p>For example, if the prefix is notes/
     * and the delimiter is a slash (/) as in notes/summer/july, the common prefix is
     * notes/summer/. All of the keys that roll up into a common prefix count as a
     * single return when calculating the number of returns.</p>
     */
    inline ListObjectsResult& WithCommonPrefixes(Aws::Vector<CommonPrefix>&& value) { SetCommonPrefixes(std::move(value)); return *this;}

    /**
     * <p>All of the keys rolled up in a common prefix count as a single return when
     * calculating the number of returns. </p> <p>A response can contain CommonPrefixes
     * only if you specify a delimiter.</p> <p>CommonPrefixes contains all (if there
     * are any) keys between Prefix and the next occurrence of the string specified by
     * the delimiter.</p> <p> CommonPrefixes lists keys that act like subdirectories in
     * the directory specified by Prefix.</p> <p>For example, if the prefix is notes/
     * and the delimiter is a slash (/) as in notes/summer/july, the common prefix is
     * notes/summer/. All of the keys that roll up into a common prefix count as a
     * single return when calculating the number of returns.</p>
     */
    inline ListObjectsResult& AddCommonPrefixes(const CommonPrefix& value) { m_commonPrefixes.push_back(value); return *this; }

    /**
     * <p>All of the keys rolled up in a common prefix count as a single return when
     * calculating the number of returns. </p> <p>A response can contain CommonPrefixes
     * only if you specify a delimiter.</p> <p>CommonPrefixes contains all (if there
     * are any) keys between Prefix and the next occurrence of the string specified by
     * the delimiter.</p> <p> CommonPrefixes lists keys that act like subdirectories in
     * the directory specified by Prefix.</p> <p>For example, if the prefix is notes/
     * and the delimiter is a slash (/) as in notes/summer/july, the common prefix is
     * notes/summer/. All of the keys that roll up into a common prefix count as a
     * single return when calculating the number of returns.</p>
     */
    inline ListObjectsResult& AddCommonPrefixes(CommonPrefix&& value) { m_commonPrefixes.push_back(std::move(value)); return *this; }


    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline const EncodingType& GetEncodingType() const{ return m_encodingType; }

    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline void SetEncodingType(const EncodingType& value) { m_encodingType = value; }

    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline void SetEncodingType(EncodingType&& value) { m_encodingType = std::move(value); }

    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline ListObjectsResult& WithEncodingType(const EncodingType& value) { SetEncodingType(value); return *this;}

    /**
     * <p>Encoding type used by Amazon S3 to encode object keys in the response.</p>
     */
    inline ListObjectsResult& WithEncodingType(EncodingType&& value) { SetEncodingType(std::move(value)); return *this;}

  private:

    bool m_isTruncated;

    Aws::String m_marker;

    Aws::String m_nextMarker;

    Aws::Vector<Object> m_contents;

    Aws::String m_name;

    Aws::String m_prefix;

    Aws::String m_delimiter;

    int m_maxKeys;

    Aws::Vector<CommonPrefix> m_commonPrefixes;

    EncodingType m_encodingType;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
