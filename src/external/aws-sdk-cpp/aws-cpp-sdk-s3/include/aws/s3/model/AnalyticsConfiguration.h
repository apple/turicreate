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
#include <aws/s3/model/AnalyticsFilter.h>
#include <aws/s3/model/StorageClassAnalysis.h>
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
   * <p> Specifies the configuration and any analyses for the analytics filter of an
   * Amazon S3 bucket.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/AnalyticsConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API AnalyticsConfiguration
  {
  public:
    AnalyticsConfiguration();
    AnalyticsConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    AnalyticsConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The ID that identifies the analytics configuration.</p>
     */
    inline const Aws::String& GetId() const{ return m_id; }

    /**
     * <p>The ID that identifies the analytics configuration.</p>
     */
    inline bool IdHasBeenSet() const { return m_idHasBeenSet; }

    /**
     * <p>The ID that identifies the analytics configuration.</p>
     */
    inline void SetId(const Aws::String& value) { m_idHasBeenSet = true; m_id = value; }

    /**
     * <p>The ID that identifies the analytics configuration.</p>
     */
    inline void SetId(Aws::String&& value) { m_idHasBeenSet = true; m_id = std::move(value); }

    /**
     * <p>The ID that identifies the analytics configuration.</p>
     */
    inline void SetId(const char* value) { m_idHasBeenSet = true; m_id.assign(value); }

    /**
     * <p>The ID that identifies the analytics configuration.</p>
     */
    inline AnalyticsConfiguration& WithId(const Aws::String& value) { SetId(value); return *this;}

    /**
     * <p>The ID that identifies the analytics configuration.</p>
     */
    inline AnalyticsConfiguration& WithId(Aws::String&& value) { SetId(std::move(value)); return *this;}

    /**
     * <p>The ID that identifies the analytics configuration.</p>
     */
    inline AnalyticsConfiguration& WithId(const char* value) { SetId(value); return *this;}


    /**
     * <p>The filter used to describe a set of objects for analyses. A filter must have
     * exactly one prefix, one tag, or one conjunction (AnalyticsAndOperator). If no
     * filter is provided, all objects will be considered in any analysis.</p>
     */
    inline const AnalyticsFilter& GetFilter() const{ return m_filter; }

    /**
     * <p>The filter used to describe a set of objects for analyses. A filter must have
     * exactly one prefix, one tag, or one conjunction (AnalyticsAndOperator). If no
     * filter is provided, all objects will be considered in any analysis.</p>
     */
    inline bool FilterHasBeenSet() const { return m_filterHasBeenSet; }

    /**
     * <p>The filter used to describe a set of objects for analyses. A filter must have
     * exactly one prefix, one tag, or one conjunction (AnalyticsAndOperator). If no
     * filter is provided, all objects will be considered in any analysis.</p>
     */
    inline void SetFilter(const AnalyticsFilter& value) { m_filterHasBeenSet = true; m_filter = value; }

    /**
     * <p>The filter used to describe a set of objects for analyses. A filter must have
     * exactly one prefix, one tag, or one conjunction (AnalyticsAndOperator). If no
     * filter is provided, all objects will be considered in any analysis.</p>
     */
    inline void SetFilter(AnalyticsFilter&& value) { m_filterHasBeenSet = true; m_filter = std::move(value); }

    /**
     * <p>The filter used to describe a set of objects for analyses. A filter must have
     * exactly one prefix, one tag, or one conjunction (AnalyticsAndOperator). If no
     * filter is provided, all objects will be considered in any analysis.</p>
     */
    inline AnalyticsConfiguration& WithFilter(const AnalyticsFilter& value) { SetFilter(value); return *this;}

    /**
     * <p>The filter used to describe a set of objects for analyses. A filter must have
     * exactly one prefix, one tag, or one conjunction (AnalyticsAndOperator). If no
     * filter is provided, all objects will be considered in any analysis.</p>
     */
    inline AnalyticsConfiguration& WithFilter(AnalyticsFilter&& value) { SetFilter(std::move(value)); return *this;}


    /**
     * <p> Contains data related to access patterns to be collected and made available
     * to analyze the tradeoffs between different storage classes. </p>
     */
    inline const StorageClassAnalysis& GetStorageClassAnalysis() const{ return m_storageClassAnalysis; }

    /**
     * <p> Contains data related to access patterns to be collected and made available
     * to analyze the tradeoffs between different storage classes. </p>
     */
    inline bool StorageClassAnalysisHasBeenSet() const { return m_storageClassAnalysisHasBeenSet; }

    /**
     * <p> Contains data related to access patterns to be collected and made available
     * to analyze the tradeoffs between different storage classes. </p>
     */
    inline void SetStorageClassAnalysis(const StorageClassAnalysis& value) { m_storageClassAnalysisHasBeenSet = true; m_storageClassAnalysis = value; }

    /**
     * <p> Contains data related to access patterns to be collected and made available
     * to analyze the tradeoffs between different storage classes. </p>
     */
    inline void SetStorageClassAnalysis(StorageClassAnalysis&& value) { m_storageClassAnalysisHasBeenSet = true; m_storageClassAnalysis = std::move(value); }

    /**
     * <p> Contains data related to access patterns to be collected and made available
     * to analyze the tradeoffs between different storage classes. </p>
     */
    inline AnalyticsConfiguration& WithStorageClassAnalysis(const StorageClassAnalysis& value) { SetStorageClassAnalysis(value); return *this;}

    /**
     * <p> Contains data related to access patterns to be collected and made available
     * to analyze the tradeoffs between different storage classes. </p>
     */
    inline AnalyticsConfiguration& WithStorageClassAnalysis(StorageClassAnalysis&& value) { SetStorageClassAnalysis(std::move(value)); return *this;}

  private:

    Aws::String m_id;
    bool m_idHasBeenSet;

    AnalyticsFilter m_filter;
    bool m_filterHasBeenSet;

    StorageClassAnalysis m_storageClassAnalysis;
    bool m_storageClassAnalysisHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
