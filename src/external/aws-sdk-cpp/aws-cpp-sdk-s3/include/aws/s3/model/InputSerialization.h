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
#include <aws/s3/model/CSVInput.h>
#include <aws/s3/model/CompressionType.h>
#include <aws/s3/model/JSONInput.h>
#include <aws/s3/model/ParquetInput.h>
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
   * <p>Describes the serialization format of the object.</p><p><h3>See Also:</h3>  
   * <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/InputSerialization">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API InputSerialization
  {
  public:
    InputSerialization();
    InputSerialization(const Aws::Utils::Xml::XmlNode& xmlNode);
    InputSerialization& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Describes the serialization of a CSV-encoded object.</p>
     */
    inline const CSVInput& GetCSV() const{ return m_cSV; }

    /**
     * <p>Describes the serialization of a CSV-encoded object.</p>
     */
    inline bool CSVHasBeenSet() const { return m_cSVHasBeenSet; }

    /**
     * <p>Describes the serialization of a CSV-encoded object.</p>
     */
    inline void SetCSV(const CSVInput& value) { m_cSVHasBeenSet = true; m_cSV = value; }

    /**
     * <p>Describes the serialization of a CSV-encoded object.</p>
     */
    inline void SetCSV(CSVInput&& value) { m_cSVHasBeenSet = true; m_cSV = std::move(value); }

    /**
     * <p>Describes the serialization of a CSV-encoded object.</p>
     */
    inline InputSerialization& WithCSV(const CSVInput& value) { SetCSV(value); return *this;}

    /**
     * <p>Describes the serialization of a CSV-encoded object.</p>
     */
    inline InputSerialization& WithCSV(CSVInput&& value) { SetCSV(std::move(value)); return *this;}


    /**
     * <p>Specifies object's compression format. Valid values: NONE, GZIP, BZIP2.
     * Default Value: NONE.</p>
     */
    inline const CompressionType& GetCompressionType() const{ return m_compressionType; }

    /**
     * <p>Specifies object's compression format. Valid values: NONE, GZIP, BZIP2.
     * Default Value: NONE.</p>
     */
    inline bool CompressionTypeHasBeenSet() const { return m_compressionTypeHasBeenSet; }

    /**
     * <p>Specifies object's compression format. Valid values: NONE, GZIP, BZIP2.
     * Default Value: NONE.</p>
     */
    inline void SetCompressionType(const CompressionType& value) { m_compressionTypeHasBeenSet = true; m_compressionType = value; }

    /**
     * <p>Specifies object's compression format. Valid values: NONE, GZIP, BZIP2.
     * Default Value: NONE.</p>
     */
    inline void SetCompressionType(CompressionType&& value) { m_compressionTypeHasBeenSet = true; m_compressionType = std::move(value); }

    /**
     * <p>Specifies object's compression format. Valid values: NONE, GZIP, BZIP2.
     * Default Value: NONE.</p>
     */
    inline InputSerialization& WithCompressionType(const CompressionType& value) { SetCompressionType(value); return *this;}

    /**
     * <p>Specifies object's compression format. Valid values: NONE, GZIP, BZIP2.
     * Default Value: NONE.</p>
     */
    inline InputSerialization& WithCompressionType(CompressionType&& value) { SetCompressionType(std::move(value)); return *this;}


    /**
     * <p>Specifies JSON as object's input serialization format.</p>
     */
    inline const JSONInput& GetJSON() const{ return m_jSON; }

    /**
     * <p>Specifies JSON as object's input serialization format.</p>
     */
    inline bool JSONHasBeenSet() const { return m_jSONHasBeenSet; }

    /**
     * <p>Specifies JSON as object's input serialization format.</p>
     */
    inline void SetJSON(const JSONInput& value) { m_jSONHasBeenSet = true; m_jSON = value; }

    /**
     * <p>Specifies JSON as object's input serialization format.</p>
     */
    inline void SetJSON(JSONInput&& value) { m_jSONHasBeenSet = true; m_jSON = std::move(value); }

    /**
     * <p>Specifies JSON as object's input serialization format.</p>
     */
    inline InputSerialization& WithJSON(const JSONInput& value) { SetJSON(value); return *this;}

    /**
     * <p>Specifies JSON as object's input serialization format.</p>
     */
    inline InputSerialization& WithJSON(JSONInput&& value) { SetJSON(std::move(value)); return *this;}


    /**
     * <p>Specifies Parquet as object's input serialization format.</p>
     */
    inline const ParquetInput& GetParquet() const{ return m_parquet; }

    /**
     * <p>Specifies Parquet as object's input serialization format.</p>
     */
    inline bool ParquetHasBeenSet() const { return m_parquetHasBeenSet; }

    /**
     * <p>Specifies Parquet as object's input serialization format.</p>
     */
    inline void SetParquet(const ParquetInput& value) { m_parquetHasBeenSet = true; m_parquet = value; }

    /**
     * <p>Specifies Parquet as object's input serialization format.</p>
     */
    inline void SetParquet(ParquetInput&& value) { m_parquetHasBeenSet = true; m_parquet = std::move(value); }

    /**
     * <p>Specifies Parquet as object's input serialization format.</p>
     */
    inline InputSerialization& WithParquet(const ParquetInput& value) { SetParquet(value); return *this;}

    /**
     * <p>Specifies Parquet as object's input serialization format.</p>
     */
    inline InputSerialization& WithParquet(ParquetInput&& value) { SetParquet(std::move(value)); return *this;}

  private:

    CSVInput m_cSV;
    bool m_cSVHasBeenSet;

    CompressionType m_compressionType;
    bool m_compressionTypeHasBeenSet;

    JSONInput m_jSON;
    bool m_jSONHasBeenSet;

    ParquetInput m_parquet;
    bool m_parquetHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
