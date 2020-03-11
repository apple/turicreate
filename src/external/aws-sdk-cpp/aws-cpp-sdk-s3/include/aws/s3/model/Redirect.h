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
#include <aws/s3/model/Protocol.h>
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
   * <p>Specifies how requests are redirected. In the event of an error, you can
   * specify a different error code to return.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/Redirect">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API Redirect
  {
  public:
    Redirect();
    Redirect(const Aws::Utils::Xml::XmlNode& xmlNode);
    Redirect& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The host name to use in the redirect request.</p>
     */
    inline const Aws::String& GetHostName() const{ return m_hostName; }

    /**
     * <p>The host name to use in the redirect request.</p>
     */
    inline bool HostNameHasBeenSet() const { return m_hostNameHasBeenSet; }

    /**
     * <p>The host name to use in the redirect request.</p>
     */
    inline void SetHostName(const Aws::String& value) { m_hostNameHasBeenSet = true; m_hostName = value; }

    /**
     * <p>The host name to use in the redirect request.</p>
     */
    inline void SetHostName(Aws::String&& value) { m_hostNameHasBeenSet = true; m_hostName = std::move(value); }

    /**
     * <p>The host name to use in the redirect request.</p>
     */
    inline void SetHostName(const char* value) { m_hostNameHasBeenSet = true; m_hostName.assign(value); }

    /**
     * <p>The host name to use in the redirect request.</p>
     */
    inline Redirect& WithHostName(const Aws::String& value) { SetHostName(value); return *this;}

    /**
     * <p>The host name to use in the redirect request.</p>
     */
    inline Redirect& WithHostName(Aws::String&& value) { SetHostName(std::move(value)); return *this;}

    /**
     * <p>The host name to use in the redirect request.</p>
     */
    inline Redirect& WithHostName(const char* value) { SetHostName(value); return *this;}


    /**
     * <p>The HTTP redirect code to use on the response. Not required if one of the
     * siblings is present.</p>
     */
    inline const Aws::String& GetHttpRedirectCode() const{ return m_httpRedirectCode; }

    /**
     * <p>The HTTP redirect code to use on the response. Not required if one of the
     * siblings is present.</p>
     */
    inline bool HttpRedirectCodeHasBeenSet() const { return m_httpRedirectCodeHasBeenSet; }

    /**
     * <p>The HTTP redirect code to use on the response. Not required if one of the
     * siblings is present.</p>
     */
    inline void SetHttpRedirectCode(const Aws::String& value) { m_httpRedirectCodeHasBeenSet = true; m_httpRedirectCode = value; }

    /**
     * <p>The HTTP redirect code to use on the response. Not required if one of the
     * siblings is present.</p>
     */
    inline void SetHttpRedirectCode(Aws::String&& value) { m_httpRedirectCodeHasBeenSet = true; m_httpRedirectCode = std::move(value); }

    /**
     * <p>The HTTP redirect code to use on the response. Not required if one of the
     * siblings is present.</p>
     */
    inline void SetHttpRedirectCode(const char* value) { m_httpRedirectCodeHasBeenSet = true; m_httpRedirectCode.assign(value); }

    /**
     * <p>The HTTP redirect code to use on the response. Not required if one of the
     * siblings is present.</p>
     */
    inline Redirect& WithHttpRedirectCode(const Aws::String& value) { SetHttpRedirectCode(value); return *this;}

    /**
     * <p>The HTTP redirect code to use on the response. Not required if one of the
     * siblings is present.</p>
     */
    inline Redirect& WithHttpRedirectCode(Aws::String&& value) { SetHttpRedirectCode(std::move(value)); return *this;}

    /**
     * <p>The HTTP redirect code to use on the response. Not required if one of the
     * siblings is present.</p>
     */
    inline Redirect& WithHttpRedirectCode(const char* value) { SetHttpRedirectCode(value); return *this;}


    /**
     * <p>Protocol to use when redirecting requests. The default is the protocol that
     * is used in the original request.</p>
     */
    inline const Protocol& GetProtocol() const{ return m_protocol; }

    /**
     * <p>Protocol to use when redirecting requests. The default is the protocol that
     * is used in the original request.</p>
     */
    inline bool ProtocolHasBeenSet() const { return m_protocolHasBeenSet; }

    /**
     * <p>Protocol to use when redirecting requests. The default is the protocol that
     * is used in the original request.</p>
     */
    inline void SetProtocol(const Protocol& value) { m_protocolHasBeenSet = true; m_protocol = value; }

    /**
     * <p>Protocol to use when redirecting requests. The default is the protocol that
     * is used in the original request.</p>
     */
    inline void SetProtocol(Protocol&& value) { m_protocolHasBeenSet = true; m_protocol = std::move(value); }

    /**
     * <p>Protocol to use when redirecting requests. The default is the protocol that
     * is used in the original request.</p>
     */
    inline Redirect& WithProtocol(const Protocol& value) { SetProtocol(value); return *this;}

    /**
     * <p>Protocol to use when redirecting requests. The default is the protocol that
     * is used in the original request.</p>
     */
    inline Redirect& WithProtocol(Protocol&& value) { SetProtocol(std::move(value)); return *this;}


    /**
     * <p>The object key prefix to use in the redirect request. For example, to
     * redirect requests for all pages with prefix <code>docs/</code> (objects in the
     * <code>docs/</code> folder) to <code>documents/</code>, you can set a condition
     * block with <code>KeyPrefixEquals</code> set to <code>docs/</code> and in the
     * Redirect set <code>ReplaceKeyPrefixWith</code> to <code>/documents</code>. Not
     * required if one of the siblings is present. Can be present only if
     * <code>ReplaceKeyWith</code> is not provided.</p>
     */
    inline const Aws::String& GetReplaceKeyPrefixWith() const{ return m_replaceKeyPrefixWith; }

    /**
     * <p>The object key prefix to use in the redirect request. For example, to
     * redirect requests for all pages with prefix <code>docs/</code> (objects in the
     * <code>docs/</code> folder) to <code>documents/</code>, you can set a condition
     * block with <code>KeyPrefixEquals</code> set to <code>docs/</code> and in the
     * Redirect set <code>ReplaceKeyPrefixWith</code> to <code>/documents</code>. Not
     * required if one of the siblings is present. Can be present only if
     * <code>ReplaceKeyWith</code> is not provided.</p>
     */
    inline bool ReplaceKeyPrefixWithHasBeenSet() const { return m_replaceKeyPrefixWithHasBeenSet; }

    /**
     * <p>The object key prefix to use in the redirect request. For example, to
     * redirect requests for all pages with prefix <code>docs/</code> (objects in the
     * <code>docs/</code> folder) to <code>documents/</code>, you can set a condition
     * block with <code>KeyPrefixEquals</code> set to <code>docs/</code> and in the
     * Redirect set <code>ReplaceKeyPrefixWith</code> to <code>/documents</code>. Not
     * required if one of the siblings is present. Can be present only if
     * <code>ReplaceKeyWith</code> is not provided.</p>
     */
    inline void SetReplaceKeyPrefixWith(const Aws::String& value) { m_replaceKeyPrefixWithHasBeenSet = true; m_replaceKeyPrefixWith = value; }

    /**
     * <p>The object key prefix to use in the redirect request. For example, to
     * redirect requests for all pages with prefix <code>docs/</code> (objects in the
     * <code>docs/</code> folder) to <code>documents/</code>, you can set a condition
     * block with <code>KeyPrefixEquals</code> set to <code>docs/</code> and in the
     * Redirect set <code>ReplaceKeyPrefixWith</code> to <code>/documents</code>. Not
     * required if one of the siblings is present. Can be present only if
     * <code>ReplaceKeyWith</code> is not provided.</p>
     */
    inline void SetReplaceKeyPrefixWith(Aws::String&& value) { m_replaceKeyPrefixWithHasBeenSet = true; m_replaceKeyPrefixWith = std::move(value); }

    /**
     * <p>The object key prefix to use in the redirect request. For example, to
     * redirect requests for all pages with prefix <code>docs/</code> (objects in the
     * <code>docs/</code> folder) to <code>documents/</code>, you can set a condition
     * block with <code>KeyPrefixEquals</code> set to <code>docs/</code> and in the
     * Redirect set <code>ReplaceKeyPrefixWith</code> to <code>/documents</code>. Not
     * required if one of the siblings is present. Can be present only if
     * <code>ReplaceKeyWith</code> is not provided.</p>
     */
    inline void SetReplaceKeyPrefixWith(const char* value) { m_replaceKeyPrefixWithHasBeenSet = true; m_replaceKeyPrefixWith.assign(value); }

    /**
     * <p>The object key prefix to use in the redirect request. For example, to
     * redirect requests for all pages with prefix <code>docs/</code> (objects in the
     * <code>docs/</code> folder) to <code>documents/</code>, you can set a condition
     * block with <code>KeyPrefixEquals</code> set to <code>docs/</code> and in the
     * Redirect set <code>ReplaceKeyPrefixWith</code> to <code>/documents</code>. Not
     * required if one of the siblings is present. Can be present only if
     * <code>ReplaceKeyWith</code> is not provided.</p>
     */
    inline Redirect& WithReplaceKeyPrefixWith(const Aws::String& value) { SetReplaceKeyPrefixWith(value); return *this;}

    /**
     * <p>The object key prefix to use in the redirect request. For example, to
     * redirect requests for all pages with prefix <code>docs/</code> (objects in the
     * <code>docs/</code> folder) to <code>documents/</code>, you can set a condition
     * block with <code>KeyPrefixEquals</code> set to <code>docs/</code> and in the
     * Redirect set <code>ReplaceKeyPrefixWith</code> to <code>/documents</code>. Not
     * required if one of the siblings is present. Can be present only if
     * <code>ReplaceKeyWith</code> is not provided.</p>
     */
    inline Redirect& WithReplaceKeyPrefixWith(Aws::String&& value) { SetReplaceKeyPrefixWith(std::move(value)); return *this;}

    /**
     * <p>The object key prefix to use in the redirect request. For example, to
     * redirect requests for all pages with prefix <code>docs/</code> (objects in the
     * <code>docs/</code> folder) to <code>documents/</code>, you can set a condition
     * block with <code>KeyPrefixEquals</code> set to <code>docs/</code> and in the
     * Redirect set <code>ReplaceKeyPrefixWith</code> to <code>/documents</code>. Not
     * required if one of the siblings is present. Can be present only if
     * <code>ReplaceKeyWith</code> is not provided.</p>
     */
    inline Redirect& WithReplaceKeyPrefixWith(const char* value) { SetReplaceKeyPrefixWith(value); return *this;}


    /**
     * <p>The specific object key to use in the redirect request. For example, redirect
     * request to <code>error.html</code>. Not required if one of the siblings is
     * present. Can be present only if <code>ReplaceKeyPrefixWith</code> is not
     * provided.</p>
     */
    inline const Aws::String& GetReplaceKeyWith() const{ return m_replaceKeyWith; }

    /**
     * <p>The specific object key to use in the redirect request. For example, redirect
     * request to <code>error.html</code>. Not required if one of the siblings is
     * present. Can be present only if <code>ReplaceKeyPrefixWith</code> is not
     * provided.</p>
     */
    inline bool ReplaceKeyWithHasBeenSet() const { return m_replaceKeyWithHasBeenSet; }

    /**
     * <p>The specific object key to use in the redirect request. For example, redirect
     * request to <code>error.html</code>. Not required if one of the siblings is
     * present. Can be present only if <code>ReplaceKeyPrefixWith</code> is not
     * provided.</p>
     */
    inline void SetReplaceKeyWith(const Aws::String& value) { m_replaceKeyWithHasBeenSet = true; m_replaceKeyWith = value; }

    /**
     * <p>The specific object key to use in the redirect request. For example, redirect
     * request to <code>error.html</code>. Not required if one of the siblings is
     * present. Can be present only if <code>ReplaceKeyPrefixWith</code> is not
     * provided.</p>
     */
    inline void SetReplaceKeyWith(Aws::String&& value) { m_replaceKeyWithHasBeenSet = true; m_replaceKeyWith = std::move(value); }

    /**
     * <p>The specific object key to use in the redirect request. For example, redirect
     * request to <code>error.html</code>. Not required if one of the siblings is
     * present. Can be present only if <code>ReplaceKeyPrefixWith</code> is not
     * provided.</p>
     */
    inline void SetReplaceKeyWith(const char* value) { m_replaceKeyWithHasBeenSet = true; m_replaceKeyWith.assign(value); }

    /**
     * <p>The specific object key to use in the redirect request. For example, redirect
     * request to <code>error.html</code>. Not required if one of the siblings is
     * present. Can be present only if <code>ReplaceKeyPrefixWith</code> is not
     * provided.</p>
     */
    inline Redirect& WithReplaceKeyWith(const Aws::String& value) { SetReplaceKeyWith(value); return *this;}

    /**
     * <p>The specific object key to use in the redirect request. For example, redirect
     * request to <code>error.html</code>. Not required if one of the siblings is
     * present. Can be present only if <code>ReplaceKeyPrefixWith</code> is not
     * provided.</p>
     */
    inline Redirect& WithReplaceKeyWith(Aws::String&& value) { SetReplaceKeyWith(std::move(value)); return *this;}

    /**
     * <p>The specific object key to use in the redirect request. For example, redirect
     * request to <code>error.html</code>. Not required if one of the siblings is
     * present. Can be present only if <code>ReplaceKeyPrefixWith</code> is not
     * provided.</p>
     */
    inline Redirect& WithReplaceKeyWith(const char* value) { SetReplaceKeyWith(value); return *this;}

  private:

    Aws::String m_hostName;
    bool m_hostNameHasBeenSet;

    Aws::String m_httpRedirectCode;
    bool m_httpRedirectCodeHasBeenSet;

    Protocol m_protocol;
    bool m_protocolHasBeenSet;

    Aws::String m_replaceKeyPrefixWith;
    bool m_replaceKeyPrefixWithHasBeenSet;

    Aws::String m_replaceKeyWith;
    bool m_replaceKeyWithHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
