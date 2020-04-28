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
#include <aws/s3/model/RedirectAllRequestsTo.h>
#include <aws/s3/model/IndexDocument.h>
#include <aws/s3/model/ErrorDocument.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/RoutingRule.h>
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
  class AWS_S3_API GetBucketWebsiteResult
  {
  public:
    GetBucketWebsiteResult();
    GetBucketWebsiteResult(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    GetBucketWebsiteResult& operator=(const Aws::AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);


    /**
     * <p>Specifies the redirect behavior of all requests to a website endpoint of an
     * Amazon S3 bucket.</p>
     */
    inline const RedirectAllRequestsTo& GetRedirectAllRequestsTo() const{ return m_redirectAllRequestsTo; }

    /**
     * <p>Specifies the redirect behavior of all requests to a website endpoint of an
     * Amazon S3 bucket.</p>
     */
    inline void SetRedirectAllRequestsTo(const RedirectAllRequestsTo& value) { m_redirectAllRequestsTo = value; }

    /**
     * <p>Specifies the redirect behavior of all requests to a website endpoint of an
     * Amazon S3 bucket.</p>
     */
    inline void SetRedirectAllRequestsTo(RedirectAllRequestsTo&& value) { m_redirectAllRequestsTo = std::move(value); }

    /**
     * <p>Specifies the redirect behavior of all requests to a website endpoint of an
     * Amazon S3 bucket.</p>
     */
    inline GetBucketWebsiteResult& WithRedirectAllRequestsTo(const RedirectAllRequestsTo& value) { SetRedirectAllRequestsTo(value); return *this;}

    /**
     * <p>Specifies the redirect behavior of all requests to a website endpoint of an
     * Amazon S3 bucket.</p>
     */
    inline GetBucketWebsiteResult& WithRedirectAllRequestsTo(RedirectAllRequestsTo&& value) { SetRedirectAllRequestsTo(std::move(value)); return *this;}


    /**
     * <p>The name of the index document for the website.</p>
     */
    inline const IndexDocument& GetIndexDocument() const{ return m_indexDocument; }

    /**
     * <p>The name of the index document for the website.</p>
     */
    inline void SetIndexDocument(const IndexDocument& value) { m_indexDocument = value; }

    /**
     * <p>The name of the index document for the website.</p>
     */
    inline void SetIndexDocument(IndexDocument&& value) { m_indexDocument = std::move(value); }

    /**
     * <p>The name of the index document for the website.</p>
     */
    inline GetBucketWebsiteResult& WithIndexDocument(const IndexDocument& value) { SetIndexDocument(value); return *this;}

    /**
     * <p>The name of the index document for the website.</p>
     */
    inline GetBucketWebsiteResult& WithIndexDocument(IndexDocument&& value) { SetIndexDocument(std::move(value)); return *this;}


    /**
     * <p>The name of the error document for the website.</p>
     */
    inline const ErrorDocument& GetErrorDocument() const{ return m_errorDocument; }

    /**
     * <p>The name of the error document for the website.</p>
     */
    inline void SetErrorDocument(const ErrorDocument& value) { m_errorDocument = value; }

    /**
     * <p>The name of the error document for the website.</p>
     */
    inline void SetErrorDocument(ErrorDocument&& value) { m_errorDocument = std::move(value); }

    /**
     * <p>The name of the error document for the website.</p>
     */
    inline GetBucketWebsiteResult& WithErrorDocument(const ErrorDocument& value) { SetErrorDocument(value); return *this;}

    /**
     * <p>The name of the error document for the website.</p>
     */
    inline GetBucketWebsiteResult& WithErrorDocument(ErrorDocument&& value) { SetErrorDocument(std::move(value)); return *this;}


    /**
     * <p>Rules that define when a redirect is applied and the redirect behavior.</p>
     */
    inline const Aws::Vector<RoutingRule>& GetRoutingRules() const{ return m_routingRules; }

    /**
     * <p>Rules that define when a redirect is applied and the redirect behavior.</p>
     */
    inline void SetRoutingRules(const Aws::Vector<RoutingRule>& value) { m_routingRules = value; }

    /**
     * <p>Rules that define when a redirect is applied and the redirect behavior.</p>
     */
    inline void SetRoutingRules(Aws::Vector<RoutingRule>&& value) { m_routingRules = std::move(value); }

    /**
     * <p>Rules that define when a redirect is applied and the redirect behavior.</p>
     */
    inline GetBucketWebsiteResult& WithRoutingRules(const Aws::Vector<RoutingRule>& value) { SetRoutingRules(value); return *this;}

    /**
     * <p>Rules that define when a redirect is applied and the redirect behavior.</p>
     */
    inline GetBucketWebsiteResult& WithRoutingRules(Aws::Vector<RoutingRule>&& value) { SetRoutingRules(std::move(value)); return *this;}

    /**
     * <p>Rules that define when a redirect is applied and the redirect behavior.</p>
     */
    inline GetBucketWebsiteResult& AddRoutingRules(const RoutingRule& value) { m_routingRules.push_back(value); return *this; }

    /**
     * <p>Rules that define when a redirect is applied and the redirect behavior.</p>
     */
    inline GetBucketWebsiteResult& AddRoutingRules(RoutingRule&& value) { m_routingRules.push_back(std::move(value)); return *this; }

  private:

    RedirectAllRequestsTo m_redirectAllRequestsTo;

    IndexDocument m_indexDocument;

    ErrorDocument m_errorDocument;

    Aws::Vector<RoutingRule> m_routingRules;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
