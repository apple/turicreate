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
#include <aws/s3/model/MFADelete.h>
#include <aws/s3/model/BucketVersioningStatus.h>
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
   * <p>Describes the versioning state of an Amazon S3 bucket. For more information,
   * see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTVersioningStatus.html">PUT
   * Bucket versioning</a> in the <i>Amazon Simple Storage Service API
   * Reference</i>.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/VersioningConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API VersioningConfiguration
  {
  public:
    VersioningConfiguration();
    VersioningConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    VersioningConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies whether MFA delete is enabled in the bucket versioning
     * configuration. This element is only returned if the bucket has been configured
     * with MFA delete. If the bucket has never been so configured, this element is not
     * returned.</p>
     */
    inline const MFADelete& GetMFADelete() const{ return m_mFADelete; }

    /**
     * <p>Specifies whether MFA delete is enabled in the bucket versioning
     * configuration. This element is only returned if the bucket has been configured
     * with MFA delete. If the bucket has never been so configured, this element is not
     * returned.</p>
     */
    inline bool MFADeleteHasBeenSet() const { return m_mFADeleteHasBeenSet; }

    /**
     * <p>Specifies whether MFA delete is enabled in the bucket versioning
     * configuration. This element is only returned if the bucket has been configured
     * with MFA delete. If the bucket has never been so configured, this element is not
     * returned.</p>
     */
    inline void SetMFADelete(const MFADelete& value) { m_mFADeleteHasBeenSet = true; m_mFADelete = value; }

    /**
     * <p>Specifies whether MFA delete is enabled in the bucket versioning
     * configuration. This element is only returned if the bucket has been configured
     * with MFA delete. If the bucket has never been so configured, this element is not
     * returned.</p>
     */
    inline void SetMFADelete(MFADelete&& value) { m_mFADeleteHasBeenSet = true; m_mFADelete = std::move(value); }

    /**
     * <p>Specifies whether MFA delete is enabled in the bucket versioning
     * configuration. This element is only returned if the bucket has been configured
     * with MFA delete. If the bucket has never been so configured, this element is not
     * returned.</p>
     */
    inline VersioningConfiguration& WithMFADelete(const MFADelete& value) { SetMFADelete(value); return *this;}

    /**
     * <p>Specifies whether MFA delete is enabled in the bucket versioning
     * configuration. This element is only returned if the bucket has been configured
     * with MFA delete. If the bucket has never been so configured, this element is not
     * returned.</p>
     */
    inline VersioningConfiguration& WithMFADelete(MFADelete&& value) { SetMFADelete(std::move(value)); return *this;}


    /**
     * <p>The versioning state of the bucket.</p>
     */
    inline const BucketVersioningStatus& GetStatus() const{ return m_status; }

    /**
     * <p>The versioning state of the bucket.</p>
     */
    inline bool StatusHasBeenSet() const { return m_statusHasBeenSet; }

    /**
     * <p>The versioning state of the bucket.</p>
     */
    inline void SetStatus(const BucketVersioningStatus& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * <p>The versioning state of the bucket.</p>
     */
    inline void SetStatus(BucketVersioningStatus&& value) { m_statusHasBeenSet = true; m_status = std::move(value); }

    /**
     * <p>The versioning state of the bucket.</p>
     */
    inline VersioningConfiguration& WithStatus(const BucketVersioningStatus& value) { SetStatus(value); return *this;}

    /**
     * <p>The versioning state of the bucket.</p>
     */
    inline VersioningConfiguration& WithStatus(BucketVersioningStatus&& value) { SetStatus(std::move(value)); return *this;}

  private:

    MFADelete m_mFADelete;
    bool m_mFADeleteHasBeenSet;

    BucketVersioningStatus m_status;
    bool m_statusHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
