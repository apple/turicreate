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
#include <aws/s3/model/Condition.h>
#include <aws/s3/model/Redirect.h>
#include <utility>

namespace Aws
{
namespace Utils
{
namespace Xml
{
  class XmlNode;
} // namespace Xml
} // namespace Utils
namespace S3
{
namespace Model
{

  /**
   * <p>Specifies the redirect behavior and when a redirect is applied.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/RoutingRule">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API RoutingRule
  {
  public:
    RoutingRule();
    RoutingRule(const Aws::Utils::Xml::XmlNode& xmlNode);
    RoutingRule& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>A container for describing a condition that must be met for the specified
     * redirect to apply. For example, 1. If request is for pages in the
     * <code>/docs</code> folder, redirect to the <code>/documents</code> folder. 2. If
     * request results in HTTP error 4xx, redirect request to another host where you
     * might process the error.</p>
     */
    inline const Condition& GetCondition() const{ return m_condition; }

    /**
     * <p>A container for describing a condition that must be met for the specified
     * redirect to apply. For example, 1. If request is for pages in the
     * <code>/docs</code> folder, redirect to the <code>/documents</code> folder. 2. If
     * request results in HTTP error 4xx, redirect request to another host where you
     * might process the error.</p>
     */
    inline bool ConditionHasBeenSet() const { return m_conditionHasBeenSet; }

    /**
     * <p>A container for describing a condition that must be met for the specified
     * redirect to apply. For example, 1. If request is for pages in the
     * <code>/docs</code> folder, redirect to the <code>/documents</code> folder. 2. If
     * request results in HTTP error 4xx, redirect request to another host where you
     * might process the error.</p>
     */
    inline void SetCondition(const Condition& value) { m_conditionHasBeenSet = true; m_condition = value; }

    /**
     * <p>A container for describing a condition that must be met for the specified
     * redirect to apply. For example, 1. If request is for pages in the
     * <code>/docs</code> folder, redirect to the <code>/documents</code> folder. 2. If
     * request results in HTTP error 4xx, redirect request to another host where you
     * might process the error.</p>
     */
    inline void SetCondition(Condition&& value) { m_conditionHasBeenSet = true; m_condition = std::move(value); }

    /**
     * <p>A container for describing a condition that must be met for the specified
     * redirect to apply. For example, 1. If request is for pages in the
     * <code>/docs</code> folder, redirect to the <code>/documents</code> folder. 2. If
     * request results in HTTP error 4xx, redirect request to another host where you
     * might process the error.</p>
     */
    inline RoutingRule& WithCondition(const Condition& value) { SetCondition(value); return *this;}

    /**
     * <p>A container for describing a condition that must be met for the specified
     * redirect to apply. For example, 1. If request is for pages in the
     * <code>/docs</code> folder, redirect to the <code>/documents</code> folder. 2. If
     * request results in HTTP error 4xx, redirect request to another host where you
     * might process the error.</p>
     */
    inline RoutingRule& WithCondition(Condition&& value) { SetCondition(std::move(value)); return *this;}


    /**
     * <p>Container for redirect information. You can redirect requests to another
     * host, to another page, or with another protocol. In the event of an error, you
     * can specify a different error code to return.</p>
     */
    inline const Redirect& GetRedirect() const{ return m_redirect; }

    /**
     * <p>Container for redirect information. You can redirect requests to another
     * host, to another page, or with another protocol. In the event of an error, you
     * can specify a different error code to return.</p>
     */
    inline bool RedirectHasBeenSet() const { return m_redirectHasBeenSet; }

    /**
     * <p>Container for redirect information. You can redirect requests to another
     * host, to another page, or with another protocol. In the event of an error, you
     * can specify a different error code to return.</p>
     */
    inline void SetRedirect(const Redirect& value) { m_redirectHasBeenSet = true; m_redirect = value; }

    /**
     * <p>Container for redirect information. You can redirect requests to another
     * host, to another page, or with another protocol. In the event of an error, you
     * can specify a different error code to return.</p>
     */
    inline void SetRedirect(Redirect&& value) { m_redirectHasBeenSet = true; m_redirect = std::move(value); }

    /**
     * <p>Container for redirect information. You can redirect requests to another
     * host, to another page, or with another protocol. In the event of an error, you
     * can specify a different error code to return.</p>
     */
    inline RoutingRule& WithRedirect(const Redirect& value) { SetRedirect(value); return *this;}

    /**
     * <p>Container for redirect information. You can redirect requests to another
     * host, to another page, or with another protocol. In the event of an error, you
     * can specify a different error code to return.</p>
     */
    inline RoutingRule& WithRedirect(Redirect&& value) { SetRedirect(std::move(value)); return *this;}

  private:

    Condition m_condition;
    bool m_conditionHasBeenSet;

    Redirect m_redirect;
    bool m_redirectHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
