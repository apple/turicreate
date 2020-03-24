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
#include <aws/s3/model/ExistingObjectReplicationStatus.h>
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
   * <p>Optional configuration to replicate existing source bucket objects. For more
   * information, see <a href="
   * https://docs.aws.amazon.com/AmazonS3/latest/dev/replication-what-is-isnot-replicated.html#existing-object-replication">Replicating
   * Existing Objects</a> in the <i>Amazon S3 Developer Guide</i>. </p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ExistingObjectReplication">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ExistingObjectReplication
  {
  public:
    ExistingObjectReplication();
    ExistingObjectReplication(const Aws::Utils::Xml::XmlNode& xmlNode);
    ExistingObjectReplication& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p/>
     */
    inline const ExistingObjectReplicationStatus& GetStatus() const{ return m_status; }

    /**
     * <p/>
     */
    inline bool StatusHasBeenSet() const { return m_statusHasBeenSet; }

    /**
     * <p/>
     */
    inline void SetStatus(const ExistingObjectReplicationStatus& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * <p/>
     */
    inline void SetStatus(ExistingObjectReplicationStatus&& value) { m_statusHasBeenSet = true; m_status = std::move(value); }

    /**
     * <p/>
     */
    inline ExistingObjectReplication& WithStatus(const ExistingObjectReplicationStatus& value) { SetStatus(value); return *this;}

    /**
     * <p/>
     */
    inline ExistingObjectReplication& WithStatus(ExistingObjectReplicationStatus&& value) { SetStatus(std::move(value)); return *this;}

  private:

    ExistingObjectReplicationStatus m_status;
    bool m_statusHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
