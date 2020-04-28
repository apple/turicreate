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
#include <aws/s3/model/ObjectLockRetention.h>
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
  class AWS_S3_API GetObjectRetentionResult
  {
  public:
    GetObjectRetentionResult();
    GetObjectRetentionResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    GetObjectRetentionResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>The container element for an object's retention settings.</p>
     */
    inline const ObjectLockRetention& GetRetention() const{ return m_retention; }

    /**
     * <p>The container element for an object's retention settings.</p>
     */
    inline void SetRetention(const ObjectLockRetention& value) { m_retention = value; }

    /**
     * <p>The container element for an object's retention settings.</p>
     */
    inline void SetRetention(ObjectLockRetention&& value) { m_retention = std::move(value); }

    /**
     * <p>The container element for an object's retention settings.</p>
     */
    inline GetObjectRetentionResult& WithRetention(const ObjectLockRetention& value) { SetRetention(value); return *this;}

    /**
     * <p>The container element for an object's retention settings.</p>
     */
    inline GetObjectRetentionResult& WithRetention(ObjectLockRetention&& value) { SetRetention(std::move(value)); return *this;}

  private:

    ObjectLockRetention m_retention;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
