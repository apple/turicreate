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

#include <aws/core/http/windows/WinConnectionPoolMgr.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <Windows.h>
#include <algorithm>

using namespace Aws::Utils::Logging;
using namespace Aws::Http;

const char WIN_CONNECTION_CONTAINER_TAG[] = "WinConnectionContainer";

WinConnectionPoolMgr::WinConnectionPoolMgr(void* iOpenHandle,
                                           unsigned maxConnectionsPerHost,
                                           long requestTimeoutMs,
                                           long connectTimeoutMs) :
    m_iOpenHandle(iOpenHandle),
    m_maxConnectionsPerHost(maxConnectionsPerHost),
    m_requestTimeoutMs(requestTimeoutMs),
    m_connectTimeoutMs(connectTimeoutMs),
    m_enableTcpKeepAlive(true),
    m_tcpKeepAliveIntervalMs(30000)
{
    AWS_LOGSTREAM_INFO(GetLogTag(), "Creating connection pool mgr with handle " << iOpenHandle << ", and max connections per host "
         << maxConnectionsPerHost <<  ", request timeout " << requestTimeoutMs << " ms, and connect timeout in " << connectTimeoutMs << " ms.");

}

WinConnectionPoolMgr::WinConnectionPoolMgr(void* iOpenHandle,
                                           unsigned maxConnectionsPerHost,
                                           long requestTimeoutMs,
                                           long connectTimeoutMs,
                                           bool enableTcpKeepAlive,
                                           unsigned long tcpKeepAliveIntervalMs) :
    m_iOpenHandle(iOpenHandle),
    m_maxConnectionsPerHost(maxConnectionsPerHost),
    m_requestTimeoutMs(requestTimeoutMs),
    m_connectTimeoutMs(connectTimeoutMs),
    m_enableTcpKeepAlive(enableTcpKeepAlive),
    m_tcpKeepAliveIntervalMs(tcpKeepAliveIntervalMs)
{
    AWS_LOGSTREAM_INFO(GetLogTag(), "Creating connection pool mgr with handle " << iOpenHandle << ", and max connections per host "
         << maxConnectionsPerHost <<  ", request timeout " << requestTimeoutMs << " ms, and connect timeout in " << connectTimeoutMs << " ms, "
         << (enableTcpKeepAlive ? "enabling" : "disabling") << " TCP keep-alive.");

}

WinConnectionPoolMgr::~WinConnectionPoolMgr()
{
    if (!m_hostConnections.empty())
    {
        AWS_LOGSTREAM_WARN(GetLogTag(), "Connection pool manager clearing with host connections not empty!");
    }
}

void WinConnectionPoolMgr::DoCleanup()
{
    AWS_LOGSTREAM_INFO(GetLogTag(), "Cleaning up connection pool mgr.");
    for (auto& hostHandles : m_hostConnections)
    {
        for(void* handleToClose : hostHandles.second->hostConnections.ShutdownAndWait(hostHandles.second->currentPoolSize))
        {
            AWS_LOGSTREAM_DEBUG(GetLogTag(), "Closing handle " << handleToClose);
            DoCloseHandle(handleToClose);
        }

        Aws::Delete(hostHandles.second);
    }
    m_hostConnections.clear();
}

void* WinConnectionPoolMgr::AcquireConnectionForHost(const Aws::String& host, uint16_t port)
{
    Aws::StringStream ss;
    ss << host << ":" << port;
    AWS_LOGSTREAM_INFO(GetLogTag(), "Attempting to acquire connection for " << ss.str());
    HostConnectionContainer* hostConnectionContainer;

    //let's go ahead and prevent that nasty little race condition.
    {
        std::lock_guard<std::mutex> hostsLocker(m_hostConnectionsMutex);
        Aws::Map<Aws::String, HostConnectionContainer*>::iterator foundPool = m_hostConnections.find(ss.str());

        if (foundPool != m_hostConnections.end())
        {
            AWS_LOGSTREAM_DEBUG(GetLogTag(), "Pool found, reusing");
            hostConnectionContainer = foundPool->second;
        }
        else
        {
            AWS_LOGSTREAM_DEBUG(GetLogTag(), "Pool doesn't exist for endpoint, creating...");
            //mutex doesn't have move. We have to dynamically allocate.
            HostConnectionContainer* newHostContainer = Aws::New<HostConnectionContainer>(GetLogTag());
            newHostContainer->currentPoolSize = 0;
            newHostContainer->port = port;

            m_hostConnections[ss.str()] = newHostContainer;
            hostConnectionContainer = newHostContainer;
        }
    }

    if(!hostConnectionContainer->hostConnections.HasResourcesAvailable())
    {
        AWS_LOGSTREAM_DEBUG(GetLogTag(), "Pool has no available existing connections for endpoint, attempting to grow pool.");
        CheckAndGrowPool(host, *hostConnectionContainer);
    }

    void* handle = hostConnectionContainer->hostConnections.Acquire();
    AWS_LOGSTREAM_INFO(GetLogTag(), "Connection now available, continuing.");
    AWS_LOGSTREAM_DEBUG(GetLogTag(), "Returning connection handle " << handle);
    return handle;
}

void WinConnectionPoolMgr::ReleaseConnectionForHost(const Aws::String& host, unsigned port, void* connection)
{
    if (connection != nullptr)
    {
        Aws::StringStream ss;
        ss << host << ":" << port;
        AWS_LOGSTREAM_DEBUG(GetLogTag(), "Releasing connection to endpoint " << ss.str());
        Aws::Map<Aws::String, HostConnectionContainer*>::iterator foundPool;

        //protect reads and writes on the connectionPool itself.
        {
            std::lock_guard<std::mutex> hostsLocker(m_hostConnectionsMutex);
            foundPool = m_hostConnections.find(ss.str());
        }

        if (foundPool != m_hostConnections.end())
        {
            foundPool->second->hostConnections.Release(connection);
        }
    }
}

bool WinConnectionPoolMgr::CheckAndGrowPool(const Aws::String& host, HostConnectionContainer& connectionContainer)
{
    std::lock_guard<std::mutex> locker(m_containerLock);
    if (connectionContainer.currentPoolSize < m_maxConnectionsPerHost)
    {
        unsigned multiplier = connectionContainer.currentPoolSize > 0 ? connectionContainer.currentPoolSize : 1;
        unsigned amountToAdd = (std::min)(multiplier * 2, m_maxConnectionsPerHost - connectionContainer.currentPoolSize);
        unsigned actuallyAdded = 0;
        for (unsigned i = 0; i < amountToAdd; ++i)
        {
            void* newConnection = CreateNewConnection(host, connectionContainer);

            if (newConnection)
            {
                connectionContainer.hostConnections.Release(newConnection);
                ++actuallyAdded;
            }
            else
            {
                AWS_LOGSTREAM_ERROR(WIN_CONNECTION_CONTAINER_TAG, "CreateNewConnection failed to allocate Win Http connection handles.");
                break;
            }
        }
        AWS_LOGSTREAM_INFO(WIN_CONNECTION_CONTAINER_TAG, "Pool grown by " << actuallyAdded);
        connectionContainer.currentPoolSize += actuallyAdded;

        return actuallyAdded > 0;
    }

    AWS_LOGSTREAM_INFO(WIN_CONNECTION_CONTAINER_TAG, "Pool cannot be grown any further, already at max size.");
    return false;
}
