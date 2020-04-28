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
#include <aws/s3/model/DeleteMarkerReplicationStatus.h>
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
   * <p>Specifies whether Amazon S3 replicates the delete markers. If you specify a
   * <code>Filter</code>, you must specify this element. However, in the latest
   * version of replication configuration (when <code>Filter</code> is specified),
   * Amazon S3 doesn't replicate delete markers. Therefore, the
   * <code>DeleteMarkerReplication</code> element can contain only
   * &lt;Status&gt;Disabled&lt;/Status&gt;. For an example configuration, see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-add-config.html#replication-config-min-rule-config">Basic
   * Rule Configuration</a>. </p> <note> <p> If you don't specify the
   * <code>Filter</code> element, Amazon S3 assumes that the replication
   * configuration is the earlier version, V1. In the earlier version, Amazon S3
   * handled replication of delete markers differently. For more information, see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-add-config.html#replication-backward-compat-considerations">Backward
   * Compatibility</a>.</p> </note><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/DeleteMarkerReplication">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API DeleteMarkerReplication
  {
  public:
    DeleteMarkerReplication();
    DeleteMarkerReplication(const Aws::Utils::Xml::XmlNode& xmlNode);
    DeleteMarkerReplication& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Indicates whether to replicate delete markers.</p> <note> <p> In the current
     * implementation, Amazon S3 doesn't replicate the delete markers. The status must
     * be <code>Disabled</code>. </p> </note>
     */
    inline const DeleteMarkerReplicationStatus& GetStatus() const{ return m_status; }

    /**
     * <p>Indicates whether to replicate delete markers.</p> <note> <p> In the current
     * implementation, Amazon S3 doesn't replicate the delete markers. The status must
     * be <code>Disabled</code>. </p> </note>
     */
    inline bool StatusHasBeenSet() const { return m_statusHasBeenSet; }

    /**
     * <p>Indicates whether to replicate delete markers.</p> <note> <p> In the current
     * implementation, Amazon S3 doesn't replicate the delete markers. The status must
     * be <code>Disabled</code>. </p> </note>
     */
    inline void SetStatus(const DeleteMarkerReplicationStatus& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * <p>Indicates whether to replicate delete markers.</p> <note> <p> In the current
     * implementation, Amazon S3 doesn't replicate the delete markers. The status must
     * be <code>Disabled</code>. </p> </note>
     */
    inline void SetStatus(DeleteMarkerReplicationStatus&& value) { m_statusHasBeenSet = true; m_status = std::move(value); }

    /**
     * <p>Indicates whether to replicate delete markers.</p> <note> <p> In the current
     * implementation, Amazon S3 doesn't replicate the delete markers. The status must
     * be <code>Disabled</code>. </p> </note>
     */
    inline DeleteMarkerReplication& WithStatus(const DeleteMarkerReplicationStatus& value) { SetStatus(value); return *this;}

    /**
     * <p>Indicates whether to replicate delete markers.</p> <note> <p> In the current
     * implementation, Amazon S3 doesn't replicate the delete markers. The status must
     * be <code>Disabled</code>. </p> </note>
     */
    inline DeleteMarkerReplication& WithStatus(DeleteMarkerReplicationStatus&& value) { SetStatus(std::move(value)); return *this;}

  private:

    DeleteMarkerReplicationStatus m_status;
    bool m_statusHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
