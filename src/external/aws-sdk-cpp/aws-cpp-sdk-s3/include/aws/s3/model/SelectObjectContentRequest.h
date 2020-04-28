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
#include <aws/s3/model/SelectObjectContentHandler.h>
#include <aws/core/utils/event/EventStreamDecoder.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/ExpressionType.h>
#include <aws/s3/model/RequestProgress.h>
#include <aws/s3/model/InputSerialization.h>
#include <aws/s3/model/OutputSerialization.h>
#include <aws/s3/model/ScanRange.h>
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
   * <p>Request to filter the contents of an Amazon S3 object based on a simple
   * Structured Query Language (SQL) statement. In the request, along with the SQL
   * expression, you must specify a data serialization format (JSON or CSV) of the
   * object. Amazon S3 uses this to parse object data into records. It returns only
   * records that match the specified SQL expression. You must also specify the data
   * serialization format for the response. For more information, see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTObjectSELECTContent.html">S3Select
   * API Documentation</a>.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/SelectObjectContentRequest">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API SelectObjectContentRequest : public S3Request
  {
  public:
    SelectObjectContentRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "SelectObjectContent"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;

    /**
     * Underlying Event Stream Decoder.
     */
    inline Aws::Utils::Event::EventStreamDecoder& GetEventStreamDecoder() { return m_decoder; }

    /**
     * Underlying Event Stream Handler which is used to define callback functions.
     */
    inline const SelectObjectContentHandler& GetEventStreamHandler() const { return m_handler; }

    /**
     * Underlying Event Stream Handler which is used to define callback functions.
     */
    inline void SetEventStreamHandler(const SelectObjectContentHandler& value) { m_handler = value; m_decoder.ResetEventStreamHandler(&m_handler); }

    /**
     * Underlying Event Stream Handler which is used to define callback functions.
     */
    inline SelectObjectContentRequest& WithEventStreamHandler(const SelectObjectContentHandler& value) { SetEventStreamHandler(value); return *this; }


    /**
     * <p>The S3 bucket.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>The S3 bucket.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The S3 bucket.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The S3 bucket.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The S3 bucket.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The S3 bucket.</p>
     */
    inline SelectObjectContentRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The S3 bucket.</p>
     */
    inline SelectObjectContentRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The S3 bucket.</p>
     */
    inline SelectObjectContentRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>The object key.</p>
     */
    inline const Aws::String& GetKey() const{ return m_key; }

    /**
     * <p>The object key.</p>
     */
    inline bool KeyHasBeenSet() const { return m_keyHasBeenSet; }

    /**
     * <p>The object key.</p>
     */
    inline void SetKey(const Aws::String& value) { m_keyHasBeenSet = true; m_key = value; }

    /**
     * <p>The object key.</p>
     */
    inline void SetKey(Aws::String&& value) { m_keyHasBeenSet = true; m_key = std::move(value); }

    /**
     * <p>The object key.</p>
     */
    inline void SetKey(const char* value) { m_keyHasBeenSet = true; m_key.assign(value); }

    /**
     * <p>The object key.</p>
     */
    inline SelectObjectContentRequest& WithKey(const Aws::String& value) { SetKey(value); return *this;}

    /**
     * <p>The object key.</p>
     */
    inline SelectObjectContentRequest& WithKey(Aws::String&& value) { SetKey(std::move(value)); return *this;}

    /**
     * <p>The object key.</p>
     */
    inline SelectObjectContentRequest& WithKey(const char* value) { SetKey(value); return *this;}


    /**
     * <p>The SSE Algorithm used to encrypt the object. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline const Aws::String& GetSSECustomerAlgorithm() const{ return m_sSECustomerAlgorithm; }

    /**
     * <p>The SSE Algorithm used to encrypt the object. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline bool SSECustomerAlgorithmHasBeenSet() const { return m_sSECustomerAlgorithmHasBeenSet; }

    /**
     * <p>The SSE Algorithm used to encrypt the object. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline void SetSSECustomerAlgorithm(const Aws::String& value) { m_sSECustomerAlgorithmHasBeenSet = true; m_sSECustomerAlgorithm = value; }

    /**
     * <p>The SSE Algorithm used to encrypt the object. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline void SetSSECustomerAlgorithm(Aws::String&& value) { m_sSECustomerAlgorithmHasBeenSet = true; m_sSECustomerAlgorithm = std::move(value); }

    /**
     * <p>The SSE Algorithm used to encrypt the object. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline void SetSSECustomerAlgorithm(const char* value) { m_sSECustomerAlgorithmHasBeenSet = true; m_sSECustomerAlgorithm.assign(value); }

    /**
     * <p>The SSE Algorithm used to encrypt the object. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline SelectObjectContentRequest& WithSSECustomerAlgorithm(const Aws::String& value) { SetSSECustomerAlgorithm(value); return *this;}

    /**
     * <p>The SSE Algorithm used to encrypt the object. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline SelectObjectContentRequest& WithSSECustomerAlgorithm(Aws::String&& value) { SetSSECustomerAlgorithm(std::move(value)); return *this;}

    /**
     * <p>The SSE Algorithm used to encrypt the object. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline SelectObjectContentRequest& WithSSECustomerAlgorithm(const char* value) { SetSSECustomerAlgorithm(value); return *this;}


    /**
     * <p>The SSE Customer Key. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline const Aws::String& GetSSECustomerKey() const{ return m_sSECustomerKey; }

    /**
     * <p>The SSE Customer Key. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline bool SSECustomerKeyHasBeenSet() const { return m_sSECustomerKeyHasBeenSet; }

    /**
     * <p>The SSE Customer Key. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline void SetSSECustomerKey(const Aws::String& value) { m_sSECustomerKeyHasBeenSet = true; m_sSECustomerKey = value; }

    /**
     * <p>The SSE Customer Key. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline void SetSSECustomerKey(Aws::String&& value) { m_sSECustomerKeyHasBeenSet = true; m_sSECustomerKey = std::move(value); }

    /**
     * <p>The SSE Customer Key. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline void SetSSECustomerKey(const char* value) { m_sSECustomerKeyHasBeenSet = true; m_sSECustomerKey.assign(value); }

    /**
     * <p>The SSE Customer Key. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline SelectObjectContentRequest& WithSSECustomerKey(const Aws::String& value) { SetSSECustomerKey(value); return *this;}

    /**
     * <p>The SSE Customer Key. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline SelectObjectContentRequest& WithSSECustomerKey(Aws::String&& value) { SetSSECustomerKey(std::move(value)); return *this;}

    /**
     * <p>The SSE Customer Key. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline SelectObjectContentRequest& WithSSECustomerKey(const char* value) { SetSSECustomerKey(value); return *this;}


    /**
     * <p>The SSE Customer Key MD5. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline const Aws::String& GetSSECustomerKeyMD5() const{ return m_sSECustomerKeyMD5; }

    /**
     * <p>The SSE Customer Key MD5. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline bool SSECustomerKeyMD5HasBeenSet() const { return m_sSECustomerKeyMD5HasBeenSet; }

    /**
     * <p>The SSE Customer Key MD5. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline void SetSSECustomerKeyMD5(const Aws::String& value) { m_sSECustomerKeyMD5HasBeenSet = true; m_sSECustomerKeyMD5 = value; }

    /**
     * <p>The SSE Customer Key MD5. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline void SetSSECustomerKeyMD5(Aws::String&& value) { m_sSECustomerKeyMD5HasBeenSet = true; m_sSECustomerKeyMD5 = std::move(value); }

    /**
     * <p>The SSE Customer Key MD5. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline void SetSSECustomerKeyMD5(const char* value) { m_sSECustomerKeyMD5HasBeenSet = true; m_sSECustomerKeyMD5.assign(value); }

    /**
     * <p>The SSE Customer Key MD5. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline SelectObjectContentRequest& WithSSECustomerKeyMD5(const Aws::String& value) { SetSSECustomerKeyMD5(value); return *this;}

    /**
     * <p>The SSE Customer Key MD5. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline SelectObjectContentRequest& WithSSECustomerKeyMD5(Aws::String&& value) { SetSSECustomerKeyMD5(std::move(value)); return *this;}

    /**
     * <p>The SSE Customer Key MD5. For more information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/ServerSideEncryptionCustomerKeys.html">Server-Side
     * Encryption (Using Customer-Provided Encryption Keys</a>. </p>
     */
    inline SelectObjectContentRequest& WithSSECustomerKeyMD5(const char* value) { SetSSECustomerKeyMD5(value); return *this;}


    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline const Aws::String& GetExpression() const{ return m_expression; }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline bool ExpressionHasBeenSet() const { return m_expressionHasBeenSet; }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline void SetExpression(const Aws::String& value) { m_expressionHasBeenSet = true; m_expression = value; }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline void SetExpression(Aws::String&& value) { m_expressionHasBeenSet = true; m_expression = std::move(value); }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline void SetExpression(const char* value) { m_expressionHasBeenSet = true; m_expression.assign(value); }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline SelectObjectContentRequest& WithExpression(const Aws::String& value) { SetExpression(value); return *this;}

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline SelectObjectContentRequest& WithExpression(Aws::String&& value) { SetExpression(std::move(value)); return *this;}

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline SelectObjectContentRequest& WithExpression(const char* value) { SetExpression(value); return *this;}


    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline const ExpressionType& GetExpressionType() const{ return m_expressionType; }

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline bool ExpressionTypeHasBeenSet() const { return m_expressionTypeHasBeenSet; }

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline void SetExpressionType(const ExpressionType& value) { m_expressionTypeHasBeenSet = true; m_expressionType = value; }

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline void SetExpressionType(ExpressionType&& value) { m_expressionTypeHasBeenSet = true; m_expressionType = std::move(value); }

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline SelectObjectContentRequest& WithExpressionType(const ExpressionType& value) { SetExpressionType(value); return *this;}

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline SelectObjectContentRequest& WithExpressionType(ExpressionType&& value) { SetExpressionType(std::move(value)); return *this;}


    /**
     * <p>Specifies if periodic request progress information should be enabled.</p>
     */
    inline const RequestProgress& GetRequestProgress() const{ return m_requestProgress; }

    /**
     * <p>Specifies if periodic request progress information should be enabled.</p>
     */
    inline bool RequestProgressHasBeenSet() const { return m_requestProgressHasBeenSet; }

    /**
     * <p>Specifies if periodic request progress information should be enabled.</p>
     */
    inline void SetRequestProgress(const RequestProgress& value) { m_requestProgressHasBeenSet = true; m_requestProgress = value; }

    /**
     * <p>Specifies if periodic request progress information should be enabled.</p>
     */
    inline void SetRequestProgress(RequestProgress&& value) { m_requestProgressHasBeenSet = true; m_requestProgress = std::move(value); }

    /**
     * <p>Specifies if periodic request progress information should be enabled.</p>
     */
    inline SelectObjectContentRequest& WithRequestProgress(const RequestProgress& value) { SetRequestProgress(value); return *this;}

    /**
     * <p>Specifies if periodic request progress information should be enabled.</p>
     */
    inline SelectObjectContentRequest& WithRequestProgress(RequestProgress&& value) { SetRequestProgress(std::move(value)); return *this;}


    /**
     * <p>Describes the format of the data in the object that is being queried.</p>
     */
    inline const InputSerialization& GetInputSerialization() const{ return m_inputSerialization; }

    /**
     * <p>Describes the format of the data in the object that is being queried.</p>
     */
    inline bool InputSerializationHasBeenSet() const { return m_inputSerializationHasBeenSet; }

    /**
     * <p>Describes the format of the data in the object that is being queried.</p>
     */
    inline void SetInputSerialization(const InputSerialization& value) { m_inputSerializationHasBeenSet = true; m_inputSerialization = value; }

    /**
     * <p>Describes the format of the data in the object that is being queried.</p>
     */
    inline void SetInputSerialization(InputSerialization&& value) { m_inputSerializationHasBeenSet = true; m_inputSerialization = std::move(value); }

    /**
     * <p>Describes the format of the data in the object that is being queried.</p>
     */
    inline SelectObjectContentRequest& WithInputSerialization(const InputSerialization& value) { SetInputSerialization(value); return *this;}

    /**
     * <p>Describes the format of the data in the object that is being queried.</p>
     */
    inline SelectObjectContentRequest& WithInputSerialization(InputSerialization&& value) { SetInputSerialization(std::move(value)); return *this;}


    /**
     * <p>Describes the format of the data that you want Amazon S3 to return in
     * response.</p>
     */
    inline const OutputSerialization& GetOutputSerialization() const{ return m_outputSerialization; }

    /**
     * <p>Describes the format of the data that you want Amazon S3 to return in
     * response.</p>
     */
    inline bool OutputSerializationHasBeenSet() const { return m_outputSerializationHasBeenSet; }

    /**
     * <p>Describes the format of the data that you want Amazon S3 to return in
     * response.</p>
     */
    inline void SetOutputSerialization(const OutputSerialization& value) { m_outputSerializationHasBeenSet = true; m_outputSerialization = value; }

    /**
     * <p>Describes the format of the data that you want Amazon S3 to return in
     * response.</p>
     */
    inline void SetOutputSerialization(OutputSerialization&& value) { m_outputSerializationHasBeenSet = true; m_outputSerialization = std::move(value); }

    /**
     * <p>Describes the format of the data that you want Amazon S3 to return in
     * response.</p>
     */
    inline SelectObjectContentRequest& WithOutputSerialization(const OutputSerialization& value) { SetOutputSerialization(value); return *this;}

    /**
     * <p>Describes the format of the data that you want Amazon S3 to return in
     * response.</p>
     */
    inline SelectObjectContentRequest& WithOutputSerialization(OutputSerialization&& value) { SetOutputSerialization(std::move(value)); return *this;}


    /**
     * <p>Specifies the byte range of the object to get the records from. A record is
     * processed when its first byte is contained by the range. This parameter is
     * optional, but when specified, it must not be empty. See RFC 2616, Section
     * 14.35.1 about how to specify the start and end of the range.</p> <p>
     * <code>ScanRange</code>may be used in the following ways:</p> <ul> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;end&gt;100&lt;/end&gt;&lt;/scanrange&gt;</code>
     * - process only the records starting between the bytes 50 and 100 (inclusive,
     * counting from zero)</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code> -
     * process only the records starting after the byte 50</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> -
     * process only the records within the last 50 bytes of the file.</p> </li> </ul>
     */
    inline const ScanRange& GetScanRange() const{ return m_scanRange; }

    /**
     * <p>Specifies the byte range of the object to get the records from. A record is
     * processed when its first byte is contained by the range. This parameter is
     * optional, but when specified, it must not be empty. See RFC 2616, Section
     * 14.35.1 about how to specify the start and end of the range.</p> <p>
     * <code>ScanRange</code>may be used in the following ways:</p> <ul> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;end&gt;100&lt;/end&gt;&lt;/scanrange&gt;</code>
     * - process only the records starting between the bytes 50 and 100 (inclusive,
     * counting from zero)</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code> -
     * process only the records starting after the byte 50</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> -
     * process only the records within the last 50 bytes of the file.</p> </li> </ul>
     */
    inline bool ScanRangeHasBeenSet() const { return m_scanRangeHasBeenSet; }

    /**
     * <p>Specifies the byte range of the object to get the records from. A record is
     * processed when its first byte is contained by the range. This parameter is
     * optional, but when specified, it must not be empty. See RFC 2616, Section
     * 14.35.1 about how to specify the start and end of the range.</p> <p>
     * <code>ScanRange</code>may be used in the following ways:</p> <ul> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;end&gt;100&lt;/end&gt;&lt;/scanrange&gt;</code>
     * - process only the records starting between the bytes 50 and 100 (inclusive,
     * counting from zero)</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code> -
     * process only the records starting after the byte 50</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> -
     * process only the records within the last 50 bytes of the file.</p> </li> </ul>
     */
    inline void SetScanRange(const ScanRange& value) { m_scanRangeHasBeenSet = true; m_scanRange = value; }

    /**
     * <p>Specifies the byte range of the object to get the records from. A record is
     * processed when its first byte is contained by the range. This parameter is
     * optional, but when specified, it must not be empty. See RFC 2616, Section
     * 14.35.1 about how to specify the start and end of the range.</p> <p>
     * <code>ScanRange</code>may be used in the following ways:</p> <ul> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;end&gt;100&lt;/end&gt;&lt;/scanrange&gt;</code>
     * - process only the records starting between the bytes 50 and 100 (inclusive,
     * counting from zero)</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code> -
     * process only the records starting after the byte 50</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> -
     * process only the records within the last 50 bytes of the file.</p> </li> </ul>
     */
    inline void SetScanRange(ScanRange&& value) { m_scanRangeHasBeenSet = true; m_scanRange = std::move(value); }

    /**
     * <p>Specifies the byte range of the object to get the records from. A record is
     * processed when its first byte is contained by the range. This parameter is
     * optional, but when specified, it must not be empty. See RFC 2616, Section
     * 14.35.1 about how to specify the start and end of the range.</p> <p>
     * <code>ScanRange</code>may be used in the following ways:</p> <ul> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;end&gt;100&lt;/end&gt;&lt;/scanrange&gt;</code>
     * - process only the records starting between the bytes 50 and 100 (inclusive,
     * counting from zero)</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code> -
     * process only the records starting after the byte 50</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> -
     * process only the records within the last 50 bytes of the file.</p> </li> </ul>
     */
    inline SelectObjectContentRequest& WithScanRange(const ScanRange& value) { SetScanRange(value); return *this;}

    /**
     * <p>Specifies the byte range of the object to get the records from. A record is
     * processed when its first byte is contained by the range. This parameter is
     * optional, but when specified, it must not be empty. See RFC 2616, Section
     * 14.35.1 about how to specify the start and end of the range.</p> <p>
     * <code>ScanRange</code>may be used in the following ways:</p> <ul> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;end&gt;100&lt;/end&gt;&lt;/scanrange&gt;</code>
     * - process only the records starting between the bytes 50 and 100 (inclusive,
     * counting from zero)</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code> -
     * process only the records starting after the byte 50</p> </li> <li> <p>
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> -
     * process only the records within the last 50 bytes of the file.</p> </li> </ul>
     */
    inline SelectObjectContentRequest& WithScanRange(ScanRange&& value) { SetScanRange(std::move(value)); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline SelectObjectContentRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline SelectObjectContentRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline SelectObjectContentRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline SelectObjectContentRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline SelectObjectContentRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline SelectObjectContentRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline SelectObjectContentRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline SelectObjectContentRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline SelectObjectContentRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    Aws::String m_key;
    bool m_keyHasBeenSet;

    Aws::String m_sSECustomerAlgorithm;
    bool m_sSECustomerAlgorithmHasBeenSet;

    Aws::String m_sSECustomerKey;
    bool m_sSECustomerKeyHasBeenSet;

    Aws::String m_sSECustomerKeyMD5;
    bool m_sSECustomerKeyMD5HasBeenSet;

    Aws::String m_expression;
    bool m_expressionHasBeenSet;

    ExpressionType m_expressionType;
    bool m_expressionTypeHasBeenSet;

    RequestProgress m_requestProgress;
    bool m_requestProgressHasBeenSet;

    InputSerialization m_inputSerialization;
    bool m_inputSerializationHasBeenSet;

    OutputSerialization m_outputSerialization;
    bool m_outputSerializationHasBeenSet;

    ScanRange m_scanRange;
    bool m_scanRangeHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
    Aws::Utils::Event::EventStreamDecoder m_decoder;
    SelectObjectContentHandler m_handler;

  };

} // namespace Model
} // namespace S3
} // namespace Aws
