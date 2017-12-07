/*
  * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/client/ClientConfiguration.h>

#include <aws/core/client/DefaultRetryStrategy.h>
#include <aws/core/platform/OSVersionInfo.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/threading/Executor.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/Version.h>

namespace Aws
{
namespace Client
{

static const char* CLIENT_CONFIGURATION_ALLOCATION_TAG = "ClientConfiguration";

static Aws::String ComputeUserAgentString()
{
  Aws::StringStream ss;
  ss << "aws-sdk-cpp/" << Version::GetVersionString() << " " <<  Aws::OSVersionInfo::ComputeOSVersionString();
  return ss.str();
}

ClientConfiguration::ClientConfiguration() : 
    userAgent(ComputeUserAgentString()), 
    scheme(Aws::Http::Scheme::HTTPS), 
    region(Region::US_EAST_1),
    useDualStack(false),
    maxConnections(25), 
    requestTimeoutMs(3000), 
    connectTimeoutMs(1000),
    retryStrategy(Aws::MakeShared<DefaultRetryStrategy>(CLIENT_CONFIGURATION_ALLOCATION_TAG)),
    proxyPort(0),
    executor(Aws::MakeShared<Aws::Utils::Threading::DefaultExecutor>(CLIENT_CONFIGURATION_ALLOCATION_TAG)),
    verifySSL(true),
    writeRateLimiter(nullptr),
    readRateLimiter(nullptr),
    httpLibOverride(Aws::Http::TransferLibType::DEFAULT_CLIENT),
    followRedirects(true)
{
}

} // namespace Client
} // namespace Aws
