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
#include <aws/s3/model/RequestCharged.h>
#include <aws/s3/model/DeletedObject.h>
#include <aws/s3/model/Error.h>
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
  class AWS_S3_API DeleteObjectsResult
  {
  public:
    DeleteObjectsResult();
    DeleteObjectsResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    DeleteObjectsResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>Container element for a successful delete. It identifies the object that was
     * successfully deleted.</p>
     */
    inline const Aws::Vector<DeletedObject>& GetDeleted() const{ return m_deleted; }

    /**
     * <p>Container element for a successful delete. It identifies the object that was
     * successfully deleted.</p>
     */
    inline void SetDeleted(const Aws::Vector<DeletedObject>& value) { m_deleted = value; }

    /**
     * <p>Container element for a successful delete. It identifies the object that was
     * successfully deleted.</p>
     */
    inline void SetDeleted(Aws::Vector<DeletedObject>&& value) { m_deleted = std::move(value); }

    /**
     * <p>Container element for a successful delete. It identifies the object that was
     * successfully deleted.</p>
     */
    inline DeleteObjectsResult& WithDeleted(const Aws::Vector<DeletedObject>& value) { SetDeleted(value); return *this;}

    /**
     * <p>Container element for a successful delete. It identifies the object that was
     * successfully deleted.</p>
     */
    inline DeleteObjectsResult& WithDeleted(Aws::Vector<DeletedObject>&& value) { SetDeleted(std::move(value)); return *this;}

    /**
     * <p>Container element for a successful delete. It identifies the object that was
     * successfully deleted.</p>
     */
    inline DeleteObjectsResult& AddDeleted(const DeletedObject& value) { m_deleted.push_back(value); return *this; }

    /**
     * <p>Container element for a successful delete. It identifies the object that was
     * successfully deleted.</p>
     */
    inline DeleteObjectsResult& AddDeleted(DeletedObject&& value) { m_deleted.push_back(std::move(value)); return *this; }


    
    inline const RequestCharged& GetRequestCharged() const{ return m_requestCharged; }

    
    inline void SetRequestCharged(const RequestCharged& value) { m_requestCharged = value; }

    
    inline void SetRequestCharged(RequestCharged&& value) { m_requestCharged = std::move(value); }

    
    inline DeleteObjectsResult& WithRequestCharged(const RequestCharged& value) { SetRequestCharged(value); return *this;}

    
    inline DeleteObjectsResult& WithRequestCharged(RequestCharged&& value) { SetRequestCharged(std::move(value)); return *this;}


    /**
     * <p>Container for a failed delete operation that describes the object that Amazon
     * S3 attempted to delete and the error it encountered.</p>
     */
    inline const Aws::Vector<Error>& GetErrors() const{ return m_errors; }

    /**
     * <p>Container for a failed delete operation that describes the object that Amazon
     * S3 attempted to delete and the error it encountered.</p>
     */
    inline void SetErrors(const Aws::Vector<Error>& value) { m_errors = value; }

    /**
     * <p>Container for a failed delete operation that describes the object that Amazon
     * S3 attempted to delete and the error it encountered.</p>
     */
    inline void SetErrors(Aws::Vector<Error>&& value) { m_errors = std::move(value); }

    /**
     * <p>Container for a failed delete operation that describes the object that Amazon
     * S3 attempted to delete and the error it encountered.</p>
     */
    inline DeleteObjectsResult& WithErrors(const Aws::Vector<Error>& value) { SetErrors(value); return *this;}

    /**
     * <p>Container for a failed delete operation that describes the object that Amazon
     * S3 attempted to delete and the error it encountered.</p>
     */
    inline DeleteObjectsResult& WithErrors(Aws::Vector<Error>&& value) { SetErrors(std::move(value)); return *this;}

    /**
     * <p>Container for a failed delete operation that describes the object that Amazon
     * S3 attempted to delete and the error it encountered.</p>
     */
    inline DeleteObjectsResult& AddErrors(const Error& value) { m_errors.push_back(value); return *this; }

    /**
     * <p>Container for a failed delete operation that describes the object that Amazon
     * S3 attempted to delete and the error it encountered.</p>
     */
    inline DeleteObjectsResult& AddErrors(Error&& value) { m_errors.push_back(std::move(value)); return *this; }

  private:

    Aws::Vector<DeletedObject> m_deleted;

    RequestCharged m_requestCharged;

    Aws::Vector<Error> m_errors;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
