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
#include <aws/s3/model/PublicAccessBlockConfiguration.h>
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
  class AWS_S3_API GetPublicAccessBlockResult
  {
  public:
    GetPublicAccessBlockResult();
    GetPublicAccessBlockResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    GetPublicAccessBlockResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>The <code>PublicAccessBlock</code> configuration currently in effect for this
     * Amazon S3 bucket.</p>
     */
    inline const PublicAccessBlockConfiguration& GetPublicAccessBlockConfiguration() const{ return m_publicAccessBlockConfiguration; }

    /**
     * <p>The <code>PublicAccessBlock</code> configuration currently in effect for this
     * Amazon S3 bucket.</p>
     */
    inline void SetPublicAccessBlockConfiguration(const PublicAccessBlockConfiguration& value) { m_publicAccessBlockConfiguration = value; }

    /**
     * <p>The <code>PublicAccessBlock</code> configuration currently in effect for this
     * Amazon S3 bucket.</p>
     */
    inline void SetPublicAccessBlockConfiguration(PublicAccessBlockConfiguration&& value) { m_publicAccessBlockConfiguration = std::move(value); }

    /**
     * <p>The <code>PublicAccessBlock</code> configuration currently in effect for this
     * Amazon S3 bucket.</p>
     */
    inline GetPublicAccessBlockResult& WithPublicAccessBlockConfiguration(const PublicAccessBlockConfiguration& value) { SetPublicAccessBlockConfiguration(value); return *this;}

    /**
     * <p>The <code>PublicAccessBlock</code> configuration currently in effect for this
     * Amazon S3 bucket.</p>
     */
    inline GetPublicAccessBlockResult& WithPublicAccessBlockConfiguration(PublicAccessBlockConfiguration&& value) { SetPublicAccessBlockConfiguration(std::move(value)); return *this;}

  private:

    PublicAccessBlockConfiguration m_publicAccessBlockConfiguration;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
