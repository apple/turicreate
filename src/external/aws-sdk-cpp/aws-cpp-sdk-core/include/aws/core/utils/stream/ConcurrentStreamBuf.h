/*
 * Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/Core_EXPORTS.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/common/array_list.h>

#include <mutex>
#include <condition_variable>
#include <streambuf>
#include <ios>
#include <cassert>

namespace Aws
{
    namespace Utils
    {
        namespace Stream
        {
            /**
             * A thread-safe streambuf implementation that allows simultaneous reading and writing.
             * NOTE: iostreams maintain state for readers and writers. This means that you can have at most two
             * concurrent threads, one for reading and one for writing. Multiple readers or multiple writers are not
             * thread-safe and will result in race-conditions.
             */
            class AWS_CORE_API ConcurrentStreamBuf : public std::streambuf
            {
            public:

                explicit ConcurrentStreamBuf(size_t bufferLength = 4 * 1024);

                void SetEof();

            protected:
                std::streampos seekoff(std::streamoff off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
                std::streampos seekpos(std::streampos pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

                int underflow() override;
                int overflow(int ch) override;
                int sync() override;
                std::streamsize showmanyc() override;

                void FlushPutArea();

            private:
                Aws::Vector<unsigned char> m_getArea;
                Aws::Vector<unsigned char> m_putArea;
                Aws::Vector<unsigned char> m_backbuf; // used to shuttle data from the put area to the get area
                std::mutex m_lock; // synchronize access to the common backbuffer
                std::condition_variable m_signal;
                bool m_eof;
            };
        }
    }
}
