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
#include <aws/s3/model/StorageClassAnalysisDataExport.h>
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
   * <p>Specifies data related to access patterns to be collected and made available
   * to analyze the tradeoffs between different storage classes for an Amazon S3
   * bucket.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/StorageClassAnalysis">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API StorageClassAnalysis
  {
  public:
    StorageClassAnalysis();
    StorageClassAnalysis(const Aws::Utils::Xml::XmlNode& xmlNode);
    StorageClassAnalysis& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies how data related to the storage class analysis for an Amazon S3
     * bucket should be exported.</p>
     */
    inline const StorageClassAnalysisDataExport& GetDataExport() const{ return m_dataExport; }

    /**
     * <p>Specifies how data related to the storage class analysis for an Amazon S3
     * bucket should be exported.</p>
     */
    inline bool DataExportHasBeenSet() const { return m_dataExportHasBeenSet; }

    /**
     * <p>Specifies how data related to the storage class analysis for an Amazon S3
     * bucket should be exported.</p>
     */
    inline void SetDataExport(const StorageClassAnalysisDataExport& value) { m_dataExportHasBeenSet = true; m_dataExport = value; }

    /**
     * <p>Specifies how data related to the storage class analysis for an Amazon S3
     * bucket should be exported.</p>
     */
    inline void SetDataExport(StorageClassAnalysisDataExport&& value) { m_dataExportHasBeenSet = true; m_dataExport = std::move(value); }

    /**
     * <p>Specifies how data related to the storage class analysis for an Amazon S3
     * bucket should be exported.</p>
     */
    inline StorageClassAnalysis& WithDataExport(const StorageClassAnalysisDataExport& value) { SetDataExport(value); return *this;}

    /**
     * <p>Specifies how data related to the storage class analysis for an Amazon S3
     * bucket should be exported.</p>
     */
    inline StorageClassAnalysis& WithDataExport(StorageClassAnalysisDataExport&& value) { SetDataExport(std::move(value)); return *this;}

  private:

    StorageClassAnalysisDataExport m_dataExport;
    bool m_dataExportHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
