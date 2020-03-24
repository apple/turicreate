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
#include <aws/s3/model/ObjectLockConfiguration.h>
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
  class AWS_S3_API GetObjectLockConfigurationResult
  {
  public:
    GetObjectLockConfigurationResult();
    GetObjectLockConfigurationResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    GetObjectLockConfigurationResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>The specified bucket's Object Lock configuration.</p>
     */
    inline const ObjectLockConfiguration& GetObjectLockConfiguration() const{ return m_objectLockConfiguration; }

    /**
     * <p>The specified bucket's Object Lock configuration.</p>
     */
    inline void SetObjectLockConfiguration(const ObjectLockConfiguration& value) { m_objectLockConfiguration = value; }

    /**
     * <p>The specified bucket's Object Lock configuration.</p>
     */
    inline void SetObjectLockConfiguration(ObjectLockConfiguration&& value) { m_objectLockConfiguration = std::move(value); }

    /**
     * <p>The specified bucket's Object Lock configuration.</p>
     */
    inline GetObjectLockConfigurationResult& WithObjectLockConfiguration(const ObjectLockConfiguration& value) { SetObjectLockConfiguration(value); return *this;}

    /**
     * <p>The specified bucket's Object Lock configuration.</p>
     */
    inline GetObjectLockConfigurationResult& WithObjectLockConfiguration(ObjectLockConfiguration&& value) { SetObjectLockConfiguration(std::move(value)); return *this;}

  private:

    ObjectLockConfiguration m_objectLockConfiguration;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
