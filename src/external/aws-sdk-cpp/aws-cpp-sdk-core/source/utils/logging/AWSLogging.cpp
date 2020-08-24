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


#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/LogSystemInterface.h>
#include <aws/core/utils/memory/stl/AWSStack.h>

#include <memory>

using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

static std::shared_ptr<LogSystemInterface> AWSLogSystem(nullptr);
static std::shared_ptr<LogSystemInterface> OldLogger(nullptr);

namespace Aws
{
namespace Utils
{
namespace Logging {

void InitializeAWSLogging(const std::shared_ptr<LogSystemInterface> &logSystem) {
    AWSLogSystem = logSystem;
}

void ShutdownAWSLogging(void) {
    InitializeAWSLogging(nullptr);
}

LogSystemInterface *GetLogSystem() {
    return AWSLogSystem.get();
}

void PushLogger(const std::shared_ptr<LogSystemInterface> &logSystem)
{
    OldLogger = AWSLogSystem;
    AWSLogSystem = logSystem;
}

void PopLogger()
{
    AWSLogSystem = OldLogger;
    OldLogger = nullptr;
}

} // namespace Logging
} // namespace Utils
} // namespace Aws