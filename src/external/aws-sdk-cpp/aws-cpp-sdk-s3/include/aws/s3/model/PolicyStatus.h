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
   * <p>The container element for a bucket's policy status.</p><p><h3>See Also:</h3> 
   * <a href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/PolicyStatus">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API PolicyStatus
  {
  public:
    PolicyStatus();
    PolicyStatus(const Aws::Utils::Xml::XmlNode& xmlNode);
    PolicyStatus& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The policy status for this bucket. <code>TRUE</code> indicates that this
     * bucket is public. <code>FALSE</code> indicates that the bucket is not
     * public.</p>
     */
    inline bool GetIsPublic() const{ return m_isPublic; }

    /**
     * <p>The policy status for this bucket. <code>TRUE</code> indicates that this
     * bucket is public. <code>FALSE</code> indicates that the bucket is not
     * public.</p>
     */
    inline bool IsPublicHasBeenSet() const { return m_isPublicHasBeenSet; }

    /**
     * <p>The policy status for this bucket. <code>TRUE</code> indicates that this
     * bucket is public. <code>FALSE</code> indicates that the bucket is not
     * public.</p>
     */
    inline void SetIsPublic(bool value) { m_isPublicHasBeenSet = true; m_isPublic = value; }

    /**
     * <p>The policy status for this bucket. <code>TRUE</code> indicates that this
     * bucket is public. <code>FALSE</code> indicates that the bucket is not
     * public.</p>
     */
    inline PolicyStatus& WithIsPublic(bool value) { SetIsPublic(value); return *this;}

  private:

    bool m_isPublic;
    bool m_isPublicHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
