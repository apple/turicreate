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

#include <time.h>

namespace Aws
{
namespace Time
{

time_t TimeGM(struct tm* const t)
{
    return _mkgmtime(t);
}

void LocalTime(tm* t, std::time_t time)
{
    localtime_s(t, &time);
}

void GMTime(tm* t, std::time_t time)
{
    gmtime_s(t, &time);
}

} // namespace Time
} // namespace Aws
