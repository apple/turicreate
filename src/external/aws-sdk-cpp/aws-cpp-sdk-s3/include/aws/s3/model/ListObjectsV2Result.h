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
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/core/utils/memory/stl/AWSString.h>
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
  class AWS_S3_API ListObjectsV2Result
  {
  public:
    ListObjectsV2Result();
    ListObjectsV2Result(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    ListObjectsV2Result& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>Set to false if all of the results were returned. Set to true if more keys
     * are available to return. If the number of results exceeds that specified by
     * MaxKeys, all of the results might not be returned.</p>
     */
    inline bool GetIsTruncated() const{ return m_isTruncated; }

    /**
     * <p>Set to false if all of the results were returned. Set to true if more keys
     * are available to return. If the number of results exceeds that specified by
     * MaxKeys, all of the results might not be returned.</p>
     */
    inline void SetIsTruncated(bool value) { m_isTruncated = value; }

    /**
     * <p>Set to false if all of the results were returned. Set to true if more keys
     * are available to return. If the number of results exceeds that specified by
     * MaxKeys, all of the results might not be returned.</p>
     */
    inline ListObjectsV2Result& WithIsTruncated(bool value) { SetIsTruncated(value); return *this;}


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
    inline ListObjectsV2Result& WithContents(const Aws::Vector<Object>& value) { SetContents(value); return *this;}

    /**
     * <p>Metadata about each object returned.</p>
     */
    inline ListObjectsV2Result& WithContents(Aws::Vector<Object>&& value) { SetContents(std::move(value)); return *this;}

    /**
     * <p>Metadata about each object returned.</p>
     */
    inline ListObjectsV2Result& AddContents(const Object& value) { m_contents.push_back(value); return *this; }

    /**
     * <p>Metadata about each object returned.</p>
     */
    inline ListObjectsV2Result& AddContents(Object&& value) { m_contents.push_back(std::move(value)); return *this; }


    /**
     * <p>Bucket name. </p> <p>When using this API with an access point, you must
     * direct requests to the access point hostname. The access point hostname takes
     * the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline const Aws::String& GetName() const{ return m_name; }

    /**
     * <p>Bucket name. </p> <p>When using this API with an access point, you must
     * direct requests to the access point hostname. The access point hostname takes
     * the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetName(const Aws::String& value) { m_name = value; }

    /**
     * <p>Bucket name. </p> <p>When using this API with an access point, you must
     * direct requests to the access point hostname. The access point hostname takes
     * the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetName(Aws::String&& value) { m_name = std::move(value); }

    /**
     * <p>Bucket name. </p> <p>When using this API with an access point, you must
     * direct requests to the access point hostname. The access point hostname takes
     * the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetName(const char* value) { m_name.assign(value); }

    /**
     * <p>Bucket name. </p> <p>When using this API with an access point, you must
     * direct requests to the access point hostname. The access point hostname takes
     * the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ListObjectsV2Result& WithName(const Aws::String& value) { SetName(value); return *this;}

    /**
     * <p>Bucket name. </p> <p>When using this API with an access point, you must
     * direct requests to the access point hostname. The access point hostname takes
     * the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ListObjectsV2Result& WithName(Aws::String&& value) { SetName(std::move(value)); return *this;}

    /**
     * <p>Bucket name. </p> <p>When using this API with an access point, you must
     * direct requests to the access point hostname. The access point hostname takes
     * the form
     * <i>AccessPointName</i>-<i>AccountId</i>.s3-accesspoint.<i>Region</i>.amazonaws.com.
     * When using this operation using an access point through the AWS SDKs, you
     * provide the access point ARN in place of the bucket name. For more information
     * about access point ARNs, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/using-access-points.html">Using
     * Access Points</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ListObjectsV2Result& WithName(const char* value) { SetName(value); return *this;}


    /**
     * <p> Keys that begin with the indicated prefix.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p> Keys that begin with the indicated prefix.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefix = value; }

    /**
     * <p> Keys that begin with the indicated prefix.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefix = std::move(value); }

    /**
     * <p> Keys that begin with the indicated prefix.</p>
     */
    inline void SetPrefix(const char* value) { m_prefix.assign(value); }

    /**
     * <p> Keys that begin with the indicated prefix.</p>
     */
    inline ListObjectsV2Result& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p> Keys that begin with the indicated prefix.</p>
     */
    inline ListObjectsV2Result& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p> Keys that begin with the indicated prefix.</p>
     */
    inline ListObjectsV2Result& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * CommonPrefixes collection. These rolled-up keys are not returned elsewhere in
     * the response. Each rolled-up result counts as only one return against the
     * <code>MaxKeys</code> value.</p>
     */
    inline const Aws::String& GetDelimiter() const{ return m_delimiter; }

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * CommonPrefixes collection. These rolled-up keys are not returned elsewhere in
     * the response. Each rolled-up result counts as only one return against the
     * <code>MaxKeys</code> value.</p>
     */
    inline void SetDelimiter(const Aws::String& value) { m_delimiter = value; }

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * CommonPrefixes collection. These rolled-up keys are not returned elsewhere in
     * the response. Each rolled-up result counts as only one return against the
     * <code>MaxKeys</code> value.</p>
     */
    inline void SetDelimiter(Aws::String&& value) { m_delimiter = std::move(value); }

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * CommonPrefixes collection. These rolled-up keys are not returned elsewhere in
     * the response. Each rolled-up result counts as only one return against the
     * <code>MaxKeys</code> value.</p>
     */
    inline void SetDelimiter(const char* value) { m_delimiter.assign(value); }

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * CommonPrefixes collection. These rolled-up keys are not returned elsewhere in
     * the response. Each rolled-up result counts as only one return against the
     * <code>MaxKeys</code> value.</p>
     */
    inline ListObjectsV2Result& WithDelimiter(const Aws::String& value) { SetDelimiter(value); return *this;}

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * CommonPrefixes collection. These rolled-up keys are not returned elsewhere in
     * the response. Each rolled-up result counts as only one return against the
     * <code>MaxKeys</code> value.</p>
     */
    inline ListObjectsV2Result& WithDelimiter(Aws::String&& value) { SetDelimiter(std::move(value)); return *this;}

    /**
     * <p>Causes keys that contain the same string between the prefix and the first
     * occurrence of the delimiter to be rolled up into a single result element in the
     * CommonPrefixes collection. These rolled-up keys are not returned elsewhere in
     * the response. Each rolled-up result counts as only one return against the
     * <code>MaxKeys</code> value.</p>
     */
    inline ListObjectsV2Result& WithDelimiter(const char* value) { SetDelimiter(value); return *this;}


    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more.</p>
     */
    inline int GetMaxKeys() const{ return m_maxKeys; }

    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more.</p>
     */
    inline void SetMaxKeys(int value) { m_maxKeys = value; }

    /**
     * <p>Sets the maximum number of keys returned in the response. The response might
     * contain fewer keys but will never contain more.</p>
     */
    inline ListObjectsV2Result& WithMaxKeys(int value) { SetMaxKeys(value); return *this;}


    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p> <p>A response can contain
     * <code>CommonPrefixes</code> only if you specify a delimiter.</p> <p>
     * <code>CommonPrefixes</code> contains all (if there are any) keys between
     * <code>Prefix</code> and the next occurrence of the string specified by a
     * delimiter.</p> <p> <code>CommonPrefixes</code> lists keys that act like
     * subdirectories in the directory specified by <code>Prefix</code>.</p> <p>For
     * example, if the prefix is <code>notes/</code> and the delimiter is a slash
     * (<code>/</code>) as in <code>notes/summer/july</code>, the common prefix is
     * <code>notes/summer/</code>. All of the keys that roll up into a common prefix
     * count as a single return when calculating the number of returns. </p>
     */
    inline const Aws::Vector<CommonPrefix>& GetCommonPrefixes() const{ return m_commonPrefixes; }

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p> <p>A response can contain
     * <code>CommonPrefixes</code> only if you specify a delimiter.</p> <p>
     * <code>CommonPrefixes</code> contains all (if there are any) keys between
     * <code>Prefix</code> and the next occurrence of the string specified by a
     * delimiter.</p> <p> <code>CommonPrefixes</code> lists keys that act like
     * subdirectories in the directory specified by <code>Prefix</code>.</p> <p>For
     * example, if the prefix is <code>notes/</code> and the delimiter is a slash
     * (<code>/</code>) as in <code>notes/summer/july</code>, the common prefix is
     * <code>notes/summer/</code>. All of the keys that roll up into a common prefix
     * count as a single return when calculating the number of returns. </p>
     */
    inline void SetCommonPrefixes(const Aws::Vector<CommonPrefix>& value) { m_commonPrefixes = value; }

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p> <p>A response can contain
     * <code>CommonPrefixes</code> only if you specify a delimiter.</p> <p>
     * <code>CommonPrefixes</code> contains all (if there are any) keys between
     * <code>Prefix</code> and the next occurrence of the string specified by a
     * delimiter.</p> <p> <code>CommonPrefixes</code> lists keys that act like
     * subdirectories in the directory specified by <code>Prefix</code>.</p> <p>For
     * example, if the prefix is <code>notes/</code> and the delimiter is a slash
     * (<code>/</code>) as in <code>notes/summer/july</code>, the common prefix is
     * <code>notes/summer/</code>. All of the keys that roll up into a common prefix
     * count as a single return when calculating the number of returns. </p>
     */
    inline void SetCommonPrefixes(Aws::Vector<CommonPrefix>&& value) { m_commonPrefixes = std::move(value); }

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p> <p>A response can contain
     * <code>CommonPrefixes</code> only if you specify a delimiter.</p> <p>
     * <code>CommonPrefixes</code> contains all (if there are any) keys between
     * <code>Prefix</code> and the next occurrence of the string specified by a
     * delimiter.</p> <p> <code>CommonPrefixes</code> lists keys that act like
     * subdirectories in the directory specified by <code>Prefix</code>.</p> <p>For
     * example, if the prefix is <code>notes/</code> and the delimiter is a slash
     * (<code>/</code>) as in <code>notes/summer/july</code>, the common prefix is
     * <code>notes/summer/</code>. All of the keys that roll up into a common prefix
     * count as a single return when calculating the number of returns. </p>
     */
    inline ListObjectsV2Result& WithCommonPrefixes(const Aws::Vector<CommonPrefix>& value) { SetCommonPrefixes(value); return *this;}

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p> <p>A response can contain
     * <code>CommonPrefixes</code> only if you specify a delimiter.</p> <p>
     * <code>CommonPrefixes</code> contains all (if there are any) keys between
     * <code>Prefix</code> and the next occurrence of the string specified by a
     * delimiter.</p> <p> <code>CommonPrefixes</code> lists keys that act like
     * subdirectories in the directory specified by <code>Prefix</code>.</p> <p>For
     * example, if the prefix is <code>notes/</code> and the delimiter is a slash
     * (<code>/</code>) as in <code>notes/summer/july</code>, the common prefix is
     * <code>notes/summer/</code>. All of the keys that roll up into a common prefix
     * count as a single return when calculating the number of returns. </p>
     */
    inline ListObjectsV2Result& WithCommonPrefixes(Aws::Vector<CommonPrefix>&& value) { SetCommonPrefixes(std::move(value)); return *this;}

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p> <p>A response can contain
     * <code>CommonPrefixes</code> only if you specify a delimiter.</p> <p>
     * <code>CommonPrefixes</code> contains all (if there are any) keys between
     * <code>Prefix</code> and the next occurrence of the string specified by a
     * delimiter.</p> <p> <code>CommonPrefixes</code> lists keys that act like
     * subdirectories in the directory specified by <code>Prefix</code>.</p> <p>For
     * example, if the prefix is <code>notes/</code> and the delimiter is a slash
     * (<code>/</code>) as in <code>notes/summer/july</code>, the common prefix is
     * <code>notes/summer/</code>. All of the keys that roll up into a common prefix
     * count as a single return when calculating the number of returns. </p>
     */
    inline ListObjectsV2Result& AddCommonPrefixes(const CommonPrefix& value) { m_commonPrefixes.push_back(value); return *this; }

    /**
     * <p>All of the keys rolled up into a common prefix count as a single return when
     * calculating the number of returns.</p> <p>A response can contain
     * <code>CommonPrefixes</code> only if you specify a delimiter.</p> <p>
     * <code>CommonPrefixes</code> contains all (if there are any) keys between
     * <code>Prefix</code> and the next occurrence of the string specified by a
     * delimiter.</p> <p> <code>CommonPrefixes</code> lists keys that act like
     * subdirectories in the directory specified by <code>Prefix</code>.</p> <p>For
     * example, if the prefix is <code>notes/</code> and the delimiter is a slash
     * (<code>/</code>) as in <code>notes/summer/july</code>, the common prefix is
     * <code>notes/summer/</code>. All of the keys that roll up into a common prefix
     * count as a single return when calculating the number of returns. </p>
     */
    inline ListObjectsV2Result& AddCommonPrefixes(CommonPrefix&& value) { m_commonPrefixes.push_back(std::move(value)); return *this; }


    /**
     * <p>Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify the encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>Delimiter, Prefix, Key,</code>
     * and <code>StartAfter</code>.</p>
     */
    inline const EncodingType& GetEncodingType() const{ return m_encodingType; }

    /**
     * <p>Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify the encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>Delimiter, Prefix, Key,</code>
     * and <code>StartAfter</code>.</p>
     */
    inline void SetEncodingType(const EncodingType& value) { m_encodingType = value; }

    /**
     * <p>Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify the encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>Delimiter, Prefix, Key,</code>
     * and <code>StartAfter</code>.</p>
     */
    inline void SetEncodingType(EncodingType&& value) { m_encodingType = std::move(value); }

    /**
     * <p>Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify the encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>Delimiter, Prefix, Key,</code>
     * and <code>StartAfter</code>.</p>
     */
    inline ListObjectsV2Result& WithEncodingType(const EncodingType& value) { SetEncodingType(value); return *this;}

    /**
     * <p>Encoding type used by Amazon S3 to encode object key names in the XML
     * response.</p> <p>If you specify the encoding-type request parameter, Amazon S3
     * includes this element in the response, and returns encoded key name values in
     * the following response elements:</p> <p> <code>Delimiter, Prefix, Key,</code>
     * and <code>StartAfter</code>.</p>
     */
    inline ListObjectsV2Result& WithEncodingType(EncodingType&& value) { SetEncodingType(std::move(value)); return *this;}


    /**
     * <p>KeyCount is the number of keys returned with this request. KeyCount will
     * always be less than equals to MaxKeys field. Say you ask for 50 keys, your
     * result will include less than equals 50 keys </p>
     */
    inline int GetKeyCount() const{ return m_keyCount; }

    /**
     * <p>KeyCount is the number of keys returned with this request. KeyCount will
     * always be less than equals to MaxKeys field. Say you ask for 50 keys, your
     * result will include less than equals 50 keys </p>
     */
    inline void SetKeyCount(int value) { m_keyCount = value; }

    /**
     * <p>KeyCount is the number of keys returned with this request. KeyCount will
     * always be less than equals to MaxKeys field. Say you ask for 50 keys, your
     * result will include less than equals 50 keys </p>
     */
    inline ListObjectsV2Result& WithKeyCount(int value) { SetKeyCount(value); return *this;}


    /**
     * <p> If ContinuationToken was sent with the request, it is included in the
     * response.</p>
     */
    inline const Aws::String& GetContinuationToken() const{ return m_continuationToken; }

    /**
     * <p> If ContinuationToken was sent with the request, it is included in the
     * response.</p>
     */
    inline void SetContinuationToken(const Aws::String& value) { m_continuationToken = value; }

    /**
     * <p> If ContinuationToken was sent with the request, it is included in the
     * response.</p>
     */
    inline void SetContinuationToken(Aws::String&& value) { m_continuationToken = std::move(value); }

    /**
     * <p> If ContinuationToken was sent with the request, it is included in the
     * response.</p>
     */
    inline void SetContinuationToken(const char* value) { m_continuationToken.assign(value); }

    /**
     * <p> If ContinuationToken was sent with the request, it is included in the
     * response.</p>
     */
    inline ListObjectsV2Result& WithContinuationToken(const Aws::String& value) { SetContinuationToken(value); return *this;}

    /**
     * <p> If ContinuationToken was sent with the request, it is included in the
     * response.</p>
     */
    inline ListObjectsV2Result& WithContinuationToken(Aws::String&& value) { SetContinuationToken(std::move(value)); return *this;}

    /**
     * <p> If ContinuationToken was sent with the request, it is included in the
     * response.</p>
     */
    inline ListObjectsV2Result& WithContinuationToken(const char* value) { SetContinuationToken(value); return *this;}


    /**
     * <p> <code>NextContinuationToken</code> is sent when <code>isTruncated</code> is
     * true, which means there are more keys in the bucket that can be listed. The next
     * list requests to Amazon S3 can be continued with this
     * <code>NextContinuationToken</code>. <code>NextContinuationToken</code> is
     * obfuscated and is not a real key</p>
     */
    inline const Aws::String& GetNextContinuationToken() const{ return m_nextContinuationToken; }

    /**
     * <p> <code>NextContinuationToken</code> is sent when <code>isTruncated</code> is
     * true, which means there are more keys in the bucket that can be listed. The next
     * list requests to Amazon S3 can be continued with this
     * <code>NextContinuationToken</code>. <code>NextContinuationToken</code> is
     * obfuscated and is not a real key</p>
     */
    inline void SetNextContinuationToken(const Aws::String& value) { m_nextContinuationToken = value; }

    /**
     * <p> <code>NextContinuationToken</code> is sent when <code>isTruncated</code> is
     * true, which means there are more keys in the bucket that can be listed. The next
     * list requests to Amazon S3 can be continued with this
     * <code>NextContinuationToken</code>. <code>NextContinuationToken</code> is
     * obfuscated and is not a real key</p>
     */
    inline void SetNextContinuationToken(Aws::String&& value) { m_nextContinuationToken = std::move(value); }

    /**
     * <p> <code>NextContinuationToken</code> is sent when <code>isTruncated</code> is
     * true, which means there are more keys in the bucket that can be listed. The next
     * list requests to Amazon S3 can be continued with this
     * <code>NextContinuationToken</code>. <code>NextContinuationToken</code> is
     * obfuscated and is not a real key</p>
     */
    inline void SetNextContinuationToken(const char* value) { m_nextContinuationToken.assign(value); }

    /**
     * <p> <code>NextContinuationToken</code> is sent when <code>isTruncated</code> is
     * true, which means there are more keys in the bucket that can be listed. The next
     * list requests to Amazon S3 can be continued with this
     * <code>NextContinuationToken</code>. <code>NextContinuationToken</code> is
     * obfuscated and is not a real key</p>
     */
    inline ListObjectsV2Result& WithNextContinuationToken(const Aws::String& value) { SetNextContinuationToken(value); return *this;}

    /**
     * <p> <code>NextContinuationToken</code> is sent when <code>isTruncated</code> is
     * true, which means there are more keys in the bucket that can be listed. The next
     * list requests to Amazon S3 can be continued with this
     * <code>NextContinuationToken</code>. <code>NextContinuationToken</code> is
     * obfuscated and is not a real key</p>
     */
    inline ListObjectsV2Result& WithNextContinuationToken(Aws::String&& value) { SetNextContinuationToken(std::move(value)); return *this;}

    /**
     * <p> <code>NextContinuationToken</code> is sent when <code>isTruncated</code> is
     * true, which means there are more keys in the bucket that can be listed. The next
     * list requests to Amazon S3 can be continued with this
     * <code>NextContinuationToken</code>. <code>NextContinuationToken</code> is
     * obfuscated and is not a real key</p>
     */
    inline ListObjectsV2Result& WithNextContinuationToken(const char* value) { SetNextContinuationToken(value); return *this;}


    /**
     * <p>If StartAfter was sent with the request, it is included in the response.</p>
     */
    inline const Aws::String& GetStartAfter() const{ return m_startAfter; }

    /**
     * <p>If StartAfter was sent with the request, it is included in the response.</p>
     */
    inline void SetStartAfter(const Aws::String& value) { m_startAfter = value; }

    /**
     * <p>If StartAfter was sent with the request, it is included in the response.</p>
     */
    inline void SetStartAfter(Aws::String&& value) { m_startAfter = std::move(value); }

    /**
     * <p>If StartAfter was sent with the request, it is included in the response.</p>
     */
    inline void SetStartAfter(const char* value) { m_startAfter.assign(value); }

    /**
     * <p>If StartAfter was sent with the request, it is included in the response.</p>
     */
    inline ListObjectsV2Result& WithStartAfter(const Aws::String& value) { SetStartAfter(value); return *this;}

    /**
     * <p>If StartAfter was sent with the request, it is included in the response.</p>
     */
    inline ListObjectsV2Result& WithStartAfter(Aws::String&& value) { SetStartAfter(std::move(value)); return *this;}

    /**
     * <p>If StartAfter was sent with the request, it is included in the response.</p>
     */
    inline ListObjectsV2Result& WithStartAfter(const char* value) { SetStartAfter(value); return *this;}

  private:

    bool m_isTruncated;

    Aws::Vector<Object> m_contents;

    Aws::String m_name;

    Aws::String m_prefix;

    Aws::String m_delimiter;

    int m_maxKeys;

    Aws::Vector<CommonPrefix> m_commonPrefixes;

    EncodingType m_encodingType;

    int m_keyCount;

    Aws::String m_continuationToken;

    Aws::String m_nextContinuationToken;

    Aws::String m_startAfter;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
