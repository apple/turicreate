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

#include <aws/core/utils/memory/stl/SimpleStringStream.h>

namespace Aws
{

SimpleStringStream::SimpleStringStream() :
    base(&m_streamBuffer),
    m_streamBuffer()
{
}

SimpleStringStream::SimpleStringStream(const Aws::String& value) :
    base(&m_streamBuffer),
    m_streamBuffer(value)
{
}

void SimpleStringStream::str(const Aws::String& value)
{
    m_streamBuffer.str(value);
}

//

SimpleIStringStream::SimpleIStringStream() :
    base(&m_streamBuffer),
    m_streamBuffer()
{
}

SimpleIStringStream::SimpleIStringStream(const Aws::String& value) :
    base(&m_streamBuffer),
    m_streamBuffer(value)
{
}

void SimpleIStringStream::str(const Aws::String& value)
{
    m_streamBuffer.str(value);
}

//

SimpleOStringStream::SimpleOStringStream() :
    base(&m_streamBuffer),
    m_streamBuffer()
{
}

SimpleOStringStream::SimpleOStringStream(const Aws::String& value) :
    base(&m_streamBuffer),
    m_streamBuffer(value)
{
}

void SimpleOStringStream::str(const Aws::String& value)
{
    m_streamBuffer.str(value);
}

} // namespace Aws
