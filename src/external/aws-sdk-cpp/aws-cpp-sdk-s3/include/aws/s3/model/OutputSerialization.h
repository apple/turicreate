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
#include <aws/s3/model/CSVOutput.h>
#include <aws/s3/model/JSONOutput.h>
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
   * <p>Describes how results of the Select job are serialized.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/OutputSerialization">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API OutputSerialization
  {
  public:
    OutputSerialization();
    OutputSerialization(const Aws::Utils::Xml::XmlNode& xmlNode);
    OutputSerialization& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Describes the serialization of CSV-encoded Select results.</p>
     */
    inline const CSVOutput& GetCSV() const{ return m_cSV; }

    /**
     * <p>Describes the serialization of CSV-encoded Select results.</p>
     */
    inline bool CSVHasBeenSet() const { return m_cSVHasBeenSet; }

    /**
     * <p>Describes the serialization of CSV-encoded Select results.</p>
     */
    inline void SetCSV(const CSVOutput& value) { m_cSVHasBeenSet = true; m_cSV = value; }

    /**
     * <p>Describes the serialization of CSV-encoded Select results.</p>
     */
    inline void SetCSV(CSVOutput&& value) { m_cSVHasBeenSet = true; m_cSV = std::move(value); }

    /**
     * <p>Describes the serialization of CSV-encoded Select results.</p>
     */
    inline OutputSerialization& WithCSV(const CSVOutput& value) { SetCSV(value); return *this;}

    /**
     * <p>Describes the serialization of CSV-encoded Select results.</p>
     */
    inline OutputSerialization& WithCSV(CSVOutput&& value) { SetCSV(std::move(value)); return *this;}


    /**
     * <p>Specifies JSON as request's output serialization format.</p>
     */
    inline const JSONOutput& GetJSON() const{ return m_jSON; }

    /**
     * <p>Specifies JSON as request's output serialization format.</p>
     */
    inline bool JSONHasBeenSet() const { return m_jSONHasBeenSet; }

    /**
     * <p>Specifies JSON as request's output serialization format.</p>
     */
    inline void SetJSON(const JSONOutput& value) { m_jSONHasBeenSet = true; m_jSON = value; }

    /**
     * <p>Specifies JSON as request's output serialization format.</p>
     */
    inline void SetJSON(JSONOutput&& value) { m_jSONHasBeenSet = true; m_jSON = std::move(value); }

    /**
     * <p>Specifies JSON as request's output serialization format.</p>
     */
    inline OutputSerialization& WithJSON(const JSONOutput& value) { SetJSON(value); return *this;}

    /**
     * <p>Specifies JSON as request's output serialization format.</p>
     */
    inline OutputSerialization& WithJSON(JSONOutput&& value) { SetJSON(std::move(value)); return *this;}

  private:

    CSVOutput m_cSV;
    bool m_cSVHasBeenSet;

    JSONOutput m_jSON;
    bool m_jSONHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
