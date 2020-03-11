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
   * <p>A container for describing a condition that must be met for the specified
   * redirect to apply. For example, 1. If request is for pages in the
   * <code>/docs</code> folder, redirect to the <code>/documents</code> folder. 2. If
   * request results in HTTP error 4xx, redirect request to another host where you
   * might process the error.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/Condition">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API Condition
  {
  public:
    Condition();
    Condition(const Aws::Utils::Xml::XmlNode& xmlNode);
    Condition& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The HTTP error code when the redirect is applied. In the event of an error,
     * if the error code equals this value, then the specified redirect is applied.
     * Required when parent element <code>Condition</code> is specified and sibling
     * <code>KeyPrefixEquals</code> is not specified. If both are specified, then both
     * must be true for the redirect to be applied.</p>
     */
    inline const Aws::String& GetHttpErrorCodeReturnedEquals() const{ return m_httpErrorCodeReturnedEquals; }

    /**
     * <p>The HTTP error code when the redirect is applied. In the event of an error,
     * if the error code equals this value, then the specified redirect is applied.
     * Required when parent element <code>Condition</code> is specified and sibling
     * <code>KeyPrefixEquals</code> is not specified. If both are specified, then both
     * must be true for the redirect to be applied.</p>
     */
    inline bool HttpErrorCodeReturnedEqualsHasBeenSet() const { return m_httpErrorCodeReturnedEqualsHasBeenSet; }

    /**
     * <p>The HTTP error code when the redirect is applied. In the event of an error,
     * if the error code equals this value, then the specified redirect is applied.
     * Required when parent element <code>Condition</code> is specified and sibling
     * <code>KeyPrefixEquals</code> is not specified. If both are specified, then both
     * must be true for the redirect to be applied.</p>
     */
    inline void SetHttpErrorCodeReturnedEquals(const Aws::String& value) { m_httpErrorCodeReturnedEqualsHasBeenSet = true; m_httpErrorCodeReturnedEquals = value; }

    /**
     * <p>The HTTP error code when the redirect is applied. In the event of an error,
     * if the error code equals this value, then the specified redirect is applied.
     * Required when parent element <code>Condition</code> is specified and sibling
     * <code>KeyPrefixEquals</code> is not specified. If both are specified, then both
     * must be true for the redirect to be applied.</p>
     */
    inline void SetHttpErrorCodeReturnedEquals(Aws::String&& value) { m_httpErrorCodeReturnedEqualsHasBeenSet = true; m_httpErrorCodeReturnedEquals = std::move(value); }

    /**
     * <p>The HTTP error code when the redirect is applied. In the event of an error,
     * if the error code equals this value, then the specified redirect is applied.
     * Required when parent element <code>Condition</code> is specified and sibling
     * <code>KeyPrefixEquals</code> is not specified. If both are specified, then both
     * must be true for the redirect to be applied.</p>
     */
    inline void SetHttpErrorCodeReturnedEquals(const char* value) { m_httpErrorCodeReturnedEqualsHasBeenSet = true; m_httpErrorCodeReturnedEquals.assign(value); }

    /**
     * <p>The HTTP error code when the redirect is applied. In the event of an error,
     * if the error code equals this value, then the specified redirect is applied.
     * Required when parent element <code>Condition</code> is specified and sibling
     * <code>KeyPrefixEquals</code> is not specified. If both are specified, then both
     * must be true for the redirect to be applied.</p>
     */
    inline Condition& WithHttpErrorCodeReturnedEquals(const Aws::String& value) { SetHttpErrorCodeReturnedEquals(value); return *this;}

    /**
     * <p>The HTTP error code when the redirect is applied. In the event of an error,
     * if the error code equals this value, then the specified redirect is applied.
     * Required when parent element <code>Condition</code> is specified and sibling
     * <code>KeyPrefixEquals</code> is not specified. If both are specified, then both
     * must be true for the redirect to be applied.</p>
     */
    inline Condition& WithHttpErrorCodeReturnedEquals(Aws::String&& value) { SetHttpErrorCodeReturnedEquals(std::move(value)); return *this;}

    /**
     * <p>The HTTP error code when the redirect is applied. In the event of an error,
     * if the error code equals this value, then the specified redirect is applied.
     * Required when parent element <code>Condition</code> is specified and sibling
     * <code>KeyPrefixEquals</code> is not specified. If both are specified, then both
     * must be true for the redirect to be applied.</p>
     */
    inline Condition& WithHttpErrorCodeReturnedEquals(const char* value) { SetHttpErrorCodeReturnedEquals(value); return *this;}


    /**
     * <p>The object key name prefix when the redirect is applied. For example, to
     * redirect requests for <code>ExamplePage.html</code>, the key prefix will be
     * <code>ExamplePage.html</code>. To redirect request for all pages with the prefix
     * <code>docs/</code>, the key prefix will be <code>/docs</code>, which identifies
     * all objects in the <code>docs/</code> folder. Required when the parent element
     * <code>Condition</code> is specified and sibling
     * <code>HttpErrorCodeReturnedEquals</code> is not specified. If both conditions
     * are specified, both must be true for the redirect to be applied.</p>
     */
    inline const Aws::String& GetKeyPrefixEquals() const{ return m_keyPrefixEquals; }

    /**
     * <p>The object key name prefix when the redirect is applied. For example, to
     * redirect requests for <code>ExamplePage.html</code>, the key prefix will be
     * <code>ExamplePage.html</code>. To redirect request for all pages with the prefix
     * <code>docs/</code>, the key prefix will be <code>/docs</code>, which identifies
     * all objects in the <code>docs/</code> folder. Required when the parent element
     * <code>Condition</code> is specified and sibling
     * <code>HttpErrorCodeReturnedEquals</code> is not specified. If both conditions
     * are specified, both must be true for the redirect to be applied.</p>
     */
    inline bool KeyPrefixEqualsHasBeenSet() const { return m_keyPrefixEqualsHasBeenSet; }

    /**
     * <p>The object key name prefix when the redirect is applied. For example, to
     * redirect requests for <code>ExamplePage.html</code>, the key prefix will be
     * <code>ExamplePage.html</code>. To redirect request for all pages with the prefix
     * <code>docs/</code>, the key prefix will be <code>/docs</code>, which identifies
     * all objects in the <code>docs/</code> folder. Required when the parent element
     * <code>Condition</code> is specified and sibling
     * <code>HttpErrorCodeReturnedEquals</code> is not specified. If both conditions
     * are specified, both must be true for the redirect to be applied.</p>
     */
    inline void SetKeyPrefixEquals(const Aws::String& value) { m_keyPrefixEqualsHasBeenSet = true; m_keyPrefixEquals = value; }

    /**
     * <p>The object key name prefix when the redirect is applied. For example, to
     * redirect requests for <code>ExamplePage.html</code>, the key prefix will be
     * <code>ExamplePage.html</code>. To redirect request for all pages with the prefix
     * <code>docs/</code>, the key prefix will be <code>/docs</code>, which identifies
     * all objects in the <code>docs/</code> folder. Required when the parent element
     * <code>Condition</code> is specified and sibling
     * <code>HttpErrorCodeReturnedEquals</code> is not specified. If both conditions
     * are specified, both must be true for the redirect to be applied.</p>
     */
    inline void SetKeyPrefixEquals(Aws::String&& value) { m_keyPrefixEqualsHasBeenSet = true; m_keyPrefixEquals = std::move(value); }

    /**
     * <p>The object key name prefix when the redirect is applied. For example, to
     * redirect requests for <code>ExamplePage.html</code>, the key prefix will be
     * <code>ExamplePage.html</code>. To redirect request for all pages with the prefix
     * <code>docs/</code>, the key prefix will be <code>/docs</code>, which identifies
     * all objects in the <code>docs/</code> folder. Required when the parent element
     * <code>Condition</code> is specified and sibling
     * <code>HttpErrorCodeReturnedEquals</code> is not specified. If both conditions
     * are specified, both must be true for the redirect to be applied.</p>
     */
    inline void SetKeyPrefixEquals(const char* value) { m_keyPrefixEqualsHasBeenSet = true; m_keyPrefixEquals.assign(value); }

    /**
     * <p>The object key name prefix when the redirect is applied. For example, to
     * redirect requests for <code>ExamplePage.html</code>, the key prefix will be
     * <code>ExamplePage.html</code>. To redirect request for all pages with the prefix
     * <code>docs/</code>, the key prefix will be <code>/docs</code>, which identifies
     * all objects in the <code>docs/</code> folder. Required when the parent element
     * <code>Condition</code> is specified and sibling
     * <code>HttpErrorCodeReturnedEquals</code> is not specified. If both conditions
     * are specified, both must be true for the redirect to be applied.</p>
     */
    inline Condition& WithKeyPrefixEquals(const Aws::String& value) { SetKeyPrefixEquals(value); return *this;}

    /**
     * <p>The object key name prefix when the redirect is applied. For example, to
     * redirect requests for <code>ExamplePage.html</code>, the key prefix will be
     * <code>ExamplePage.html</code>. To redirect request for all pages with the prefix
     * <code>docs/</code>, the key prefix will be <code>/docs</code>, which identifies
     * all objects in the <code>docs/</code> folder. Required when the parent element
     * <code>Condition</code> is specified and sibling
     * <code>HttpErrorCodeReturnedEquals</code> is not specified. If both conditions
     * are specified, both must be true for the redirect to be applied.</p>
     */
    inline Condition& WithKeyPrefixEquals(Aws::String&& value) { SetKeyPrefixEquals(std::move(value)); return *this;}

    /**
     * <p>The object key name prefix when the redirect is applied. For example, to
     * redirect requests for <code>ExamplePage.html</code>, the key prefix will be
     * <code>ExamplePage.html</code>. To redirect request for all pages with the prefix
     * <code>docs/</code>, the key prefix will be <code>/docs</code>, which identifies
     * all objects in the <code>docs/</code> folder. Required when the parent element
     * <code>Condition</code> is specified and sibling
     * <code>HttpErrorCodeReturnedEquals</code> is not specified. If both conditions
     * are specified, both must be true for the redirect to be applied.</p>
     */
    inline Condition& WithKeyPrefixEquals(const char* value) { SetKeyPrefixEquals(value); return *this;}

  private:

    Aws::String m_httpErrorCodeReturnedEquals;
    bool m_httpErrorCodeReturnedEqualsHasBeenSet;

    Aws::String m_keyPrefixEquals;
    bool m_keyPrefixEqualsHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
