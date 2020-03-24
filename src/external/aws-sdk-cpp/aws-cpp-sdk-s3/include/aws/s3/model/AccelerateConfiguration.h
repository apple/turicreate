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
#include <aws/s3/model/BucketAccelerateStatus.h>
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
   * <p>Configures the transfer acceleration state for an Amazon S3 bucket. For more
   * information, see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/transfer-acceleration.html">Amazon
   * S3 Transfer Acceleration</a> in the <i>Amazon Simple Storage Service Developer
   * Guide</i>.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/AccelerateConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API AccelerateConfiguration
  {
  public:
    AccelerateConfiguration();
    AccelerateConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    AccelerateConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies the transfer acceleration status of the bucket.</p>
     */
    inline const BucketAccelerateStatus& GetStatus() const{ return m_status; }

    /**
     * <p>Specifies the transfer acceleration status of the bucket.</p>
     */
    inline bool StatusHasBeenSet() const { return m_statusHasBeenSet; }

    /**
     * <p>Specifies the transfer acceleration status of the bucket.</p>
     */
    inline void SetStatus(const BucketAccelerateStatus& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * <p>Specifies the transfer acceleration status of the bucket.</p>
     */
    inline void SetStatus(BucketAccelerateStatus&& value) { m_statusHasBeenSet = true; m_status = std::move(value); }

    /**
     * <p>Specifies the transfer acceleration status of the bucket.</p>
     */
    inline AccelerateConfiguration& WithStatus(const BucketAccelerateStatus& value) { SetStatus(value); return *this;}

    /**
     * <p>Specifies the transfer acceleration status of the bucket.</p>
     */
    inline AccelerateConfiguration& WithStatus(BucketAccelerateStatus&& value) { SetStatus(std::move(value)); return *this;}

  private:

    BucketAccelerateStatus m_status;
    bool m_statusHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
