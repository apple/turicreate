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

#include <aws/core/utils/logging/android/LogcatLogSystem.h>

#include <android/log.h>

using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

static const char *tag = "NativeSDK";

void LogcatLogSystem::ProcessFormattedStatement(Aws::String&& statement)
{
    __android_log_write(ANDROID_LOG_DEBUG, tag, statement.c_str());
}


