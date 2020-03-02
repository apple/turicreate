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
#include <aws/s3/model/FileHeaderInfo.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <utility>

namespace Aws
{
namespace Utils
{
namespace Xml
{
  class XmlNode;
} // namespace Xml
} // namespace Utils
namespace S3
{
namespace Model
{

  /**
   * <p>Describes how an uncompressed comma-separated values (CSV)-formatted input
   * object is formatted.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/CSVInput">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API CSVInput
  {
  public:
    CSVInput();
    CSVInput(const Aws::Utils::Xml::XmlNode& xmlNode);
    CSVInput& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Describes the first line of input. Valid values are:</p> <ul> <li> <p>
     * <code>NONE</code>: First line is not a header.</p> </li> <li> <p>
     * <code>IGNORE</code>: First line is a header, but you can't use the header values
     * to indicate the column in an expression. You can use column position (such as
     * _1, _2, …) to indicate the column (<code>SELECT s._1 FROM OBJECT s</code>).</p>
     * </li> <li> <p> <code>Use</code>: First line is a header, and you can use the
     * header value to identify a column in an expression (<code>SELECT "name" FROM
     * OBJECT</code>). </p> </li> </ul>
     */
    inline const FileHeaderInfo& GetFileHeaderInfo() const{ return m_fileHeaderInfo; }

    /**
     * <p>Describes the first line of input. Valid values are:</p> <ul> <li> <p>
     * <code>NONE</code>: First line is not a header.</p> </li> <li> <p>
     * <code>IGNORE</code>: First line is a header, but you can't use the header values
     * to indicate the column in an expression. You can use column position (such as
     * _1, _2, …) to indicate the column (<code>SELECT s._1 FROM OBJECT s</code>).</p>
     * </li> <li> <p> <code>Use</code>: First line is a header, and you can use the
     * header value to identify a column in an expression (<code>SELECT "name" FROM
     * OBJECT</code>). </p> </li> </ul>
     */
    inline bool FileHeaderInfoHasBeenSet() const { return m_fileHeaderInfoHasBeenSet; }

    /**
     * <p>Describes the first line of input. Valid values are:</p> <ul> <li> <p>
     * <code>NONE</code>: First line is not a header.</p> </li> <li> <p>
     * <code>IGNORE</code>: First line is a header, but you can't use the header values
     * to indicate the column in an expression. You can use column position (such as
     * _1, _2, …) to indicate the column (<code>SELECT s._1 FROM OBJECT s</code>).</p>
     * </li> <li> <p> <code>Use</code>: First line is a header, and you can use the
     * header value to identify a column in an expression (<code>SELECT "name" FROM
     * OBJECT</code>). </p> </li> </ul>
     */
    inline void SetFileHeaderInfo(const FileHeaderInfo& value) { m_fileHeaderInfoHasBeenSet = true; m_fileHeaderInfo = value; }

    /**
     * <p>Describes the first line of input. Valid values are:</p> <ul> <li> <p>
     * <code>NONE</code>: First line is not a header.</p> </li> <li> <p>
     * <code>IGNORE</code>: First line is a header, but you can't use the header values
     * to indicate the column in an expression. You can use column position (such as
     * _1, _2, …) to indicate the column (<code>SELECT s._1 FROM OBJECT s</code>).</p>
     * </li> <li> <p> <code>Use</code>: First line is a header, and you can use the
     * header value to identify a column in an expression (<code>SELECT "name" FROM
     * OBJECT</code>). </p> </li> </ul>
     */
    inline void SetFileHeaderInfo(FileHeaderInfo&& value) { m_fileHeaderInfoHasBeenSet = true; m_fileHeaderInfo = std::move(value); }

    /**
     * <p>Describes the first line of input. Valid values are:</p> <ul> <li> <p>
     * <code>NONE</code>: First line is not a header.</p> </li> <li> <p>
     * <code>IGNORE</code>: First line is a header, but you can't use the header values
     * to indicate the column in an expression. You can use column position (such as
     * _1, _2, …) to indicate the column (<code>SELECT s._1 FROM OBJECT s</code>).</p>
     * </li> <li> <p> <code>Use</code>: First line is a header, and you can use the
     * header value to identify a column in an expression (<code>SELECT "name" FROM
     * OBJECT</code>). </p> </li> </ul>
     */
    inline CSVInput& WithFileHeaderInfo(const FileHeaderInfo& value) { SetFileHeaderInfo(value); return *this;}

    /**
     * <p>Describes the first line of input. Valid values are:</p> <ul> <li> <p>
     * <code>NONE</code>: First line is not a header.</p> </li> <li> <p>
     * <code>IGNORE</code>: First line is a header, but you can't use the header values
     * to indicate the column in an expression. You can use column position (such as
     * _1, _2, …) to indicate the column (<code>SELECT s._1 FROM OBJECT s</code>).</p>
     * </li> <li> <p> <code>Use</code>: First line is a header, and you can use the
     * header value to identify a column in an expression (<code>SELECT "name" FROM
     * OBJECT</code>). </p> </li> </ul>
     */
    inline CSVInput& WithFileHeaderInfo(FileHeaderInfo&& value) { SetFileHeaderInfo(std::move(value)); return *this;}


    /**
     * <p>A single character used to indicate that a row should be ignored when the
     * character is present at the start of that row. You can specify any character to
     * indicate a comment line.</p>
     */
    inline const Aws::String& GetComments() const{ return m_comments; }

    /**
     * <p>A single character used to indicate that a row should be ignored when the
     * character is present at the start of that row. You can specify any character to
     * indicate a comment line.</p>
     */
    inline bool CommentsHasBeenSet() const { return m_commentsHasBeenSet; }

    /**
     * <p>A single character used to indicate that a row should be ignored when the
     * character is present at the start of that row. You can specify any character to
     * indicate a comment line.</p>
     */
    inline void SetComments(const Aws::String& value) { m_commentsHasBeenSet = true; m_comments = value; }

    /**
     * <p>A single character used to indicate that a row should be ignored when the
     * character is present at the start of that row. You can specify any character to
     * indicate a comment line.</p>
     */
    inline void SetComments(Aws::String&& value) { m_commentsHasBeenSet = true; m_comments = std::move(value); }

    /**
     * <p>A single character used to indicate that a row should be ignored when the
     * character is present at the start of that row. You can specify any character to
     * indicate a comment line.</p>
     */
    inline void SetComments(const char* value) { m_commentsHasBeenSet = true; m_comments.assign(value); }

    /**
     * <p>A single character used to indicate that a row should be ignored when the
     * character is present at the start of that row. You can specify any character to
     * indicate a comment line.</p>
     */
    inline CSVInput& WithComments(const Aws::String& value) { SetComments(value); return *this;}

    /**
     * <p>A single character used to indicate that a row should be ignored when the
     * character is present at the start of that row. You can specify any character to
     * indicate a comment line.</p>
     */
    inline CSVInput& WithComments(Aws::String&& value) { SetComments(std::move(value)); return *this;}

    /**
     * <p>A single character used to indicate that a row should be ignored when the
     * character is present at the start of that row. You can specify any character to
     * indicate a comment line.</p>
     */
    inline CSVInput& WithComments(const char* value) { SetComments(value); return *this;}


    /**
     * <p>A single character used for escaping the quotation mark character inside an
     * already escaped value. For example, the value """ a , b """ is parsed as " a , b
     * ".</p>
     */
    inline const Aws::String& GetQuoteEscapeCharacter() const{ return m_quoteEscapeCharacter; }

    /**
     * <p>A single character used for escaping the quotation mark character inside an
     * already escaped value. For example, the value """ a , b """ is parsed as " a , b
     * ".</p>
     */
    inline bool QuoteEscapeCharacterHasBeenSet() const { return m_quoteEscapeCharacterHasBeenSet; }

    /**
     * <p>A single character used for escaping the quotation mark character inside an
     * already escaped value. For example, the value """ a , b """ is parsed as " a , b
     * ".</p>
     */
    inline void SetQuoteEscapeCharacter(const Aws::String& value) { m_quoteEscapeCharacterHasBeenSet = true; m_quoteEscapeCharacter = value; }

    /**
     * <p>A single character used for escaping the quotation mark character inside an
     * already escaped value. For example, the value """ a , b """ is parsed as " a , b
     * ".</p>
     */
    inline void SetQuoteEscapeCharacter(Aws::String&& value) { m_quoteEscapeCharacterHasBeenSet = true; m_quoteEscapeCharacter = std::move(value); }

    /**
     * <p>A single character used for escaping the quotation mark character inside an
     * already escaped value. For example, the value """ a , b """ is parsed as " a , b
     * ".</p>
     */
    inline void SetQuoteEscapeCharacter(const char* value) { m_quoteEscapeCharacterHasBeenSet = true; m_quoteEscapeCharacter.assign(value); }

    /**
     * <p>A single character used for escaping the quotation mark character inside an
     * already escaped value. For example, the value """ a , b """ is parsed as " a , b
     * ".</p>
     */
    inline CSVInput& WithQuoteEscapeCharacter(const Aws::String& value) { SetQuoteEscapeCharacter(value); return *this;}

    /**
     * <p>A single character used for escaping the quotation mark character inside an
     * already escaped value. For example, the value """ a , b """ is parsed as " a , b
     * ".</p>
     */
    inline CSVInput& WithQuoteEscapeCharacter(Aws::String&& value) { SetQuoteEscapeCharacter(std::move(value)); return *this;}

    /**
     * <p>A single character used for escaping the quotation mark character inside an
     * already escaped value. For example, the value """ a , b """ is parsed as " a , b
     * ".</p>
     */
    inline CSVInput& WithQuoteEscapeCharacter(const char* value) { SetQuoteEscapeCharacter(value); return *this;}


    /**
     * <p>A single character used to separate individual records in the input. Instead
     * of the default value, you can specify an arbitrary delimiter.</p>
     */
    inline const Aws::String& GetRecordDelimiter() const{ return m_recordDelimiter; }

    /**
     * <p>A single character used to separate individual records in the input. Instead
     * of the default value, you can specify an arbitrary delimiter.</p>
     */
    inline bool RecordDelimiterHasBeenSet() const { return m_recordDelimiterHasBeenSet; }

    /**
     * <p>A single character used to separate individual records in the input. Instead
     * of the default value, you can specify an arbitrary delimiter.</p>
     */
    inline void SetRecordDelimiter(const Aws::String& value) { m_recordDelimiterHasBeenSet = true; m_recordDelimiter = value; }

    /**
     * <p>A single character used to separate individual records in the input. Instead
     * of the default value, you can specify an arbitrary delimiter.</p>
     */
    inline void SetRecordDelimiter(Aws::String&& value) { m_recordDelimiterHasBeenSet = true; m_recordDelimiter = std::move(value); }

    /**
     * <p>A single character used to separate individual records in the input. Instead
     * of the default value, you can specify an arbitrary delimiter.</p>
     */
    inline void SetRecordDelimiter(const char* value) { m_recordDelimiterHasBeenSet = true; m_recordDelimiter.assign(value); }

    /**
     * <p>A single character used to separate individual records in the input. Instead
     * of the default value, you can specify an arbitrary delimiter.</p>
     */
    inline CSVInput& WithRecordDelimiter(const Aws::String& value) { SetRecordDelimiter(value); return *this;}

    /**
     * <p>A single character used to separate individual records in the input. Instead
     * of the default value, you can specify an arbitrary delimiter.</p>
     */
    inline CSVInput& WithRecordDelimiter(Aws::String&& value) { SetRecordDelimiter(std::move(value)); return *this;}

    /**
     * <p>A single character used to separate individual records in the input. Instead
     * of the default value, you can specify an arbitrary delimiter.</p>
     */
    inline CSVInput& WithRecordDelimiter(const char* value) { SetRecordDelimiter(value); return *this;}


    /**
     * <p>A single character used to separate individual fields in a record. You can
     * specify an arbitrary delimiter.</p>
     */
    inline const Aws::String& GetFieldDelimiter() const{ return m_fieldDelimiter; }

    /**
     * <p>A single character used to separate individual fields in a record. You can
     * specify an arbitrary delimiter.</p>
     */
    inline bool FieldDelimiterHasBeenSet() const { return m_fieldDelimiterHasBeenSet; }

    /**
     * <p>A single character used to separate individual fields in a record. You can
     * specify an arbitrary delimiter.</p>
     */
    inline void SetFieldDelimiter(const Aws::String& value) { m_fieldDelimiterHasBeenSet = true; m_fieldDelimiter = value; }

    /**
     * <p>A single character used to separate individual fields in a record. You can
     * specify an arbitrary delimiter.</p>
     */
    inline void SetFieldDelimiter(Aws::String&& value) { m_fieldDelimiterHasBeenSet = true; m_fieldDelimiter = std::move(value); }

    /**
     * <p>A single character used to separate individual fields in a record. You can
     * specify an arbitrary delimiter.</p>
     */
    inline void SetFieldDelimiter(const char* value) { m_fieldDelimiterHasBeenSet = true; m_fieldDelimiter.assign(value); }

    /**
     * <p>A single character used to separate individual fields in a record. You can
     * specify an arbitrary delimiter.</p>
     */
    inline CSVInput& WithFieldDelimiter(const Aws::String& value) { SetFieldDelimiter(value); return *this;}

    /**
     * <p>A single character used to separate individual fields in a record. You can
     * specify an arbitrary delimiter.</p>
     */
    inline CSVInput& WithFieldDelimiter(Aws::String&& value) { SetFieldDelimiter(std::move(value)); return *this;}

    /**
     * <p>A single character used to separate individual fields in a record. You can
     * specify an arbitrary delimiter.</p>
     */
    inline CSVInput& WithFieldDelimiter(const char* value) { SetFieldDelimiter(value); return *this;}


    /**
     * <p>A single character used for escaping when the field delimiter is part of the
     * value. For example, if the value is <code>a, b</code>, Amazon S3 wraps this
     * field value in quotation marks, as follows: <code>" a , b "</code>.</p> <p>Type:
     * String</p> <p>Default: <code>"</code> </p> <p>Ancestors: <code>CSV</code> </p>
     */
    inline const Aws::String& GetQuoteCharacter() const{ return m_quoteCharacter; }

    /**
     * <p>A single character used for escaping when the field delimiter is part of the
     * value. For example, if the value is <code>a, b</code>, Amazon S3 wraps this
     * field value in quotation marks, as follows: <code>" a , b "</code>.</p> <p>Type:
     * String</p> <p>Default: <code>"</code> </p> <p>Ancestors: <code>CSV</code> </p>
     */
    inline bool QuoteCharacterHasBeenSet() const { return m_quoteCharacterHasBeenSet; }

    /**
     * <p>A single character used for escaping when the field delimiter is part of the
     * value. For example, if the value is <code>a, b</code>, Amazon S3 wraps this
     * field value in quotation marks, as follows: <code>" a , b "</code>.</p> <p>Type:
     * String</p> <p>Default: <code>"</code> </p> <p>Ancestors: <code>CSV</code> </p>
     */
    inline void SetQuoteCharacter(const Aws::String& value) { m_quoteCharacterHasBeenSet = true; m_quoteCharacter = value; }

    /**
     * <p>A single character used for escaping when the field delimiter is part of the
     * value. For example, if the value is <code>a, b</code>, Amazon S3 wraps this
     * field value in quotation marks, as follows: <code>" a , b "</code>.</p> <p>Type:
     * String</p> <p>Default: <code>"</code> </p> <p>Ancestors: <code>CSV</code> </p>
     */
    inline void SetQuoteCharacter(Aws::String&& value) { m_quoteCharacterHasBeenSet = true; m_quoteCharacter = std::move(value); }

    /**
     * <p>A single character used for escaping when the field delimiter is part of the
     * value. For example, if the value is <code>a, b</code>, Amazon S3 wraps this
     * field value in quotation marks, as follows: <code>" a , b "</code>.</p> <p>Type:
     * String</p> <p>Default: <code>"</code> </p> <p>Ancestors: <code>CSV</code> </p>
     */
    inline void SetQuoteCharacter(const char* value) { m_quoteCharacterHasBeenSet = true; m_quoteCharacter.assign(value); }

    /**
     * <p>A single character used for escaping when the field delimiter is part of the
     * value. For example, if the value is <code>a, b</code>, Amazon S3 wraps this
     * field value in quotation marks, as follows: <code>" a , b "</code>.</p> <p>Type:
     * String</p> <p>Default: <code>"</code> </p> <p>Ancestors: <code>CSV</code> </p>
     */
    inline CSVInput& WithQuoteCharacter(const Aws::String& value) { SetQuoteCharacter(value); return *this;}

    /**
     * <p>A single character used for escaping when the field delimiter is part of the
     * value. For example, if the value is <code>a, b</code>, Amazon S3 wraps this
     * field value in quotation marks, as follows: <code>" a , b "</code>.</p> <p>Type:
     * String</p> <p>Default: <code>"</code> </p> <p>Ancestors: <code>CSV</code> </p>
     */
    inline CSVInput& WithQuoteCharacter(Aws::String&& value) { SetQuoteCharacter(std::move(value)); return *this;}

    /**
     * <p>A single character used for escaping when the field delimiter is part of the
     * value. For example, if the value is <code>a, b</code>, Amazon S3 wraps this
     * field value in quotation marks, as follows: <code>" a , b "</code>.</p> <p>Type:
     * String</p> <p>Default: <code>"</code> </p> <p>Ancestors: <code>CSV</code> </p>
     */
    inline CSVInput& WithQuoteCharacter(const char* value) { SetQuoteCharacter(value); return *this;}


    /**
     * <p>Specifies that CSV field values may contain quoted record delimiters and such
     * records should be allowed. Default value is FALSE. Setting this value to TRUE
     * may lower performance.</p>
     */
    inline bool GetAllowQuotedRecordDelimiter() const{ return m_allowQuotedRecordDelimiter; }

    /**
     * <p>Specifies that CSV field values may contain quoted record delimiters and such
     * records should be allowed. Default value is FALSE. Setting this value to TRUE
     * may lower performance.</p>
     */
    inline bool AllowQuotedRecordDelimiterHasBeenSet() const { return m_allowQuotedRecordDelimiterHasBeenSet; }

    /**
     * <p>Specifies that CSV field values may contain quoted record delimiters and such
     * records should be allowed. Default value is FALSE. Setting this value to TRUE
     * may lower performance.</p>
     */
    inline void SetAllowQuotedRecordDelimiter(bool value) { m_allowQuotedRecordDelimiterHasBeenSet = true; m_allowQuotedRecordDelimiter = value; }

    /**
     * <p>Specifies that CSV field values may contain quoted record delimiters and such
     * records should be allowed. Default value is FALSE. Setting this value to TRUE
     * may lower performance.</p>
     */
    inline CSVInput& WithAllowQuotedRecordDelimiter(bool value) { SetAllowQuotedRecordDelimiter(value); return *this;}

  private:

    FileHeaderInfo m_fileHeaderInfo;
    bool m_fileHeaderInfoHasBeenSet;

    Aws::String m_comments;
    bool m_commentsHasBeenSet;

    Aws::String m_quoteEscapeCharacter;
    bool m_quoteEscapeCharacterHasBeenSet;

    Aws::String m_recordDelimiter;
    bool m_recordDelimiterHasBeenSet;

    Aws::String m_fieldDelimiter;
    bool m_fieldDelimiterHasBeenSet;

    Aws::String m_quoteCharacter;
    bool m_quoteCharacterHasBeenSet;

    bool m_allowQuotedRecordDelimiter;
    bool m_allowQuotedRecordDelimiterHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
