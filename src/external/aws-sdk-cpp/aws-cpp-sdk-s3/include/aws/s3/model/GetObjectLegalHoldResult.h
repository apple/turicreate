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
#include <aws/s3/model/ObjectLockLegalHold.h>
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
  class AWS_S3_API GetObjectLegalHoldResult
  {
  public:
    GetObjectLegalHoldResult();
    GetObjectLegalHoldResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    GetObjectLegalHoldResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>The current Legal Hold status for the specified object.</p>
     */
    inline const ObjectLockLegalHold& GetLegalHold() const{ return m_legalHold; }

    /**
     * <p>The current Legal Hold status for the specified object.</p>
     */
    inline void SetLegalHold(const ObjectLockLegalHold& value) { m_legalHold = value; }

    /**
     * <p>The current Legal Hold status for the specified object.</p>
     */
    inline void SetLegalHold(ObjectLockLegalHold&& value) { m_legalHold = std::move(value); }

    /**
     * <p>The current Legal Hold status for the specified object.</p>
     */
    inline GetObjectLegalHoldResult& WithLegalHold(const ObjectLockLegalHold& value) { SetLegalHold(value); return *this;}

    /**
     * <p>The current Legal Hold status for the specified object.</p>
     */
    inline GetObjectLegalHoldResult& WithLegalHold(ObjectLockLegalHold&& value) { SetLegalHold(std::move(value)); return *this;}

  private:

    ObjectLockLegalHold m_legalHold;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
