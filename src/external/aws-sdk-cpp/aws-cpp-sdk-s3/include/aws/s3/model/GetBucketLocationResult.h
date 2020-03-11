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
#include <aws/s3/model/BucketLocationConstraint.h>
#include <utility>

namespace Aws
{
template<typename RESULT_TYPE>
class AmazonWebServiceResult;

namespace Utils
{
namespace Xml
{
  class XmlDocument;
} // namespace Xml
} // namespace Utils
namespace S3
{
namespace Model
{
  class AWS_S3_API GetBucketLocationResult
  {
  public:
    GetBucketLocationResult();
    GetBucketLocationResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    GetBucketLocationResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>Specifies the Region where the bucket resides. For a list of all the Amazon
     * S3 supported location constraints by Region, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/rande.html#s3_region">Regions
     * and Endpoints</a>.</p>
     */
    inline const BucketLocationConstraint& GetLocationConstraint() const{ return m_locationConstraint; }

    /**
     * <p>Specifies the Region where the bucket resides. For a list of all the Amazon
     * S3 supported location constraints by Region, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/rande.html#s3_region">Regions
     * and Endpoints</a>.</p>
     */
    inline void SetLocationConstraint(const BucketLocationConstraint& value) { m_locationConstraint = value; }

    /**
     * <p>Specifies the Region where the bucket resides. For a list of all the Amazon
     * S3 supported location constraints by Region, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/rande.html#s3_region">Regions
     * and Endpoints</a>.</p>
     */
    inline void SetLocationConstraint(BucketLocationConstraint&& value) { m_locationConstraint = std::move(value); }

    /**
     * <p>Specifies the Region where the bucket resides. For a list of all the Amazon
     * S3 supported location constraints by Region, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/rande.html#s3_region">Regions
     * and Endpoints</a>.</p>
     */
    inline GetBucketLocationResult& WithLocationConstraint(const BucketLocationConstraint& value) { SetLocationConstraint(value); return *this;}

    /**
     * <p>Specifies the Region where the bucket resides. For a list of all the Amazon
     * S3 supported location constraints by Region, see <a
     * href="https://docs.aws.amazon.com/general/latest/gr/rande.html#s3_region">Regions
     * and Endpoints</a>.</p>
     */
    inline GetBucketLocationResult& WithLocationConstraint(BucketLocationConstraint&& value) { SetLocationConstraint(std::move(value)); return *this;}

  private:

    BucketLocationConstraint m_locationConstraint;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
