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
#include <aws/s3/model/MetricsFilter.h>
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
   * <p>Specifies a metrics configuration for the CloudWatch request metrics
   * (specified by the metrics configuration ID) from an Amazon S3 bucket. If you're
   * updating an existing metrics configuration, note that this is a full replacement
   * of the existing metrics configuration. If you don't include the elements you
   * want to keep, they are erased. For more information, see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTMetricConfiguration.html">
   * PUT Bucket metrics</a> in the <i>Amazon Simple Storage Service API
   * Reference</i>.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/MetricsConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API MetricsConfiguration
  {
  public:
    MetricsConfiguration();
    MetricsConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    MetricsConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The ID used to identify the metrics configuration.</p>
     */
    inline const Aws::String& GetId() const{ return m_id; }

    /**
     * <p>The ID used to identify the metrics configuration.</p>
     */
    inline bool IdHasBeenSet() const { return m_idHasBeenSet; }

    /**
     * <p>The ID used to identify the metrics configuration.</p>
     */
    inline void SetId(const Aws::String& value) { m_idHasBeenSet = true; m_id = value; }

    /**
     * <p>The ID used to identify the metrics configuration.</p>
     */
    inline void SetId(Aws::String&& value) { m_idHasBeenSet = true; m_id = std::move(value); }

    /**
     * <p>The ID used to identify the metrics configuration.</p>
     */
    inline void SetId(const char* value) { m_idHasBeenSet = true; m_id.assign(value); }

    /**
     * <p>The ID used to identify the metrics configuration.</p>
     */
    inline MetricsConfiguration& WithId(const Aws::String& value) { SetId(value); return *this;}

    /**
     * <p>The ID used to identify the metrics configuration.</p>
     */
    inline MetricsConfiguration& WithId(Aws::String&& value) { SetId(std::move(value)); return *this;}

    /**
     * <p>The ID used to identify the metrics configuration.</p>
     */
    inline MetricsConfiguration& WithId(const char* value) { SetId(value); return *this;}


    /**
     * <p>Specifies a metrics configuration filter. The metrics configuration will only
     * include objects that meet the filter's criteria. A filter must be a prefix, a
     * tag, or a conjunction (MetricsAndOperator).</p>
     */
    inline const MetricsFilter& GetFilter() const{ return m_filter; }

    /**
     * <p>Specifies a metrics configuration filter. The metrics configuration will only
     * include objects that meet the filter's criteria. A filter must be a prefix, a
     * tag, or a conjunction (MetricsAndOperator).</p>
     */
    inline bool FilterHasBeenSet() const { return m_filterHasBeenSet; }

    /**
     * <p>Specifies a metrics configuration filter. The metrics configuration will only
     * include objects that meet the filter's criteria. A filter must be a prefix, a
     * tag, or a conjunction (MetricsAndOperator).</p>
     */
    inline void SetFilter(const MetricsFilter& value) { m_filterHasBeenSet = true; m_filter = value; }

    /**
     * <p>Specifies a metrics configuration filter. The metrics configuration will only
     * include objects that meet the filter's criteria. A filter must be a prefix, a
     * tag, or a conjunction (MetricsAndOperator).</p>
     */
    inline void SetFilter(MetricsFilter&& value) { m_filterHasBeenSet = true; m_filter = std::move(value); }

    /**
     * <p>Specifies a metrics configuration filter. The metrics configuration will only
     * include objects that meet the filter's criteria. A filter must be a prefix, a
     * tag, or a conjunction (MetricsAndOperator).</p>
     */
    inline MetricsConfiguration& WithFilter(const MetricsFilter& value) { SetFilter(value); return *this;}

    /**
     * <p>Specifies a metrics configuration filter. The metrics configuration will only
     * include objects that meet the filter's criteria. A filter must be a prefix, a
     * tag, or a conjunction (MetricsAndOperator).</p>
     */
    inline MetricsConfiguration& WithFilter(MetricsFilter&& value) { SetFilter(std::move(value)); return *this;}

  private:

    Aws::String m_id;
    bool m_idHasBeenSet;

    MetricsFilter m_filter;
    bool m_filterHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
