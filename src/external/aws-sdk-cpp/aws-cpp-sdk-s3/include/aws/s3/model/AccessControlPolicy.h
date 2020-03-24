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
#include <aws/s3/model/Owner.h>
#include <aws/s3/model/Grant.h>
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
   * <p>Contains the elements that set the ACL permissions for an object per
   * grantee.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/AccessControlPolicy">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API AccessControlPolicy
  {
  public:
    AccessControlPolicy();
    AccessControlPolicy(const Aws::Utils::Xml::XmlNode& xmlNode);
    AccessControlPolicy& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>A list of grants.</p>
     */
    inline const Aws::Vector<Grant>& GetGrants() const{ return m_grants; }

    /**
     * <p>A list of grants.</p>
     */
    inline bool GrantsHasBeenSet() const { return m_grantsHasBeenSet; }

    /**
     * <p>A list of grants.</p>
     */
    inline void SetGrants(const Aws::Vector<Grant>& value) { m_grantsHasBeenSet = true; m_grants = value; }

    /**
     * <p>A list of grants.</p>
     */
    inline void SetGrants(Aws::Vector<Grant>&& value) { m_grantsHasBeenSet = true; m_grants = std::move(value); }

    /**
     * <p>A list of grants.</p>
     */
    inline AccessControlPolicy& WithGrants(const Aws::Vector<Grant>& value) { SetGrants(value); return *this;}

    /**
     * <p>A list of grants.</p>
     */
    inline AccessControlPolicy& WithGrants(Aws::Vector<Grant>&& value) { SetGrants(std::move(value)); return *this;}

    /**
     * <p>A list of grants.</p>
     */
    inline AccessControlPolicy& AddGrants(const Grant& value) { m_grantsHasBeenSet = true; m_grants.push_back(value); return *this; }

    /**
     * <p>A list of grants.</p>
     */
    inline AccessControlPolicy& AddGrants(Grant&& value) { m_grantsHasBeenSet = true; m_grants.push_back(std::move(value)); return *this; }


    /**
     * <p>Container for the bucket owner's display name and ID.</p>
     */
    inline const Owner& GetOwner() const{ return m_owner; }

    /**
     * <p>Container for the bucket owner's display name and ID.</p>
     */
    inline bool OwnerHasBeenSet() const { return m_ownerHasBeenSet; }

    /**
     * <p>Container for the bucket owner's display name and ID.</p>
     */
    inline void SetOwner(const Owner& value) { m_ownerHasBeenSet = true; m_owner = value; }

    /**
     * <p>Container for the bucket owner's display name and ID.</p>
     */
    inline void SetOwner(Owner&& value) { m_ownerHasBeenSet = true; m_owner = std::move(value); }

    /**
     * <p>Container for the bucket owner's display name and ID.</p>
     */
    inline AccessControlPolicy& WithOwner(const Owner& value) { SetOwner(value); return *this;}

    /**
     * <p>Container for the bucket owner's display name and ID.</p>
     */
    inline AccessControlPolicy& WithOwner(Owner&& value) { SetOwner(std::move(value)); return *this;}

  private:

    Aws::Vector<Grant> m_grants;
    bool m_grantsHasBeenSet;

    Owner m_owner;
    bool m_ownerHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
