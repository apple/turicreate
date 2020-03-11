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
  class AWS_S3_API CreateBucketResult
  {
  public:
    CreateBucketResult();
    CreateBucketResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    CreateBucketResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>Specifies the Region where the bucket will be created. If you are creating a
     * bucket on the US East (N. Virginia) Region (us-east-1), you do not need to
     * specify the location.</p>
     */
    inline const Aws::String& GetLocation() const{ return m_location; }

    /**
     * <p>Specifies the Region where the bucket will be created. If you are creating a
     * bucket on the US East (N. Virginia) Region (us-east-1), you do not need to
     * specify the location.</p>
     */
    inline void SetLocation(const Aws::String& value) { m_location = value; }

    /**
     * <p>Specifies the Region where the bucket will be created. If you are creating a
     * bucket on the US East (N. Virginia) Region (us-east-1), you do not need to
     * specify the location.</p>
     */
    inline void SetLocation(Aws::String&& value) { m_location = std::move(value); }

    /**
     * <p>Specifies the Region where the bucket will be created. If you are creating a
     * bucket on the US East (N. Virginia) Region (us-east-1), you do not need to
     * specify the location.</p>
     */
    inline void SetLocation(const char* value) { m_location.assign(value); }

    /**
     * <p>Specifies the Region where the bucket will be created. If you are creating a
     * bucket on the US East (N. Virginia) Region (us-east-1), you do not need to
     * specify the location.</p>
     */
    inline CreateBucketResult& WithLocation(const Aws::String& value) { SetLocation(value); return *this;}

    /**
     * <p>Specifies the Region where the bucket will be created. If you are creating a
     * bucket on the US East (N. Virginia) Region (us-east-1), you do not need to
     * specify the location.</p>
     */
    inline CreateBucketResult& WithLocation(Aws::String&& value) { SetLocation(std::move(value)); return *this;}

    /**
     * <p>Specifies the Region where the bucket will be created. If you are creating a
     * bucket on the US East (N. Virginia) Region (us-east-1), you do not need to
     * specify the location.</p>
     */
    inline CreateBucketResult& WithLocation(const char* value) { SetLocation(value); return *this;}

  private:

    Aws::String m_location;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
