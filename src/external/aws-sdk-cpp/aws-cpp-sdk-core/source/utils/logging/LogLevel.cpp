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

#include <aws/core/utils/logging/LogLevel.h>

#include <aws/core/utils/memory/stl/AWSMap.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <cassert>

using namespace Aws::Utils::Logging;

namespace Aws
{
namespace Utils
{
namespace Logging
{

Aws::String GetLogLevelName(LogLevel logLevel) 
{ 
	switch (logLevel)
	{
	case LogLevel::Fatal:
		return "FATAL";
	case LogLevel::Error:
		return "ERROR";
	case LogLevel::Warn:
		return "WARN";
	case LogLevel::Info:
		return "INFO";
	case LogLevel::Debug:
		return "DEBUG";
	case LogLevel::Trace:
		return "TRACE";
	default:
		assert(0);
		return "";
	}   
}

} // namespace Logging
} // namespace Utils
} // namespace Aws
