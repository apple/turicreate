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

#include <aws/core/http/standard/StandardHttpRequest.h>

#include <aws/core/utils/StringUtils.h>

#include <iostream>
#include <algorithm>
#include <cassert>

using namespace Aws::Http;
using namespace Aws::Http::Standard;
using namespace Aws::Utils;

static bool IsDefaultPort(const URI& uri)
{
    switch(uri.GetPort())
    {
        case 80:
            return uri.GetScheme() == Scheme::HTTP;
        case 443:
            return uri.GetScheme() == Scheme::HTTPS;
        default:
            return false;
    }
}

StandardHttpRequest::StandardHttpRequest(const URI& uri, HttpMethod method) :
    HttpRequest(uri, method), 
    bodyStream(nullptr),
    m_responseStreamFactory()
{
    if(IsDefaultPort(uri))
    {
        StandardHttpRequest::SetHeaderValue(HOST_HEADER, uri.GetAuthority());
    }
    else
    {
        Aws::StringStream host;
        host << uri.GetAuthority() << ":" << uri.GetPort();
        StandardHttpRequest::SetHeaderValue(HOST_HEADER, host.str());
    }
}

HeaderValueCollection StandardHttpRequest::GetHeaders() const
{
    HeaderValueCollection headers;

    for (HeaderValueCollection::const_iterator iter = headerMap.begin(); iter != headerMap.end(); ++iter)
    {
        headers.emplace(HeaderValuePair(iter->first, iter->second));
    }

    return headers;
}

const Aws::String& StandardHttpRequest::GetHeaderValue(const char* headerName) const
{
    auto iter = headerMap.find(headerName);
    assert (iter != headerMap.end());
    return iter->second;
}

void StandardHttpRequest::SetHeaderValue(const char* headerName, const Aws::String& headerValue)
{
    headerMap[StringUtils::ToLower(headerName)] = StringUtils::Trim(headerValue.c_str());
}

void StandardHttpRequest::SetHeaderValue(const Aws::String& headerName, const Aws::String& headerValue)
{
    headerMap[StringUtils::ToLower(headerName.c_str())] = StringUtils::Trim(headerValue.c_str());
}

void StandardHttpRequest::DeleteHeader(const char* headerName)
{
    headerMap.erase(StringUtils::ToLower(headerName));
}

bool StandardHttpRequest::HasHeader(const char* headerName) const
{
    return headerMap.find(StringUtils::ToLower(headerName)) != headerMap.end();
}

int64_t StandardHttpRequest::GetSize() const
{
    int64_t size = 0;

    std::for_each(headerMap.cbegin(), headerMap.cend(), [&](const HeaderValueCollection::value_type& kvPair){ size += kvPair.first.length(); size += kvPair.second.length(); });

    return size;
}

const Aws::IOStreamFactory& StandardHttpRequest::GetResponseStreamFactory() const 
{ 
    return m_responseStreamFactory; 
}

void StandardHttpRequest::SetResponseStreamFactory(const Aws::IOStreamFactory& factory) 
{ 
    m_responseStreamFactory = factory; 
}
