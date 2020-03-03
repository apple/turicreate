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
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/ReplicationRule.h>
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
   * <p>A container for replication rules. You can add up to 1,000 rules. The maximum
   * size of a replication configuration is 2 MB.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ReplicationConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ReplicationConfiguration
  {
  public:
    ReplicationConfiguration();
    ReplicationConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    ReplicationConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The Amazon Resource Name (ARN) of the AWS Identity and Access Management
     * (IAM) role that Amazon S3 assumes when replicating objects. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-how-setup.html">How
     * to Set Up Replication</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline const Aws::String& GetRole() const{ return m_role; }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS Identity and Access Management
     * (IAM) role that Amazon S3 assumes when replicating objects. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-how-setup.html">How
     * to Set Up Replication</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline bool RoleHasBeenSet() const { return m_roleHasBeenSet; }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS Identity and Access Management
     * (IAM) role that Amazon S3 assumes when replicating objects. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-how-setup.html">How
     * to Set Up Replication</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetRole(const Aws::String& value) { m_roleHasBeenSet = true; m_role = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS Identity and Access Management
     * (IAM) role that Amazon S3 assumes when replicating objects. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-how-setup.html">How
     * to Set Up Replication</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetRole(Aws::String&& value) { m_roleHasBeenSet = true; m_role = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS Identity and Access Management
     * (IAM) role that Amazon S3 assumes when replicating objects. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-how-setup.html">How
     * to Set Up Replication</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline void SetRole(const char* value) { m_roleHasBeenSet = true; m_role.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS Identity and Access Management
     * (IAM) role that Amazon S3 assumes when replicating objects. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-how-setup.html">How
     * to Set Up Replication</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ReplicationConfiguration& WithRole(const Aws::String& value) { SetRole(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS Identity and Access Management
     * (IAM) role that Amazon S3 assumes when replicating objects. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-how-setup.html">How
     * to Set Up Replication</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ReplicationConfiguration& WithRole(Aws::String&& value) { SetRole(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the AWS Identity and Access Management
     * (IAM) role that Amazon S3 assumes when replicating objects. For more
     * information, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-how-setup.html">How
     * to Set Up Replication</a> in the <i>Amazon Simple Storage Service Developer
     * Guide</i>.</p>
     */
    inline ReplicationConfiguration& WithRole(const char* value) { SetRole(value); return *this;}


    /**
     * <p>A container for one or more replication rules. A replication configuration
     * must have at least one rule and can contain a maximum of 1,000 rules. </p>
     */
    inline const Aws::Vector<ReplicationRule>& GetRules() const{ return m_rules; }

    /**
     * <p>A container for one or more replication rules. A replication configuration
     * must have at least one rule and can contain a maximum of 1,000 rules. </p>
     */
    inline bool RulesHasBeenSet() const { return m_rulesHasBeenSet; }

    /**
     * <p>A container for one or more replication rules. A replication configuration
     * must have at least one rule and can contain a maximum of 1,000 rules. </p>
     */
    inline void SetRules(const Aws::Vector<ReplicationRule>& value) { m_rulesHasBeenSet = true; m_rules = value; }

    /**
     * <p>A container for one or more replication rules. A replication configuration
     * must have at least one rule and can contain a maximum of 1,000 rules. </p>
     */
    inline void SetRules(Aws::Vector<ReplicationRule>&& value) { m_rulesHasBeenSet = true; m_rules = std::move(value); }

    /**
     * <p>A container for one or more replication rules. A replication configuration
     * must have at least one rule and can contain a maximum of 1,000 rules. </p>
     */
    inline ReplicationConfiguration& WithRules(const Aws::Vector<ReplicationRule>& value) { SetRules(value); return *this;}

    /**
     * <p>A container for one or more replication rules. A replication configuration
     * must have at least one rule and can contain a maximum of 1,000 rules. </p>
     */
    inline ReplicationConfiguration& WithRules(Aws::Vector<ReplicationRule>&& value) { SetRules(std::move(value)); return *this;}

    /**
     * <p>A container for one or more replication rules. A replication configuration
     * must have at least one rule and can contain a maximum of 1,000 rules. </p>
     */
    inline ReplicationConfiguration& AddRules(const ReplicationRule& value) { m_rulesHasBeenSet = true; m_rules.push_back(value); return *this; }

    /**
     * <p>A container for one or more replication rules. A replication configuration
     * must have at least one rule and can contain a maximum of 1,000 rules. </p>
     */
    inline ReplicationConfiguration& AddRules(ReplicationRule&& value) { m_rulesHasBeenSet = true; m_rules.push_back(std::move(value)); return *this; }

  private:

    Aws::String m_role;
    bool m_roleHasBeenSet;

    Aws::Vector<ReplicationRule> m_rules;
    bool m_rulesHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
