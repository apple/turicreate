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
   * <p>Specifies the byte range of the object to get the records from. A record is
   * processed when its first byte is contained by the range. This parameter is
   * optional, but when specified, it must not be empty. See RFC 2616, Section
   * 14.35.1 about how to specify the start and end of the range.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ScanRange">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API ScanRange
  {
  public:
    ScanRange();
    ScanRange(const Aws::Utils::Xml::XmlNode& xmlNode);
    ScanRange& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies the start of the byte range. This parameter is optional. Valid
     * values: non-negative integers. The default value is 0. If only start is
     * supplied, it means scan from that point to the end of the file.For example;
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code>
     * means scan from byte 50 until the end of the file.</p>
     */
    inline long long GetStart() const{ return m_start; }

    /**
     * <p>Specifies the start of the byte range. This parameter is optional. Valid
     * values: non-negative integers. The default value is 0. If only start is
     * supplied, it means scan from that point to the end of the file.For example;
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code>
     * means scan from byte 50 until the end of the file.</p>
     */
    inline bool StartHasBeenSet() const { return m_startHasBeenSet; }

    /**
     * <p>Specifies the start of the byte range. This parameter is optional. Valid
     * values: non-negative integers. The default value is 0. If only start is
     * supplied, it means scan from that point to the end of the file.For example;
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code>
     * means scan from byte 50 until the end of the file.</p>
     */
    inline void SetStart(long long value) { m_startHasBeenSet = true; m_start = value; }

    /**
     * <p>Specifies the start of the byte range. This parameter is optional. Valid
     * values: non-negative integers. The default value is 0. If only start is
     * supplied, it means scan from that point to the end of the file.For example;
     * <code>&lt;scanrange&gt;&lt;start&gt;50&lt;/start&gt;&lt;/scanrange&gt;</code>
     * means scan from byte 50 until the end of the file.</p>
     */
    inline ScanRange& WithStart(long long value) { SetStart(value); return *this;}


    /**
     * <p>Specifies the end of the byte range. This parameter is optional. Valid
     * values: non-negative integers. The default value is one less than the size of
     * the object being queried. If only the End parameter is supplied, it is
     * interpreted to mean scan the last N bytes of the file. For example,
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> means
     * scan the last 50 bytes.</p>
     */
    inline long long GetEnd() const{ return m_end; }

    /**
     * <p>Specifies the end of the byte range. This parameter is optional. Valid
     * values: non-negative integers. The default value is one less than the size of
     * the object being queried. If only the End parameter is supplied, it is
     * interpreted to mean scan the last N bytes of the file. For example,
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> means
     * scan the last 50 bytes.</p>
     */
    inline bool EndHasBeenSet() const { return m_endHasBeenSet; }

    /**
     * <p>Specifies the end of the byte range. This parameter is optional. Valid
     * values: non-negative integers. The default value is one less than the size of
     * the object being queried. If only the End parameter is supplied, it is
     * interpreted to mean scan the last N bytes of the file. For example,
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> means
     * scan the last 50 bytes.</p>
     */
    inline void SetEnd(long long value) { m_endHasBeenSet = true; m_end = value; }

    /**
     * <p>Specifies the end of the byte range. This parameter is optional. Valid
     * values: non-negative integers. The default value is one less than the size of
     * the object being queried. If only the End parameter is supplied, it is
     * interpreted to mean scan the last N bytes of the file. For example,
     * <code>&lt;scanrange&gt;&lt;end&gt;50&lt;/end&gt;&lt;/scanrange&gt;</code> means
     * scan the last 50 bytes.</p>
     */
    inline ScanRange& WithEnd(long long value) { SetEnd(value); return *this;}

  private:

    long long m_start;
    bool m_startHasBeenSet;

    long long m_end;
    bool m_endHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
