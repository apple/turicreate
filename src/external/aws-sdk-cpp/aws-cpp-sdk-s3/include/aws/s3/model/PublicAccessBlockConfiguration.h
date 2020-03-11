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
   * <p>The PublicAccessBlock configuration that you want to apply to this Amazon S3
   * bucket. You can enable the configuration options in any combination. For more
   * information about when Amazon S3 considers a bucket or object public, see <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/access-control-block-public-access.html#access-control-block-public-access-policy-status">The
   * Meaning of "Public"</a> in the <i>Amazon Simple Storage Service Developer
   * Guide</i>. </p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/PublicAccessBlockConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API PublicAccessBlockConfiguration
  {
  public:
    PublicAccessBlockConfiguration();
    PublicAccessBlockConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    PublicAccessBlockConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies whether Amazon S3 should block public access control lists (ACLs)
     * for this bucket and objects in this bucket. Setting this element to
     * <code>TRUE</code> causes the following behavior:</p> <ul> <li> <p>PUT Bucket acl
     * and PUT Object acl calls fail if the specified ACL is public.</p> </li> <li>
     * <p>PUT Object calls fail if the request includes a public ACL.</p> </li> <li>
     * <p>PUT Bucket calls fail if the request includes a public ACL.</p> </li> </ul>
     * <p>Enabling this setting doesn't affect existing policies or ACLs.</p>
     */
    inline bool GetBlockPublicAcls() const{ return m_blockPublicAcls; }

    /**
     * <p>Specifies whether Amazon S3 should block public access control lists (ACLs)
     * for this bucket and objects in this bucket. Setting this element to
     * <code>TRUE</code> causes the following behavior:</p> <ul> <li> <p>PUT Bucket acl
     * and PUT Object acl calls fail if the specified ACL is public.</p> </li> <li>
     * <p>PUT Object calls fail if the request includes a public ACL.</p> </li> <li>
     * <p>PUT Bucket calls fail if the request includes a public ACL.</p> </li> </ul>
     * <p>Enabling this setting doesn't affect existing policies or ACLs.</p>
     */
    inline bool BlockPublicAclsHasBeenSet() const { return m_blockPublicAclsHasBeenSet; }

    /**
     * <p>Specifies whether Amazon S3 should block public access control lists (ACLs)
     * for this bucket and objects in this bucket. Setting this element to
     * <code>TRUE</code> causes the following behavior:</p> <ul> <li> <p>PUT Bucket acl
     * and PUT Object acl calls fail if the specified ACL is public.</p> </li> <li>
     * <p>PUT Object calls fail if the request includes a public ACL.</p> </li> <li>
     * <p>PUT Bucket calls fail if the request includes a public ACL.</p> </li> </ul>
     * <p>Enabling this setting doesn't affect existing policies or ACLs.</p>
     */
    inline void SetBlockPublicAcls(bool value) { m_blockPublicAclsHasBeenSet = true; m_blockPublicAcls = value; }

    /**
     * <p>Specifies whether Amazon S3 should block public access control lists (ACLs)
     * for this bucket and objects in this bucket. Setting this element to
     * <code>TRUE</code> causes the following behavior:</p> <ul> <li> <p>PUT Bucket acl
     * and PUT Object acl calls fail if the specified ACL is public.</p> </li> <li>
     * <p>PUT Object calls fail if the request includes a public ACL.</p> </li> <li>
     * <p>PUT Bucket calls fail if the request includes a public ACL.</p> </li> </ul>
     * <p>Enabling this setting doesn't affect existing policies or ACLs.</p>
     */
    inline PublicAccessBlockConfiguration& WithBlockPublicAcls(bool value) { SetBlockPublicAcls(value); return *this;}


    /**
     * <p>Specifies whether Amazon S3 should ignore public ACLs for this bucket and
     * objects in this bucket. Setting this element to <code>TRUE</code> causes Amazon
     * S3 to ignore all public ACLs on this bucket and objects in this bucket.</p>
     * <p>Enabling this setting doesn't affect the persistence of any existing ACLs and
     * doesn't prevent new public ACLs from being set.</p>
     */
    inline bool GetIgnorePublicAcls() const{ return m_ignorePublicAcls; }

    /**
     * <p>Specifies whether Amazon S3 should ignore public ACLs for this bucket and
     * objects in this bucket. Setting this element to <code>TRUE</code> causes Amazon
     * S3 to ignore all public ACLs on this bucket and objects in this bucket.</p>
     * <p>Enabling this setting doesn't affect the persistence of any existing ACLs and
     * doesn't prevent new public ACLs from being set.</p>
     */
    inline bool IgnorePublicAclsHasBeenSet() const { return m_ignorePublicAclsHasBeenSet; }

    /**
     * <p>Specifies whether Amazon S3 should ignore public ACLs for this bucket and
     * objects in this bucket. Setting this element to <code>TRUE</code> causes Amazon
     * S3 to ignore all public ACLs on this bucket and objects in this bucket.</p>
     * <p>Enabling this setting doesn't affect the persistence of any existing ACLs and
     * doesn't prevent new public ACLs from being set.</p>
     */
    inline void SetIgnorePublicAcls(bool value) { m_ignorePublicAclsHasBeenSet = true; m_ignorePublicAcls = value; }

    /**
     * <p>Specifies whether Amazon S3 should ignore public ACLs for this bucket and
     * objects in this bucket. Setting this element to <code>TRUE</code> causes Amazon
     * S3 to ignore all public ACLs on this bucket and objects in this bucket.</p>
     * <p>Enabling this setting doesn't affect the persistence of any existing ACLs and
     * doesn't prevent new public ACLs from being set.</p>
     */
    inline PublicAccessBlockConfiguration& WithIgnorePublicAcls(bool value) { SetIgnorePublicAcls(value); return *this;}


    /**
     * <p>Specifies whether Amazon S3 should block public bucket policies for this
     * bucket. Setting this element to <code>TRUE</code> causes Amazon S3 to reject
     * calls to PUT Bucket policy if the specified bucket policy allows public access.
     * </p> <p>Enabling this setting doesn't affect existing bucket policies.</p>
     */
    inline bool GetBlockPublicPolicy() const{ return m_blockPublicPolicy; }

    /**
     * <p>Specifies whether Amazon S3 should block public bucket policies for this
     * bucket. Setting this element to <code>TRUE</code> causes Amazon S3 to reject
     * calls to PUT Bucket policy if the specified bucket policy allows public access.
     * </p> <p>Enabling this setting doesn't affect existing bucket policies.</p>
     */
    inline bool BlockPublicPolicyHasBeenSet() const { return m_blockPublicPolicyHasBeenSet; }

    /**
     * <p>Specifies whether Amazon S3 should block public bucket policies for this
     * bucket. Setting this element to <code>TRUE</code> causes Amazon S3 to reject
     * calls to PUT Bucket policy if the specified bucket policy allows public access.
     * </p> <p>Enabling this setting doesn't affect existing bucket policies.</p>
     */
    inline void SetBlockPublicPolicy(bool value) { m_blockPublicPolicyHasBeenSet = true; m_blockPublicPolicy = value; }

    /**
     * <p>Specifies whether Amazon S3 should block public bucket policies for this
     * bucket. Setting this element to <code>TRUE</code> causes Amazon S3 to reject
     * calls to PUT Bucket policy if the specified bucket policy allows public access.
     * </p> <p>Enabling this setting doesn't affect existing bucket policies.</p>
     */
    inline PublicAccessBlockConfiguration& WithBlockPublicPolicy(bool value) { SetBlockPublicPolicy(value); return *this;}


    /**
     * <p>Specifies whether Amazon S3 should restrict public bucket policies for this
     * bucket. Setting this element to <code>TRUE</code> restricts access to this
     * bucket to only AWS services and authorized users within this account if the
     * bucket has a public policy.</p> <p>Enabling this setting doesn't affect
     * previously stored bucket policies, except that public and cross-account access
     * within any public bucket policy, including non-public delegation to specific
     * accounts, is blocked.</p>
     */
    inline bool GetRestrictPublicBuckets() const{ return m_restrictPublicBuckets; }

    /**
     * <p>Specifies whether Amazon S3 should restrict public bucket policies for this
     * bucket. Setting this element to <code>TRUE</code> restricts access to this
     * bucket to only AWS services and authorized users within this account if the
     * bucket has a public policy.</p> <p>Enabling this setting doesn't affect
     * previously stored bucket policies, except that public and cross-account access
     * within any public bucket policy, including non-public delegation to specific
     * accounts, is blocked.</p>
     */
    inline bool RestrictPublicBucketsHasBeenSet() const { return m_restrictPublicBucketsHasBeenSet; }

    /**
     * <p>Specifies whether Amazon S3 should restrict public bucket policies for this
     * bucket. Setting this element to <code>TRUE</code> restricts access to this
     * bucket to only AWS services and authorized users within this account if the
     * bucket has a public policy.</p> <p>Enabling this setting doesn't affect
     * previously stored bucket policies, except that public and cross-account access
     * within any public bucket policy, including non-public delegation to specific
     * accounts, is blocked.</p>
     */
    inline void SetRestrictPublicBuckets(bool value) { m_restrictPublicBucketsHasBeenSet = true; m_restrictPublicBuckets = value; }

    /**
     * <p>Specifies whether Amazon S3 should restrict public bucket policies for this
     * bucket. Setting this element to <code>TRUE</code> restricts access to this
     * bucket to only AWS services and authorized users within this account if the
     * bucket has a public policy.</p> <p>Enabling this setting doesn't affect
     * previously stored bucket policies, except that public and cross-account access
     * within any public bucket policy, including non-public delegation to specific
     * accounts, is blocked.</p>
     */
    inline PublicAccessBlockConfiguration& WithRestrictPublicBuckets(bool value) { SetRestrictPublicBuckets(value); return *this;}

  private:

    bool m_blockPublicAcls;
    bool m_blockPublicAclsHasBeenSet;

    bool m_ignorePublicAcls;
    bool m_ignorePublicAclsHasBeenSet;

    bool m_blockPublicPolicy;
    bool m_blockPublicPolicyHasBeenSet;

    bool m_restrictPublicBuckets;
    bool m_restrictPublicBucketsHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
