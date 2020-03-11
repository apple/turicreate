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

#include <aws/core/utils/stream/ResponseStream.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#if defined(_GLIBCXX_FULLY_DYNAMIC_STRING) && _GLIBCXX_FULLY_DYNAMIC_STRING == 0 && defined(__ANDROID__)
#include <aws/core/utils/stream/SimpleStreamBuf.h>
using DefaultStreamBufType = Aws::Utils::Stream::SimpleStreamBuf;
#else
using DefaultStreamBufType = Aws::StringBuf;
#endif

using namespace Aws::Utils::Stream;

ResponseStream::ResponseStream(void) :
    m_underlyingStream(nullptr)
{
}

ResponseStream::ResponseStream(Aws::IOStream* underlyingStreamToManage) :
    m_underlyingStream(underlyingStreamToManage)
{
}

ResponseStream::ResponseStream(const Aws::IOStreamFactory& factory) :
    m_underlyingStream(factory())
{
}

ResponseStream::ResponseStream(ResponseStream&& toMove) : m_underlyingStream(toMove.m_underlyingStream)
{
    toMove.m_underlyingStream = nullptr;
}

ResponseStream& ResponseStream::operator=(ResponseStream&& toMove)
{
    if(m_underlyingStream == toMove.m_underlyingStream)
    {
        return *this;
    }

    ReleaseStream();
    m_underlyingStream = toMove.m_underlyingStream;
    toMove.m_underlyingStream = nullptr;

    return *this;
}

ResponseStream::~ResponseStream()
{
    ReleaseStream();
}

void ResponseStream::ReleaseStream()
{
    if (m_underlyingStream)
    {
        m_underlyingStream->flush();
        Aws::Delete(m_underlyingStream);
    }

    m_underlyingStream = nullptr;
}

static const char *DEFAULT_STREAM_TAG = "DefaultUnderlyingStream";

DefaultUnderlyingStream::DefaultUnderlyingStream() :
    Base( Aws::New< DefaultStreamBufType >( DEFAULT_STREAM_TAG ) )
{}

DefaultUnderlyingStream::DefaultUnderlyingStream(Aws::UniquePtr<std::streambuf> buf) :
    Base(buf.release())
{}

DefaultUnderlyingStream::~DefaultUnderlyingStream()
{
    if( rdbuf() )
    {
        Aws::Delete( rdbuf() );
    }
}

static const char* RESPONSE_STREAM_FACTORY_TAG = "ResponseStreamFactory";

Aws::IOStream* Aws::Utils::Stream::DefaultResponseStreamFactoryMethod() 
{
    return Aws::New<Aws::Utils::Stream::DefaultUnderlyingStream>(RESPONSE_STREAM_FACTORY_TAG);
}
