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

#include <aws/core/http/windows/WinHttpConnectionPoolMgr.h>

#include <aws/core/utils/StringUtils.h>

#include <Windows.h>
#include <winhttp.h>

using namespace Aws::Http;
using namespace Aws::Utils;

WinHttpConnectionPoolMgr::WinHttpConnectionPoolMgr(void* iOpenHandle, unsigned maxConnectionsPerHost, long requestTimeout, long connectTimeout) :
WinConnectionPoolMgr(iOpenHandle, maxConnectionsPerHost, requestTimeout, connectTimeout)
{

}

WinHttpConnectionPoolMgr::WinHttpConnectionPoolMgr(void* iOpenHandle, unsigned maxConnectionsPerHost, long requestTimeout, long connectTimeout,
                                                   bool enableTcpKeepAlive, unsigned long tcpKeepAliveIntervalMs) :
WinConnectionPoolMgr(iOpenHandle, maxConnectionsPerHost, requestTimeout, connectTimeout, enableTcpKeepAlive, tcpKeepAliveIntervalMs)
{

}

WinHttpConnectionPoolMgr::~WinHttpConnectionPoolMgr()
{
    DoCleanup();
}

void WinHttpConnectionPoolMgr::DoCloseHandle(void* handle) const
{
    WinHttpCloseHandle(handle);
}

void* WinHttpConnectionPoolMgr::CreateNewConnection(const Aws::String& host, HostConnectionContainer& connectionContainer) const
{
    HINTERNET newConnection = WinHttpConnect(GetOpenHandle(), StringUtils::ToWString(host.c_str()).c_str(), connectionContainer.port, 0);

    DWORD timeoutMs = GetConnectTimeout();
    DWORD requestMs = GetRequestTimeout();

    WinHttpSetOption(newConnection, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
    WinHttpSetOption(newConnection, WINHTTP_OPTION_RECEIVE_TIMEOUT, &requestMs, sizeof(requestMs));

    return newConnection;
}

