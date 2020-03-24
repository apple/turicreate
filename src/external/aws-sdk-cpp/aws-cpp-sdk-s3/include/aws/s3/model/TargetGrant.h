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
#include <aws/s3/model/Grantee.h>
#include <aws/s3/model/BucketLogsPermission.h>
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
   * <p>Container for granting information.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/TargetGrant">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API TargetGrant
  {
  public:
    TargetGrant();
    TargetGrant(const Aws::Utils::Xml::XmlNode& xmlNode);
    TargetGrant& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Container for the person being granted permissions.</p>
     */
    inline const Grantee& GetGrantee() const{ return m_grantee; }

    /**
     * <p>Container for the person being granted permissions.</p>
     */
    inline bool GranteeHasBeenSet() const { return m_granteeHasBeenSet; }

    /**
     * <p>Container for the person being granted permissions.</p>
     */
    inline void SetGrantee(const Grantee& value) { m_granteeHasBeenSet = true; m_grantee = value; }

    /**
     * <p>Container for the person being granted permissions.</p>
     */
    inline void SetGrantee(Grantee&& value) { m_granteeHasBeenSet = true; m_grantee = std::move(value); }

    /**
     * <p>Container for the person being granted permissions.</p>
     */
    inline TargetGrant& WithGrantee(const Grantee& value) { SetGrantee(value); return *this;}

    /**
     * <p>Container for the person being granted permissions.</p>
     */
    inline TargetGrant& WithGrantee(Grantee&& value) { SetGrantee(std::move(value)); return *this;}


    /**
     * <p>Logging permissions assigned to the Grantee for the bucket.</p>
     */
    inline const BucketLogsPermission& GetPermission() const{ return m_permission; }

    /**
     * <p>Logging permissions assigned to the Grantee for the bucket.</p>
     */
    inline bool PermissionHasBeenSet() const { return m_permissionHasBeenSet; }

    /**
     * <p>Logging permissions assigned to the Grantee for the bucket.</p>
     */
    inline void SetPermission(const BucketLogsPermission& value) { m_permissionHasBeenSet = true; m_permission = value; }

    /**
     * <p>Logging permissions assigned to the Grantee for the bucket.</p>
     */
    inline void SetPermission(BucketLogsPermission&& value) { m_permissionHasBeenSet = true; m_permission = std::move(value); }

    /**
     * <p>Logging permissions assigned to the Grantee for the bucket.</p>
     */
    inline TargetGrant& WithPermission(const BucketLogsPermission& value) { SetPermission(value); return *this;}

    /**
     * <p>Logging permissions assigned to the Grantee for the bucket.</p>
     */
    inline TargetGrant& WithPermission(BucketLogsPermission&& value) { SetPermission(std::move(value)); return *this;}

  private:

    Grantee m_grantee;
    bool m_granteeHasBeenSet;

    BucketLogsPermission m_permission;
    bool m_permissionHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
