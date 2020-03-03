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

#include <aws/core/platform/Time.h>

#include <limits.h>
#include <time.h>

namespace Aws
{
namespace Time
{

#if defined(__ANDROID__) && !defined(__LP64__)

// TODO: verify this is ok legally, push into separate file?

// timegm doesn't exist for some forms of android.  Chromium has a substitute 
// implementation that we cut-and-paste here,
// including the full license notice from Chromium

// Copyright 2014 The Chromium Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// From src/base/os_compat_android.cc:


#include <time64.h>

// 32-bit Android has only timegm64() and not timegm().
// We replicate the behaviour of timegm() when the result overflows time_t.
time_t TimeGM(struct tm* const t) 
{
    // time_t is signed on Android.
    static const time_t kTimeMax = ~(1L << (sizeof(time_t) * CHAR_BIT - 1));
    static const time_t kTimeMin = (1L << (sizeof(time_t) * CHAR_BIT - 1));
    time64_t result = timegm64(t);
    if (result < kTimeMin || result > kTimeMax)
    {
        return -1;
    }
    return result;
}

#else 

time_t TimeGM(struct tm* const t)
{
    return timegm(t);
}

#endif 

void LocalTime(tm* t, std::time_t time)
{
    localtime_r(&time, t);
}

void GMTime(tm* t, std::time_t time)
{
    gmtime_r(&time, t);
}

} // namespace Time
} // namespace Aws
