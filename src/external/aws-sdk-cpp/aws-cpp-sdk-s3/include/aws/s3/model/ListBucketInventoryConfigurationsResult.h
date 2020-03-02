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
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/InventoryConfiguration.h>
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
  class AWS_S3_API ListBucketInventoryConfigurationsResult
  {
  public:
    ListBucketInventoryConfigurationsResult();
    ListBucketInventoryConfigurationsResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    ListBucketInventoryConfigurationsResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>If sent in the request, the marker that is used as a starting point for this
     * inventory configuration list response.</p>
     */
    inline const Aws::String& GetContinuationToken() const{ return m_continuationToken; }

    /**
     * <p>If sent in the request, the marker that is used as a starting point for this
     * inventory configuration list response.</p>
     */
    inline void SetContinuationToken(const Aws::String& value) { m_continuationToken = value; }

    /**
     * <p>If sent in the request, the marker that is used as a starting point for this
     * inventory configuration list response.</p>
     */
    inline void SetContinuationToken(Aws::String&& value) { m_continuationToken = std::move(value); }

    /**
     * <p>If sent in the request, the marker that is used as a starting point for this
     * inventory configuration list response.</p>
     */
    inline void SetContinuationToken(const char* value) { m_continuationToken.assign(value); }

    /**
     * <p>If sent in the request, the marker that is used as a starting point for this
     * inventory configuration list response.</p>
     */
    inline ListBucketInventoryConfigurationsResult& WithContinuationToken(const Aws::String& value) { SetContinuationToken(value); return *this;}

    /**
     * <p>If sent in the request, the marker that is used as a starting point for this
     * inventory configuration list response.</p>
     */
    inline ListBucketInventoryConfigurationsResult& WithContinuationToken(Aws::String&& value) { SetContinuationToken(std::move(value)); return *this;}

    /**
     * <p>If sent in the request, the marker that is used as a starting point for this
     * inventory configuration list response.</p>
     */
    inline ListBucketInventoryConfigurationsResult& WithContinuationToken(const char* value) { SetContinuationToken(value); return *this;}


    /**
     * <p>The list of inventory configurations for a bucket.</p>
     */
    inline const Aws::Vector<InventoryConfiguration>& GetInventoryConfigurationList() const{ return m_inventoryConfigurationList; }

    /**
     * <p>The list of inventory configurations for a bucket.</p>
     */
    inline void SetInventoryConfigurationList(const Aws::Vector<InventoryConfiguration>& value) { m_inventoryConfigurationList = value; }

    /**
     * <p>The list of inventory configurations for a bucket.</p>
     */
    inline void SetInventoryConfigurationList(Aws::Vector<InventoryConfiguration>&& value) { m_inventoryConfigurationList = std::move(value); }

    /**
     * <p>The list of inventory configurations for a bucket.</p>
     */
    inline ListBucketInventoryConfigurationsResult& WithInventoryConfigurationList(const Aws::Vector<InventoryConfiguration>& value) { SetInventoryConfigurationList(value); return *this;}

    /**
     * <p>The list of inventory configurations for a bucket.</p>
     */
    inline ListBucketInventoryConfigurationsResult& WithInventoryConfigurationList(Aws::Vector<InventoryConfiguration>&& value) { SetInventoryConfigurationList(std::move(value)); return *this;}

    /**
     * <p>The list of inventory configurations for a bucket.</p>
     */
    inline ListBucketInventoryConfigurationsResult& AddInventoryConfigurationList(const InventoryConfiguration& value) { m_inventoryConfigurationList.push_back(value); return *this; }

    /**
     * <p>The list of inventory configurations for a bucket.</p>
     */
    inline ListBucketInventoryConfigurationsResult& AddInventoryConfigurationList(InventoryConfiguration&& value) { m_inventoryConfigurationList.push_back(std::move(value)); return *this; }


    /**
     * <p>Tells whether the returned list of inventory configurations is complete. A
     * value of true indicates that the list is not complete and the
     * NextContinuationToken is provided for a subsequent request.</p>
     */
    inline bool GetIsTruncated() const{ return m_isTruncated; }

    /**
     * <p>Tells whether the returned list of inventory configurations is complete. A
     * value of true indicates that the list is not complete and the
     * NextContinuationToken is provided for a subsequent request.</p>
     */
    inline void SetIsTruncated(bool value) { m_isTruncated = value; }

    /**
     * <p>Tells whether the returned list of inventory configurations is complete. A
     * value of true indicates that the list is not complete and the
     * NextContinuationToken is provided for a subsequent request.</p>
     */
    inline ListBucketInventoryConfigurationsResult& WithIsTruncated(bool value) { SetIsTruncated(value); return *this;}


    /**
     * <p>The marker used to continue this inventory configuration listing. Use the
     * <code>NextContinuationToken</code> from this response to continue the listing in
     * a subsequent request. The continuation token is an opaque value that Amazon S3
     * understands.</p>
     */
    inline const Aws::String& GetNextContinuationToken() const{ return m_nextContinuationToken; }

    /**
     * <p>The marker used to continue this inventory configuration listing. Use the
     * <code>NextContinuationToken</code> from this response to continue the listing in
     * a subsequent request. The continuation token is an opaque value that Amazon S3
     * understands.</p>
     */
    inline void SetNextContinuationToken(const Aws::String& value) { m_nextContinuationToken = value; }

    /**
     * <p>The marker used to continue this inventory configuration listing. Use the
     * <code>NextContinuationToken</code> from this response to continue the listing in
     * a subsequent request. The continuation token is an opaque value that Amazon S3
     * understands.</p>
     */
    inline void SetNextContinuationToken(Aws::String&& value) { m_nextContinuationToken = std::move(value); }

    /**
     * <p>The marker used to continue this inventory configuration listing. Use the
     * <code>NextContinuationToken</code> from this response to continue the listing in
     * a subsequent request. The continuation token is an opaque value that Amazon S3
     * understands.</p>
     */
    inline void SetNextContinuationToken(const char* value) { m_nextContinuationToken.assign(value); }

    /**
     * <p>The marker used to continue this inventory configuration listing. Use the
     * <code>NextContinuationToken</code> from this response to continue the listing in
     * a subsequent request. The continuation token is an opaque value that Amazon S3
     * understands.</p>
     */
    inline ListBucketInventoryConfigurationsResult& WithNextContinuationToken(const Aws::String& value) { SetNextContinuationToken(value); return *this;}

    /**
     * <p>The marker used to continue this inventory configuration listing. Use the
     * <code>NextContinuationToken</code> from this response to continue the listing in
     * a subsequent request. The continuation token is an opaque value that Amazon S3
     * understands.</p>
     */
    inline ListBucketInventoryConfigurationsResult& WithNextContinuationToken(Aws::String&& value) { SetNextContinuationToken(std::move(value)); return *this;}

    /**
     * <p>The marker used to continue this inventory configuration listing. Use the
     * <code>NextContinuationToken</code> from this response to continue the listing in
     * a subsequent request. The continuation token is an opaque value that Amazon S3
     * understands.</p>
     */
    inline ListBucketInventoryConfigurationsResult& WithNextContinuationToken(const char* value) { SetNextContinuationToken(value); return *this;}

  private:

    Aws::String m_continuationToken;

    Aws::Vector<InventoryConfiguration> m_inventoryConfigurationList;

    bool m_isTruncated;

    Aws::String m_nextContinuationToken;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
