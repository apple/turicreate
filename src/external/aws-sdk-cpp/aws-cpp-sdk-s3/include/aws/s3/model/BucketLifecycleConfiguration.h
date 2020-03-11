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
#include <aws/s3/model/LifecycleRule.h>
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
   * <p>Specifies the lifecycle configuration for objects in an Amazon S3 bucket. For
   * more information, see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/object-lifecycle-mgmt.html">Object
   * Lifecycle Management</a> in the <i>Amazon Simple Storage Service Developer
   * Guide</i>.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/BucketLifecycleConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API BucketLifecycleConfiguration
  {
  public:
    BucketLifecycleConfiguration();
    BucketLifecycleConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    BucketLifecycleConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>A lifecycle rule for individual objects in an Amazon S3 bucket.</p>
     */
    inline const Aws::Vector<LifecycleRule>& GetRules() const{ return m_rules; }

    /**
     * <p>A lifecycle rule for individual objects in an Amazon S3 bucket.</p>
     */
    inline bool RulesHasBeenSet() const { return m_rulesHasBeenSet; }

    /**
     * <p>A lifecycle rule for individual objects in an Amazon S3 bucket.</p>
     */
    inline void SetRules(const Aws::Vector<LifecycleRule>& value) { m_rulesHasBeenSet = true; m_rules = value; }

    /**
     * <p>A lifecycle rule for individual objects in an Amazon S3 bucket.</p>
     */
    inline void SetRules(Aws::Vector<LifecycleRule>&& value) { m_rulesHasBeenSet = true; m_rules = std::move(value); }

    /**
     * <p>A lifecycle rule for individual objects in an Amazon S3 bucket.</p>
     */
    inline BucketLifecycleConfiguration& WithRules(const Aws::Vector<LifecycleRule>& value) { SetRules(value); return *this;}

    /**
     * <p>A lifecycle rule for individual objects in an Amazon S3 bucket.</p>
     */
    inline BucketLifecycleConfiguration& WithRules(Aws::Vector<LifecycleRule>&& value) { SetRules(std::move(value)); return *this;}

    /**
     * <p>A lifecycle rule for individual objects in an Amazon S3 bucket.</p>
     */
    inline BucketLifecycleConfiguration& AddRules(const LifecycleRule& value) { m_rulesHasBeenSet = true; m_rules.push_back(value); return *this; }

    /**
     * <p>A lifecycle rule for individual objects in an Amazon S3 bucket.</p>
     */
    inline BucketLifecycleConfiguration& AddRules(LifecycleRule&& value) { m_rulesHasBeenSet = true; m_rules.push_back(std::move(value)); return *this; }

  private:

    Aws::Vector<LifecycleRule> m_rules;
    bool m_rulesHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
