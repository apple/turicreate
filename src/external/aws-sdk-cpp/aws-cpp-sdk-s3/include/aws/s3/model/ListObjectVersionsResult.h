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
#include <aws/s3/model/ObjectVersion.h>
#include <aws/s3/model/DeleteMarkerEntry.h>
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
  class AWS_S3_API ListObjectVersionsResult
  {
  public:
    ListObjectVersionsResult();
    ListObjectVersionsResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    ListObjectVersionsResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>A flag that indicates whether Amazon S3 returned all of the results that
     * satisfied the search criteria. If your results were truncated, you can make a
     * follow-up paginated request using the NextKeyMarker and NextVersionIdMarker
     * response parameters as a starting place in another request to return the rest of
     * the results.</p>
     */
    inline bool GetIsTruncated() const{ return m_isTruncated; }

    /**
     * <p>A flag that indicates whether Amazon S3 returned all of the results that
     * satisfied the search criteria. If your results were truncated, you can make a
     * follow-up paginated request using the NextKeyMarker and NextVersionIdMarker
     * response parameters as a starting place in another request to return the rest of
     * the results.</p>
     */
    inline void SetIsTruncated(bool value) { m_isTruncated = value; }

    /**
     * <p>A flag that indicates whether Amazon S3 returned all of the results that
     * satisfied the search criteria. If your results were truncated, you can make a
     * follow-up paginated request using the NextKeyMarker and NextVersionIdMarker
     * response parameters as a starting place in another request to return the rest of
     * the results.</p>
     */
    inline ListObjectVersionsResult& WithIsTruncated(bool value) { SetIsTruncated(value); return *this;}


    /**
     * <p>Marks the last key returned in a truncated response.</p>
     */
    inline const Aws::String& GetKeyMarker() const{ return m_keyMarker; }

    /**
     * <p>Marks the last key returned in a truncated response.</p>
     */
    inline void SetKeyMarker(const Aws::String& value) { m_keyMarker = value; }

    /**
     * <p>Marks the last key returned in a truncated response.</p>
     */
    inline void SetKeyMarker(Aws::String&& value) { m_keyMarker = std::move(value); }

    /**
     * <p>Marks the last key returned in a truncated response.</p>
     */
    inline void SetKeyMarker(const char* value) { m_keyMarker.assign(value); }

    /**
     * <p>Marks the last key returned in a truncated response.</p>
     */
    inline ListObjectVersionsResult& WithKeyMarker(const Aws::String& value) { SetKeyMarker(value); return *this;}

    /**
     * <p>Marks the last key returned in a truncated response.</p>
     */
    inline ListObjectVersionsResult& WithKeyMarker(Aws::String&& value) { SetKeyMarker(std::move(value)); return *this;}

    /**
     * <p>Marks the last key returned in a truncated response.</p>
     */
    inline ListObjectVersionsResult& WithKeyMarker(const char* value) { SetKeyMarker(value); return *this;}


    /**
     * <p>Marks the last version of the key returned in a truncated response.</p>
     */
    inline const Aws::String& GetVersionIdMarker() const{ return m_versionIdMarker; }

    /**
     * <p>Marks the last version of the key returned in a truncated response.</p>
     */
    inline void SetVersionIdMarker(const Aws::String& value) { m_versionIdMarker = value; }

    /**
     * <p>Marks the last version of the key returned in a truncated response.</p>
     */
    inline void SetVersionIdMarker(Aws::String&& value) { m_versionIdMarker = std::move(value); }

    /**
     * <p>Marks the last version of the key returned in a truncated response.</p>
     */
    inline void SetVersionIdMarker(const char* value) { m_versionIdMarker.assign(value); }

    /**
     * <p>Marks the last version of the key returned in a truncated response.</p>
     */
    inline ListObjectVersionsResult& WithVersionIdMarker(const Aws::String& value) { SetVersionIdMarker(value); return *this;}

    /**
     * <p>Marks the last version of the key returned in a truncated response.</p>
     */
    inline ListObjectVersionsResult& WithVersionIdMarker(Aws::String&& value) { SetVersionIdMarker(std::move(value)); return *this;}

    /**
     * <p>Marks the last version of the key returned in a truncated response.</p>
     */
    inline ListObjectVersionsResult& WithVersionIdMarker(const char* value) { SetVersionIdMarker(value); return *this;}


    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextKeyMarker</code> specifies the first key not returned that satisfies
     * the search criteria. Use this value for the key-marker request parameter in a
     * subsequent request.</p>
     */
    inline const Aws::String& GetNextKeyMarker() const{ return m_nextKeyMarker; }

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextKeyMarker</code> specifies the first key not returned that satisfies
     * the search criteria. Use this value for the key-marker request parameter in a
     * subsequent request.</p>
     */
    inline void SetNextKeyMarker(const Aws::String& value) { m_nextKeyMarker = value; }

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextKeyMarker</code> specifies the first key not returned that satisfies
     * the search criteria. Use this value for the key-marker request parameter in a
     * subsequent request.</p>
     */
    inline void SetNextKeyMarker(Aws::String&& value) { m_nextKeyMarker = std::move(value); }

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextKeyMarker</code> specifies the first key not returned that satisfies
     * the search criteria. Use this value for the key-marker request parameter in a
     * subsequent request.</p>
     */
    inline void SetNextKeyMarker(const char* value) { m_nextKeyMarker.assign(value); }

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextKeyMarker</code> specifies the first key not returned that satisfies
     * the search criteria. Use this value for the key-marker request parameter in a
     * subsequent request.</p>
     */
    inline ListObjectVersionsResult& WithNextKeyMarker(const Aws::String& value) { SetNextKeyMarker(value); return *this;}

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextKeyMarker</code> specifies the first key not returned that satisfies
     * the search criteria. Use this value for the key-marker request parameter in a
     * subsequent request.</p>
     */
    inline ListObjectVersionsResult& WithNextKeyMarker(Aws::String&& value) { SetNextKeyMarker(std::move(value)); return *this;}

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextKeyMarker</code> specifies the first key not returned that satisfies
     * the search criteria. Use this value for the key-marker request parameter in a
     * subsequent request.</p>
     */
    inline ListObjectVersionsResult& WithNextKeyMarker(const char* value) { SetNextKeyMarker(value); return *this;}


    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextVersionIdMarker</code> specifies the first object version not returned
     * that satisfies the search criteria. Use this value for the version-id-marker
     * request parameter in a subsequent request.</p>
     */
    inline const Aws::String& GetNextVersionIdMarker() const{ return m_nextVersionIdMarker; }

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextVersionIdMarker</code> specifies the first object version not returned
     * that satisfies the search criteria. Use this value for the version-id-marker
     * request parameter in a subsequent request.</p>
     */
    inline void SetNextVersionIdMarker(const Aws::String& value) { m_nextVersionIdMarker = value; }

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextVersionIdMarker</code> specifies the first object version not returned
     * that satisfies the search criteria. Use this value for the version-id-marker
     * request parameter in a subsequent request.</p>
     */
    inline void SetNextVersionIdMarker(Aws::String&& value) { m_nextVersionIdMarker = std::move(value); }

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextVersionIdMarker</code> specifies the first object version not returned
     * that satisfies the search criteria. Use this value for the version-id-marker
     * request parameter in a subsequent request.</p>
     */
    inline void SetNextVersionIdMarker(const char* value) { m_nextVersionIdMarker.assign(value); }

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextVersionIdMarker</code> specifies the first object version not returned
     * that satisfies the search criteria. Use this value for the version-id-marker
     * request parameter in a subsequent request.</p>
     */
    inline ListObjectVersionsResult& WithNextVersionIdMarker(const Aws::String& value) { SetNextVersionIdMarker(value); return *this;}

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextVersionIdMarker</code> specifies the first object version not returned
     * that satisfies the search criteria. Use this value for the version-id-marker
     * request parameter in a subsequent request.</p>
     */
    inline ListObjectVersionsResult& WithNextVersionIdMarker(Aws::String&& value) { SetNextVersionIdMarker(std::move(value)); return *this;}

    /**
     * <p>When the number of responses exceeds the value of <code>MaxKeys</code>,
     * <code>NextVersionIdMarker</code> specifies the first object version not returned
     * that satisfies the search criteria. Use this value for the version-id-marker
     * request parameter in a subsequent request.</p>
     */
    inline ListObjectVersionsResult& WithNextVersionIdMarker(const char* value) { SetNextVersionIdMarker(value); return *this;}


    /**
     * <p>Container for version information.</p>
     */
    inline const Aws::Vector<ObjectVersion>& GetVersions() const{ return m_versions; }

    /**
     * <p>Container for version information.</p>
     */
    inline void SetVersions(const Aws::Vector<ObjectVersion>& value) { m_versions = value; }

    /**
     * <p>Container for version information.</p>
     */
    inline void SetVersions(Aws::Vector<ObjectVersion>&& value) { m_versions = std::move(value); }

    /**
     * <p>Container for version information.</p>
     */
    inline ListObjectVersionsResult& WithVersions(const Aws::Vector<ObjectVersion>& value) { SetVersions(value); return *this;}

    /**
     * <p>Container for version information.</p>
     */
    inline ListObjectVersionsResult& WithVersions(Aws::Vector<ObjectVersion>&& value) { SetVersions(std::move(value)); return *this;}

    /**
     * <p>Container for version information.</p>
     */
    inline ListObjectVersionsResult& AddVersions(const ObjectVersion& value) { m_versions.push_back(value); return *this; }

    /**
     * <p>Container for version information.</p>
     */
    inline ListObjectVersionsResult& AddVersions(ObjectVersion&& value) { m_versions.push_back(std::move(value)); return *this; }


    /**
     * <p>Container for an object that is a delete marker.</p>
     */
    inline const Aws::Vector<DeleteMarkerEntry>& GetDeleteMarkers() const{ return m_deleteMarkers; }

    /**
     * <p>Container for an object that is a delete marker.</p>
     */
    inline void SetDeleteMarkers(const Aws::Vector<DeleteMarkerEntry>& value) { m_deleteMarkers = value; }

    /**
     * <p>Container for an object that is a delete marker.</p>
     */
    inline void SetDeleteMarkers(Aws::Vector<DeleteMarkerEntry>&& value) { m_deleteMarkers = std::move(value); }

    /**
     * <p>Container for an object that is a delete marker.</p>
     */
    inline ListObjectVersionsResult& WithDeleteMarkers(const Aws::Vector<DeleteMarkerEntry>& value) { SetDeleteMarkers(value); return *this;}

    /**
     * <p>Container for an object that is a delete marker.</p>
     */
    inline ListObjectVersionsResult& WithDeleteMarkers(Aws::Vector<DeleteMarkerEntry>&& value) { SetDeleteMarkers(std::move(value)); return *this;}

    /**
     * <p>Container for an object that is a delete marker.</p>
     */
    inline ListObjectVersionsResult& AddDeleteMarkers(const DeleteMarkerEntry& value) { m_deleteMarkers.push_back(value); return *this; }

    /**
     * <p>Container for an object that is a delete marker.</p>
     */
    inline ListObjectVersionsResult& AddDeleteMarkers(DeleteMarkerEntry&& value) { m_deleteMarkers.push_back(std::move(value)); return *this; }


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
    inline ListObjectVersionsResult& WithName(const Aws::String& value) { SetName(value); return *this;}

    /**
     * <p>Bucket name.</p>
     */
    inline ListObjectVersionsResult& WithName(Aws::String&& value) { SetName(std::move(value)); return *this;}

    /**
     * <p>Bucket name.</p>
     */
    inline ListObjectVersionsResult& WithName(const char* value) { SetName(value); return *this;}


    /**
     * <p>Selects objects that start with the value supplied by this parameter.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>Selects objects that start with the value supplied by this parameter.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefix = value; }

    /**
     * <p>Selects objects that start with the value supplied by this parameter.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefix = std::move(value); }

    /**
     * <p>Selects objects that start with the value supplied by this parameter.</p>
     */
    inline void SetPrefix(const char* value) { m_prefix.assign(value); }

    /**
     * <p>Selects objects that start with the value supplied by this parameter.</p>
     */
    inline ListObjectVersionsResult& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>Selects objects that start with the value supplied by this parameter.</p>
     */
    inline ListObjectVersionsResult& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>Selects objects that start with the value supplied by this parameter.</p>
     */
    inline ListObjectVersionsResult& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>The delimiter grouping the included keys. A delimiter is a character that you
     * specify to group keys. All keys that contain the same string between the prefix
     * and the first occurrence of the delimiter are grouped under a single result
     * element in <code>CommonPrefixes</code>. These groups are counted as one result
     * against the max-keys limitation. These keys are not returned elsewhere in the
     * response.</p>
     */
    inline const Aws::String& GetDelimiter() const{ return m_delimiter; }

    /**
     * <p>The delimiter grouping the included keys. A delimiter is a character that you
     * specify to group keys. All keys that contain the same string between the prefix
     * and the first occurrence of the delimiter are grouped under a single result
     * element in <code>CommonPrefixes</code>. These groups are counted as one result
     * against the max-keys limitation. These keys are not returned elsewhere in the
     * response.</p>
     */
    inline void SetDelimiter(const Aws::String& value) { m_delimiter = value; }

    /**
     * <p>The delimiter grouping the included keys. A delimiter is a character that you
     * specify to group keys. All keys that contain the same string between the prefix
     * and the first occurrence of the delimiter are grouped under a single result
     * element in <code>CommonPrefixes</code>. These groups are counted as one result
     * against the max-keys limitation. These keys are not returned elsewhere in the
     * response.</p>
     */
    inline void SetDelimiter(Aws::String&& value) { m_delimiter = std::move(value); }

    /**
     * <p>The delimiter grouping the included keys. A delimiter is a character that you
     * specify to group keys. All keys that contain the same string between the prefix
     * and the first occurrence of the delimiter are grouped under a single result
     * element in <code>CommonPrefixes</code>. These groups are counted as one result
     * against the max-keys limitation. These keys are not returned elsewhere in the
     * response.</p>
     */
    inline void SetDelimiter(const char* value) { m_delimiter.assign(value); }

    /**
     * <p>The delimiter grouping the included keys. A delimiter is a character that you
     * specify to group keys. All keys that contain the same string between the prefix
     * and the first occurrence of the delimiter are grouped under a single result
     * element in <code>CommonPrefixes</code>. These groups are counted as one result
     * against the max-keys limitation. These keys are not returned elsewhere in the
     * response.</p>
     */
    inline ListObjectVersionsResult& WithDelimiter(const Aws::String& value) { SetDelimiter(value); return *this;}

    /**
     * <p>The delimiter grouping the included keys. A delimiter is a character that you
     * specify to group keys. All keys that contain the same string between the prefix
     * and the first occurrence of the delimiter are grouped under a single result
     * element in <code>CommonPrefixes</code>. These groups are counted as one result
     * against the max-keys limitation. These keys are not returned elsewhere in the
     * response.</p>
     */
    inline ListObjectVersionsResult& WithDelimiter(Aws::String&& value) { SetDelimiter(std::move(value)); return *this;}

    /**
     * <p>The delimiter grouping the included keys. A delimiter is a character that you
     * specify to group keys. All keys that contain the same string between the prefix
     * and the first occurrence of the delimiter are grouped under a single result
     * element in <code>CommonPrefixes</code>. These groups are counted as one result
     * against the max-keys limitation. These keys are not returned elsewhere in the
     * response.</p>
     */
    inline ListObjectVersionsResult& WithDelimiter(const char* value) { SetDelimiter(value); return *this;}


    /**
     * <p>Specifies the maximum number of objects to return.</p>
     */
    inline int GetMaxKeys() const{ return m_maxKeys; }

    /**
     * <p>Specifies the maximum number of objects to return.</p>
     */
    inline void SetMaxKeys(int value) { m_maxKeys = value; }

    /**
     * <p>Specifies the maximum number of objects to return.</p>
     */
    inline ListObjectVersionsResult& WithMaxKeys(int value) { SetMaxKeys(value); return *this;}


    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p>
     */
    inline const Aws::Vector<CommonPrefix>& GetCommonPrefixes() const{ return m_commonPrefixes; }

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p>
     */
    inline void SetCommonPrefixes(const Aws::Vector<CommonPrefix>& value) { m_commonPrefixes = value; }

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p>
     */
    inline void SetCommonPrefixes(Aws::Vector<CommonPrefix>&& value) { m_commonPrefixes = std::move(value); }

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p>
     */
    inline ListObjectVersionsResult& WithCommonPrefixes(const Aws::Vector<CommonPrefix>& value) { SetCommonPrefixes(value); return *this;}

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p>
     */
    inline ListObjectVersionsResult& WithCommonPrefixes(Aws::Vector<CommonPrefix>&& value) { SetCommonPrefixes(std::move(value)); return *this;}

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p>
     */
    inline ListObjectVersionsResult& AddCommonPrefixes(const CommonPrefix& value) { m_commonPrefixes.push_back(value); return *this; }

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p>
     */
    inline ListObjectVersionsResult& AddCommonPrefixes(CommonPrefix&& value) { m_commonPrefixes.push_back(std::move(value)); return *this; }


    /**
     * <p> Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>KeyMarker, NextKeyMarker, Prefix,
     * Key</code>, and <code>Delimiter</code>.</p>
     */
    inline const EncodingType& GetEncodingType() const{ return m_encodingType; }

    /**
     * <p> Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>KeyMarker, NextKeyMarker, Prefix,
     * Key</code>, and <code>Delimiter</code>.</p>
     */
    inline void SetEncodingType(const EncodingType& value) { m_encodingType = value; }

    /**
     * <p> Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>KeyMarker, NextKeyMarker, Prefix,
     * Key</code>, and <code>Delimiter</code>.</p>
     */
    inline void SetEncodingType(EncodingType&& value) { m_encodingType = std::move(value); }

    /**
     * <p> Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>KeyMarker, NextKeyMarker, Prefix,
     * Key</code>, and <code>Delimiter</code>.</p>
     */
    inline ListObjectVersionsResult& WithEncodingType(const EncodingType& value) { SetEncodingType(value); return *this;}

    /**
     * <p> Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>KeyMarker, NextKeyMarker, Prefix,
     * Key</code>, and <code>Delimiter</code>.</p>
     */
    inline ListObjectVersionsResult& WithEncodingType(EncodingType&& value) { SetEncodingType(std::move(value)); return *this;}

  private:

    bool m_isTruncated;

    Aws::String m_keyMarker;

    Aws::String m_versionIdMarker;

    Aws::String m_nextKeyMarker;

    Aws::String m_nextVersionIdMarker;

    Aws::Vector<ObjectVersion> m_versions;

    Aws::Vector<DeleteMarkerEntry> m_deleteMarkers;

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
