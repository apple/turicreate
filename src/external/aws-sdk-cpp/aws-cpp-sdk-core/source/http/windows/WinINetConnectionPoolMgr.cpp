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

#include <aws/core/http/windows/WinINetConnectionPoolMgr.h>
#include <Windows.h>
#include <WinInet.h>

using namespace Aws::Http;

WinINetConnectionPoolMgr::WinINetConnectionPoolMgr(void* iOpenHandle, unsigned maxConnectionsPerHost, long requestTimeout, long connectTimeout) :
WinConnectionPoolMgr(iOpenHandle, maxConnectionsPerHost, requestTimeout, connectTimeout)
{

}

WinINetConnectionPoolMgr::WinINetConnectionPoolMgr(void* iOpenHandle, unsigned maxConnectionsPerHost, long requestTimeout, long connectTimeout,
                                                   bool enableTcpKeepAlive, unsigned long tcpKeepAliveIntervalMs) :
WinConnectionPoolMgr(iOpenHandle, maxConnectionsPerHost, requestTimeout, connectTimeout, enableTcpKeepAlive, tcpKeepAliveIntervalMs)
{

}

WinINetConnectionPoolMgr::~WinINetConnectionPoolMgr()
{
    DoCleanup();
}

void WinINetConnectionPoolMgr::DoCloseHandle(void* handle) const
{
    InternetCloseHandle(handle);
}

void* WinINetConnectionPoolMgr::CreateNewConnection(const Aws::String& host, HostConnectionContainer& connectionContainer) const
{
    HINTERNET newConnection = InternetConnectA(GetOpenHandle(), host.c_str(), connectionContainer.port, nullptr, nullptr,
        INTERNET_SERVICE_HTTP, INTERNET_FLAG_KEEP_CONNECTION, 0);

    DWORD timeoutMs = GetConnectTimeout();
    DWORD requestMs = GetRequestTimeout();

    InternetSetOptionA(newConnection, INTERNET_OPTION_CONNECT_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
    InternetSetOptionA(newConnection, INTERNET_OPTION_RECEIVE_TIMEOUT, &requestMs, sizeof(requestMs));

    return newConnection;
}