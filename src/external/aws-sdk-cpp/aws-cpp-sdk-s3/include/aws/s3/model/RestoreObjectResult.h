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
#include <aws/s3/model/RequestCharged.h>
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
  class AWS_S3_API RestoreObjectResult
  {
  public:
    RestoreObjectResult();
    RestoreObjectResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    RestoreObjectResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    
    inline const RequestCharged& GetRequestCharged() const{ return m_requestCharged; }

    
    inline void SetRequestCharged(const RequestCharged& value) { m_requestCharged = value; }

    
    inline void SetRequestCharged(RequestCharged&& value) { m_requestCharged = std::move(value); }

    
    inline RestoreObjectResult& WithRequestCharged(const RequestCharged& value) { SetRequestCharged(value); return *this;}

    
    inline RestoreObjectResult& WithRequestCharged(RequestCharged&& value) { SetRequestCharged(std::move(value)); return *this;}


    /**
     * <p>Indicates the path in the provided S3 output location where Select results
     * will be restored to.</p>
     */
    inline const Aws::String& GetRestoreOutputPath() const{ return m_restoreOutputPath; }

    /**
     * <p>Indicates the path in the provided S3 output location where Select results
     * will be restored to.</p>
     */
    inline void SetRestoreOutputPath(const Aws::String& value) { m_restoreOutputPath = value; }

    /**
     * <p>Indicates the path in the provided S3 output location where Select results
     * will be restored to.</p>
     */
    inline void SetRestoreOutputPath(Aws::String&& value) { m_restoreOutputPath = std::move(value); }

    /**
     * <p>Indicates the path in the provided S3 output location where Select results
     * will be restored to.</p>
     */
    inline void SetRestoreOutputPath(const char* value) { m_restoreOutputPath.assign(value); }

    /**
     * <p>Indicates the path in the provided S3 output location where Select results
     * will be restored to.</p>
     */
    inline RestoreObjectResult& WithRestoreOutputPath(const Aws::String& value) { SetRestoreOutputPath(value); return *this;}

    /**
     * <p>Indicates the path in the provided S3 output location where Select results
     * will be restored to.</p>
     */
    inline RestoreObjectResult& WithRestoreOutputPath(Aws::String&& value) { SetRestoreOutputPath(std::move(value)); return *this;}

    /**
     * <p>Indicates the path in the provided S3 output location where Select results
     * will be restored to.</p>
     */
    inline RestoreObjectResult& WithRestoreOutputPath(const char* value) { SetRestoreOutputPath(value); return *this;}

  private:

    RequestCharged m_requestCharged;

    Aws::String m_restoreOutputPath;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
