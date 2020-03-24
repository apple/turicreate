/*
* Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <Ws2tcpip.h>
#include <cassert>
#include <aws/core/net/SimpleUDP.h>
#include <aws/core/utils/logging/LogMacros.h>
namespace Aws
{
    namespace Net
    {
        static const char ALLOC_TAG[] = "SimpleUDP";
        static const char IPV4_LOOP_BACK_ADDRESS[] = "127.0.0.1";
        static const char IPV6_LOOP_BACK_ADDRESS[] = "::1";

        static inline bool IsValidIPAddress(const char* ip, int addressFamily/*AF_INET or AF_INET6*/)
        {
            char buffer[128];
            return inet_pton(addressFamily, ip, (void*)buffer) == 1 ?true :false;
        }

        static bool GetASockAddrFromHostName(const char* hostName, void* sockAddrBuffer, size_t& addrLength, int& addressFamily)
        {
            struct addrinfo hints, *res;
            
            memset(&hints, 0, sizeof(hints));

            hints.ai_family = PF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;
            if (getaddrinfo(hostName, nullptr, &hints, &res))
            {
                return false;
            }

            memcpy(sockAddrBuffer, res->ai_addr, res->ai_addrlen);
            addrLength = res->ai_addrlen;
            addressFamily = res->ai_family;            
            freeaddrinfo(res);
            return true;
        }

        static sockaddr_in BuildAddrInfoIPV4(const char* hostIP, short port)
        {
            sockaddr_in addrinfo {};
            addrinfo.sin_family = AF_INET;
            addrinfo.sin_port = htons(port);
            inet_pton(AF_INET, hostIP, &addrinfo.sin_addr);
            return addrinfo;
        }

        static sockaddr_in6 BuildAddrInfoIPV6(const char* hostIP, short port)
        {
            sockaddr_in6 addrinfo {};
            addrinfo.sin6_family = AF_INET6;
            addrinfo.sin6_port = htons(port);
            inet_pton(AF_INET6, hostIP, &addrinfo.sin6_addr);
            return addrinfo;
        }

        SimpleUDP::SimpleUDP(int addressFamily, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking):
            m_addressFamily(addressFamily), m_connected(false), m_socket(-1), m_port(0)
        {
            CreateSocket(addressFamily, sendBufSize, receiveBufSize, nonBlocking);
        }

        SimpleUDP::SimpleUDP(bool IPV4, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking) :
            m_addressFamily(IPV4 ? AF_INET : AF_INET6), m_connected(false), m_socket(-1), m_port(0)
        {
            CreateSocket(m_addressFamily, sendBufSize, receiveBufSize, nonBlocking);
        }

        SimpleUDP::SimpleUDP(const char* host, unsigned short port, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking) :
            m_addressFamily(AF_INET), m_connected(false), m_socket(-1), m_port(port)
        {
            if (IsValidIPAddress(host, AF_INET))
            {
                m_addressFamily = AF_INET;
                m_hostIP = Aws::String(host);
            }
            else if (IsValidIPAddress(host, AF_INET6))
            {
                m_addressFamily = AF_INET6;
                m_hostIP = Aws::String(host);
            }
            else 
            {
                char sockAddrBuffer[100];
                char hostBuffer[100];
                size_t addrLength = 0;
                if (GetASockAddrFromHostName(host, (void*)sockAddrBuffer, addrLength, m_addressFamily))
                {
                    if (m_addressFamily == AF_INET)
                    {
                        struct sockaddr_in* sockaddr = reinterpret_cast<struct sockaddr_in*>(sockAddrBuffer);
                        inet_ntop(m_addressFamily, &(sockaddr->sin_addr), hostBuffer, sizeof(hostBuffer));
                    }
                    else
                    {
                        struct sockaddr_in6* sockaddr = reinterpret_cast<struct sockaddr_in6*>(sockAddrBuffer);
                        inet_ntop(m_addressFamily, &(sockaddr->sin6_addr), hostBuffer, sizeof(hostBuffer));
                    }
                    m_hostIP = Aws::String(hostBuffer);
                }
                else
                {
                    AWS_LOGSTREAM_ERROR(ALLOC_TAG, "Can't retrieve a valid ip address based on provided host: " << host);
                }
            }
            CreateSocket(m_addressFamily, sendBufSize, receiveBufSize, nonBlocking);
        }

        SimpleUDP::~SimpleUDP()
        {
            closesocket(GetUnderlyingSocket());
        }

        void SimpleUDP::CreateSocket(int addressFamily, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking)
        {
            SOCKET sock = socket(addressFamily, SOCK_DGRAM, IPPROTO_UDP);
            assert(sock != INVALID_SOCKET);

            // Try to set sock to nonblocking mode.
            if (nonBlocking)
            {
                u_long enable = 1;
                ioctlsocket(sock, FIONBIO, &enable);
            }

            // if sendBufSize is not zero, try to set send buffer size
            if (sendBufSize)
            {
                int ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&sendBufSize), sizeof(sendBufSize));
                if (ret)
                {
                    AWS_LOGSTREAM_WARN(ALLOC_TAG, "Failed to set UDP send buffer size to " << sendBufSize << " for socket " << sock << " error code: " << WSAGetLastError());
                }
                assert(ret == 0);
            }

            // if receiveBufSize is not zero, try to set receive buffer size
            if (receiveBufSize)
            {
                int ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&receiveBufSize), sizeof(receiveBufSize));
                if (ret)
                {
                    AWS_LOGSTREAM_WARN(ALLOC_TAG, "Failed to set UDP receive buffer size to " << receiveBufSize << " for socket " << sock << " error code: " << WSAGetLastError());
                }
                assert(ret == 0);
            }

            SetUnderlyingSocket(static_cast<int>(sock));
        }

        int SimpleUDP::Connect(const sockaddr* address, size_t addressLength)
        {
            int ret = connect(GetUnderlyingSocket(), address, static_cast<socklen_t>(addressLength));
            m_connected = ret ? false : true;
            return ret;
        }

        int SimpleUDP::ConnectToHost(const char* hostIP, unsigned short port) const
        {
            int ret;
            if (m_addressFamily == AF_INET6)
            {
                sockaddr_in6 addrinfo = BuildAddrInfoIPV6(hostIP, port);
                ret = connect(GetUnderlyingSocket(), reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in6));
            }
            else
            {
                sockaddr_in addrinfo = BuildAddrInfoIPV4(hostIP, port);
                ret = connect(GetUnderlyingSocket(), reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in));
            }
            m_connected = ret ? false : true;
            return ret;
        }

        int SimpleUDP::ConnectToLocalHost(unsigned short port) const
        {
            if (m_addressFamily == AF_INET6)
            {
                return ConnectToHost(IPV6_LOOP_BACK_ADDRESS, port);
            }
            else
            {
                return ConnectToHost(IPV4_LOOP_BACK_ADDRESS, port);
            }
        }

        int SimpleUDP::Bind(const sockaddr* address, size_t addressLength) const
        {
            return bind(GetUnderlyingSocket(), address, static_cast<socklen_t>(addressLength));
        }

        int SimpleUDP::BindToLocalHost(unsigned short port) const
        {
            if (m_addressFamily == AF_INET6)
            {
                sockaddr_in6 addrinfo = BuildAddrInfoIPV6(IPV6_LOOP_BACK_ADDRESS, port);
                return bind(GetUnderlyingSocket(), reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in6));
            }
            else
            {
                sockaddr_in addrinfo = BuildAddrInfoIPV4(IPV4_LOOP_BACK_ADDRESS, port);
                return bind(GetUnderlyingSocket(), reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in));
            }
        }

        int SimpleUDP::SendData(const uint8_t* data, size_t dataLen) const
        {
            if (!m_connected)
            {
                ConnectToHost(m_hostIP.c_str(), m_port);
            }
            return send(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0);
        }

        int SimpleUDP::SendDataTo(const sockaddr* address, size_t addressLength, const uint8_t* data, size_t dataLen) const
        {
            if (m_connected)
            {
                return send(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0);
            }
            else
            {
                return sendto(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0, address, static_cast<socklen_t>(addressLength));
            }
        }

        int SimpleUDP::SendDataToLocalHost(const uint8_t* data, size_t dataLen, unsigned short port) const
        {
            if (m_connected)
            {
                return send(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0);
            }
            else if (m_addressFamily == AF_INET6)
            {
                sockaddr_in6 addrinfo = BuildAddrInfoIPV6(IPV6_LOOP_BACK_ADDRESS, port);
                return sendto(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0, reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in6));
            }
            else
            {
                sockaddr_in addrinfo = BuildAddrInfoIPV4(IPV4_LOOP_BACK_ADDRESS, port);
                return sendto(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0, reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in));
            }
        }

        int SimpleUDP::ReceiveData(uint8_t* buffer, size_t bufferLen) const
        {
            return recv(GetUnderlyingSocket(), reinterpret_cast<char*>(buffer), static_cast<int>(bufferLen), 0);
        }


        int SimpleUDP::ReceiveDataFrom(sockaddr* address, size_t* addressLength, uint8_t* buffer, size_t bufferLen) const
        {
            return recvfrom(GetUnderlyingSocket(), reinterpret_cast<char*>(buffer), static_cast<int>(bufferLen), 0, address, reinterpret_cast<socklen_t*>(addressLength));
        }
    }
}
