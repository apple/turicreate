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
   * <p>Specifies JSON as request's output serialization format.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/JSONOutput">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API JSONOutput
  {
  public:
    JSONOutput();
    JSONOutput(const Aws::Utils::Xml::XmlNode& xmlNode);
    JSONOutput& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The value used to separate individual records in the output.</p>
     */
    inline const Aws::String& GetRecordDelimiter() const{ return m_recordDelimiter; }

    /**
     * <p>The value used to separate individual records in the output.</p>
     */
    inline bool RecordDelimiterHasBeenSet() const { return m_recordDelimiterHasBeenSet; }

    /**
     * <p>The value used to separate individual records in the output.</p>
     */
    inline void SetRecordDelimiter(const Aws::String& value) { m_recordDelimiterHasBeenSet = true; m_recordDelimiter = value; }

    /**
     * <p>The value used to separate individual records in the output.</p>
     */
    inline void SetRecordDelimiter(Aws::String&& value) { m_recordDelimiterHasBeenSet = true; m_recordDelimiter = std::move(value); }

    /**
     * <p>The value used to separate individual records in the output.</p>
     */
    inline void SetRecordDelimiter(const char* value) { m_recordDelimiterHasBeenSet = true; m_recordDelimiter.assign(value); }

    /**
     * <p>The value used to separate individual records in the output.</p>
     */
    inline JSONOutput& WithRecordDelimiter(const Aws::String& value) { SetRecordDelimiter(value); return *this;}

    /**
     * <p>The value used to separate individual records in the output.</p>
     */
    inline JSONOutput& WithRecordDelimiter(Aws::String&& value) { SetRecordDelimiter(std::move(value)); return *this;}

    /**
     * <p>The value used to separate individual records in the output.</p>
     */
    inline JSONOutput& WithRecordDelimiter(const char* value) { SetRecordDelimiter(value); return *this;}

  private:

    Aws::String m_recordDelimiter;
    bool m_recordDelimiterHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
