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
#include <aws/core/utils/stream/ConcurrentStreamBuf.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <cstdint>
#include <cassert>

namespace Aws
{
    namespace Utils
    {
        namespace Stream
        {
            const char TAG[] = "ConcurrentStreamBuf";
            ConcurrentStreamBuf::ConcurrentStreamBuf(size_t bufferLength) :
                m_putArea(bufferLength), // we access [0] of the put area below so we must initialize it.
                m_eof(false)
            {
                m_getArea.reserve(bufferLength);
                m_backbuf.reserve(bufferLength);

                char* pbegin = reinterpret_cast<char*>(&m_putArea[0]);
                setp(pbegin, pbegin + bufferLength);
            }

            void ConcurrentStreamBuf::SetEof()
            {
                {
                    std::unique_lock<std::mutex> lock(m_lock);
                    m_eof = true;
                }
                m_signal.notify_all();
            }

            void ConcurrentStreamBuf::FlushPutArea()
            {
                const size_t bitslen = pptr() - pbase();
                if (bitslen)
                {
                    // scope the lock
                    {
                        std::unique_lock<std::mutex> lock(m_lock);
                        m_signal.wait(lock, [this, bitslen]{ return m_eof || bitslen <= (m_backbuf.capacity() - m_backbuf.size()); });
                        if (m_eof)
                        {
                            return;
                        }
                        std::copy(pbase(), pptr(), std::back_inserter(m_backbuf));
                    }
                    m_signal.notify_one();
                    char* pbegin = reinterpret_cast<char*>(&m_putArea[0]);
                    setp(pbegin, pbegin + m_putArea.size());
                }
            }

            std::streampos ConcurrentStreamBuf::seekoff(std::streamoff, std::ios_base::seekdir, std::ios_base::openmode)
            {
                return std::streamoff(-1); // Seeking is not supported.
            }

            std::streampos ConcurrentStreamBuf::seekpos(std::streampos, std::ios_base::openmode)
            {
                return std::streamoff(-1); // Seeking is not supported.
            }

            int ConcurrentStreamBuf::underflow()
            {
                {
                    std::unique_lock<std::mutex> lock(m_lock);
                    m_signal.wait(lock, [this]{ return m_backbuf.empty() == false || m_eof; });

                    if (m_eof && m_backbuf.empty())
                    {
                        return std::char_traits<char>::eof();
                    }

                    m_getArea.clear(); // keep the get-area from growing unbounded.
                    std::copy(m_backbuf.begin(), m_backbuf.end(), std::back_inserter(m_getArea));
                    m_backbuf.clear();
                }
                m_signal.notify_one();
                char* gbegin = reinterpret_cast<char*>(&m_getArea[0]);
                setg(gbegin, gbegin, gbegin + m_getArea.size());
                return std::char_traits<char>::to_int_type(*gptr());
            }

            std::streamsize ConcurrentStreamBuf::showmanyc()
            {
                std::unique_lock<std::mutex> lock(m_lock);
                AWS_LOGSTREAM_TRACE(TAG, "stream how many character? " << m_backbuf.size());
                return m_backbuf.size();
            }

            int ConcurrentStreamBuf::overflow(int ch)
            {
                const auto eof = std::char_traits<char>::eof();

                if (ch == eof)
                {
                    FlushPutArea();
                    return eof;
                }

                FlushPutArea();
                {
                    std::unique_lock<std::mutex> lock(m_lock);
                    if (m_eof)
                    {
                        return eof;
                    }
                    *pptr() = static_cast<char>(ch);
                    pbump(1);
                    return ch;
                }
            }

            int ConcurrentStreamBuf::sync()
            {
                FlushPutArea();
                return 0;
            }
        }
    }
}
